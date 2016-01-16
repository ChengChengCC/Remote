#include "StdAfx.h"
#include "Buffer.h"
#include <math.h>


CBuffer::CBuffer()
{
	m_nSize = 0;
	
	m_pPtr = m_pBase = NULL;
	InitializeCriticalSection(&m_cs);
}


CBuffer::~CBuffer()
{
	if (m_pBase)
	{
		VirtualFree(m_pBase, 0, MEM_RELEASE);
	}
	DeleteCriticalSection(&m_cs);
}


BOOL CBuffer::Write(PBYTE pData, UINT nSize)
{
	EnterCriticalSection(&m_cs);
	if (ReAllocateBuffer(nSize + GetBufferLen()) == -1)
	{
		LeaveCriticalSection(&m_cs);
		return false;
	}
	
	CopyMemory(m_pPtr,pData,nSize);   //动态数组  int[5] = 0  1  2  3  4  5

	m_pPtr+=nSize;
	LeaveCriticalSection(&m_cs);
	return nSize;
}



BOOL CBuffer::Insert(PBYTE pData, UINT nSize)   //插入头   Hello    20   5  HelloWorld 20
{
	EnterCriticalSection(&m_cs);
	if (ReAllocateBuffer(nSize + GetBufferLen()) == -1)
	{
		LeaveCriticalSection(&m_cs);
		return false;
	}
	
	MoveMemory(m_pBase+nSize,m_pBase,GetMemSize() - nSize);
	CopyMemory(m_pBase,pData,nSize);
	
	m_pPtr+=nSize;
	LeaveCriticalSection(&m_cs);
	return nSize;
}



UINT CBuffer::Read(PBYTE pData, UINT nSize)   //[][][][][][][][][][]  20   5
{                                             //[][[]10
	EnterCriticalSection(&m_cs);

	if (nSize > GetMemSize())   //1024                2048   1
	{
		LeaveCriticalSection(&m_cs);
		return 0;
	}
	if (nSize > GetBufferLen())
		nSize = GetBufferLen();

		
	if (nSize)
	{
		
		CopyMemory(pData,m_pBase,nSize);

		MoveMemory(m_pBase,m_pBase+nSize,GetMemSize() - nSize);

		m_pPtr -= nSize;
	}
		
	DeAllocateBuffer(GetBufferLen());   //1

	LeaveCriticalSection(&m_cs);
	return nSize;
}


UINT CBuffer::GetMemSize() 
{
	return m_nSize;
}

UINT CBuffer::GetBufferLen() 
{
	if (m_pBase == NULL)
		return 0;

	int nSize = 
		m_pPtr - m_pBase;
	return nSize;
}

UINT CBuffer::ReAllocateBuffer(UINT nRequestedSize)
{
	if (nRequestedSize < GetMemSize())    //1025    2048
		return 0;

	UINT nNewSize = (UINT)ceil(nRequestedSize / 1024.0) * 1024;
	PBYTE pNewBuffer = (PBYTE) VirtualAlloc(NULL,nNewSize,MEM_COMMIT,PAGE_READWRITE);
	if (pNewBuffer == NULL)
		return -1;

	UINT nBufferLen = GetBufferLen();
	CopyMemory(pNewBuffer,m_pBase,nBufferLen);

	if (m_pBase)
		VirtualFree(m_pBase,0,MEM_RELEASE);   
	                                          //[1][2][3][][]
 
	m_pBase = pNewBuffer;
	m_pPtr = m_pBase + nBufferLen;    

	m_nSize = nNewSize;

	return m_nSize;
}





//[xxxxxxxxxxx][x]
UINT CBuffer::DeAllocateBuffer(UINT nRequestedSize)    //1024    3003030303030030    1025
{
	if (nRequestedSize < GetBufferLen())
		return 0;

	UINT nNewSize = (UINT)ceil(nRequestedSize / 1024.0) * 1024;

	if (nNewSize > GetMemSize())
		return 0;

	PBYTE pNewBuffer = (PBYTE) VirtualAlloc(NULL,nNewSize,MEM_COMMIT,PAGE_READWRITE);

	UINT nBufferLen = GetBufferLen();
	CopyMemory(pNewBuffer,m_pBase,nBufferLen);

	VirtualFree(m_pBase,0,MEM_RELEASE);

	m_pBase = pNewBuffer;

	m_pPtr = m_pBase + nBufferLen;

	m_nSize = nNewSize;  

	return m_nSize;
}


int CBuffer::Scan(PBYTE pScan,UINT nPos)
{
	if (nPos > GetBufferLen() )
		return -1;

	PBYTE pStr = (PBYTE) strstr((char*)(m_pBase+nPos),(char*)pScan);
	
	int nOffset = 0;

	if (pStr)
		nOffset = (pStr - m_pBase) + strlen((char*)pScan);

	return nOffset;
}


void CBuffer::ClearBuffer()
{
	EnterCriticalSection(&m_cs);
	m_pPtr = m_pBase;

	DeAllocateBuffer(1024);
	LeaveCriticalSection(&m_cs);
}


void CBuffer::Copy(CBuffer& buffer)
{
	int nReSize = buffer.GetMemSize();   //2048               2048
	int nSize = buffer.GetBufferLen();   //1025               0     1025
	ClearBuffer();
	if (ReAllocateBuffer(nReSize) == -1)
		return;
	CopyMemory(m_pBase,buffer.GetBuffer(),buffer.GetBufferLen());
	m_pPtr = m_pBase + nSize;
}

PBYTE CBuffer::GetBuffer(UINT nPos)
{
	return m_pBase+nPos;
}

UINT CBuffer::Delete(UINT nSize)//2048    1025 1   1024
{

	if (nSize > GetMemSize())
	{
		return 0;
	}
	if (nSize > GetBufferLen())
		nSize = GetBufferLen();

		
	if (nSize)
	{
		MoveMemory(m_pBase,m_pBase+nSize,GetMemSize() - nSize);

		m_pPtr -= nSize;
	}
		
	DeAllocateBuffer(GetBufferLen());

	return nSize;
}






