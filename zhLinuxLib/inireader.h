#ifndef CONFREADER_H
#define CONFREADER_H
#include <string>
#include <map>
#include <vector>

using std::string;
using std::map;
using std::vector;

namespace NS_ZH_UTIL {

	static const string _EMPTY_STRING_ = "";

class CININode
{
	public:
		CININode() {}
		virtual int GetIntVal		(int &val, int base = 10)		{return -1;}
		virtual int GetUIntVal		(unsigned &val, int base = 10)	{return -1;}
		virtual int GetStringVal	(string &val)					{return -1;}
		virtual int GetStringVal	(char *buf, int buf_len)		{return -1;}
		virtual const string& GetKey()								{return _EMPTY_STRING_; }
		virtual CININode* const operator[](const size_t)			{return NULL;}
		virtual ~CININode() {};
};

class CINIValueNode : public CININode
{
	public:
		CINIValueNode()
		{}
		CINIValueNode(string& value):m_value(value){}
		void SetValue(string& value) {m_value = value;}
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
		CINIKeyValueNode(string& key, string& value):CINIValueNode(value),m_key(key){}
		string& GetKey() {return m_key;}
		void SetKey(string& key) {m_key = key;}
	protected:
		string m_key;
};

class CINIListNode : public CININode
{
public:
	CININode* const operator[](const size_t);
	void AddNode(CININode *child) { m_Nodes.push_back(child); }
	void AddNode(string& val) { return AddNode(new CINIValueNode(val)); }
	~CINIListNode();
	private:
		vector<CININode*> m_Nodes;
};


class CINISection
{
	public:
		CINISection(string& name):m_name(name){}
		virtual void AddNode(CININode* pNode);
		virtual void AddNode(string& key, CININode* pNode);
		virtual CININode* const operator[](const size_t);
		virtual CININode* const operator[](const string&);
		virtual ~CINISection();
		const string& GetName() { return m_name; }
		size_t size() {return m_nodes.size();}
	protected:
		vector<CININode*> m_nodes;
		map<string, CININode*> m_nodesMap;
		string m_name;
};

class CINIConfReader
{
	public:
		CINIConfReader(string &fn);
		CINIConfReader(const char* fn);
		~CINIConfReader();
		//
		int Read();
		CINISection* const operator[](const string&);
		int CurrentLine() {return m_lineNum;}
		void clear();
	private:
		void AddSection(string& name, CINISection* pSection);
		CINISection* AddSection(string& name);
		string m_cfgFileName;
		map<string, CINISection*> m_sections;
		int m_lineNum;
};

}//namespace
#endif // CONFREADER_H
