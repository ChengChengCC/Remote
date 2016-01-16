// RegeditOpt.h: interface for the RegeditOpt class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGEDITOPT_H__8E761616_A05D_4733_BA1F_908310CC15F6__INCLUDED_)
#define AFX_REGEDITOPT_H__8E761616_A05D_4733_BA1F_908310CC15F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



enum MYKEY{
    MHKEY_CLASSES_ROOT,
		MHKEY_CURRENT_USER,
		MHKEY_LOCAL_MACHINE,
		MHKEY_USERS,
		MHKEY_CURRENT_CONFIG
};
enum KEYVALUE{
	MREG_SZ,
		MREG_DWORD,
		MREG_BINARY,
		MREG_EXPAND_SZ
};

struct REGMSG{
	int count;         //名字个数
	DWORD size;             //名字大小
	DWORD valsize;     //值大小
	
};

class RegeditOpt  
{
public:
	void SetPath(char *path);
	char* FindKey();
	char* FindPath();
	RegeditOpt();
	RegeditOpt(char b);
	virtual ~RegeditOpt();
	
protected:
	char KeyPath[MAX_PATH];
	HKEY MKEY;

};

#endif // !defined(AFX_REGEDITOPT_H__8E761616_A05D_4733_BA1F_908310CC15F6__INCLUDED_)
