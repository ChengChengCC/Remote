# Microsoft Developer Studio Project File - Name="ServerDll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ServerDll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ServerDll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ServerDll.mak" CFG="ServerDll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ServerDll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ServerDll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ServerDll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERDLL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERDLL_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "ServerDll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERDLL_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SERVERDLL_EXPORTS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ServerDll - Win32 Release"
# Name "ServerDll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\KernelManager.cpp
# End Source File
# Begin Source File

SOURCE=.\Script.rc
# End Source File
# Begin Source File

SOURCE=.\ServerDll.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Talk.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DynamicAPI.h
# End Source File
# Begin Source File

SOURCE=.\KernelManager.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Talk.h
# End Source File
# Begin Source File

SOURCE=.\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "公用函数"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Common.h
# End Source File
# Begin Source File

SOURCE=.\Manager.cpp
# End Source File
# Begin Source File

SOURCE=.\Manager.h
# End Source File
# End Group
# Begin Group "连接请求类"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ClientSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\ClientSocket.h
# End Source File
# End Group
# Begin Group "登陆"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Login.cpp
# End Source File
# Begin Source File

SOURCE=.\Login.h
# End Source File
# End Group
# Begin Group "注册表"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RegEditEx.cpp
# End Source File
# Begin Source File

SOURCE=.\RegEditEx.h
# End Source File
# End Group
# Begin Group "缓冲区"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\Buffer.h
# End Source File
# End Group
# Begin Group "远程终端"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ShellManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ShellManager.h
# End Source File
# End Group
# Begin Group "系统管理"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SystemManager.cpp
# End Source File
# Begin Source File

SOURCE=.\SystemManager.h
# End Source File
# End Group
# Begin Group "远程桌面"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ScreenManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ScreenManager.h
# End Source File
# Begin Source File

SOURCE=.\ScreenSpy.cpp
# End Source File
# Begin Source File

SOURCE=.\ScreenSpy.h
# End Source File
# End Group
# Begin Group "远程文件"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FileManager.cpp
# End Source File
# Begin Source File

SOURCE=.\FileManager.h
# End Source File
# End Group
# Begin Group "远程音频"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Audio.cpp
# End Source File
# Begin Source File

SOURCE=.\Audio.h
# End Source File
# Begin Source File

SOURCE=.\AudioManager.cpp
# End Source File
# Begin Source File

SOURCE=.\AudioManager.h
# End Source File
# End Group
# Begin Group "远程视频"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CaptureVideo.cpp
# End Source File
# Begin Source File

SOURCE=.\CaptureVideo.h
# End Source File
# Begin Source File

SOURCE=.\VideoCap.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoCap.h
# End Source File
# Begin Source File

SOURCE=.\VideoCodec.h
# End Source File
# Begin Source File

SOURCE=.\VideoManager.cpp
# End Source File
# Begin Source File

SOURCE=.\VideoManager.h
# End Source File
# End Group
# Begin Group "远程注册表"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\RegeditOpt.cpp
# End Source File
# Begin Source File

SOURCE=.\RegeditOpt.h
# End Source File
# Begin Source File

SOURCE=.\RegManager.cpp
# End Source File
# Begin Source File

SOURCE=.\RegManager.h
# End Source File
# End Group
# Begin Group "服务管理"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ServerManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ServerManager.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
