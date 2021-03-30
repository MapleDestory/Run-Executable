#include "main.h"

static std::vector<unsigned char>	Data;
static unsigned char*				ImageBase;
static unsigned char*				EntryPoint;

static PIMAGE_NT_HEADERS GetNtHeaders(unsigned char* lpData)
{
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(lpData);
	if (!DosHeader || DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return Debug<PIMAGE_NT_HEADERS, nullptr>("Invalid DOS Signature.\n");

	PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(&lpData[DosHeader->e_lfanew]);
	if (!NtHeaders || NtHeaders->Signature != IMAGE_NT_SIGNATURE)
		return Debug<PIMAGE_NT_HEADERS, nullptr>("Invalid DOS Signature.\n");

	return NtHeaders;
}

static void BaseRelocate(unsigned char* lpData, PIMAGE_NT_HEADERS NtHeaders)
{
	maplr Delta = static_cast<maplr>(reinterpret_cast<maplr>(ImageBase) -
		NtHeaders->OptionalHeader.ImageBase);

	DWORD Size = 0;

	PIMAGE_BASE_RELOCATION BaseRelocation = reinterpret_cast<IMAGE_BASE_RELOCATION*>(lpData +
		NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	while (Size < NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size)
	{
		PWORD Entry = reinterpret_cast<PWORD>(reinterpret_cast<unsigned char*>(BaseRelocation) + sizeof(IMAGE_BASE_RELOCATION));

		for (DWORD i = 0; i < ((BaseRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD)); i++)
		{
			if (((*Entry >> 12) & IMAGE_REL_BASED_HIGHLOW))
				*reinterpret_cast<maplr*>(lpData + static_cast<maplr>(BaseRelocation->VirtualAddress) + ((*Entry) & 0xFFF)) += Delta;
			Entry++;
		}

		Size += BaseRelocation->SizeOfBlock;
		BaseRelocation = reinterpret_cast<PIMAGE_BASE_RELOCATION>(Entry);
	}
}

static bool RebuildImportTable(unsigned char* lpData, PIMAGE_NT_HEADERS NtHeaders)
{
	if (!NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
		return false;

	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(lpData +
		NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	while (ImportDescriptor->Name != NULL)
	{
		LPSTR lpFileDLL = reinterpret_cast<PCHAR>(lpData + ImportDescriptor->Name);

		HMODULE hModule = ::LoadLibraryA(lpFileDLL);
		Debug("Load module: %s\n", lpFileDLL);

		PIMAGE_THUNK_DATA Characteristics = reinterpret_cast<PIMAGE_THUNK_DATA>(lpData + ImportDescriptor->Characteristics);
		PIMAGE_THUNK_DATA FirstThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(lpData + ImportDescriptor->FirstThunk);
		PIMAGE_THUNK_DATA ThunkData = reinterpret_cast<PIMAGE_THUNK_DATA>(lpData + ImportDescriptor->FirstThunk);
		for (; Characteristics->u1.AddressOfData; Characteristics++, FirstThunk++, ThunkData++)
		{
			if (Characteristics->u1.AddressOfData & IMAGE_ORDINAL_FLAG)
				*reinterpret_cast<FARPROC*>(ThunkData) = ::GetProcAddress(hModule, MAKEINTRESOURCEA(Characteristics->u1.AddressOfData));
			else
			{
				PIMAGE_IMPORT_BY_NAME ImportByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(lpData + Characteristics->u1.AddressOfData);
				*reinterpret_cast<FARPROC*>(ThunkData) = ::GetProcAddress(hModule, reinterpret_cast<LPCSTR>(&ImportByName->Name));
			}
		}

		FreeLibrary(hModule);
		ImportDescriptor++;
	}

	return true;
}

MAPPER_API bool Mapper::LoadRaw(unsigned char* lpData)
{
	PIMAGE_NT_HEADERS NtHeaders = GetNtHeaders(lpData);
	if (!NtHeaders)
		return false;

	HANDLE hMapping = ::CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, 
		PAGE_READWRITE, 0, NtHeaders->OptionalHeader.SizeOfImage, nullptr);

	if (INVALID_HANDLE_VALUE == hMapping)
		return Debug<bool, false>("Failed to create mapping.\n");

	PVOID lpMapping = ::MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
	if (!lpMapping)
		return Debug<bool, false>("Failed to mapping memory.\n");

	// Copying data to memory
	memcpy(lpMapping, lpData, NtHeaders->OptionalHeader.SizeOfHeaders);

	PIMAGE_SECTION_HEADER SectionHeader = IMAGE_FIRST_SECTION(NtHeaders);
	for (unsigned short i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++, SectionHeader++)
	{
		memcpy(static_cast<void*>(static_cast<unsigned char*>(lpMapping) + SectionHeader->VirtualAddress),
			static_cast<void*>(lpData + SectionHeader->PointerToRawData), SectionHeader->SizeOfRawData);
	}

	Data = std::vector<BYTE>(reinterpret_cast<LPBYTE>(lpMapping),
		reinterpret_cast<LPBYTE>(lpMapping) + NtHeaders->OptionalHeader.SizeOfImage);

	if (lpMapping)
		UnmapViewOfFile(lpMapping);

	if (hMapping)
		CloseHandle(hMapping);

	return true;
}

MAPPER_API bool Mapper::Builder(void)
{
	PIMAGE_DOS_HEADER DosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(Data.data());
	PIMAGE_NT_HEADERS NtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(Data.data() + DosHeader->e_lfanew);

	// Get the alloc memory
	ImageBase = static_cast<unsigned char*>(::VirtualAlloc(nullptr, NtHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if (!ImageBase)
		return Debug<bool, false>("Failed to allocate memory.\n");

	// Copying memory to Image
	memcpy(ImageBase, Data.data(), Data.size());

	// Rebuild import header
	RebuildImportTable(ImageBase, NtHeaders);

	// Relocate header
	BaseRelocate(ImageBase, NtHeaders);

	// Get entry point
	EntryPoint = static_cast<unsigned char*>(ImageBase + NtHeaders->OptionalHeader.AddressOfEntryPoint);

	 return true;
}

MAPPER_API bool Mapper::Execute(void)
{
	// Call entry point
	reinterpret_cast<void(*)()>(EntryPoint)();

	return true;
}
