#pragma once

class CBuffer  
{
	
protected:
	PBYTE	m_pBase;
	PBYTE	m_pPtr;
	UINT	m_nSize;
protected:
	UINT ReAllocateBuffer(UINT nRequestedSize);
	UINT DeAllocateBuffer(UINT nRequestedSize);
	UINT GetMemSize();
public:
	void ClearBuffer();

	UINT Delete(UINT nSize);
	UINT Read(PBYTE pData, UINT nSize);
	BOOL Write(PBYTE pData, UINT nSize);
	BOOL Write(CString& strData);
	UINT GetBufferLen();
	int Scan(PBYTE pScan,UINT nPos);
	BOOL Insert(PBYTE pData, UINT nSize);
	BOOL Insert(CString& strData);

	void Copy(CBuffer& buffer);	

	PBYTE GetBuffer(UINT nPos=0);

	CBuffer();
	virtual ~CBuffer();

	void FileWrite(const CString& strFileName);

};