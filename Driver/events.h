#pragma once

bool Connect() {

	process::base_address = (ULONG64)PsGetProcessSectionBaseAddress(process::process);
	print("\n[+] base address:    0x%llX", process::base_address);

	process::STRUCT_OFFSET_ADDRESS = ULONG64(process::base_address + OUTPUT_ADDRESS);
	print("\n[+] struct address:  0x%llX -> offset: 0x%llX", process::STRUCT_OFFSET_ADDRESS, (ULONG64)(process::STRUCT_OFFSET_ADDRESS - process::base_address)); //address where the struct address get stored

	process::STATUS_CODE_ADDRESS = ULONG64(process::base_address + CODE_ADDRESS);
	process::STATUS_CODE_ADDRESS_REAL = readlocal<ULONG64>(process::STATUS_CODE_ADDRESS); //get the real address
	print("\n[+] code address:    0x%llX -> offset: 0x%llX => 0x%llX", process::STATUS_CODE_ADDRESS, (ULONG64)(process::STATUS_CODE_ADDRESS - process::base_address), process::STATUS_CODE_ADDRESS_REAL); //this just saves the address to the status 
	

	int CURRCODE = readlocal<int>(process::STATUS_CODE_ADDRESS_REAL); // read it
	print("\n[+] code: %d", CURRCODE);


	ULONG64 CURRSTRUCT_REAL = readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS);
	int CURRSTRUCT = readlocal<int>(CURRSTRUCT_REAL);
	print("\n[+] struct: %d", CURRSTRUCT);

	if (CURRCODE != 3) {
		return false;
	}
	print("\n[+] connected!");
	status::ONLINE();
	return true;
}

void Disconnect() {
	//print("\n[+] stopping thread and closing...");
	

	PVOID Kthread = reinterpret_cast<PVOID>(KeGetCurrentThread());
	PVOID InitialStack		= (PVOID)((ULONG64)Kthread + GInitialStack);
	PVOID VCreateTime		= (PVOID)((ULONG64)Kthread + GVCreateTime);
	PVOID StartAddress		= (PVOID)((ULONG64)Kthread + GStartAddress);
	PVOID Win32StartAddress	= (PVOID)((ULONG64)Kthread + GWin32StartAddress);
	PVOID KernelStack		= (PVOID)((ULONG64)Kthread + GKernelStack);
	PVOID ExitStatus		= (PVOID)((ULONG64)Kthread + GExitStatus);
	PVOID CID				= (PVOID)((ULONG64)Kthread + GCID);


	//print("\n[+] resetting vars....");
	*(PVOID*)(VCreateTime)			= _VCreateTime;
	*(PVOID*)(StartAddress)			= _StartAddress;
	*(PVOID*)(Win32StartAddress)	= _Win32StartAddress;
	*(PVOID*)(KernelStack)			= _KernelStack;
	*(PVOID*)(ExitStatus)			= _ExitStatus;
	*(PVOID*)(CID)					= _CID;

	print("\n[+] Bye!");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

void InitTarget() {
	process::target_pid = readlocal<ULONG64>(readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS));//double reading because STRUCT_OFFSET_ADDRESS saves a pointer that has pid
	print("\n[+] process:: target_pid: %d", process::target_pid);
	if (process::target_pid != 0) {
		status::SUCESSFUL();
	}
	else {
		status::ERROR();
	}
}

void GetBase() {
	if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)process::target_pid, &process::target_process))) {
		ULONG64 base = (ULONG64)PsGetProcessSectionBaseAddress(process::target_process);
		ULONG64 StructAddress = readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS); //no double because direct writing
		writelocal<ULONG64>(&base, (PVOID)StructAddress);

		print("\n[+] base:  0x%llX", base);

		status::SUCESSFUL();
	}
	else {
		status::ERROR();
	}
}

void GetPeb() {
	if (NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)process::target_pid, &process::target_process))) {
		PPEB peb = PsGetProcessPeb(process::target_process);

		KAPC_STATE state;
		KeStackAttachProcess(process::target_process, &state);

		PPEB_LDR_DATA ldr = peb->Ldr;

		if (!ldr)
		{
			DbgPrintEx(0, 0, "Error pLdr not found£¡ \n");
			KeUnstackDetachProcess(&state);
			status::ERROR();
		}

		for (PLIST_ENTRY listEntry = (PLIST_ENTRY)ldr->ModuleListLoadOrder.Flink;
			listEntry != &ldr->ModuleListLoadOrder;
			listEntry = (PLIST_ENTRY)listEntry->Flink)
		{
			UNICODE_STRING module_name;
			RtlInitUnicodeString(&module_name, L"UnityPlayer.dll");
			PLDR_DATA_TABLE_ENTRY ldrEntry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
			if (RtlCompareUnicodeString(&ldrEntry->BaseDllName, &module_name, TRUE) ==
				0) {
				ULONG64 baseAddr = (ULONG64)ldrEntry->DllBase;
				KeUnstackDetachProcess(&state);

				ULONG64 StructAddress = readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS); //no double because direct writing
				writelocal<ULONG64>(&baseAddr, (PVOID)StructAddress);

				print("\n[+] peb:  0x%llX", (uint64_t)peb);

				status::SUCESSFUL();

				return;

			}

		}
		status::ERROR();
		DbgPrintEx(0, 0, "Error exiting funcion nothing was found found \n");
		KeUnstackDetachProcess(&state);


	}
	else {
		status::ERROR();
	}
}

void Read() {
	readd StructAddress = {};
	SIZE_T BytesRead{ 0 };
	(MmCopyVirtualMemory(process::process, (PVOID)readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS), PsGetCurrentProcess(), &StructAddress, sizeof(readd), KernelMode, &BytesRead));

	//StructAddress = readlocal<readd>(readlocal<ULONG64>(process::STRUCT_OFFSET_ADDRESS)); //double reading because STRUCT_OFFSET_ADDRESS points to the struct and then we read the content

	print("\n[+] read1:  0x%llX , %d", StructAddress.address, StructAddress.size);

	int get1 = readTarget<int>(StructAddress.address);

	if (StructAddress.address < 0x7FFFFFFFFFF && StructAddress.address > 0 && StructAddress.size > 0 && StructAddress.size < 200) {
		read(StructAddress.address, StructAddress.output, StructAddress.size);
		print("\n[+] read inner:    0x%llX", StructAddress.output);
	}
	status::SUCESSFUL(); //we cant check if its unsuccessful

	int get2 = readlocal<int>(StructAddress.address);
}



