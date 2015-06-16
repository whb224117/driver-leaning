//_stdcall
#include "mini_ddk.h"
#include "Mem.h"
#include "linktype_sys.h"
#include "38.h"
#pragma  INITCODE
JMPCODE oldCode;//用来保存前5字节 以便恢复
PJMPCODE pcur;
bool ishook=false;
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject,PUNICODE_STRING B) //TYPEDEF LONG NTSTATUS
{ // EXCEPTION_EXECUTE_HANDLER
	ULONG cur,old;
	JMPCODE JmpCode;
	//CharTest();
	//StringInitTest();
	//StringCopyTest();
	//StringToUpperTest() ;
	//StringToIntegerTest();
	StringConverTest();
   //ProbeForRead_Test();
	//ASSERT_test();
	//test1();
  // Memaccess_Test();
	//Link_Test();
	//memtest();
	cur=GetNt_CurAddr();//A
	old=GetNt_OldAddr();//C
	if (cur!=old)
	{   //hook
		ishook=true;
		//保存前5字节
		pcur=(PJMPCODE)(cur);//初始化指针
		oldCode.E9=pcur->E9;//1字节
		oldCode.JMPADDR=pcur->JMPADDR;//4字节
		//

		JmpCode.E9=0xE9; 
		JmpCode.JMPADDR=old-cur-5;
	    KdPrint(("要写入的地址%X",JmpCode.JMPADDR));
        //写入JMP   C-A-5=B //实际要写入地址
		__asm //去掉页面保护
		{
			cli
				mov eax,cr0
				and eax,not 10000h //and eax,0FFFEFFFFh
				mov cr0,eax

		}
         
		 pcur->E9=0xE9;//jmp
		 pcur->JMPADDR=JmpCode.JMPADDR;//要跳转到的地址

		__asm //恢复页保护
		{
			mov eax,cr0
				or  eax,10000h //or eax,not 0FFFEFFFFh
				mov cr0,eax
				sti
		}

		KdPrint(("NtOpenProcess被HOOK了"));
	}
 /* ULONG SSDT_NtOpenProcess_Cur_Addr;
KdPrint(("驱动成功被加载...OK++++++++\n\n"));
 //读取SSDT表中 NtOpenProcess当前地址 KeServiceDescriptorTable
 // [[KeServiceDescriptorTable]+0x7A*4] 

__asm
{    int 3
	push ebx
	push eax
		mov ebx,KeServiceDescriptorTable
		mov ebx,[ebx] //表的基地址
		mov eax,0x7a
		shl eax,2//0x7A*4 //imul eax,eax,4//shl eax,2
		add ebx,eax//[KeServiceDescriptorTable]+0x7A*4
		mov ebx,[ebx]
        mov SSDT_NtOpenProcess_Cur_Addr,ebx
	pop  eax	
	pop  ebx
}
KdPrint(("SSDT_NtOpenProcess_Cur_Addr=%x\n\n",SSDT_NtOpenProcess_Cur_Addr));*/
 //注册派遣函数
pDriverObject->MajorFunction[IRP_MJ_CREATE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_READ]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_CLOSE]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=ddk_DispatchRoutine_CONTROL; //IRP_MJ_CREATE相关IRP处理函数
 CreateMyDevice(pDriverObject);//创建相应的设备
 pDriverObject->DriverUnload=DDK_Unload;
  
return (1);
}
//#pragma code_seg("PAGE")
#pragma PAGECODE
VOID DDK_Unload (IN PDRIVER_OBJECT pDriverObject)
{
  PDEVICE_OBJECT pDev;//用来取得要删除设备对象
  UNICODE_STRING symLinkName; // 
  
  if (ishook)
  {//unhook


  __asm //去掉页面保护
  {
	  cli
		  mov eax,cr0
		  and eax,not 10000h //and eax,0FFFEFFFFh
		  mov cr0,eax

  }

 
 pcur->E9= oldCode.E9;//1字节
 pcur->JMPADDR= oldCode.JMPADDR;//4字节
  __asm //恢复页保护
  {
	  mov eax,cr0
		  or  eax,10000h //or eax,not 0FFFEFFFFh
		  mov cr0,eax
		  sti
  }
  } //end unhook
  pDev=pDriverObject->DeviceObject;
  IoDeleteDevice(pDev); //删除设备
  
  //取符号链接名字
   RtlInitUnicodeString(&symLinkName,L"\\??\\My_DriverLinkName");
  //删除符号链接
   IoDeleteSymbolicLink(&symLinkName);
 KdPrint(("驱动成功被卸载...OK-----------")); //sprintf,printf
 //取得要删除设备对象
//删掉所有设备
 DbgPrint("卸载成功");
}
#pragma PAGECODE
NTSTATUS ddk_DispatchRoutine_CONTROL(IN PDEVICE_OBJECT pDevobj,IN PIRP pIrp	)
{   //
	ULONG info;
	//得到当前栈指针
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG mf=stack->MajorFunction;//区分IRP
	switch (mf)
	{
	case IRP_MJ_DEVICE_CONTROL:
		{ 	KdPrint(("Enter myDriver_DeviceIOControl\n"));
		NTSTATUS status = STATUS_SUCCESS;	

		//得到输入缓冲区大小
		ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
		//得到输出缓冲区大小
		ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
		//得到IOCTL码
		ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
		switch (code)
		{ 
		case add_code:
			{  		
				int a,b;
				KdPrint(("add_code  Direct mode \n"));
				//缓冲区方式IOCTL
				//获取缓冲区数据	a,b		
				int * InputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
				//KdPrint(("Port内核映射地址:=%x \n",InputBuffer));
				_asm
				{
					   mov eax,InputBuffer
						mov ebx,[eax]
						mov a,ebx
						mov ebx,[eax+4]
						mov b,ebx
				}
				KdPrint(("a=%d,b=%d \n", a,b));

				a=a+b;
				//C、驱动层返回数据至用户层
				//操作输出缓冲区
				//int* OutputBuffer = (int*)pIrp->AssociatedIrp.SystemBuffer;
				
				int OutputBuffer =(int)MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
					NormalPagePriority
					);
				int ep=(int)PsGetCurrentProcess();		
				 int* pid=(int*)(ep+0x84);//Xp系统下的进程ID偏移
				 PTSTR pname=(PTSTR)(ep+0x174);
				
				KdPrint(("Pid=%d ,,--%s-- ,Buffret内核映射地址:=%x \n",*pid,pname,OutputBuffer));
				_asm
				{
					    mov eax,a
						mov ebx,OutputBuffer
						mov [ebx],eax //bufferet=a+b

				}
				KdPrint(("a+b=%d \n",a));

				//设置实际操作输出缓冲区长度
				info = 4;
				break;
			}
		case sub_code:
			{
				break;
			}
		}//end code switch
		break;
		}
	case IRP_MJ_CREATE:
		{
			break;
		}
	case IRP_MJ_CLOSE:
		{
			break;
		}
	case IRP_MJ_READ:
		{
			break;
		}

	}

	//对相应的IPR进行处理
	pIrp->IoStatus.Information=info;//设置操作的字节数为0，这里无实际意义
	pIrp->IoStatus.Status=STATUS_SUCCESS;//返回成功
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);//指示完成此IRP
	KdPrint(("离开派遣函数\n"));//调试信息
	return STATUS_SUCCESS; //返回成功
}