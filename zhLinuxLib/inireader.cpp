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
//class CINIListNode
CININode* const CINIListNode::operator[](const size_t index)
{
	if(index >= m_Nodes.size()) return NULL;
	return m_Nodes[index];
}

CINIListNode::~CINIListNode()
{
	vector<CININode*>::iterator it = m_Nodes.begin();
	for (; it != m_Nodes.end(); ++it) delete *it;
	m_Nodes.clear();
}
//
//class CINISection
void CINISection::AddNode(CININode* pNode)
{
	m_nodes.push_back(pNode);
}
void CINISection::AddNode(string& key, CININode* pNode)
{
	m_nodes.push_back(pNode);
	m_nodesMap[key] = pNode;
}
CININode* const CINISection::operator[](const size_t index)
{
	if (index >= m_nodes.size()) return NULL;
	return m_nodes[index];
}

CININode* const CINISection::operator[](const string& key)
{
	map<string, CININode*>::iterator it = m_nodesMap.find(key);
	if (it != m_nodesMap.end()) return it->second;
	return NULL;
}

//CININode* const CINISection::operator[](const char* key)
//{
//	return (*this)[string(key)];
//}

CINISection::~CINISection()
{
	vector<CININode*>::iterator it = m_nodes.begin();
	for (; it != m_nodes.end(); ++it) delete *it;
	m_nodes.clear();
	m_nodesMap.clear();
}
//class CINIConfReader
CINIConfReader::CINIConfReader(string& fn)
:m_cfgFileName(fn)
, m_lineNum(0)
{
}

CINIConfReader::CINIConfReader(const char* fn)
: m_lineNum(0)
{
	assert(NULL != fn);
	m_cfgFileName = fn;
}
void CINIConfReader::AddSection(string& name, CINISection* pSection)
{
	map<string, CINISection*>::iterator it = m_sections.find(name);
	if (it != m_sections.end()) delete it->second;
	m_sections[name] = pSection;
}

CINISection* CINIConfReader::AddSection(string& name)
{
	CINISection *pSection = new CINISection(name);
	AddSection(name, pSection);
	return pSection;
}

CINIConfReader::~CINIConfReader()
{
	map<string, CINISection*>::iterator it = m_sections.begin();
	while(it != m_sections.end()) {delete it->second; ++it;}
	m_sections.clear();
}

CINISection* const CINIConfReader::operator[](const string& name)
{
	map<string, CINISection*>::iterator it = m_sections.find(name);
	if(it != m_sections.end()) return it->second;
	return NULL;
}

void CINIConfReader::clear()
{
	map<string, CINISection*>::iterator it = m_sections.begin();
	while(it != m_sections.end()) {delete it->second; ++it;}
	m_sections.clear();
}
//
//#define JumpOverLeadingBlank(p) {while('\0' != *p &&(' ' == *p || '\t' == *p)) ++p;}
//#define TrimBlank(pStart, pEnd) {while(pStart <= pEnd && (' ' == *pStart || '\t' == *pStart)) ++pStart;while(pEnd >= pStart && (' ' == *pEnd || '\t' == *pEnd)) --pEnd;}
char* JumpOverLeadingBlank(char* p) { while ('\0' != *p && (' ' == *p || '\t' == *p)) ++p; return p; }
void TrimBlank(char* &pStart, char* &pEnd)
{
	while (pStart <= pEnd && (' ' == *pStart || '\t' == *pStart)) ++pStart;
	while (pEnd >= pStart && (' ' == *pEnd || '\t' == *pEnd)) --pEnd;
}
enum INI_SECTION_TYPE
{
	INI_SECTION_TYPE_LIST = 1,
	INI_SECTION_TYPE_KV
};

