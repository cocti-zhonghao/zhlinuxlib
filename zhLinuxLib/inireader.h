#ifndef CONFREADER_H
#define CONFREADER_H
#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;
using std::vector;

namespace NS_ZH_UTIL {

class CININode
{
	public:
		CININode() {}
		virtual int GetIntVal	 (int &val, int base = 10) {return -1;}
		virtual int GetUIntVal	 (unsigned &val, int base = 10) {return -1;}
		virtual int GetStringVal(string &val) {return -1;}
		virtual int GetStringVal(char *buf, int buf_len) {return -1;}
		virtual CININode* const operator[](const size_t) {return NULL;}
		virtual CININode* const operator[](const string&) {return NULL;}
		//virtual void AddNode(CININode* child) {}
		virtual ~CININode() {};
};

class CINIValueNode : public CININode
{
	public:
		CINIValueNode()
		{}
		CINIValueNode(string& value)
		:m_value(value){}
		string& getValue() {return m_value;}
		void setValue(string& value) {m_value = value;}
		void operator=(const string& value) {m_value = value;}
	public:
		int GetIntVal	(int &val, int base = 10);
		int GetUIntVal	(unsigned &val, int base = 10);
		int GetStringVal(string &val);
		int GetStringVal(char *buf, int buf_len);
	protected:
		string m_value;
};

class CINIKeyValueNode : public CINIValueNode
{
	public:
		CINIKeyValueNode()
		{}
		CINIKeyValueNode(string& key, string& value)
		:CINIValueNode(value)
		,m_key(key){}
		string& getKey() {return m_key;}
		void setKey(string& key) {m_key = key;}
	protected:
		string m_key;
};

class CINIListNode : public CININode
{
	public:
		CININode* const operator[](const size_t);
		void AddNode(CINIValueNode& child) {m_Nodes.push_back(child);}
	private:
		vector<CINIValueNode> m_Nodes;
};

class CINISectionNode : public CININode
{
};

class CINIKeyValueSection : public CINISectionNode
{
	public:
		CININode* const operator[](const string&);
		void AddNode(CINIKeyValueNode& child) {m_Nodes[child.getKey()] = child;}
	private:
		map<string, CINIKeyValueNode> m_Nodes;
};

class CINIConfReader;
class CINIListSection : public CINISectionNode
{
	friend class CINIConfReader;
	public:
		CININode* const operator[](const size_t);
		void AddNode(CINIListNode& child) {m_Nodes.push_back(child);}
	private:
		vector<CINIListNode> m_Nodes;
};

class CINIConfReader
{
	public:
		CINIConfReader(string &fn);
		CINIConfReader(const char* fn);
		~CINIConfReader();
		//
		int Read();
		CINISectionNode* const operator[](const string&);
		int CurrentLine() {return m_lineNum;}
		void clear();
	private:
		void AddKeyValueNode(string& section, CINIKeyValueNode& node);
		void AddListValueNode(string& section, CINIValueNode& node, bool newListItem=false);
		string m_cfgFileName;
		map<string, CINISectionNode*> m_sections;
		int m_lineNum;
};

}//namespace
#endif // CONFREADER_H
