#include "defines.h"
#include "utilities.h"



void Check() {
	// PsGetProcessId
	PEPROCESS umProcess = 0;
	process_by_name( "UM.exe", &umProcess);
	ULONG64 PID = (ULONG64)PsGetProcessId(umProcess);
	print("[+] PID: %d", PID);
	process::pid = PID;

	while (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)process::pid, &process::process))) {
		ObDereferenceObject(process::process);
		sleep(995);
	}
	//process::pid = reinterpret_cast<ULONG>(PsGetProcessId(process::process));
	print("\n[+] found process! Pid: %i", process::pid);

}

template< typename T >
void SpoofAddress(PVOID address, PVOID* save, PVOID target = 0) {
	*save = *(PVOID*)(address); //save current stuff
	//print("\n[+] saved orig data");
	*(PVOID*)(address) = target; //overwrite
	T spoof = *(T*)((ULONG64)address);
	//print("\n[+] spoofed! -> 0x%llX (%d)", spoof, spoof);
}

void HideThread() {

	//this is not needed lol, i just use this to get a random address ill set the start address to
	PVOID ntoskrnlbase = (PVOID)((ULONG64)get_system_module_base(skCrypt("\\SystemRoot\\system32\\ntoskrnl.exe")) + 0x23810a);
	//PVOID ntoskrnlbase = (PVOID)((ULONG64)get_system_module_base("\\SystemRoot\\system32\\ntoskrnl.exe"));
	//print("\n[+] Base: 0x%llX", ntoskrnlbase);
	PVOID Kthread = reinterpret_cast<PVOID>(KeGetCurrentThread());
	//print("\n[+] Kthread: 0x%llX", Kthread);

	PVOID InitialStack		= (PVOID)((ULONG64)Kthread + GInitialStack);
	PVOID VCreateTime		= (PVOID)((ULONG64)Kthread + GVCreateTime);
	PVOID StartAddress		= (PVOID)((ULONG64)Kthread + GStartAddress);
	PVOID Win32StartAddress = (PVOID)((ULONG64)Kthread + GWin32StartAddress);
	PVOID KernelStack		= (PVOID)((ULONG64)Kthread + GKernelStack);
	PVOID CID				= (PVOID)((ULONG64)Kthread + GCID);
	PVOID ExitStatus		= (PVOID)((ULONG64)Kthread + GExitStatus);
	
	//print("\n[+] CreateTime: 0x%llX", VCreateTime);
	SpoofAddress<LARGE_INTEGER>(VCreateTime, &_VCreateTime, (PVOID)2147483247);

	//print("\n[+] StartAddress: 0x%llX", StartAddress);
	SpoofAddress<void*>(StartAddress, &_StartAddress, ntoskrnlbase);

	//print("\n[+] Win32StartAddress: 0x%llX", Win32StartAddress);
	SpoofAddress<void*>(Win32StartAddress, &_Win32StartAddress, ntoskrnlbase);

	//print("\n[+] KernelStack: 0x%llX", KernelStack);
	SpoofAddress<LARGE_INTEGER>(KernelStack, &_KernelStack);

	SpoofAddress<CLIENT_ID>(CID, &_CID);


	SpoofAddress<LONG>(ExitStatus, &_ExitStatus);

}

int errors = 0;
int CheckCode() {

	return readlocal<int>(process::STATUS_CODE_ADDRESS_REAL);
}
OSVERSIONINFOW GetOSVersion() {
	OSVERSIONINFOW OSInfo{ 0 };
	RtlGetVersion(&OSInfo);
	return OSInfo;
}

