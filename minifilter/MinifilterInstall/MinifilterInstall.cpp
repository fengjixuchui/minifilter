// MinifilterInstall.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include <winsvc.h>
#include <stdio.h>
#include <conio.h>

//#include <fltuser.h>
//#include <dontuse.h>


#define DRIVER_NAME "nullfilter"
#define DRIVER_PATH ".\\nullfilter.sys"
#define DRIVER_ALTITUDE	 "370020" //��ͬѧ������������û��ʹ��UNICODE���룬���°�װ����Ч

// SYS�ļ����������ͬ��Ŀ¼��
// InstallDriver(DRIVER_NAME,DRIVER_PATH,DRIVER_ALTITUDE);
// ������������ StartDriver(DRIVER_NAME);
// ֹͣ�������� StopDriver(DRIVER_NAME);
// ж�ط��� DeleteDriver(DRIVER_NAME);

BOOL InstallDriver(const char* lpszDriverName,const char* lpszDriverPath,const char* lpszAltitude)
{
    char    szTempStr[MAX_PATH];
    HKEY    hKey;
    DWORD    dwData;
    char    szDriverImagePath[MAX_PATH];    

    if( NULL==lpszDriverName || NULL==lpszDriverPath )
    {
        return FALSE;
    }
    //�õ�����������·��
    GetFullPathName(lpszDriverPath, MAX_PATH, szDriverImagePath, NULL);

    SC_HANDLE hServiceMgr=NULL;// SCM�������ľ��
    SC_HANDLE hService=NULL;// NT��������ķ�����

    //�򿪷�����ƹ�����
    hServiceMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if( hServiceMgr == NULL ) 
    {
        // OpenSCManagerʧ��
        CloseServiceHandle(hServiceMgr);
        return FALSE;        
    }

    // OpenSCManager�ɹ�  

    //������������Ӧ�ķ���
    hService = CreateService( hServiceMgr,
        lpszDriverName,             // �����������ע����е�����
        lpszDriverName,             // ע������������DisplayName ֵ
        SERVICE_ALL_ACCESS,         // ������������ķ���Ȩ��
        SERVICE_FILE_SYSTEM_DRIVER, // ��ʾ���صķ������ļ�ϵͳ��������
        SERVICE_DEMAND_START,       // ע������������Start ֵ
        SERVICE_ERROR_IGNORE,       // ע������������ErrorControl ֵ
        szDriverImagePath,          // ע������������ImagePath ֵ
        "FSFilter Activity Monitor",// ע������������Group ֵ
        NULL, 
        "FltMgr",                   // ע������������DependOnService ֵ
        NULL, 
        NULL);

    if( hService == NULL ) 
    {        
        if( GetLastError() == ERROR_SERVICE_EXISTS ) 
        {
            //���񴴽�ʧ�ܣ������ڷ����Ѿ�������
            CloseServiceHandle(hService);       // ������
            CloseServiceHandle(hServiceMgr);    // SCM���
            return TRUE; 
        }
        else 
        {
            CloseServiceHandle(hService);       // ������
            CloseServiceHandle(hServiceMgr);    // SCM���
            return FALSE;
        }
    }
    CloseServiceHandle(hService);       // ������
    CloseServiceHandle(hServiceMgr);    // SCM���

    //-------------------------------------------------------------------------------------------------------
    // SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances�ӽ��µļ�ֵ�� 
    //-------------------------------------------------------------------------------------------------------
    strcpy(szTempStr,"SYSTEM\\CurrentControlSet\\Services\\");
    strcat(szTempStr,lpszDriverName);
    strcat(szTempStr,"\\Instances");
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,szTempStr,0,"",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,(LPDWORD)&dwData)!=ERROR_SUCCESS)
    {
        return FALSE;
    }
    // ע������������DefaultInstance ֵ 
    strcpy(szTempStr,lpszDriverName);
    strcat(szTempStr," Instance");
    if(RegSetValueEx(hKey,"DefaultInstance",0,REG_SZ,(CONST BYTE*)szTempStr,(DWORD)strlen(szTempStr))!=ERROR_SUCCESS)
    {
        return FALSE;
    }
    RegFlushKey(hKey);//ˢ��ע���
    RegCloseKey(hKey);
 

    //-------------------------------------------------------------------------------------------------------
    // SYSTEM\\CurrentControlSet\\Services\\DriverName\\Instances\\DriverName Instance�ӽ��µļ�ֵ�� 
    //-------------------------------------------------------------------------------------------------------
    strcpy(szTempStr,"SYSTEM\\CurrentControlSet\\Services\\");
    strcat(szTempStr,lpszDriverName);
    strcat(szTempStr,"\\Instances\\");
    strcat(szTempStr,lpszDriverName);
    strcat(szTempStr," Instance");
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,szTempStr,0,"",REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,(LPDWORD)&dwData)!=ERROR_SUCCESS)
    {
        return FALSE;
    }
    // ע������������Altitude ֵ
    strcpy(szTempStr,lpszAltitude);
    if(RegSetValueEx(hKey,"Altitude",0,REG_SZ,(CONST BYTE*)szTempStr,(DWORD)strlen(szTempStr))!=ERROR_SUCCESS)
    {
        return FALSE;
    }
    // ע������������Flags ֵ
    dwData=0x0;
    if(RegSetValueEx(hKey,"Flags",0,REG_DWORD,(CONST BYTE*)&dwData,sizeof(DWORD))!=ERROR_SUCCESS)
    {
        return FALSE;
    }
    RegFlushKey(hKey);//ˢ��ע���
    RegCloseKey(hKey);

    return TRUE;
}

