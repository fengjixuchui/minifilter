//author tomzhou
//email:soundfuture@sohu.com

#include "precom.h"
#include "Ioctlcmd.h"
#include "main.h"


#define		DEVICE_NAME		L"\\device\\sandbox"
#define		LINK_NAME		L"\\dosDevices\\sandbox"

LIST_ENTRY g_PROCESS_LIST_ENTRYList;
FAST_MUTEX	g_PROCESS_LIST_ENTRYListLock;



//����Ӧ�ò��create()����
NTSTATUS DispatchCreate (
	IN PDEVICE_OBJECT	pDevObj,
	IN PIRP	pIrp) 
{
	//����IO״̬��Ϣ
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	//���IRP�����������²���������
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}
//����Ӧ�ò��write()����
NTSTATUS DispatchWrite (
    IN PDEVICE_OBJECT	pDevObj,
    IN PIRP	pIrp) 
{
	NTSTATUS 		status = STATUS_SUCCESS;
	PVOID 		userBuffer;
	PVOID 		drvBuffer;
	ULONG 		xferSize;
	//���IRP��ջ�ĵ�ǰλ��
	PIO_STACK_LOCATION pIrpStack =
		IoGetCurrentIrpStackLocation( pIrp );
	//��õ�ǰд�ĳ��Ⱥͻ���
	xferSize = pIrpStack->Parameters.Write.Length;
	userBuffer = pIrp->AssociatedIrp.SystemBuffer;
	drvBuffer = ExAllocatePoolWithTag(PagedPool, xferSize, 'tseT');
	if (drvBuffer == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		xferSize = 0;
	}
	//����ǰ�����е�����д��
	RtlCopyMemory( drvBuffer, userBuffer, xferSize );
	//���IO����д���״̬�ʹ�������ݳ���
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = xferSize;
	//���IRP�������²㴫��
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return status;
}
//����Ӧ�ò��read()����
NTSTATUS DispatchRead (
    IN PDEVICE_OBJECT	pDevObj,
    IN PIRP	pIrp) 
{
	NTSTATUS 		status = STATUS_SUCCESS;
	PVOID 		userBuffer;
	ULONG 		xferSize;
	//��ȡIRP��ջ�ĵ�ǰλ��
	PIO_STACK_LOCATION pIrpStack =
		IoGetCurrentIrpStackLocation( pIrp );
	//��ȡ������ֽ����ͻ���
	xferSize = pIrpStack->Parameters.Read.Length;
	userBuffer = pIrp->AssociatedIrp.SystemBuffer;	
	//�������ж�����
	RtlCopyMemory( userBuffer, L"Hello, world",
	    xferSize );
	//��дIRP�е����״̬������IRP�����������²㷢��
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = xferSize;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return status;
}
//����Ӧ�ò��DeviceIoControl()
NTSTATUS DispatchControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )
{
    	PIO_STACK_LOCATION      	irpStack;
    	PVOID                   	inputBuffer;
    	PVOID                   	outputBuffer;
    	//PVOID		    			userBuffer;
    	ULONG                   	inputBufferLength;
    	ULONG                   	outputBufferLength;
    	ULONG                   	ioControlCode;
    	NTSTATUS		     		ntstatus;
	
    	ntstatus = Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		//��ȡ��ǰIRP��ջλ��
		irpStack = IoGetCurrentIrpStackLocation (Irp);
		//������뻺��ͳ���
		inputBuffer = Irp->AssociatedIrp.SystemBuffer;
		inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
		//����������ͳ���
		outputBuffer = Irp->AssociatedIrp.SystemBuffer;
		outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
		//��ȡ������
		ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
		switch (irpStack->MajorFunction)
		{
		    case IRP_MJ_CREATE:
		        break;
		    case IRP_MJ_SHUTDOWN:
		        break;
			//�豸������չ
		    case IRP_MJ_DEVICE_CONTROL:
		 		if(IOCTL_TRANSFER_TYPE(ioControlCode) == METHOD_NEITHER) 
				{
		           	outputBuffer = Irp->UserBuffer;
		        }
				//��Բ�ͬ�Ŀ����룬���в�ͬ�Ĳ���
				switch (ioControlCode ) 
				{
					case IOCTL_XXX_YYY:
						DbgPrint("%d\n", *(ULONG *)inputBuffer);
						*(ULONG *)outputBuffer = *(ULONG *)inputBuffer + 1;
						Irp->IoStatus.Information = sizeof(ULONG);
						break;
					default:
						break;
				}
				break;
		    default:
				break;
		}
		IoCompleteRequest( Irp, IO_NO_INCREMENT );
		return ntstatus;  
}
//����Ӧ�ò��close()����
NTSTATUS DispatchClose (
    IN PDEVICE_OBJECT	pDevObj,
    IN PIRP	pIrp) 
{
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

static VOID CleanPROCESS_LIST_ENTRYList()
{
	PPROCESS_LIST_ENTRY PROCESS_LIST_ENTRYEntry = NULL;
	PLIST_ENTRY tmpEntry = NULL;

	while(IsListEmpty(&g_PROCESS_LIST_ENTRYList))
	{
		tmpEntry = RemoveHeadList(&g_PROCESS_LIST_ENTRYList); 
		PROCESS_LIST_ENTRYEntry = CONTAINING_RECORD(tmpEntry, PROCESS_LIST_ENTRY, Entry);
		RemoveEntryList(tmpEntry);
		ExFreePool(PROCESS_LIST_ENTRYEntry);
	}
}
//����Unload��������
VOID DriverUnload (
    IN PDRIVER_OBJECT	pDriverObject) 
{
	UNICODE_STRING         deviceLink;
	PDEVICE_OBJECT	   	p_NextObj;

	freeIPC(); //ipc
	UnloadMiniMonitor(pDriverObject);

	p_NextObj = pDriverObject->DeviceObject;
	if (p_NextObj != NULL)
	{
		RtlInitUnicodeString( &deviceLink, LINK_NAME);
		IoDeleteSymbolicLink( &deviceLink);
		IoDeleteDevice( pDriverObject->DeviceObject );
	}
	CleanPROCESS_LIST_ENTRYList();
	return;
}

//����������ڣ���ɸ��ֳ�ʼ�������������豸����
NTSTATUS DriverEntry (
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath) 
{
	NTSTATUS 		ntStatus;
	PDEVICE_OBJECT 	pDevObj;
	UNICODE_STRING 	uDevName;
	UNICODE_STRING 	uLinkName;
	PROCESS_LIST_ENTRY *pPROCESS_LIST_ENTRYEntry;
	DbgPrint("Driver Load begin!\n");

	ExInitializeFastMutex( &g_PROCESS_LIST_ENTRYListLock );
	InitializeListHead(&g_PROCESS_LIST_ENTRYList);

	pPROCESS_LIST_ENTRYEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_LIST_ENTRY), 'XBBS');
	if (pPROCESS_LIST_ENTRYEntry == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pPROCESS_LIST_ENTRYEntry, sizeof(PROCESS_LIST_ENTRY));

	wcscpy(pPROCESS_LIST_ENTRYEntry->NameBuffer, L"notepad.exe");
	InsertHeadList(&g_PROCESS_LIST_ENTRYList, &pPROCESS_LIST_ENTRYEntry->Entry);


	pPROCESS_LIST_ENTRYEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_LIST_ENTRY), 'XBBS');
	if (pPROCESS_LIST_ENTRYEntry == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pPROCESS_LIST_ENTRYEntry, sizeof(PROCESS_LIST_ENTRY));
	
	wcscpy(pPROCESS_LIST_ENTRYEntry->NameBuffer, L"iexplore.exe");
	InsertHeadList(&g_PROCESS_LIST_ENTRYList, &pPROCESS_LIST_ENTRYEntry->Entry);


	ntStatus = initFileMonitor(pDriverObject);
	
	if(! NT_SUCCESS(ntStatus))
		return ntStatus;
	
	ntStatus = initIPC( );
	
	if(! NT_SUCCESS(ntStatus))
	{
		stopMiniMonitor( );
		return ntStatus;
	}

	//Sandbox����Ҫ��ʼ����
	InitSb();

	ntStatus = startMiniMonitor( );
	
	if(! NT_SUCCESS(ntStatus))
	{
		stopMiniMonitor( );
		closeIPC( );
		return STATUS_SUCCESS;
	}

	//��ʼ����������
	pDriverObject->MajorFunction[IRP_MJ_CREATE] =
				DispatchCreate;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] =
				DispatchClose;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] =
				DispatchWrite;
	pDriverObject->MajorFunction[IRP_MJ_READ] =
				DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = 
				DispatchControl;
	pDriverObject->DriverUnload	= 
				DriverUnload;

	RtlInitUnicodeString(&uDevName, DEVICE_NAME);
	//���������豸
	ntStatus = IoCreateDevice( pDriverObject,
			0,//sizeof(DEVICE_EXTENSION)
			&uDevName,
			FILE_DEVICE_SANDBOX,
			0, TRUE,
			&pDevObj );
	if (!NT_SUCCESS(ntStatus))
	{
		DbgPrint("IoCreateDevice Failed:%x\n", ntStatus);
		return ntStatus;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&uLinkName, LINK_NAME);
	//������������
	ntStatus = IoCreateSymbolicLink( &uLinkName,
		    &uDevName );
	if (!NT_SUCCESS(ntStatus)) 
	{
		//STATUS_INSUFFICIENT_RESOURCES 	��Դ����
		//STATUS_OBJECT_NAME_EXISTS 		ָ������������
		//STATUS_OBJECT_NAME_COLLISION 	�������г�ͻ
		DbgPrint("IoCreateSymbolicLink Failed:%x\n", ntStatus);
		IoDeleteDevice( pDevObj );
		return ntStatus;
	}
	DbgPrint("Driver Load success!\n");
	return ntStatus;
}

