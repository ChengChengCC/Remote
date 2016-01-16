// Audio.h: interface for the CAudio class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AUDIO_H__25133B85_A758_4425_B111_24D6FFC79E49__INCLUDED_)
#define AFX_AUDIO_H__25133B85_A758_4425_B111_24D6FFC79E49__INCLUDED_

#if _MSC_VER > 1000
#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#pragma comment(lib, "Winmm.lib")


#endif // _MSC_VER > 1000

class CAudio  
{
public:
	CAudio();
	virtual ~CAudio();

	int m_nBufferLength;
	
	LPBYTE m_lpInAudioData[2]; // 保持声音的连续
	LPBYTE m_lpOutAudioData[2];
	
	HWAVEIN m_hWaveIn;
	int m_nWaveInIndex;
	int m_nWaveOutIndex;
	
	HANDLE	m_hEventWaveIn; 
	HANDLE	m_hStartRecord; 
	
	LPBYTE GetRecordBuffer(LPDWORD lpdwBytes);
	bool PlayBuffer(LPBYTE lpWaveBuffer, DWORD dwBytes);
private:
	HANDLE	m_hThreadCallBack;
	
	LPWAVEHDR m_lpInAudioHdr[2];
	LPWAVEHDR m_lpOutAudioHdr[2];
	
	HWAVEOUT m_hWaveOut;
	
	bool	m_bIsWaveInUsed;
	bool	m_bIsWaveOutUsed;
	GSM610WAVEFORMAT m_GSMWavefmt;
	
	bool InitializeWaveIn();
	bool InitializeWaveOut();
	
	static DWORD WINAPI waveInCallBack(LPVOID lparam);

};

#endif // !defined(AFX_AUDIO_H__25133B85_A758_4425_B111_24D6FFC79E49__INCLUDED_)
