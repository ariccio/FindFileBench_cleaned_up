#include "stdafx.h"
#include "FileFindBench.h"

constexpr bool writeToScreen = true;

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

_Success_( return != UINT64_MAX )
std::uint64_t GetCompressedFileSize_filename( const std::wstring path ) {
	ULARGE_INTEGER ret;
	const auto last_err_old = ::GetLastError( );
	ret.QuadPart = 0;//zero initializing this is critical!
	ret.LowPart = ::GetCompressedFileSizeW( path.c_str( ), &ret.HighPart );
	const auto last_err = ::GetLastError( );
	if ( ret.LowPart == INVALID_FILE_SIZE ) {
		if ( ret.HighPart != NULL ) {
			if ( ( last_err != NO_ERROR ) && ( last_err != last_err_old ) ) {
				fwprintf( stderr, L"Error! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
				return UINT64_MAX;// IN case of an error return size from CFileFind object
				}
			fwprintf( stderr, L"WTF ERROR! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
			return UINT64_MAX;
			}
		else {
			if ( ( last_err != NO_ERROR ) && ( last_err != last_err_old ) ) {
				fwprintf( stderr, L"Error! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
				return UINT64_MAX;
				}
			return ret.QuadPart;
			}
		}
	return ret.QuadPart;
	}


void stdRecurseFindFutures( const std::wstring raw_dir ) {
	NtdllWrap ntdll;
	const std::wstring dir = L"\\\\?\\" + raw_dir;

	std::uint64_t total_size = 0;
	std::uint64_t numItems = 0;

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

	
	//This zeros just enough of the idInfo buffer ( after the end of valid data ) to halt the forthcoming while loop at the last valid data. This saves the effort of zeroing larger parts of the buffer.
	for ( size_t i = bufSizeWritten; i < bufSizeWritten + ( sizeof( THIS_FILE_INFORMATION_CLASS ) + ( MAX_PATH * sizeof( wchar_t ) ) * 2 ); ++i ) {
		assert( i < bufSize );
		idInfo[ i ] = 0;
		}

	
	const ULONG_PTR count_records = bufSizeWritten / sizeof( THIS_FILE_INFORMATION_CLASS );
	PTHIS_FILE_INFORMATION_CLASS pFileInf = reinterpret_cast<PTHIS_FILE_INFORMATION_CLASS>( idInfo.get( ) );


	assert( pFileInf != NULL );
	while ( NT_SUCCESS( query_directory_result ) && ( pFileInf != NULL ) ) {
		//PFILE_ID_BOTH_DIR_INFORMATION pFileInf = ( FILE_ID_BOTH_DIR_INFORMATION* ) buffer;

		assert( pFileInf->FileNameLength > 1 );
		if ( pFileInf->FileName[ 0 ] == L'.' && ( pFileInf->FileName[ 1 ] == 0 || ( pFileInf->FileName[ 1 ] == '.' ) ) ) {
			//continue;
			goto nextItem;
			}

		total_size += pFileInf->AllocationSize.QuadPart;
		//const auto lores = GetCompressedFileSizeW( , ) 
		{
		const std::wstring this_file_name( pFileInf->FileName, ( pFileInf->FileNameLength / sizeof( WCHAR ) ) );
		const std::wstring some_name( dir + L'\\' + this_file_name );
		const auto comp_file_size = GetCompressedFileSize_filename( some_name );
		if ( !( pFileInf->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ) {
			if ( writeToScreen && !( std::uint64_t( pFileInf->AllocationSize.QuadPart ) == comp_file_size ) ) {
				/*
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  
#define FILE_ATTRIBUTE_INTEGRITY_STREAM     0x00008000  
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000  
#define FILE_ATTRIBUTE_NO_SCRUB_DATA        0x00020000  

				*/
				wprintf( L"Attributes for file: %s\r\n", some_name.c_str( ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_READONLY", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_READONLY ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_HIDDEN", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_HIDDEN ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_SYSTEM", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_SYSTEM ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_DIRECTORY", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_ARCHIVE", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_ARCHIVE ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_DEVICE", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_DEVICE ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_NORMAL", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_NORMAL ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_TEMPORARY", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_TEMPORARY ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_SPARSE_FILE", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_REPARSE_POINT", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_COMPRESSED", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_COMPRESSED ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_OFFLINE", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_OFFLINE ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_NOT_CONTENT_INDEXED", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_ENCRYPTED", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_INTEGRITY_STREAM", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_VIRTUAL", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_VIRTUAL ) ? L"YES" : L"NO" ) );
				wprintf( L"%s: %s\r\n", L"FILE_ATTRIBUTE_NO_SCRUB_DATA", ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) ? L"YES" : L"NO" ) );

				//_CrtDbgBreak( );
				}
			}
		}
		++numItems;
		if ( ( pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) || writeToScreen ) {//I'd like to avoid building a null terminated string unless it is necessary
			fNameVect.clear( );
			fNameVect.reserve( ( pFileInf->FileNameLength / sizeof( WCHAR ) ) + 1 );
			PWCHAR end = pFileInf->FileName + ( pFileInf->FileNameLength / sizeof( WCHAR ) );
			fNameVect.insert( fNameVect.end( ), pFileInf->FileName, end );
			fNameVect.emplace_back( L'\0' );
			PWSTR fNameChar = &( fNameVect[ 0 ] );
			
			if ( writeToScreen ) {

				//std::wcout << std::setw( std::numeric_limits<LONGLONG>::digits10 ) << pFileInf->FileId.QuadPart << L"    " << std::setw( 0 ) << curDir << L"\\" << fNameChar;
				//wprintf( L"%I64d    %s\\%s\r\n", std::int64_t( pFileInf->FileId.QuadPart ), curDir.c_str( ), fNameChar );
				if ( pFileInf->FileAttributes & FILE_ATTRIBUTE_COMPRESSED ) {
					wprintf( L"AllocationSize: %I64d    %s\\%s\r\n", std::int64_t( pFileInf->AllocationSize.QuadPart ), dir.c_str( ), fNameChar );
					}

				//auto state = std::wcout.fail( );
				//if ( state != 0 ) {
				//	std::wcout.clear( );
				//	std::wcout << std::endl << L"std::wcout.fail( ): " << state << std::endl;
				//	}
				}
			if ( pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

				if ( dir[ dir.length( ) - 1 ] != L'\\' ) {
					//breadthDirs.emplace_back( std::wstring( curDir ) + L'\\' + fNameChar + L'\\' );
					auto query = std::wstring( dir + L'\\' + fNameChar + L'\\' );
					//futureDirs.emplace_back( std::async( std::launch::async | std::launch::deferred, ListDirectory, query, writeToScreen, ntdll ) );
					puts("Imagine we're descending...\r\n");
					}
				else {
					//breadthDirs.emplace_back( std::wstring( curDir ) + fNameChar + L'\\' );
					auto query = std::wstring( dir + fNameChar + L'\\' );
					//futureDirs.emplace_back( std::async( std::launch::async | std::launch::deferred, ListDirectory, query, writeToScreen, ntdll ) );
					puts("Imagine we're descending...\r\n");
					}
				//std::wstring dirstr = curDir + L"\\" + fNameChar + L"\\";
				//breadthDirs.emplace_back( dirstr );
				//numItems += ListDirectory( dirstr.c_str( ), dirs, idInfo, writeToScreen );
				}
			}

	nextItem:
		//stat = NtQueryDirectoryFile( hDir, NULL, NULL, NULL, &iosb, &idInfo[ 0 ], idInfo.size( ), FileIdBothDirectoryInformation, TRUE, NULL, FALSE );
		if ( writeToScreen ) {
			wprintf( L"\t\tpFileInf: %p, pFileInf->NextEntryOffset: %lu, ( pFileInf + pFileInf->NextEntryOffset ): %p\r\n", pFileInf, pFileInf->NextEntryOffset, ( pFileInf + pFileInf->NextEntryOffset ) );
			//std::wcout << L"\t\tpFileInf: " << pFileInf << L", pFileInf->NextEntryOffset: " << pFileInf->NextEntryOffset << L", pFileInf + pFileInf->NextEntryOffset " << ( pFileInf + pFileInf->NextEntryOffset ) << std::endl;
			}
		pFileInf = ( pFileInf->NextEntryOffset != 0 ) ? reinterpret_cast<PTHIS_FILE_INFORMATION_CLASS>( reinterpret_cast<std::uint64_t>( pFileInf ) + ( static_cast<std::uint64_t>( pFileInf->NextEntryOffset ) ) ) : NULL;
		}


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
