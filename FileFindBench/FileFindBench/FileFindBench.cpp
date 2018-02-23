#include "stdafx.h"
#include "FileFindBench.h"

bool should_remove(const WIN32_FIND_DATA& fData) {
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM)
		return true;
	if (fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		return true;
	return false;
	}

__int64 stdRecurseFindFutures( std::wstring dir ) {
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
			const int not_this_directory = ::wcscmp( fData.cFileName, L".." );
			if ( ( not_this_directory != 0 ) && ( !should_remove(fData) ) ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( not_this_directory != 0 ) ) {
				futureDirs.emplace_back( std::async( std::launch::async|std::launch::deferred, stdRecurseFindFutures, ( normDirString + L"\\" + fData.cFileName ) ) );
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

const DOUBLE getAdjustedTimingFrequency( ) {
	LARGE_INTEGER timingFrequency = { 0 };
	(void)::QueryPerformanceFrequency( &timingFrequency );
	return ( ( DOUBLE( 1.00 ) / DOUBLE( timingFrequency.QuadPart ) ) );
	}

DOUBLE iterate( const std::wstring& arg) {
	LARGE_INTEGER startTime = { 0 };
	LARGE_INTEGER endTime = { 0 };	
	const DOUBLE adjustedTimingFrequency = getAdjustedTimingFrequency( );

	(void)::QueryPerformanceCounter( &startTime );
	wprintf(L"Working on: `%s`\r\n", arg.c_str());
	const __int64 numberFiles = stdRecurseFindFutures(L"\\\\?\\" + arg);
	wprintf(L"\r\nNumber of items: %I64d\r\n", numberFiles);
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
