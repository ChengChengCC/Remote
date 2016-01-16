#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#pragma comment(lib, "Winmm.lib")

class CAudio
{
public:
	CAudio(void);
	~CAudio(void);
	int m_nBufferLength;

	LPBYTE m_lpInAudioData[2]; 
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