BOOL StartDriver(const char* lpszDriverName)
{
    SC_HANDLE        schManager;
    SC_HANDLE        schService;

    if(NULL==lpszDriverName)
    {
        return FALSE;
    }

    schManager=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(NULL==schManager)
    {
        CloseServiceHandle(schManager);
        return FALSE;
    }
    schService=OpenService(schManager,lpszDriverName,SERVICE_ALL_ACCESS);
    if(NULL==schService)
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return FALSE;
    }

    if(!StartService(schService,0,NULL))
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        if( GetLastError() == ERROR_SERVICE_ALREADY_RUNNING ) 
        {             
            // �����Ѿ�����
            return TRUE;
        } 
        return FALSE;
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schManager);

    return TRUE;
}

BOOL StopDriver(const char* lpszDriverName)
{
    SC_HANDLE        schManager;
    SC_HANDLE        schService;
    SERVICE_STATUS    svcStatus;
    bool            bStopped=false;

    schManager=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(NULL==schManager)
    {
        return FALSE;
    }
    schService=OpenService(schManager,lpszDriverName,SERVICE_ALL_ACCESS);
    if(NULL==schService)
    {
        CloseServiceHandle(schManager);
        return FALSE;
    }    
    if(!ControlService(schService,SERVICE_CONTROL_STOP,&svcStatus) && (svcStatus.dwCurrentState!=SERVICE_STOPPED))
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return FALSE;
    }
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schManager);

    return TRUE;
}

BOOL DeleteDriver(const char* lpszDriverName)
{
    SC_HANDLE        schManager;
    SC_HANDLE        schService;
    SERVICE_STATUS    svcStatus;

    schManager=OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    if(NULL==schManager)
    {
        return FALSE;
    }
    schService=OpenService(schManager,lpszDriverName,SERVICE_ALL_ACCESS);
    if(NULL==schService)
    {
        CloseServiceHandle(schManager);
        return FALSE;
    }
    ControlService(schService,SERVICE_CONTROL_STOP,&svcStatus);
    if(!DeleteService(schService))
    {
        CloseServiceHandle(schService);
        CloseServiceHandle(schManager);
        return FALSE;
    }
    CloseServiceHandle(schService);
    CloseServiceHandle(schManager);

    return TRUE;
}

int main(int argc, char* argv[])
{
	printf("Print any key to install driver\n");
	getch();
	DeleteDriver(DRIVER_NAME);
	//��װ���������������
	BOOL bRet = InstallDriver(DRIVER_NAME, DRIVER_PATH, DRIVER_ALTITUDE);
	if (bRet == FALSE)
	{
		printf("Driver install failed\n");
		return -1;
	}
	printf("Print any key to start driver\n");
	getch();
	//�������������������
	bRet = StartDriver(DRIVER_NAME);
	if (bRet == FALSE)
	{
		printf("StartDriver failed\n");
		return -1;
	}
	printf("Print any key to stop driver\n");
	getch();
	//ֹͣ�����������
	StopDriver(DRIVER_NAME);
	//ɾ������������
	DeleteDriver(DRIVER_NAME);

	
	return 0;
}
