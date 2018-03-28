#include "stdafx.h"
#include "FileFindBench.h"



void test( _Post_ptr_invalid_ HANDLE){}

NtdllWrap::NtdllWrap( ) {
	ntdll = ::GetModuleHandleW( L"C:\\Windows\\System32\\ntdll.dll" );
	if ( !ntdll) {
		::fwprintf( stderr, L"Couldn't load ntdll.dll!\r\n" );
		std::terminate( );
		}
	NtQueryDirectoryFile_f = reinterpret_cast<NtQueryDirectoryFile_t>( ::GetProcAddress( ntdll, "NtQueryDirectoryFile" ) );
	if ( !NtQueryDirectoryFile_f ) {
		::fwprintf( stderr, L"Couldn't find NtQueryDirectoryFile in ntdll.dll!\r\n" );
		}
	RtlDosPathNameToNtPathName_U_WithStatus_f = reinterpret_cast<RTL_DOS_PATH_NAME_TO_NT_PATH_NAME_U_WITHSTATUS>( ::GetProcAddress( ntdll, "RtlDosPathNameToNtPathName_U_WithStatus" ) );
	if ( !RtlDosPathNameToNtPathName_U_WithStatus_f ) {
		::fwprintf( stderr, L"Couldn't find RtlDosPathNameToNtPathName_U_WithStatus in ntdll.dll!\r\n" );
		}
	RtlIsDosDeviceName_U_f = reinterpret_cast<RtlIsDosDeviceName_U_t>( ::GetProcAddress( ntdll, "RtlIsDosDeviceName_U" ) );
	if ( !RtlIsDosDeviceName_U_f ) {
		::fwprintf( stderr, L"Couldn't find RtlIsDosDeviceName_U in ntdll.dll!\r\n" );
		}

	RtlAppendUnicodeStringToString_f = reinterpret_cast<RtlAppendUnicodeStringToString_t>( ::GetProcAddress( ntdll, "RtlAppendUnicodeStringToString" ) );
	if ( !RtlAppendUnicodeStringToString_f ) {
		::fwprintf( stderr, L"Couldn't find RtlAppendUnicodeStringToString in ntdll.dll!\r\n" );
		}
	RtlAppendUnicodeToString_f = reinterpret_cast<RtlAppendUnicodeToString_t>( ::GetProcAddress( ntdll, "RtlAppendUnicodeToString" ) );
	if ( !RtlAppendUnicodeToString_f ) {
		::fwprintf( stderr, L"Couldn't find RtlAppendUnicodeToString_f in ntdll.dll!\r\n" );
		}

	NtOpenFile_f = reinterpret_cast<NtOpenFile_t>( ::GetProcAddress( ntdll, "NtOpenFile" ) );
	if ( !NtOpenFile_f ) {
		::fwprintf( stderr, L"Couldn't find NtOpenFile in ntdll.dll!\r\n" );
		}
	NtClose_f = reinterpret_cast<NtClose_t>( ::GetProcAddress( ntdll, "NtClose" ) );
	if ( !NtClose_f ) {
		::fwprintf( stderr, L"Couldn't find NtClose in ntdll.dll!\r\n" );
		}

	NtCreateFile_f = reinterpret_cast<NtCreateFile_t>( ::GetProcAddress( ntdll, "NtCreateFile" ) );
	if ( !NtClose_f ) {
		::fwprintf( stderr, L"Couldn't find NtCreateFile in ntdll.dll!\r\n" );
		}
	}

void stdRecurseFindFutures( std::wstring raw_dir ) {
	NtdllWrap ntdll;
	UNICODE_STRING path = {};
	RTL_RELATIVE_NAME rtl_rel_name = {};
	PCWSTR ntpart = nullptr;
	//::RtlInitUnicodeString(&path, dir.c_str());
	const NTSTATUS conversion_result = ntdll.RtlDosPathNameToNtPathName_U_WithStatus_f(raw_dir.c_str(), &path, &ntpart, &rtl_rel_name);
	if ( !NT_SUCCESS( conversion_result ) ) {
		fwprintf( stderr, L"RtlDosPathNameToNtPathName_U_WithStatus failed!\r\n" );
		std::terminate();
		}

	const ULONG is_dos_device = ntdll.RtlIsDosDeviceName_U_f( raw_dir.c_str( ) );
	if ( is_dos_device ) {
		fwprintf( stderr, L"'%s' (ntpart) is a DOS device!\r\n", ntpart );
		std::terminate();
		}
	HANDLE nt_dir_handle = INVALID_HANDLE_VALUE;
	OBJECT_ATTRIBUTES oa = {};
	IO_STATUS_BLOCK iosb = {};

	InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE , NULL, NULL);

	//const NTSTATUS open_result = ntdll.NtOpenFile_f(&nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA, &oa, &iosb,FILE_SHARE_READ, FILE_DIRECTORY_FILE| FILE_NON_DIRECTORY_FILE);
	const NTSTATUS open_result = ntdll.NtCreateFile_f( &nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA | FILE_LIST_DIRECTORY, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, NULL );


	if ( !NT_SUCCESS( open_result ) ) {
		fwprintf( stderr, L"NtOpenFile failed!\r\n" );
		std::terminate();
		}
	//NtQueryDirectory


	/*
	std::wstring dir = L"\\\\?\\" + raw_dir;
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


		*/
	NTSTATUS const close_result = ntdll.NtClose_f(nt_dir_handle);
	if (!NT_SUCCESS( close_result ) ) {
		fwprintf( stderr, L"Closing handle nt_dir_handle (%p) failed!\r\n\tResult code: %li\r\n", nt_dir_handle, close_result );
		}


	return ;
	}

int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		puts("Need more than 1 argument!\r\n");
		return ERROR_BAD_ARGUMENTS;
		}
	for ( uint32_t i = 0; i < ( UINT32_MAX - 2 ); ++i) {
		stdRecurseFindFutures(std::wstring(argv[1]));
		}
	puts("---------------------\r\n");
	return 0;
	}
