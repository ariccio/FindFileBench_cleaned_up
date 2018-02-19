#include "stdafx.h"
#include "FileFindBench.h"

#define TRACE_OUT_C_STYLE( x, fmt_spec ) ::wprintf( L"\r\n\t\t" L#x L" = `" L#fmt_spec L"` ", ##x )
#define TRACE_OUT_C_STYLE_ENDL( ) ::wprintf( L"\r\n" )

struct FileFindRecord {
	double time_seconds;
	bool operator<( const FileFindRecord& other ) {
		return time_seconds < other.time_seconds;
		}
	};

__int64 descendDirectory( _In_ const WIN32_FIND_DATA fData, _In_ const std::wstring& normSzDir) {
	std::wstring newSzDir( normSzDir );//MUST operate on copy!
	newSzDir.reserve( MAX_PATH );
	newSzDir += L"\\";
	newSzDir += fData.cFileName;
	return stdRecurseFindFutures( std::move( newSzDir ));
	}

__int64 stdRecurseFindFutures( _In_ std::wstring dir ) {
	__int64 num = 0;
	dir.reserve( MAX_PATH );
	const std::wstring normSzDir( dir );
	assert( dir.size( ) > 2 );
	if ( ( dir[ dir.length( ) - 1 ] != L'*' ) && ( dir[ dir.length( ) - 2 ] != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir[ dir.length( ) - 1 ] == L'\\' ) {
		dir += L"*";
		}
	std::vector<std::future<__int64>> futureDirs;
	futureDirs.reserve( 100 );//pseudo-arbitrary number
	WIN32_FIND_DATA fData = { 0 };
	HANDLE fDataHand = ::FindFirstFileExW(dir.c_str(), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0);

	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		//if ( ::wcscmp( fData.cFileName, L".." ) != 0 ) {
			//++num;
			//}
		BOOL res = ::FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			const auto scmpVal = ::wcscmp( fData.cFileName, L".." );
			if ( scmpVal != 0 ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( scmpVal != 0 ) ) {
				futureDirs.emplace_back( std::async( std::launch::async|std::launch::deferred, descendDirectory, fData, normSzDir) );
				}
			else if ( ( scmpVal != 0 ) && ( ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL ) ) ) ) {
				--num;
				}
			res = ::FindNextFileW( fDataHand, &fData );
			}
		}
	::FindClose( fDataHand );
	const size_t size_futureDirs = futureDirs.size( );
	for ( size_t i = 0; i < size_futureDirs; ++i ) {
		num += futureDirs[ i ].get( );
		}
	return num;
	}

void stdWorkAsync( _In_ std::wstring arg ) {
	wprintf( L"Working on: `%s`\r\n", arg.c_str( ) );
	arg.reserve( MAX_PATH );
	arg = L"\\\\?\\" + arg;
	const __int64 numberFiles = stdRecurseFindFutures( std::move( arg ) );
	wprintf( L"\r\nNumber of items: %I64d\r\n", numberFiles );
	}

const DOUBLE getAdjustedTimingFrequency( ) {
	LARGE_INTEGER timingFrequency = { 0 };
	const BOOL res1 = QueryPerformanceFrequency( &timingFrequency );
	if ( !res1 ) {
		wprintf( L"QueryPerformanceFrequency failed!!!!!! Disregard any timing data!!\r\n" );
		}
	const DOUBLE adjustedTimingFrequency = ( DOUBLE( 1.00 ) / DOUBLE( timingFrequency.QuadPart ) );
	return adjustedTimingFrequency;
	}

std::wstring formatFileFindRecord( _In_ const FileFindRecord& record ) {
	std::wstring retStr;
	retStr += L"Iteration without FIND_FIRST_EX_LARGE_FETCH, without isBasicInfo, IsAsync?: 1, Time in seconds:  ";
	retStr += std::to_wstring( record.time_seconds );
	return retStr;
	}

FileFindRecord iterate( _In_ const std::wstring& arg) {
	LARGE_INTEGER startTime = { 0 };
	LARGE_INTEGER endTime = { 0 };
	
	const DOUBLE adjustedTimingFrequency = getAdjustedTimingFrequency( );

	const BOOL res2 = ::QueryPerformanceCounter( &startTime );
	stdWorkAsync( arg );
	const BOOL res3 = ::QueryPerformanceCounter( &endTime );
	
	if ( ( !res2 ) || ( !res3 ) ) {
		wprintf( L"QueryPerformanceCounter failed!!!!!! Disregard any timing data!!\r\n" );
		}
	const DOUBLE totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;

	return { totalTime };
	}

int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		wprintf( L"Need more than 1 argument!\r\n" );
		::Sleep( 5000 );
		return ERROR_BAD_ARGUMENTS;
		}
	const std::wstring arg( argv[ 1 ] );
	std::vector<FileFindRecord> records;

	//number of runs
	for ( unsigned i = 0; i < 2; ++i ) {
		TRACE_OUT_C_STYLE( i, %u );
		TRACE_OUT_C_STYLE_ENDL( );
		records.emplace_back( iterate( arg ) );
		}

	wprintf( L"---------------------\r\n" );
	const size_t size_records = records.size( );
	std::sort( std::begin(records), std::end(records) );
	for ( size_t i = 0; i < size_records; ++i ) {
		wprintf( L"%s\r\n", formatFileFindRecord( records[ i ] ).c_str( ) );
		}
	return 0;
	}
