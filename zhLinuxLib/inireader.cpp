#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include "inireader.h"

namespace NS_ZH_UTIL {

int CINIValueNode::GetIntVal(int &val, int base)
{
	char* endptr=0;
	errno = 0;
	val = strtol(m_value.c_str(), &endptr, base);
	if(0 == errno && endptr != m_value.c_str()) return 0;
	return -1;
}

int CINIValueNode::GetUIntVal(unsigned &val, int base)
{
	char* endptr=0;
	errno = 0;
	val = strtoul(m_value.c_str(), 0, base);
	if(0 == errno && endptr != m_value.c_str()) return 0;
	return -1;
}

int CINIValueNode::GetStringVal(string &val)
{
	val = m_value;
	return 0;
}

int CINIValueNode::GetStringVal(char *buf, int buf_len)
{
	::strncpy(buf, m_value.c_str(), buf_len);
	return 0;
}

CININode* const CINIListNode::operator[](const size_t index)
{
	if(index >= m_Nodes.size()) return NULL;
	return &m_Nodes[index];
}

CININode* const CINIKeyValueSection::operator[](const string& key)
{
	map<string, CINIKeyValueNode>::iterator it = m_Nodes.find(key);
	if(it == m_Nodes.end()) return NULL;
	return &(it->second);
}

CININode* const CINIListSection::operator[](const size_t index)
{
	if(index >= m_Nodes.size()) return NULL;
	return &m_Nodes[index];
}
//

CINIConfReader::CINIConfReader(string& fn)
:m_cfgFileName(fn)
,m_lineNum(0)
{
}

CINIConfReader::CINIConfReader(const char* fn)
:m_lineNum(0)
{
	assert(NULL != fn);
	m_cfgFileName = fn;
}

//
//#define JumpOverLeadingBlank(p) {while('\0' != *p &&(' ' == *p || '\t' == *p)) ++p;}
//#define TrimBlank(pStart, pEnd) {while(pStart <= pEnd && (' ' == *pStart || '\t' == *pStart)) ++pStart;while(pEnd >= pStart && (' ' == *pEnd || '\t' == *pEnd)) --pEnd;}
char* JumpOverLeadingBlank(char* p) {while('\0' != *p &&(' ' == *p || '\t' == *p)) ++p; return p;}
void TrimBlank(char* &pStart, char* &pEnd)
{
	while(pStart <= pEnd && (' ' == *pStart || '\t' == *pStart)) ++pStart;
	while(pEnd >= pStart && (' ' == *pEnd || '\t' == *pEnd)) --pEnd;
}
enum INI_SECTION_TYPE
{
	INI_SECTION_TYPE_LIST=1,
	INI_SECTION_TYPE_KV
};
//
/*
int CINIConfReader::Read()
{
	int ret = 0;
	FILE* fd=fopen(m_cfgFileName.c_str(), "r");
	char line[1024] = {0};
	char* pLine = 0;
	if(NULL == fd) {ret = -1; goto _END;}
	//
	m_lineNum = 0;
	pLine = fgets(line, sizeof(line), fd);
	while(NULL != pLine)
	{
		++m_lineNum;
		//
		//printf("%s", pLine);
		//
		pLine = JumpOverLeadingBlank(pLine);
		//
		if('\n' == *pLine || '#' == *pLine)
		{
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			continue;
		}
		if('[' == *pLine)
		{
			++pLine;
			char* pSectionStart = pLine;
			char* pSectionEnd = 0;
			while('\0' != *pLine && ']' != *pLine) ++pLine;
			if(*pLine != ']') {ret = -2; goto _END;}; 					// file format error
			pSectionEnd = pLine-1;
			TrimBlank(pSectionStart, pSectionEnd);
			if(pSectionEnd < pSectionStart) {ret = -2; goto _END;}; 	// file format error

			string sectionName(pSectionStart, pSectionEnd-pSectionStart+1);
			//@zh for test
			printf("@@@@@@test: find section [%s]\n", sectionName.c_str());
			//@end of zh
			int sectionType = 0;
			int fieldCount = 0;
			int fieldLineIndex=0;
			int fieldIndex = 0;
			// parse section of [sectionName]
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			while(NULL != pLine)
			{
				fieldIndex = 0;
				char* pSectionLine = JumpOverLeadingBlank(pLine);
				if('\n' == *pSectionLine || '#' == *pSectionLine)
				{
					memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
					++m_lineNum;
					continue;
				}
				if('[' == *pSectionLine) break; //next section
				//parse this section line
				++m_lineNum;
				char* pKeyStart = pSectionLine;
				char* pKeyEnd = 0;
				CINIListNode listNode;
				while('\n' != *pSectionLine && '#' != *pSectionLine)
				{
					if('=' == *pSectionLine)
					{
						if(0 == sectionType) sectionType =1;
						else if(1 != sectionType) {ret = -2; goto _END;}; 	// file format error
						pKeyEnd = pSectionLine-1;
						TrimBlank(pKeyStart, pKeyEnd);
						if(pKeyEnd < pKeyStart) {ret = -2; goto _END;};	// file format error
						string key(pKeyStart, pKeyEnd-pKeyStart+1);
						++pSectionLine;
						char* pValStart = pSectionLine;
						char* pValEnd = 0;
						while(*pSectionLine != '\n' && *pSectionLine != '#') ++pSectionLine;
						pValEnd = pSectionLine-1;
						string value;
						if(pValEnd >= pValStart) value.assign(pValStart, pValEnd-pValStart+1);
						//
						CINIKeyValueNode newNode(key, value);
						AddKeyValueNode(sectionName, newNode);
						// next section line
						memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
						break;
					}
					else if(',' == *pSectionLine)
					{
						if(0 == sectionType) sectionType = 2;
						else if(2 != sectionType) {ret = -2; goto _END;}; // file format error
						pKeyEnd = pSectionLine-1;
						TrimBlank(pKeyStart, pKeyEnd);
						string key;
						if(pKeyEnd >= pKeyStart) key.assign(pKeyStart, pKeyEnd-pKeyStart+1);
						CINIValueNode fieldValue(key); listNode.AddNode(fieldValue);
						pKeyStart = pSectionLine+1; // next filed start
						fieldIndex++;
						//
						++pSectionLine;
						if('\n' == *pSectionLine || '#' == *pSectionLine)
						{
							//the last field
							fieldIndex++;
							if(0 == fieldCount) fieldCount=fieldIndex;
							else if(fieldIndex != fieldCount) {ret = -2; goto _END;}; // file format error
							//
							CINIValueNode fieldValue(key); listNode.AddNode(fieldValue);
							AddListValueNode(sectionName, listNode);
							//
							fieldLineIndex++;
							// next section line
							memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
							break;
						}
						else
						{
							// next field
							continue;
						}
					}
					else
					{
						++pSectionLine;
						if('\n' == *pSectionLine || '#' == *pSectionLine)
						{
							if(2 == sectionType)
							{
								//the last field
								fieldIndex++;
								if(0 == fieldCount) fieldCount=fieldIndex;
								else if(fieldIndex != fieldCount) {ret = -2; goto _END;}; // file format error
								//
								pKeyEnd = pSectionLine-1;
								string key;
								TrimBlank(pKeyStart, pKeyEnd);
								if(pKeyEnd >= pKeyStart) key.assign(pKeyStart, pKeyEnd-pKeyStart+1);
								//
								CINIValueNode fieldValue(key); listNode.AddNode(fieldValue);
								AddListValueNode(sectionName, listNode);
								//
								fieldLineIndex++;
							}
							// next section line
							memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
							//TODO:check if the last line
							break;
						}
					}
				}
			}
		}
		else
		{
			// file format error
			ret = -2;
			goto _END;
		}
	}
	_END:
	if(fd) fclose(fd);
	return ret;
}
*/

int CINIConfReader::Read()
{
	int ret = 0;
	FILE* fd=fopen(m_cfgFileName.c_str(), "r");
	char line[1024] = {0};
	char* pLine = 0;
	if(NULL == fd) {ret = -1; goto _END;}
	//
	m_lineNum = 0;
	pLine = fgets(line, sizeof(line), fd);
	while(NULL != pLine)
	{
		++m_lineNum;
		pLine = JumpOverLeadingBlank(pLine);
		//
		if('\n' == *pLine || '#' == *pLine)
		{
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			continue;
		}
		//
		if('[' == *pLine)
		{
			++pLine;
			char* pSectionStart = pLine;
			char* pSectionEnd = 0;
			while('\0' != *pLine && ']' != *pLine) ++pLine;
			if(*pLine != ']') {ret = -2; goto _END;}; 					// file format error
			pSectionEnd = pLine-1;
			TrimBlank(pSectionStart, pSectionEnd);
			if(pSectionEnd < pSectionStart) {ret = -2; goto _END;}; 	// file format error

			string sectionName(pSectionStart, pSectionEnd-pSectionStart+1);
			//@zh for test
			//printf("@@@@@@test: find section [%s]\n", sectionName.c_str());
			//@end of zh
			int sectionType = 0;
			int fieldCount = 0;
			int fieldIndex = 0;
			char* pSectionLine = 0;
			// parse section of [sectionName]
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			while(NULL != pLine)
			{
				fieldIndex = 0;
				pSectionLine = JumpOverLeadingBlank(pLine);
				if('\n' == *pSectionLine || '#' == *pSectionLine)
				{
					memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
					++m_lineNum;
					continue;
				}
				if('[' == *pSectionLine) {break;} //next section
				//parse this section line
				++m_lineNum;
				char* pKeyStart = pSectionLine;
				char* pKeyEnd = 0;

				while('\n' != *pSectionLine && '#' != *pSectionLine)
				{
					if('=' == *pSectionLine)
					{
						if(0 == sectionType) sectionType =INI_SECTION_TYPE_KV;
						else if(INI_SECTION_TYPE_KV != sectionType) {ret = -2; goto _END;}; 	// file format error
						pKeyEnd = pSectionLine-1;
						TrimBlank(pKeyStart, pKeyEnd);
						if(pKeyEnd < pKeyStart) {ret = -2; goto _END;};	// file format error
						string key(pKeyStart, pKeyEnd-pKeyStart+1);
						++pSectionLine;
						char* pValStart = pSectionLine;
						char* pValEnd = 0;
						while(*pSectionLine != '\n' && *pSectionLine != '#') ++pSectionLine;
						pValEnd = pSectionLine-1;
						string value;
						TrimBlank(pValStart, pValEnd);
						if(pValEnd >= pValStart) value.assign(pValStart, pValEnd-pValStart+1);
						//
						CINIKeyValueNode newNode(key, value);
						AddKeyValueNode(sectionName, newNode);
						// next section line
						//memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
						break;
					}
					else if(',' == *pSectionLine)
					{
						if(0 == sectionType) sectionType = INI_SECTION_TYPE_LIST;
						else if(INI_SECTION_TYPE_LIST != sectionType) {ret = -2; goto _END;}; // file format error
						pKeyEnd = pSectionLine-1;
						TrimBlank(pKeyStart, pKeyEnd);
						string fieldVal;
						if(pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd-pKeyStart+1);
						CINIValueNode fieldValue(fieldVal);
						AddListValueNode(sectionName, fieldValue, true);
						//
						++pSectionLine;
						pKeyStart = pSectionLine; // next filed start
						++fieldIndex;
						//
						while('\n' != *pSectionLine && '#' != *pSectionLine)
						{
							if(',' == *pSectionLine)
							{
								pKeyEnd = pSectionLine-1;
								TrimBlank(pKeyStart, pKeyEnd); fieldVal.clear();
								if(pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd-pKeyStart+1);
								fieldValue = fieldVal;
								AddListValueNode(sectionName, fieldValue, false);
								//
								++pSectionLine;
								pKeyStart = pSectionLine; // next filed start
								++fieldIndex;
							}
							else
							{
								++pSectionLine;
							}
						}
						//the last field
						pKeyEnd = pSectionLine-1;
						TrimBlank(pKeyStart, pKeyEnd); fieldVal.clear();
						if(pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd-pKeyStart+1);
						fieldValue = fieldVal;
						AddListValueNode(sectionName, fieldValue, false);
						//
						++fieldIndex;
						if(0 == fieldCount) fieldCount=fieldIndex;
						else if(fieldIndex != fieldCount) {ret = -2; goto _END;}; // file format error
						// next section line
						//memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
						break;

					}
					else
					{
						++pSectionLine;
					}
				}
				//
				// next section line
				memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
				continue;
			}
		}
		else
		{
			// file format error
			ret = -2;
			goto _END;
		}
	}
	_END:
	if(fd) fclose(fd);
	return ret;
}

void CINIConfReader::AddKeyValueNode(string& section, CINIKeyValueNode& node)
{
	map<string, CINISectionNode*>::iterator it = m_sections.find(section);
	CINIKeyValueSection* pSection = NULL;
	if(it == m_sections.end())
	{
		pSection = new CINIKeyValueSection();
		m_sections[section] = pSection;
	}
	else
	{
		pSection = dynamic_cast<CINIKeyValueSection*>(it->second);
	}
	pSection->AddNode(node);
}

void CINIConfReader::AddListValueNode(string& section, CINIValueNode& node, bool newListNode)
{
	map<string, CINISectionNode*>::iterator it = m_sections.find(section);
	CINIListSection* pSection = NULL;
	if(it == m_sections.end())
	{
		pSection = new CINIListSection();
		m_sections[section] = pSection;
		//@zh for test
		//printf("@@@@@@test: add new section [%s]\n", section.c_str());
		//@end of zh
	}
	else
	{
		pSection = dynamic_cast<CINIListSection*>(it->second);
	}
	//
	if(newListNode)
	{
		CINIListNode listNode; listNode.AddNode(node);
		pSection->AddNode(listNode);
	}
	else
	{
		if(!pSection->m_Nodes.empty())
		{
			pSection->m_Nodes.back().AddNode(node);
		}
	}
}

CINIConfReader::~CINIConfReader()
{
	map<string, CINISectionNode*>::iterator it = m_sections.begin();
	while(it != m_sections.end()) {delete it->second; ++it;}
	m_sections.clear();
}

CINISectionNode* const CINIConfReader::operator[](const string& name)
{
	map<string, CINISectionNode*>::iterator it = m_sections.find(name);
	if(it != m_sections.end()) return it->second;
	return NULL;
}

void CINIConfReader::clear()
{
	map<string, CINISectionNode*>::iterator it = m_sections.begin();
	while(it != m_sections.end()) {delete it->second; ++it;}
	m_sections.clear();
}

}//namespace
