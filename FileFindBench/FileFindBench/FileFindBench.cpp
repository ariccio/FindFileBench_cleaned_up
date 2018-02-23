#include "stdafx.h"
#include "FileFindBench.h"

#define TRACE_OUT_C_STYLE( x, fmt_spec ) ::wprintf( L"\r\n\t\t" L#x L" = `" L#fmt_spec L"` ", ##x )

__int64 descendDirectory( _In_ const WIN32_FIND_DATA fData, _In_ const std::wstring& normDirString) {
	std::wstring newSzDir( normDirString );//MUST operate on copy!
	newSzDir += L"\\";
	newSzDir += fData.cFileName;
	return stdRecurseFindFutures( newSzDir );
	}

bool should_remove(const WIN32_FIND_DATA& fData) {
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL)
		return true;
	return false;
	}

__int64 stdRecurseFindFutures( _In_ std::wstring dir ) {
	__int64 num = 0;
	const std::wstring normDirString( dir );
	assert( dir.size( ) > 2 );
	if ( ( dir.back() != L'*' ) && ( dir[ dir.length( ) - 2 ] != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir.back() == L'\\' ) {
		dir += L"*";
		}
	std::vector<std::future<__int64>> futureDirs;
	WIN32_FIND_DATA fData = { 0 };
	HANDLE fDataHand = ::FindFirstFileExW(dir.c_str(), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0);

	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		BOOL res = ::FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			const int scmpVal = ::wcscmp( fData.cFileName, L".." );
			if ( scmpVal != 0 ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( scmpVal != 0 ) ) {
				futureDirs.emplace_back( std::async( std::launch::async|std::launch::deferred, descendDirectory, fData, normDirString) );
				}
			else if ( ( scmpVal != 0 ) && should_remove(fData) ) {
				--num;
				}
			res = ::FindNextFileW( fDataHand, &fData );
			}
		}
	::FindClose( fDataHand );
	for ( size_t i = 0; i < futureDirs.size(); ++i ) {
		num += futureDirs[ i ].get( );
		}
	return num;
	}

void startWorkAsync( _In_ std::wstring arg ) {
	wprintf( L"Working on: `%s`\r\n", arg.c_str( ) );
	arg = L"\\\\?\\" + arg;
	const __int64 numberFiles = stdRecurseFindFutures( arg );
	wprintf( L"\r\nNumber of items: %I64d\r\n", numberFiles );
	}

const DOUBLE getAdjustedTimingFrequency( ) {
	LARGE_INTEGER timingFrequency = { 0 };
	(void)::QueryPerformanceFrequency( &timingFrequency );
	return ( ( DOUBLE( 1.00 ) / DOUBLE( timingFrequency.QuadPart ) ) );
	}

DOUBLE iterate( _In_ const std::wstring& arg) {
	LARGE_INTEGER startTime = { 0 };
	LARGE_INTEGER endTime = { 0 };	
	const DOUBLE adjustedTimingFrequency = getAdjustedTimingFrequency( );

	(void)::QueryPerformanceCounter( &startTime );
	startWorkAsync( arg );
	(void)::QueryPerformanceCounter( &endTime );

	const DOUBLE totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;
	return { totalTime };
	}

int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		puts("Need more than 1 argument!\r\n");
		return ERROR_BAD_ARGUMENTS;
		}
	const DOUBLE record = iterate(argv[1]);

	puts("---------------------\r\n");
	wprintf(L"Time in seconds:  %s\r\n", std::to_wstring(record).c_str());
	return 0;
	}
