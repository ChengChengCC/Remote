#pragma once


int  
ReadRegEx(HKEY MainKey,LPCTSTR SubKey,LPCTSTR Vname,DWORD Type,
		  char *szData,LPBYTE szBytes,DWORD lbSize,int Mode);


//去除字符串类型前面的空格
char *DelSpace(char *szData);
