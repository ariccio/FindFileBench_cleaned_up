#include "stdafx.h"
#include "FileFindBench.h"

void stdRecurseFindFutures( std::wstring dir ) {
	const std::wstring normDirString( dir );
	assert( dir.size( ) > 2 );
	if ( ( dir.back() != L'*' ) && ( dir[ dir.length( ) - 2 ] != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir.back() == L'\\' ) {
		dir += L"*";
		}
	std::vector<std::future<void>> futureDirs;
	WIN32_FIND_DATA fData = { 0 };
	HANDLE fDataHand = ::FindFirstFileExW(dir.c_str(), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0);

	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		BOOL res = ::FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			const int not_this_directory = ::wcscmp( fData.cFileName, L".." );
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( not_this_directory != 0 ) ) {
				futureDirs.emplace_back( std::async( std::launch::async|std::launch::deferred, stdRecurseFindFutures, ( normDirString + L"\\" + fData.cFileName ) ) );
				}
			res = ::FindNextFileW( fDataHand, &fData );
			}
		}
	::FindClose( fDataHand );
	for ( size_t i = 0; i < futureDirs.size(); ++i ) {
		futureDirs[ i ].get( );
		}
	return ;
	}

int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		puts("Need more than 1 argument!\r\n");
		return ERROR_BAD_ARGUMENTS;
		}
	stdRecurseFindFutures(L"\\\\?\\" + std::wstring(argv[1]));
	puts("---------------------\r\n");
	return 0;
	}
