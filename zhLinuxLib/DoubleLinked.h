#ifndef _DOUBLE_LINKED_LIST_
#define _DOUBLE_LINKED_LIST_
#include <assert.h>
//namespace NS_ZH_UTIL {
template<typename T>
class CDoubleLinkedNode
{
public:
	CDoubleLinkedNode(T data):m_pPre(0),m_pNext(0),m_data(data) {}
	CDoubleLinkedNode<T>* m_pPre;
	CDoubleLinkedNode<T>* m_pNext;
	T& Data() {return m_data;}
private:
	T m_data;
};

template<typename T>
class CDoubleLinkedList
{
public:
	CDoubleLinkedList()
	:m_pHeader(0)
	,m_pTailer(0){}
	void push_back(CDoubleLinkedNode<T> *pData)
	{
		if(m_pTailer == 0)
		{
			m_pHeader = m_pTailer = pData;
		}
		else
		{
			pData->m_pPre = m_pTailer;
			m_pTailer->m_pNext = pData;
		}
	}
	void erase(CDoubleLinkedNode<T> *pData)
	{
		if(pData == m_pHeader)
		{
			assert(pData->m_pPre == 0);
			//
			m_pHeader = pData->m_pNext;
			//Çå0
			pData->m_pNext = 0;
			//
			if(m_pHeader) m_pHeader->m_pPre = 0;
			else
			{
				m_pTailer = 0;
			}
		}
		else if(pData == m_pTailer)
		{
			assert(pData->m_pNext == 0);
			//
			m_pTailer = pData->m_pPre;
			//Çå0
			pData->m_pPre = 0;
			//
			if(m_pTailer) m_pTailer->m_pNext = 0;
			else
			{
				m_pHeader = 0;
			}
		}
		else
		{
			assert(pData->m_pPre != 0 && pData->m_pNext != 0);
			//
			pData->m_pPre->m_pNext = pData->m_pNext;
			pData->m_pNext->m_pPre = pData->m_pPre;
			//Çå0
			pData->m_pPre=pData->m_pNext = 0;
		}
	}
	CDoubleLinkedNode<T>* header() {return m_pHeader;}
	CDoubleLinkedNode<T>* next(CDoubleLinkedNode<T>* pPre) {return pPre->m_pNext;}
private:
	CDoubleLinkedNode<T> *m_pHeader;
	CDoubleLinkedNode<T> *m_pTailer;
};
//}
#endif
