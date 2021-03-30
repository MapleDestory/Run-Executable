#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef MAIN
#define MAIN
#endif

#ifndef MAIN_API
#define MAIN_API
#endif

#ifndef MAIN_IMPL_API
#define MAIN_IMPL_API
#endif

#if defined(__cplusplus)
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#endif

#if defined(__cplusplus)
namespace fs = std::experimental::filesystem;
#endif

#include <windows.h>

#include <tchar.h>
#include <stdio.h>


#include "mapper.h"

#if (_MSC_VER >= 1910)
template <class... Args>
static inline void Debug(Args&&... args)
{
	printf(args...);
}

template <class T, T Return, class... Args>
static inline T Debug(Args&&... args)
{
	printf(args...);
	return Return;
}
#endif