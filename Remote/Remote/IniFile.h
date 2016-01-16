#pragma once


class CIniFile  
{
public:
	CIniFile();
	virtual ~CIniFile();
	void	SetIniFileName(CString FileName){ IniFileName = FileName; }
	CString	GetIniFileName(){ return IniFileName; }

	CString	GetString(CString AppName, CString KeyName, CString Default = "");
	int		GetInt(CString AppName, CString KeyName, int Default = 0);
	unsigned long	GetDWORD(CString AppName, CString KeyName, unsigned long Default = 0);

	BOOL	SetString(CString AppName, CString KeyName, CString Data);
	BOOL	SetInt(CString AppName, CString KeyName, int Data);
	BOOL	SetDouble(CString AppName, CString KeyName, double Data);
	BOOL	SetDWORD(CString AppName, CString KeyName, unsigned long Data);
private:
	CString IniFileName;
};