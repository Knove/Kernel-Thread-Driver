#pragma once
#include "stdafx.h"

void* STATUS_CODE_ADDRESS;
void* STRUCT_OFFSET_ADDRESS;

/*
0 = connected
1 = success
2 = error
3 = disconnected
4 = reading
5 = getting base
6 = init target
*/
int STATUS_CODE = 3;

//out struct we use
typedef struct readd {
	ULONG64 address;
	ULONGLONG size;
	ULONG64 output;
};


bool Await_Approval() {
	while (STATUS_CODE == 4 || STATUS_CODE == 5 || STATUS_CODE == 6 || STATUS_CODE == 7) {
		Sleep(0);
	}
	if (STATUS_CODE == 1) { //success
		return true;
	}
	if (STATUS_CODE == 2) { //error
		return false;
	}
}

void Disconnect() {
	std::cout << "disconnecting..." << std::endl;

	STATUS_CODE = 3;
	Sleep(300);

	std::cout << "disconnected!" << std::endl;
}

void Restart() {
	std::cout << "restart..." << std::endl;

	STATUS_CODE = 8;
	Sleep(300);

	std::cout << "restarted!" << std::endl;
}

template<class T>
T Read(uint64_t read_address) {
	readd Data;
	T response = {  };
	Data.output = (ULONG64)&response;

	Data.address = read_address;

	Data.size = sizeof(response);

	STRUCT_OFFSET_ADDRESS = &Data;
	STATUS_CODE = 4;

	if (Await_Approval()) {
		return response;
	}
	return response;
}

void ReadStr(uint64_t read_address, void* data, size_t size) {
	readd Data;
	Data.output = (ULONG64)&data;

	Data.address = read_address;

	Data.size = size;

	STRUCT_OFFSET_ADDRESS = &Data;
	STATUS_CODE = 4;

	Await_Approval();
}


void Connect() {
	int i = 7777;
	STATUS_CODE_ADDRESS = &STATUS_CODE;
	STRUCT_OFFSET_ADDRESS = &i;

	//just some info
	std::cout << "base address:        0x" << std::hex << GetModuleHandle(NULL) << std::endl;
	std::cout << "code address:        0x" << std::hex << &STATUS_CODE_ADDRESS << " -> offset: 0x" << (ULONG64)((ULONG64)&STATUS_CODE_ADDRESS - (ULONG64)GetModuleHandle(NULL)) << " => " << STATUS_CODE_ADDRESS << std::endl;
	std::cout << "test output address: 0x" << std::hex << &STRUCT_OFFSET_ADDRESS << " -> offset: 0x" << (ULONG64)((ULONG64)&STRUCT_OFFSET_ADDRESS - (ULONG64)GetModuleHandle(NULL)) << " => " << STRUCT_OFFSET_ADDRESS << std::endl;

	while (STATUS_CODE != 0) {
		Sleep(1000);
		std::cout << "code: " << STATUS_CODE << std::endl;
		std::cout << "waiting for communication!" << std::endl;

	}
	std::cout << "connected!\n";
}

bool initTarget(DWORD pid) {

	DWORD procid = pid;
	STRUCT_OFFSET_ADDRESS = &procid;
	STATUS_CODE = 6; //work! //but init
	if (Await_Approval()) {
		return true;
	}
	return false;
}

ULONG64 GetBase() {
	ULONG64 base = 0;
	STRUCT_OFFSET_ADDRESS = &base;
	STATUS_CODE = 5; //work! //but base
	if (Await_Approval()) {
		return base;
	}
}


ULONG64 GetPeb() {
	ULONG64 peb = 0;
	STRUCT_OFFSET_ADDRESS = &peb;
	STATUS_CODE = 7; //work! //but peb
	if (Await_Approval()) {
		return peb;
	}
}


static HWND get_process_wnd(uint32_t pid) {
	std::pair<HWND, uint32_t> params = { 0, pid };

	BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
		auto pParams = (std::pair<HWND, uint32_t>*)(lParam);
		uint32_t processId = 0;

		if (GetWindowThreadProcessId(hwnd, reinterpret_cast<LPDWORD>(&processId)) && processId == pParams->second) {
			SetLastError((uint32_t)-1);
			pParams->first = hwnd;
			return FALSE;
		}

		return TRUE;

		}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
		return params.first;

	return NULL;
}

DWORD GetProcessId(const wchar_t* ImageName) {
	HANDLE Snapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
	PROCESSENTRY32W Process{ sizeof(PROCESSENTRY32W) };

	if (Process32FirstW(Snapshot, &Process)) {
		do {
			if (!wcscmp(ImageName, Process.szExeFile)) {
				CloseHandle(Snapshot);
				return Process.th32ProcessID;
			}
		} while (Process32NextW(Snapshot, &Process));
	}

	CloseHandle(Snapshot);
	return NULL;
}

// ¶ÁÈ¡×Ö·û´®
string GetUnicodeString(uint64_t addr) {
	char16_t wcharTemp = { '\0' };

	wcharTemp = Read<char16_t>(addr);

	string u8_conv = wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(wcharTemp);
	cout << "u8_conv" << u8_conv <<  "\n";
	return u8_conv;
}


void Init(uint64_t address);

char* readString(uint64_t address)
{
	char* s;

	int index = 0;

	char c;

	do
	{
		c = static_cast<char>(Read<char>(address + index));

		index++;

	} while (c);

	s = new char[index + 1];

	ReadStr(address, reinterpret_cast<void*>(s), index);

	s[index] = '\0';

	return s;
}