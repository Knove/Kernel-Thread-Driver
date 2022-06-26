#include "stdafx.h"

DWORD pid;
DWORD64 baseaddress;

int main()
{
	//use your mapper code here:

	//send your driver following stuff: (use kdmapper or something else)

	//					  mdl pointer, STATUS_CODE_ADDRESS, STRUCT_OFFSET_ADDRESS,  pid
	//NTSTATUS EntryPoint(ULONG64 mdl, ULONG64 code,		ULONG64 output,			ULONG64 PID)


	std::cout << "Hello world!" << std::endl;
	pid = GetProcessId(L"UM.exe");

	std::cout << "UM PID:" << pid << std::endl;
	Connect();

	pid = GetProcessId(L"Test.exe");

	std::cout << "TARGET PID:" << pid << std::endl;
	//init our target
	if (!initTarget(pid)) return 1;

	baseaddress = GetBase();

	std::cout << "base: 0x" << std::hex << baseaddress << std::endl;

	//a test



	//uint64_t UWORLD = Read<uint64_t>(0xA0971AFB28 + baseaddress);

	//std::cout << "test1: 0x" << std::hex << UWORLD << std::endl;

	//uint64_t UWORLD1 = Read<uint64_t>(0x13D2EFFA98 + baseaddress);

	//std::cout << "tes2t: 0x" << std::hex << UWORLD1 << std::endl;

	Disconnect();

}