#include "stdafx.h"

DWORD pid;
DWORD targetPid;
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

	targetPid = GetProcessId(L"Test.exe");

	std::cout << "TARGET PID:" << targetPid << std::endl;

	Connect();


	//init our target
	if (!initTarget(targetPid)) return 1;

	baseaddress = GetBase();

	std::cout << "base: 0x" << std::hex << baseaddress << std::endl;

	//a test

	DWORD64 findNum = 0x00044D48FF934;
	std::cout << "findNum: 0x" << std::hex << findNum << std::endl;

	int UWORLD = Read<int>(findNum);

	std::cout << "test2: " << UWORLD << std::endl;

	//uint64_t UWORLD1 = Read<uint64_t>(0x13D2EFFA98 + baseaddress);

	//std::cout << "tes2t: 0x" << std::hex << UWORLD1 << std::endl;

	Disconnect();

	int status;

	std::cin >> status;
}