int CINIConfReader::Read()
{
	int ret = 0;
	FILE* fd = fopen(m_cfgFileName.c_str(), "r");
	char line[1024] = { 0 };
	char* pLine = 0;
	if (NULL == fd) { ret = -1; goto _END; }
	//
	m_lineNum = 0;
	pLine = fgets(line, sizeof(line), fd);
	while (NULL != pLine)
	{
		++m_lineNum;
		pLine = JumpOverLeadingBlank(pLine);
		//
		if ('\n' == *pLine || '#' == *pLine)
		{
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			continue;
		}
		//
		if ('[' == *pLine)
		{
			++pLine;
			char* pSectionStart = pLine;
			char* pSectionEnd = 0;
			while ('\0' != *pLine && ']' != *pLine) ++pLine;
			if (*pLine != ']') { ret = -2; goto _END; }; 					// file format error
			pSectionEnd = pLine - 1;
			TrimBlank(pSectionStart, pSectionEnd);
			if (pSectionEnd < pSectionStart) { ret = -2; goto _END; }; 		// file format error

			string sectionName(pSectionStart, pSectionEnd - pSectionStart + 1);
			CINISection* pSection = AddSection(sectionName);
			//@zh for test
			//printf("@@@@@@test: find section [%s]\n", sectionName.c_str());
			//@end of zh
			int sectionType = 0;
			int fieldCount = 0;
			int fieldIndex = 0;
			char* pSectionLine = 0;
			// parse section of [sectionName]
			memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
			while (NULL != pLine)
			{
				fieldIndex = 0;
				pSectionLine = JumpOverLeadingBlank(pLine);
				if ('\n' == *pSectionLine || '#' == *pSectionLine)
				{
					memset(line, 0, sizeof(line)); pLine = fgets(line, sizeof(line), fd);
					++m_lineNum;
					continue;
				}
				if ('[' == *pSectionLine) { break; } //next section
				//parse this section line
				++m_lineNum;
				char* pKeyStart = pSectionLine;
				char* pKeyEnd = 0;

				while ('\n' != *pSectionLine && '#' != *pSectionLine)
				{
					if ('=' == *pSectionLine)
					{
						pKeyEnd = pSectionLine - 1;
						TrimBlank(pKeyStart, pKeyEnd);
						if (pKeyEnd < pKeyStart) { ret = -2; goto _END; };						// file format error
						string key(pKeyStart, pKeyEnd - pKeyStart + 1);
						++pSectionLine;
						char* pValStart = pSectionLine;
						char* pValEnd = 0;
						while (*pSectionLine != '\n' && *pSectionLine != '#') ++pSectionLine;
						pValEnd = pSectionLine - 1;
						string value;
						TrimBlank(pValStart, pValEnd);
						if (pValEnd >= pValStart) value.assign(pValStart, pValEnd - pValStart + 1);
						//
						pSection->AddNode(key, new CINIKeyValueNode(key, value));
						break;
					}
					else if (',' == *pSectionLine)
					{
						CINIListNode *pList = new CINIListNode();
						pKeyEnd = pSectionLine - 1;
						TrimBlank(pKeyStart, pKeyEnd);
						string fieldVal;
						if (pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd - pKeyStart + 1);
						pList->AddNode(fieldVal);
						//
						++pSectionLine;
						pKeyStart = pSectionLine; // next filed start
						++fieldIndex;
						//
						while ('\n' != *pSectionLine && '#' != *pSectionLine)
						{
							if (',' == *pSectionLine)
							{
								pKeyEnd = pSectionLine - 1;
								TrimBlank(pKeyStart, pKeyEnd); fieldVal.clear();
								if (pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd - pKeyStart + 1);
								pList->AddNode(fieldVal);
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
						pKeyEnd = pSectionLine - 1;
						TrimBlank(pKeyStart, pKeyEnd); fieldVal.clear();
						if (pKeyEnd >= pKeyStart) fieldVal.assign(pKeyStart, pKeyEnd - pKeyStart + 1);
						pList->AddNode(fieldVal);
						pSection->AddNode(pList);
						//
						++fieldIndex;
						if (0 == fieldCount) fieldCount = fieldIndex;
						else if (fieldIndex != fieldCount) { ret = -2; goto _END; }; // file format error
						break;
					}
					else
					{
						++pSectionLine;
						if ('\n' == *pSectionLine || '#' == *pSectionLine)
						{
							pKeyEnd = pSectionLine - 1;
							TrimBlank(pKeyStart, pKeyEnd);							
							if (pKeyEnd >= pKeyStart)
							{
								string fieldVal;
								fieldVal.assign(pKeyStart, pKeyEnd - pKeyStart + 1);
								pSection->AddNode(new CINIValueNode(fieldVal));
							}
							break;
						}
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
	if (fd) fclose(fd);
	return ret;
}

}//namespace
