#pragma once

#if defined(_DEBUG) && defined(_WIN32)
#define DWIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif