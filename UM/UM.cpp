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
// offset 
uint64_t offs_gameObjectManager = 0x17FFD28; // UPDATED 7/3/2022
uint64_t offs_active_objects = 0x18; // UPDATED 7/4/2022
namespace EFTStructs
{
	struct BaseObject
	{
		uint64_t previousObjectLink; //0x0000
		uint64_t nextObjectLink; //0x0008
		uint64_t object; //0x0010
	};

	struct GameObjectManager
	{
		uint64_t lastTaggedObject; //0x0000
		uint64_t taggedObjects; //0x0008
		uint64_t lastActiveObject; //0x0010
		uint64_t activeObjects; //0x0018
	}; //Size: 0x0010

	class ListInternal
	{
	public:
		char pad_0x0000[0x20]; //0x0000
		uintptr_t* firstEntry; //0x0020 
	}; //Size=0x0028

	class List
	{
	public:
		char pad_0x0000[0x10]; //0x0000
		ListInternal* listBase; //0x0010 
		__int32 itemCount; //0x0018 
	}; //Size=0x001C
}

struct
{
	static constexpr uint64_t itemList = 0x48; //UPDATED 1/11/2020
	static constexpr uint64_t registeredPlayers = 0x0080; //UPDATED 1/7/2021
	static constexpr uint64_t m_LocalPlayerID = 0x30;
} localGameWorld_offsets;


// EFT 读取 Chain
uint64_t readEFTChain(uint64_t base, const std::vector<uint64_t>& offsets) {
	uint64_t result = Read<uint64_t>(base + offsets.at(0));
	for (int i = 1; i < offsets.size(); i++) {
		result = Read<uint64_t>(result + offsets.at(i));
	}
	return result;
}

uint64_t GetObjectFromList(uint64_t listPtr, uint64_t lastObjectPtr, const char* objectName)
{
	using EFTStructs::BaseObject;
	char name[256];
	uint64_t classNamePtr = 0x0;

	BaseObject activeObject = Read<BaseObject>(listPtr);
	BaseObject lastObject = Read<BaseObject>(lastObjectPtr);
	std::cout << "GAME || activeObject.object :" << activeObject.object << std::endl;
	if (activeObject.object != 0x0)
	{
		while (activeObject.object != 0 && activeObject.object != lastObject.object)
		{
			classNamePtr = Read<uint64_t>(activeObject.object + 0x60);
			name[256] = Read<char>(classNamePtr + 0x0);
			std::cout << "name1 :" << name << std::endl;
			//driver::read_memory(classNamePtr + 0x0, &name, sizeof(name));
			if (strcmp(name, objectName) == 0)
			{
				return activeObject.object;
			}

			activeObject = Read<BaseObject>(activeObject.nextObjectLink);
		}
	}
	if (lastObject.object != 0x0)
	{
		classNamePtr = Read<uint64_t>(lastObject.object + 0x60);
		name[256] = Read<char>(classNamePtr + 0x0);
		string name1 = GetUnicodeString(classNamePtr + 0x0);
		std::cout << "name2 :" << name << "|" << name1 << std::endl;
		if (strcmp(name, objectName) == 0)
		{
			return lastObject.object;
		}
	}

	return uint64_t();
}

void Init(uint64_t address) {

	//uint64_t gameObjectManager = Read<uint64_t>(1);

	//uint64_t unityPlayerBaseAddress = GetUnityPlayerBaseAddress(address, "UNITYPLAYER.DLL");
	uint64_t unityPlayerBaseAddress = address;
	std::cout << "GAME || unityPlayerBaseAddress :" << address << std::endl;

	uint64_t gameObjectManager = Read<uint64_t>(address + offs_gameObjectManager);
	std::cout << "GAME || gameObjectManager :" << gameObjectManager << std::endl;


	//const auto last_active = Read<uintptr_t>(gameObjectManager + 0x10);
	//const auto active_objects = Read<uintptr_t>(gameObjectManager + 0x18);
	//std::cout << "GAME || active_objects : last_active:" << last_active << "active_objects:" << active_objects << std::endl;
	//uint64_t gameWorld = GetObjectFromList(active_objects, last_active, _xor_("GameWorld"));
	//std::cout << "GAME || gameWorld :" << gameWorld << std::endl;

	auto active_objects = Read<std::array<uint64_t, 2>>(gameObjectManager + offsetof(EFTStructs::GameObjectManager, lastActiveObject));
	std::cout << "GAME || active_objects : last_active:" << active_objects[0] << "active_objects:" << active_objects[1] << std::endl;

	uint64_t gameWorld = GetObjectFromList(active_objects[1], active_objects[0], _xor_("GameWorld"));
	std::cout << "GAME || gameWorld :" << gameWorld << std::endl;

	//uint64_t buffer_gom = Read<uint64_t>(gameObjectManager + 0x28);
	//uint64_t gameWorld = 0;
	//if (buffer_gom != 0)
	//{
	//	int loop_count = 1000;
	//	for (int i = 0; i < loop_count; i++)
	//	{
	//		uint64_t object_ptr = Read<uint64_t>(buffer_gom + 0x10);
	//		uint64_t object_name_ptr = Read<uint64_t>(object_ptr + 0x60);
	//		std::cout << "GAME || find Str" << GetUnicodeString(object_name_ptr) << std::endl;
	//		if (GetUnicodeString(object_name_ptr) == "GameWorld")
	//		{
	//			gameWorld = object_name_ptr;
	//			break;
	//		}
	//	}
	//}
	//else {
	//	std::cout << "GAME || buffer_gom find fail :" << std::endl;
	//}

	uint64_t localGameWorld = readEFTChain(gameWorld, { 0x30, 0x18, 0x28 });
	std::cout << "GAME || localGameWorld :" << localGameWorld << std::endl;

	// 读取玩家信息
	uint64_t onlineusers = Read<uint64_t>(localGameWorld + localGameWorld_offsets.registeredPlayers);
	std::cout << "GAME || onlineusers :" << onlineusers << std::endl;


	uint64_t list_base = Read<uint64_t>(onlineusers + offsetof(EFTStructs::List, listBase));
	int player_count = Read<int>(onlineusers + offsetof(EFTStructs::List, itemCount));
	std::cout << "GAME || player_count :" << player_count << std::endl;

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
	if (targetPid != 0) {
		std::cout << "INIT TAEGET...." << std::endl;

		if (!initTarget(targetPid)) return 1;

		baseaddress = GetBase();

		std::cout << "base: 0x" << std::hex << baseaddress << std::endl;

		pebaddress = GetPeb();

		std::cout << "peb: 0x" << std::hex << pebaddress << std::endl;

		Init(pebaddress);
	}


	//DWORD64 findNum = 0x00044D48FF934;
	//std::cout << "findNum: 0x" << std::hex << findNum << std::endl;

	//int UWORLD = Read<int>(findNum);

	//std::cout << "test2: " << UWORLD << std::endl;

	//uint64_t UWORLD1 = Read<uint64_t>(0x13D2EFFA98 + baseaddress);

	//std::cout << "tes2t: 0x" << std::hex << UWORLD1 << std::endl;

	
	
	
	int status = 999;
	
	while (status == 999)
	{
		int flag = 0;
		std::cin >> flag;
		// 关闭驱动
		if (flag == 1)
		{
			Disconnect();
		}
		// 驱动重置
		if (flag == 2)
		{
			Restart();
		}
	}
	


	
}