void mainthread()
{
	//KeSetBasePriorityThread(KeGetCurrentThread(), 31);

	auto OsInfo = GetOSVersion();

	if (OsInfo.dwBuildNumber > 19000) { //above 1909
		GInitialStack		= InitialStack_UP;
		GVCreateTime		= VCreateTime_UP;
		GStartAddress		= StartAddress_UP;
		GWin32StartAddress	= Win32StartAddress_UP;
		GImageFileName		= ImageFileName_UP;
		GActiveThreads		= ActiveThreads_UP;
		GActiveProcessLinks = ActiveProcessLinks_UP;
		GKernelStack		= KernelStack_UP;
		GExitStatus			= ExitStatus_UP;
		GCID				= CID_UP;
	}
	else {
		GInitialStack		= InitialStack_1909;
		GVCreateTime		= VCreateTime_1909;
		GStartAddress		= StartAddress_1909;
		GWin32StartAddress	= Win32StartAddress_1909;
		GImageFileName		= ImageFileName_1909;
		GActiveThreads		= ActiveThreads_1909;
		GActiveProcessLinks = ActiveProcessLinks_1909;
		GKernelStack		= KernelStack_1909;
		GExitStatus			= ExitStatus_1909;
		GCID				= CID_1909;
	}

	HideThread();
	//print("\n[+] waiting for program");

	sleep(1000);
	Check();
	sleep(3000);
	if (Connect()) {
		while (true) {
			int code = CheckCode();
			switch (code) {
			case 3:
				Disconnect();
				break;
			case 4: 
				Read();
				break;
			case 5:
				GetBase();
				break;
			case 6:
				InitTarget();
				break;
			case 7:
				GetPeb();
				break;
			default:
				break;
			}
		}
	}
	else {
		//print("\n[+] Failed to connect! Disconnecting....");
		Disconnect();
	}


}
void* get_sys_module(const char* module_name)
{
	void* module_base = 0;
	ULONG bytes = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, bytes, &bytes);

	if (!bytes) { return NULL; }

	PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 0x4e425151);

	status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);

	if (!NT_SUCCESS(status)) { return NULL; }

	PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;

	for (ULONG i = 0; i < modules->NumberOfModules; i++)
	{
		if (strcmp((char*)module[i].FullPathName, module_name) == 0)
		{
			module_base = module[i].ImageBase;
			break;
		}
	}

	if (modules) { ExFreePoolWithTag(modules, 0x4e425151); }

	if (module_base <= 0) { return NULL; }

	return module_base;
}

void* get_sys_module_export(const char* module_name, const char* function_name)
{
	void* module = get_sys_module(module_name);

	if (!module) { return NULL; }

	return RtlFindExportedRoutineByName(module, function_name);
}

extern "C"
NTSTATUS EntryPoint(const PMDL mdl)
{
	ULONG64 code = 0x2cc80;
	ULONG64 output = 0x2cc88;

	print("[+] START!");

	OUTPUT_ADDRESS = output;
	CODE_ADDRESS = code;


	if (!null_pfn(mdl)) {
		return STATUS_UNSUCCESSFUL;
	}

	HANDLE thread_handle = nullptr;
	print("[+] code: %d", code);
	print("[+] output: %d", output);

	OBJECT_ATTRIBUTES object_attribues{ };
	InitializeObjectAttributes(&object_attribues, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);

	NTSTATUS status = PsCreateSystemThread(&thread_handle, 0, &object_attribues, nullptr, nullptr, reinterpret_cast<PKSTART_ROUTINE>(&mainthread), nullptr);
	//ZwClose(thread_handle);


	print("\n[+] Bye bye DriverEntry!");
	return STATUS_UNSUCCESSFUL;
}

//NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
//
//	UNREFERENCED_PARAMETER(driver_object);
//	UNREFERENCED_PARAMETER(registry_path);
//
//	//print("\n[+] Started driver!");
//
//	HANDLE thread_handle = nullptr;
//	OBJECT_ATTRIBUTES object_attribues{ };
//	InitializeObjectAttributes(&object_attribues, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);
//
//	NTSTATUS status = PsCreateSystemThread(&thread_handle, 0, &object_attribues, nullptr, nullptr, reinterpret_cast<PKSTART_ROUTINE>(&mainthread), nullptr);
//	//print("\n[+] Bye bye DriverEntry!");
//	return STATUS_SUCCESS;
//
//}


