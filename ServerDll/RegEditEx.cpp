#include "StdAfx.h"
#include "RegEditEx.h"


//读取注册表的指定键的数据(Mode:0-读键值数据 1-牧举子键 2-牧举指定键项 3-判断该键是否存在)
int  
ReadRegEx(HKEY MainKey,LPCTSTR SubKey,LPCTSTR Vname,
		  DWORD Type,char *szData,LPBYTE szBytes,DWORD lbSize,int Mode)
{   
	HKEY   hKey;  
	int    ValueDWORD,iResult=0;
	char*  PointStr;  
	char   KeyName[32],ValueSz[MAX_PATH],ValueTemp[MAX_PATH];	
	DWORD  szSize,KnSize,dwIndex=0;	 

	memset(KeyName,0,sizeof(KeyName));
	memset(ValueSz,0,sizeof(ValueSz));
	memset(ValueTemp,0,sizeof(ValueTemp));
	 
	__try
	{
		if(RegOpenKeyEx(MainKey,SubKey,0,KEY_READ,&hKey) != ERROR_SUCCESS)
		{
            iResult = -1;
			__leave;
		}
		switch(Mode)		 
		{
		case 0:
			switch(Type)
			{
			case REG_SZ:
			case REG_EXPAND_SZ:				 
				szSize = sizeof(ValueSz);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)ValueSz,&szSize) == ERROR_SUCCESS)
				{
					strcpy(szData,DelSpace(ValueSz));
					iResult =1;
				}
				break;
			case REG_MULTI_SZ:	
				szSize = sizeof(ValueSz);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)ValueSz,&szSize) == ERROR_SUCCESS)
				{
					for(PointStr = ValueSz; *PointStr; PointStr = strchr(PointStr,0)+1)
					{
					
						strncat(ValueTemp,PointStr,sizeof(ValueTemp));
					    strncat(ValueTemp," ",sizeof(ValueTemp));
					}
					strcpy(szData,ValueTemp);
					iResult =1;
				}
				break;				 			
			case REG_DWORD:
				szSize = sizeof(DWORD);
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,(LPBYTE)&ValueDWORD,&szSize ) == ERROR_SUCCESS)						
				{
					wsprintf(szData,"%d",ValueDWORD);
					iResult =1;
				}
				break;
            case REG_BINARY:
                szSize = lbSize;
				if(RegQueryValueEx(hKey,Vname,NULL,&Type,szBytes,&szSize) == ERROR_SUCCESS)
					iResult =1;
				break;
			}
			break;
		case 1:
			while(1)
			{				 
				memset(ValueSz,0,sizeof(ValueSz));
				szSize   = sizeof(ValueSz);
                if(RegEnumKeyEx(hKey,dwIndex++,ValueSz,&szSize,NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
					break;
                wsprintf(ValueTemp,"[%s]\r\n",ValueSz);
				strcat(szData,ValueTemp);
				iResult =1;
			}			 
			break;
		case 2:			  
			while(1)
			{				 
				memset(KeyName,0,sizeof(KeyName));
				memset(ValueSz,0,sizeof(ValueSz));
				memset(ValueTemp,0,sizeof(ValueTemp));
				KnSize = sizeof(KeyName);
                szSize = sizeof(ValueSz);
                if(RegEnumValue(hKey,dwIndex++,KeyName,&KnSize,NULL,&Type,(LPBYTE)ValueSz,&szSize) != ERROR_SUCCESS)
					break;
				switch(Type)				 				
				{				     
				case REG_SZ:					 						 
					wsprintf(ValueTemp,"%-24s %-15s %s \r\n",KeyName,"REG_SZ",ValueSz);					     
					break;
				case REG_EXPAND_SZ:                   						 
					wsprintf(ValueTemp,"%-24s %-15s %s \r\n",KeyName,"REG_EXPAND_SZ",ValueSz);
					break;
				case REG_DWORD:
					wsprintf(ValueTemp,"%-24s %-15s 0x%x(%d) \r\n",KeyName,"REG_DWORD",ValueSz,int(ValueSz));
					break;
				case REG_MULTI_SZ:
                    wsprintf(ValueTemp,"%-24s %-15s \r\n",KeyName,"REG_MULTI_SZ");
					break;
			    case REG_BINARY:
					wsprintf(ValueTemp,"%-24s %-15s \r\n",KeyName,"REG_BINARY");
					break;
				}
				lstrcat(szData,ValueTemp);
				iResult =1;
			}
			break;
		case 3:
            iResult =1;
			break;
		}
	}
	__finally
	{
        RegCloseKey(MainKey);
		RegCloseKey(hKey);
	}
     
	return iResult;
}


//去除字符串类型前面的空格
char *DelSpace(char *szData)
{
	int i=0;
	while(1)
	{
		if(strnicmp(szData+i," ",1))
			break;
		i++;			
	}
	return (szData+i);
}