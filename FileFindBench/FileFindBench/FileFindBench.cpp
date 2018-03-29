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

std::wstring handyDandyErrMsgFormatter( ) {
	const size_t msgBufSize = 2 * 1024;
	wchar_t msgBuf[ msgBufSize ] = { 0 };
	const DWORD err = ::GetLastError( );
	const DWORD ret = ::FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), msgBuf, msgBufSize, NULL );
	if ( ret > 0 ) {
		return std::wstring( msgBuf );
		}
	return std::wstring( L"FormatMessage failed to format an error!" );
	}


void stdRecurseFindFutures( const std::wstring raw_dir ) {
	NtdllWrap ntdll;
	std::wstring dir = L"\\\\?\\" + raw_dir;

	const HANDLE nt_dir_handle = ::CreateFileW(dir.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
	if ( nt_dir_handle == INVALID_HANDLE_VALUE ) {
		const DWORD err = ::GetLastError( );
		::fwprintf( stderr, L"Failed to open directory %s because of error %lu\r\n", dir.c_str( ), err );
		::fwprintf( stderr, L"err: `%lu` means: %s\r\n", err, handyDandyErrMsgFormatter( ).c_str( ) );
		}
	//I do this to ensure there are NO issues with incorrectly sized buffers or mismatching parameters (or any other bad changes)
	const FILE_INFORMATION_CLASS InfoClass = FileDirectoryInformation;
	typedef FILE_DIRECTORY_INFORMATION THIS_FILE_INFORMATION_CLASS;
	typedef THIS_FILE_INFORMATION_CLASS* PTHIS_FILE_INFORMATION_CLASS;

	
	const ULONG init_bufSize = ( ( sizeof( FILE_ID_BOTH_DIR_INFORMATION ) + ( MAX_PATH * sizeof( wchar_t ) ) ) * 100 );
	//__declspec( align( 8 ) ) wchar_t buffer[ init_bufSize ];
	ULONG bufSize = init_bufSize;
	std::unique_ptr<__declspec( align( 8 ) ) wchar_t[]> idInfo;
	idInfo = std::make_unique<__declspec( align( 8 ) ) wchar_t[]>(init_bufSize);

	std::vector<std::wstring> breadthDirs;
	std::vector<WCHAR> fNameVect;

	std::vector<std::future<std::pair<std::uint64_t, std::uint64_t>>> futureDirs;

	IO_STATUS_BLOCK iosb{};

	//UNICODE_STRING _glob;
	
	NTSTATUS query_directory_result = STATUS_PENDING;
	wprintf( L"Files in directory %s\r\n", dir.c_str( ) );
	wprintf( L"      File ID       |       File Name\r\n" );
	assert( init_bufSize > 1 );
	//auto buffer = &( idInfo[ 0 ] );
	//++numItems;
	const NTSTATUS sBefore = query_directory_result;
	query_directory_result = ntdll.NtQueryDirectoryFile_f(nt_dir_handle, NULL, NULL, NULL, &iosb, idInfo.get(), init_bufSize, InfoClass, FALSE, NULL, TRUE );
		if ( query_directory_result == STATUS_TIMEOUT ) {
		std::terminate( );
		}
	if ( query_directory_result == STATUS_PENDING ) {
		std::terminate( );
		}
	assert( NT_SUCCESS( query_directory_result ) );
	assert( query_directory_result != sBefore );
	assert( ::GetLastError( ) != ERROR_MORE_DATA );
	while ( query_directory_result == STATUS_BUFFER_OVERFLOW ) {
		idInfo.reset();
		bufSize *= 2;
		idInfo = std::make_unique<__declspec( align( 8 ) ) wchar_t[]>( bufSize );
		query_directory_result = ntdll.NtQueryDirectoryFile_f(nt_dir_handle, NULL, NULL, NULL, &iosb, idInfo.get(), bufSize, InfoClass, FALSE, NULL, TRUE );
		}
	assert( NT_SUCCESS( query_directory_result ) );
	//bool id_info_heap = false;
	const ULONG_PTR bufSizeWritten = iosb.Information;

	const auto idInfoSize = bufSize;
	
	//This zeros just enough of the idInfo buffer ( after the end of valid data ) to halt the forthcoming while loop at the last valid data. This saves the effort of zeroing larger parts of the buffer.
	for ( size_t i = bufSizeWritten; i < bufSizeWritten + ( sizeof( THIS_FILE_INFORMATION_CLASS ) + ( MAX_PATH * sizeof( wchar_t ) ) * 2 ); ++i ) {
		if ( i == idInfoSize ) {
			break;
			}
		assert( i < bufSize );
		idInfo[ i ] = 0;
		}

	
	const ULONG_PTR count_records = bufSizeWritten / sizeof( THIS_FILE_INFORMATION_CLASS );
	PTHIS_FILE_INFORMATION_CLASS pFileInf = reinterpret_cast<PTHIS_FILE_INFORMATION_CLASS>( idInfo.get( ) );
	(void)pFileInf;
	(void)count_records;

	//UNICODE_STRING path = {};
	//RTL_RELATIVE_NAME rtl_rel_name = {};
	//PCWSTR ntpart = nullptr;
	////::RtlInitUnicodeString(&path, dir.c_str());
	//const NTSTATUS conversion_result = ntdll.RtlDosPathNameToNtPathName_U_WithStatus_f(raw_dir.c_str(), &path, &ntpart, &rtl_rel_name);
	//if ( !NT_SUCCESS( conversion_result ) ) {
	//	fwprintf( stderr, L"RtlDosPathNameToNtPathName_U_WithStatus failed!\r\n" );
	//	std::terminate();
	//	}

	//const ULONG is_dos_device = ntdll.RtlIsDosDeviceName_U_f( raw_dir.c_str( ) );
	//if ( is_dos_device ) {
	//	fwprintf( stderr, L"'%s' (ntpart) is a DOS device!\r\n", ntpart );
	//	std::terminate();
	//	}
	//HANDLE nt_dir_handle = INVALID_HANDLE_VALUE;
	//OBJECT_ATTRIBUTES oa = {};
	//IO_STATUS_BLOCK iosb = {};

	//InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE , NULL, NULL);

	//const NTSTATUS open_result = ntdll.NtOpenFile_f(&nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA, &oa, &iosb,FILE_SHARE_READ, FILE_DIRECTORY_FILE| FILE_NON_DIRECTORY_FILE);
	//const NTSTATUS open_result = ntdll.NtCreateFile_f( &nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA | FILE_LIST_DIRECTORY, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, NULL );


	//if ( !NT_SUCCESS( open_result ) ) {
		//fwprintf( stderr, L"NtOpenFile failed!\r\n" );
		//std::terminate();
		//}
	//NtQueryDirectory


	/*
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
	//NTSTATUS const close_result = ntdll.NtClose_f(nt_dir_handle);
	//if (!NT_SUCCESS( close_result ) ) {
	//	fwprintf( stderr, L"Closing handle nt_dir_handle (%p) failed!\r\n\tResult code: %li\r\n", nt_dir_handle, close_result );
	//	}


	return ;
	}

int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		puts("Need more than 1 argument!\r\n");
		return ERROR_BAD_ARGUMENTS;
		}
	//for ( uint32_t i = 0; i < ( UINT32_MAX - 2 ); ++i) {
		stdRecurseFindFutures(std::wstring(argv[1]));
		//}
	puts("---------------------\r\n");
	return 0;
	}
