// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(disable:4061) //enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label. The enumerate has no associated handler in a switch statement.
#pragma warning(disable:4062) //The enumerate has no associated handler in a switch statement, and there is no default label.
#pragma warning(disable:4350) //An rvalue cannot be bound to a non-const reference. In previous versions of Visual C++, it was possible to bind an rvalue to a non-const reference in a direct initialization. This code now gives a warning.
#pragma warning(disable:4820) //'bytes' bytes padding added after construct 'member_name'. The type and order of elements caused the compiler to add padding to the end of a struct
#pragma warning(disable:4514) //'function' : unreferenced inline function has been removed
#pragma warning(disable:4710) //The given function was selected for inline expansion, but the compiler did not perform the inlining.


#pragma warning(push, 4)

#define _WIN32_WINNT 0x0600

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif


//#define MFC_C_FILE_FIND_TEST //MFC is huge, and makes compiling slow, so let's not #include it unless we want it.

#pragma comment(lib, "Advapi32.lib")
#include <Windows.h>
#include "targetver.h"


#ifndef _DEBUG
#pragma warning(disable:4555) //expression has no effect; expected expression with side-effect //Happens alot with AfxCheckMemory in debug builds.
#endif


#include <string>
#include <string.h>
#include <vector>
#include <cassert>
#include <future>
//#include <ntstatus.h>
#include <winternl.h>
#include <stdio.h>
#include <queue>
#include <functional>

#pragma warning(pop)


