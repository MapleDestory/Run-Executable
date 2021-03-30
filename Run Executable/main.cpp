#include "main.h"

static unsigned char* LoadMapFromFile(const fs::path& Filename)
{
	std::streampos size;
	std::fstream file(Filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = file.tellg();

		char* Memblock = new char[size]();

		file.seekg(0, std::ios::beg);
		file.read(Memblock, size);
		file.close();

		return reinterpret_cast<unsigned char*>(Memblock);
	}
	return nullptr;
}

INT APIENTRY _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, INT)
{
	::AllocConsole();
	::AttachConsole(::GetCurrentProcessId());
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CON", "r", stdin);
	freopen_s(&pFile, "CON", "w", stdout);
	freopen_s(&pFile, "CON", "w", stderr);

	//auto rawData = LoadMapFromFile("D:\\Qt\\Application\\Compiler\\03.302021\\binary.bin");
	LoadLibraryA("D:\\Qt\\Application\\Compiler\\03.302021\\Qt5Widgets.dll");
	//Mapper::LoadRaw(rawData);
	//Mapper::Builder();
	//Mapper::Execute();


	std::cin.get();

	fclose(pFile);
	FreeConsole();

	return -1;
}