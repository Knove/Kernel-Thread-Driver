#include "stdafx.h"

DWORD pid;
DWORD targetPid;
DWORD64 baseaddress;
DWORD64 pebaddress;

int _baseAddressOffset = 0x020;
int _intialNamePtrOffset = 0x048;
int _actualNamePtrOffset = 0x008;

uint64_t GetUnityPlayerBaseAddress(uint64_t address, string moduleName)
{
	uint64_t unityPlayerBaseAddress = address;

	/* Pointer to PEB_LDR_DATA */
	// 0x18 offset _peb_ldr_data_offset 
	uint64_t peb_ldr_data = Read<uint64_t>(address + 0x18);
	std::cout << "peb_ldr_data: 0x" << std::hex << peb_ldr_data << std::endl;
	/* Pointer to Head FLINK InMemoryOrderModuleList  */
	// 0x020 offset _inMemoryOrderModuleList_flink_head_offset 
	uint64_t InMemoryOrderModuleList_FirstModule = Read<uint64_t>(peb_ldr_data + 0x020);
	std::cout << "InMemoryOrderModuleList_FirstModule: 0x" << std::hex << InMemoryOrderModuleList_FirstModule << std::endl;

	/* Pointer to list entry linked to FLINK above - we'll start here  */
	uint64_t InMemoryOrderModuleList_CurrentModule = Read<uint64_t>(InMemoryOrderModuleList_FirstModule);
	std::cout << "InMemoryOrderModuleList_CurrentModule : 0x" << std::hex << InMemoryOrderModuleList_FirstModule << std::endl;




	while (InMemoryOrderModuleList_CurrentModule != InMemoryOrderModuleList_FirstModule)
	{
		uint64_t intialNamePtr = InMemoryOrderModuleList_CurrentModule + _intialNamePtrOffset;
		uint64_t actualNamePtr = Read<uint64_t>(intialNamePtr + _actualNamePtrOffset);
		std::cout << "actualNamePtr : 0x" << std::hex << actualNamePtr << std::endl;
		string actualName = GetUnicodeString(actualNamePtr);
		std::cout << "actualName :" << actualName << std::endl;

		break;
	}

	return unityPlayerBaseAddress;

}


void Init(uint64_t address) {

	//uint64_t gameObjectManager = Read<uint64_t>(1);

	uint64_t unityPlayerBaseAddress = GetUnityPlayerBaseAddress(address, "UNITYPLAYER.DLL");

}

void Start() {

}


int main()
{
	//use your mapper code here:

	//send your driver following stuff: (use kdmapper or something else)

	//					  mdl pointer, STATUS_CODE_ADDRESS, STRUCT_OFFSET_ADDRESS,  pid
	//NTSTATUS EntryPoint(ULONG64 mdl, ULONG64 code,		ULONG64 output,			ULONG64 PID)


	std::cout << "Hello world!!" << std::endl;
	pid = GetProcessId(L"UM.exe");

	std::cout << "UM PID:" << pid << std::endl;

	targetPid = GetProcessId(L"EscapeFromTarkov.exe");

	std::cout << "TARGET PID:" << targetPid << std::endl;

	Connect();


	//init our target
	if (!initTarget(targetPid)) return 1;

	baseaddress = GetBase();

	std::cout << "base: 0x" << std::hex << baseaddress << std::endl;

	pebaddress = GetPeb();

	std::cout << "peb: 0x" << std::hex << pebaddress << std::endl;

	Init(pebaddress);

	//DWORD64 findNum = 0x00044D48FF934;
	//std::cout << "findNum: 0x" << std::hex << findNum << std::endl;

	//int UWORLD = Read<int>(findNum);

	//std::cout << "test2: " << UWORLD << std::endl;

	//uint64_t UWORLD1 = Read<uint64_t>(0x13D2EFFA98 + baseaddress);

	//std::cout << "tes2t: 0x" << std::hex << UWORLD1 << std::endl;

	
	Disconnect();
	
	int status = 0;
	std::cin >> status;
	while (status)
	{
		std::cin >> status;
		if (status == 0)
		{
			
			status = 1;
		}
	}
	


	
}

