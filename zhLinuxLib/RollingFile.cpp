#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <glob.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <vector>
#include <algorithm>
#include <stdarg.h>

#include "RollingFile.h"

CRollingFile::CRollingFile(const char* filename, long rollSize, int rollCnt)
:m_rollSize(rollSize > 0 ? rollSize : 10*1024*1024)
,m_pFile(0)
,m_fd(-1)
,m_size(0)
,m_rollCnt(rollCnt > 0 ? rollCnt : 3)
{
	assert(filename != NULL && filename[0] != '\0');
	::strncpy(m_filename, filename, sizeof(m_filename) - 1);
}

CRollingFile::~CRollingFile()
{
	if(m_pFile)
	{
		TEMP_FAILURE_RETRY(::fclose(m_pFile));
		m_pFile = 0;
		m_fd = -1;
	}
}

int CRollingFile::fopen(const char* mode)
{
	do
	{
		m_pFile = ::fopen(m_filename, mode);
	}
	while(0 == m_pFile && errno == EINTR);
	//
	m_fd = m_pFile ? ::fileno(m_pFile) : -1;
	if(m_fd >= 0)
	{
		struct stat st;
		::fstat(m_fd, &st);
		m_size = st.st_size;
	}
	else
	{
		m_size = 0;
	}
	return m_pFile ? 0 : -1;
}

int CRollingFile::fsync()
{
	if(0 == m_pFile) return -1;
	return TEMP_FAILURE_RETRY(::fsync(m_fd));
}

int CRollingFile::fclose()
{
	if(0 == m_pFile) return -1;
	TEMP_FAILURE_RETRY(::fclose(m_pFile));
	m_pFile = 0;
	m_fd = -1;
	return 0;
}

void CRollingFile::roll()
{
	this->fclose();
	glob_t globbuf;
	char *pPattern = new char[PATH_MAX]; ::memset(pPattern, 0, PATH_MAX);
	::snprintf(pPattern, PATH_MAX - 1, "%s.*", m_filename);
	::glob(pPattern, 0, 0, &globbuf);
	delete [] pPattern; pPattern = 0;

	int suffix = 0;
	int suffix_offset = ::strlen(m_filename)+1;
	char* endptr = 0;
	struct stat st;
	std::vector<int> rollfiles;
	for(int ii = 0; ii < globbuf.gl_pathc; ++ii)
	{
		if((globbuf.gl_pathv[ii][suffix_offset]>='1' && globbuf.gl_pathv[ii][suffix_offset]<='9') &&
			::stat(globbuf.gl_pathv[ii], &st) == 0 &&
			S_ISREG(st.st_mode))
		{
			suffix = ::strtol(globbuf.gl_pathv[ii]+suffix_offset, &endptr, 10);
			if(suffix > 0 && suffix <= m_rollCnt && endptr[0]=='\0')
			{
				rollfiles.push_back(suffix);
			}
		}
	}
	::globfree(&globbuf);
	//
	char *pNFn = new char[PATH_MAX];
	if(rollfiles.size() > 0)
	{
		char *pFn = new char[PATH_MAX];
		std::sort(rollfiles.begin(), rollfiles.end());
		std::vector<int>::reverse_iterator rit = rollfiles.rbegin();
		while(rit != rollfiles.rend())
		{
			{
				if(*rit == m_rollCnt)
				{
					::memset(pFn, 0, PATH_MAX); ::snprintf(pFn, PATH_MAX - 1, "%s.%d", m_filename, *rit);
					::remove(pFn);
				}
				else
				{
					::memset(pFn, 0, PATH_MAX); ::snprintf(pFn, PATH_MAX - 1, "%s.%d", m_filename, *rit);
					::memset(pNFn, 0, PATH_MAX); ::snprintf(pNFn, PATH_MAX - 1, "%s.%d", m_filename, *rit+1);
					::rename(pFn, pNFn);
				}
			}
			++rit;
		}
		delete [] pFn; pFn = 0;
	}
	//
	::memset(pNFn, 0, PATH_MAX); ::snprintf(pNFn, PATH_MAX - 1, "%s.1", m_filename);
	::rename(m_filename, pNFn);
	 delete [] pNFn; pNFn = 0;
	this->fopen("w+");
}

int CRollingFile::fprintf(const char * format, ...)
{
	if(0 == m_pFile) return -1;
	va_list args;
	va_start(args, format);
	int iWrite = TEMP_FAILURE_RETRY(::vfprintf(m_pFile, format, args));
	va_end(args);
	if(iWrite >= 0) m_size += iWrite;
	if(m_size >= m_rollSize) this->roll();
	return iWrite;
}

int CRollingFile::fwrite(const void * ptr, size_t size, size_t nitems)
{
	if(0 == m_pFile) return -1;
	int iWrite = TEMP_FAILURE_RETRY(::fwrite(ptr, size, nitems, m_pFile));
	if(iWrite >= 0) m_size += iWrite*size;
	if(m_size >= m_rollSize) this->roll();
	return iWrite;
}

int CRollingFile::fwritev(const struct iovec *iov, int iovcnt)
{
	if(m_fd == -1) return -1;
	int iWrite = ::writev(m_fd, iov, iovcnt);
	if(iWrite >= 0) m_size += iWrite;
	if(m_size >= m_rollSize) this->roll();
	return iWrite;
}
