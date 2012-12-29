#ifndef CROLLINGFILE_H
#define CROLLINGFILE_H

#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/uio.h>

class CRollingFile
{
	public:
		CRollingFile(const char* filename, long rollSize=10*1024*1024, int rollCnt=3);
		virtual ~CRollingFile();
		//
		int fopen(const char* mode="a+");
		int fsync();
		int fclose();
		int fprintf(const char * format, ...);
		int fwrite(const void * ptr, size_t size, size_t nitems);
		int fwritev(const struct iovec *iov, int iovcnt);
		long size() {return m_pFile ? -1 : m_size;}
	protected:
	private:
		void roll();
		//
		char m_filename[PATH_MAX];
		long m_rollSize;
		long m_size;
		int m_rollCnt;
		FILE* m_pFile;
		int m_fd;
};

#endif // CROLLINGFILE_H
