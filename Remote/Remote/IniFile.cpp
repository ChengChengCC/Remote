
#include "stdafx.h"
#include "IniFile.h"


#define MAX_LENGTH 256
CIniFile::CIniFile()
{
	char szAppName[MAX_PATH];
	int  len;

	::GetModuleFileName(NULL, szAppName, sizeof(szAppName));    //得到自身文件名
	len = strlen(szAppName);
	for(int i=len; i>0; i--)                                    //查找扩展名中的'.'
	{
		if(szAppName[i] == '.')
		{
			szAppName[i+1] = '\0';                              //删除这个'.'以后的字符 即  ".exe"
			break;
		}
	}
	strcat(szAppName, "ini");                                   //加入  ".ini"
	IniFileName = szAppName;                                    //赋值给文件名 查看 一个成员函数  IniFileName
}

CIniFile::~CIniFile()
{

}

CString CIniFile::GetString(CString AppName,CString KeyName,CString Default)
{
	TCHAR buf[MAX_LENGTH];
	::GetPrivateProfileString(AppName, KeyName, Default, buf, sizeof(buf), IniFileName);  //最后一个参数就是文件名了
	return buf;
}

int CIniFile::GetInt(CString AppName,CString KeyName,int Default)
{
	return ::GetPrivateProfileInt(AppName, KeyName, Default, IniFileName);
}

unsigned long CIniFile::GetDWORD(CString AppName,CString KeyName,unsigned long Default)
{
	TCHAR buf[MAX_LENGTH];
	CString temp;
	temp.Format("%u",Default);
	::GetPrivateProfileString(AppName, KeyName, temp, buf, sizeof(buf), IniFileName);
	return atol(buf);
}

BOOL CIniFile::SetString(CString AppName,CString KeyName,CString Data)
{
	return ::WritePrivateProfileString(AppName, KeyName, Data, IniFileName);
}

BOOL CIniFile::SetInt(CString AppName,CString KeyName,int Data)
{
	CString temp;
	temp.Format("%d", Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}

BOOL CIniFile::SetDouble(CString AppName,CString KeyName,double Data)
{
	CString temp;
	temp.Format("%f",Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}

BOOL CIniFile::SetDWORD(CString AppName,CString KeyName,unsigned long Data)
{
	CString temp;
	temp.Format("%u",Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}