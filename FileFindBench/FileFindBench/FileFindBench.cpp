#include "stdafx.h"
#include "FileFindBench.h"
#include <experimental/coroutine>

constexpr const bool writeToScreen = false;

//Kenny Kerr
template <typename Type, typename Traits>
class unique_handle {
	unique_handle( unique_handle const & ) = delete;
	unique_handle & operator=( unique_handle const & ) = delete;
	void close( ) noexcept {
		if ( m_value != Traits::invalid( ) ) {
			Traits::close( m_value );
			}
		}
	Type m_value;
public:
	explicit unique_handle( Type value = Traits::invalid( ) ) noexcept :
		m_value( value ) { }
	~unique_handle( ) noexcept {
		close( );
		}
	unique_handle( unique_handle && other ) noexcept :
		m_value( other.release( ) ) { }
	unique_handle & operator=( unique_handle && other ) noexcept {
		reset( other.release( ) );
		return *this;
		}
	struct boolean_struct {
		int member;
		};
public:
	Type get( ) const noexcept {
		return m_value;
		}
	void reset( Type value = Traits::invalid( ) ) noexcept {
		if ( m_value != value ) {
			close( );
			m_value = value;
			}
		return;
		}
	Type release( ) noexcept {
		const auto value = m_value;
		m_value = Traits::invalid( );
		return value;
		}
};

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


struct file_traits {
	static HANDLE invalid( ) noexcept {
		return INVALID_HANDLE_VALUE;
		}

	static void close ( const HANDLE value ) noexcept {
		const BOOL close_result = ::CloseHandle( value );
		if ( close_result == 0 ) {
			::puts("CloseHandle error!\r\n");
			::wprintf_s(L"CloseHandle error: %s\r\n", handyDandyErrMsgFormatter( ).c_str( ) );
			std::terminate();
			}
		}
	};

//struct work_traits {
//	static PTP_WORK invalid( ) noexcept {
//		return nullptr;
//		}
//
//	static void close( PTP_WORK const value ) noexcept {
//		::CloseThreadpoolWork( value );
//		}
//	};

//typedef unique_handle<PTP_WORK, work_traits> work;

struct check_failed {
	explicit check_failed( long result ) :
		error( result ) { }
	long error;
	};

//inline void check_bool( const work& result ) {
//	if ( !(result.get()) ) {
//		throw check_failed( ::GetLastError( ) );
//		}
//	}
//
//inline void check_bool( const BOOL& result ) {
//	if ( !(result) ) {
//		throw check_failed( ::GetLastError( ) );
//		}
//	}

template <typename T>
void check( T expected, T actual ) {
	if ( expected != actual ) {
		throw check_failed( 0 );
		}
	}


struct pool_traits {
	static PTP_POOL invalid( ) noexcept {
		return nullptr;
		}

	static void close( PTP_POOL pool ) noexcept {
		::CloseThreadpool( pool );
		}
	};

struct cleanup_group_traits {
	static PTP_CLEANUP_GROUP invalid( ) noexcept {
		return nullptr;
		}

	static void close( PTP_CLEANUP_GROUP cleanupgroup ) noexcept {
		::CloseThreadpoolCleanupGroup( cleanupgroup );
		}
	};


struct unicode_string_dynamic_memory_manager {
	unicode_string_dynamic_memory_manager(_In_range_( <, ( USHRT_MAX / sizeof(wchar_t) ) ) rsize_t size) : storage(size) {
		assert( ( size * sizeof(wchar_t) ) < USHRT_MAX );
		managed.MaximumLength = static_cast<USHORT>( size * sizeof(wchar_t) );
		managed.Length = 0;
		managed.Buffer = nullptr;
	}
	unicode_string_dynamic_memory_manager(std::wstring string) : storage(std::size(string) + 10) {
		if ( storage.size() >= USHRT_MAX ) {
			storage.resize( USHRT_MAX );
			}

		storage.assign(std::begin(string), std::end(string));
		storage.emplace_back(L'\0');
		assert( ( storage.capacity() * sizeof(wchar_t) ) < USHRT_MAX );
		managed.MaximumLength = static_cast<USHORT>( storage.capacity() * sizeof(wchar_t) );
		managed.Length = static_cast<USHORT>( storage.size() * sizeof(wchar_t) );
		managed.Buffer = storage.data();
	}

	unicode_string_dynamic_memory_manager(unicode_string_dynamic_memory_manager&) = delete;
	unicode_string_dynamic_memory_manager& operator=(unicode_string_dynamic_memory_manager&) =delete;
	std::vector<wchar_t> storage;
	UNICODE_STRING managed;
};



//template <PTP_WORK_CALLBACK callback, PVOID const context>
//class functional_pool {
//	TP_CALLBACK_ENVIRON CallBackEnviron;
//	work m_work;
//	PTP_CLEANUP_GROUP cleanupgroup;
//	PTP_POOL pool;
//
//public:
//	functional_pool( ) {
//		::InitializeThreadpoolEnvironment( &CallBackEnviron );
//		pool = ::CreateThreadpool( NULL )
//		if ( NULL == pool ) {
//			wprintf_s(L"CreateThreadpool failed. LastError: %u\n", ::GetLastError( ) );
//			std::terminate();
//			}
//		cleanupgroup = ::CreateThreadpoolCleanupGroup( );
//		if ( NULL == cleanupgroup ) {
//			wprintf_s( L"CreateThreadpoolCleanupGroup failed. LastError: %u\n", ::GetLastError( ) );
//			std::terminate();
//			}
//
//		// Associate the callback environment with our thread pool.
//		::SetThreadpoolCallbackPool( &CallBackEnviron, pool );
//		::SetThreadpoolCallbackCleanupGroup( &CallBackEnviron, cleanupgroup, NULL );
//
//		m_work = ::CreateThreadpoolWork( callback, context, nullptr )
//		check_bool( m_work );
//		}
//	//template <typename Function>
//	//void submit( Function const & function ) {
//	//	::SubmitThreadpoolWork( m_work.get( ) );
//	//	}
//	~functional_pool( ) {
//		::WaitForThreadpoolWorkCallbacks( m_work.get( ), true );
//		::CloseThreadpoolCleanupGroupMembers( cleanupgroup, FALSE, NULL );
//		::CloseThreadpoolCleanupGroup( cleanupgroup );
//		::CloseThreadpool( pool );
//		}
//	};

void CALLBACK hard_work(PTP_CALLBACK_INSTANCE, void* /*context*/, PTP_WORK) {}
void CALLBACK simple_work(PTP_CALLBACK_INSTANCE, void * /*context*/) {}

NtdllWrap::NtdllWrap( ) {
	hntdll = ::GetModuleHandleW( L"C:\\Windows\\System32\\ntdll.dll" );
	if ( !hntdll) {
		::fwprintf_s( stderr, L"Couldn't load ntdll.dll!\r\n" );
		std::terminate( );
		}
	NtQueryDirectoryFile_f = reinterpret_cast<NtQueryDirectoryFile_t>( ::GetProcAddress( hntdll, "NtQueryDirectoryFile" ) );
	if ( !NtQueryDirectoryFile_f ) {
		::fwprintf_s( stderr, L"Couldn't find NtQueryDirectoryFile in ntdll.dll!\r\n" );
		}
	RtlDosPathNameToNtPathName_U_WithStatus_f = reinterpret_cast<RTL_DOS_PATH_NAME_TO_NT_PATH_NAME_U_WITHSTATUS>( ::GetProcAddress( hntdll, "RtlDosPathNameToNtPathName_U_WithStatus" ) );
	if ( !RtlDosPathNameToNtPathName_U_WithStatus_f ) {
		::fwprintf_s( stderr, L"Couldn't find RtlDosPathNameToNtPathName_U_WithStatus in ntdll.dll!\r\n" );
		}
	RtlIsDosDeviceName_U_f = reinterpret_cast<RtlIsDosDeviceName_U_t>( ::GetProcAddress( hntdll, "RtlIsDosDeviceName_U" ) );
	if ( !RtlIsDosDeviceName_U_f ) {
		::fwprintf_s( stderr, L"Couldn't find RtlIsDosDeviceName_U in ntdll.dll!\r\n" );
		}

	RtlAppendUnicodeStringToString_f = reinterpret_cast<RtlAppendUnicodeStringToString_t>( ::GetProcAddress( hntdll, "RtlAppendUnicodeStringToString" ) );
	if ( !RtlAppendUnicodeStringToString_f ) {
		::fwprintf_s( stderr, L"Couldn't find RtlAppendUnicodeStringToString in ntdll.dll!\r\n" );
		}
	RtlAppendUnicodeToString_f = reinterpret_cast<RtlAppendUnicodeToString_t>( ::GetProcAddress( hntdll, "RtlAppendUnicodeToString" ) );
	if ( !RtlAppendUnicodeToString_f ) {
		::fwprintf_s( stderr, L"Couldn't find RtlAppendUnicodeToString_f in ntdll.dll!\r\n" );
		}

	NtOpenFile_f = reinterpret_cast<NtOpenFile_t>( ::GetProcAddress( hntdll, "NtOpenFile" ) );
	if ( !NtOpenFile_f ) {
		::fwprintf_s( stderr, L"Couldn't find NtOpenFile in ntdll.dll!\r\n" );
		}
	NtClose_f = reinterpret_cast<NtClose_t>( ::GetProcAddress( hntdll, "NtClose" ) );
	if ( !NtClose_f ) {
		::fwprintf_s( stderr, L"Couldn't find NtClose in ntdll.dll!\r\n" );
		}

	NtCreateFile_f = reinterpret_cast<NtCreateFile_t>( ::GetProcAddress( hntdll, "NtCreateFile" ) );
	if ( !NtClose_f ) {
		::fwprintf_s( stderr, L"Couldn't find NtCreateFile in ntdll.dll!\r\n" );
		}
	RtlReleaseRelativeName_f = reinterpret_cast<RtlReleaseRelativeName_t>( ::GetProcAddress( hntdll, "RtlReleaseRelativeName" ) );
	if ( !RtlReleaseRelativeName_f ) {
		::fwprintf_s(stderr, L"Couldn't find RtlReleaseRelativeName in ntdll.dll!\r\n");
		} 
	}

static NtdllWrap ntdll;


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
				::fwprintf_s( stderr, L"Error! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
				return UINT64_MAX;// IN case of an error return size from CFileFind object
				}
			::fwprintf_s( stderr, L"WTF ERROR! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
			return UINT64_MAX;
			}
		if ( ( last_err != NO_ERROR ) && ( last_err != last_err_old ) ) {
			::fwprintf_s( stderr, L"Error! Filepath: %s, Filepath length: %i, GetLastError: %s\r\n", path.c_str( ), int( path.length( ) ), handyDandyErrMsgFormatter( ).c_str( ) );
			return UINT64_MAX;
			}
		}
	return ret.QuadPart;
	}

//template <typename T>
//struct directory_walker {
//	using coroutine_handle_type = std::experimental::coroutine_handle<awaitable_directory_type>;
//	coroutine_handle_type m_coroutine;
//	struct awaitable_directory_type {
//		std::atomic<bool> done = false;
//		bool await_ready() const noexcept {
//			return (!m_coroutine) || m_coroutine.done( );
//			}
//		void await_suspend(std::experimental::coroutine_handle<>);
//		T await_resume();
//		};
//	};


	//I do this to ensure there are NO issues with incorrectly sized buffers or mismatching parameters (or any other bad changes)
constexpr FILE_INFORMATION_CLASS InfoClass = FileDirectoryInformation;
typedef FILE_DIRECTORY_INFORMATION THIS_FILE_INFORMATION_CLASS;
typedef THIS_FILE_INFORMATION_CLASS* PTHIS_FILE_INFORMATION_CLASS;


void displayInfo( const THIS_FILE_INFORMATION_CLASS* const pFileInf, const std::wstring &dir, PCWSTR const level_str ) {
	if ( writeToScreen ) {
		const std::wstring this_file_name( pFileInf->FileName, (pFileInf->FileNameLength / sizeof( WCHAR )) );
		const std::wstring some_name( dir + L'\\' + this_file_name );
		if ( !(pFileInf->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ) {
			::wprintf_s( L"%sAttributes for file: %s\r\n", level_str, some_name.c_str( ) );
			::wprintf_s( L"%s\tFILE_ATTRIBUTE_DIRECTORY: %s\r\n", level_str, ((pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? L"YES" : L"NO") );
			::wprintf_s( L"%s\tFILE_ATTRIBUTE_COMPRESSED: %s\r\n", level_str, ((pFileInf->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) ? L"YES" : L"NO") );
			}
		}
	}

constexpr rsize_t FileDirectoryInformationFileNameRequiredBufferCountWithNull( const FILE_DIRECTORY_INFORMATION* const pFileInf ) {
	return ( pFileInf->FileNameLength / sizeof( WCHAR ) ) + 1;
	}

constexpr PCWCHAR FileDirectoryInformationFileNameEndPointer( const FILE_DIRECTORY_INFORMATION* const pFileInf ) {
	return pFileInf->FileName + ( pFileInf->FileNameLength / sizeof( WCHAR ) );
	}

void writeCompressedFileSizeInfoToScreen( const THIS_FILE_INFORMATION_CLASS* const pFileInf, const std::wstring &dir, const PCWSTR &fNameChar, PCWSTR const level_str ) {
	//wprintf_s( L"%I64d    %s\\%s\r\n", std::int64_t( pFileInf->FileId.QuadPart ), curDir.c_str( ), fNameChar );
	if ( pFileInf->FileAttributes & FILE_ATTRIBUTE_COMPRESSED ) {
		::wprintf_s( L"%sAllocationSize: %I64d    %s\\%s\r\n", level_str, std::int64_t( pFileInf->AllocationSize.QuadPart ), dir.c_str( ), fNameChar );
	}
}

bool openDirectoryHandle( const std::wstring& dir, _Out_ HANDLE* const new_handle ) {
	(*new_handle) = ::CreateFileW(dir.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED, NULL);
	if ( (*new_handle) == INVALID_HANDLE_VALUE ) {
		const DWORD err = ::GetLastError( );
		if ( err == ERROR_ACCESS_DENIED ) {
			return false;
			}
		::fwprintf_s( stderr, L"Failed to open directory %s because of error %lu\r\n", dir.c_str( ), err );
		::fwprintf_s( stderr, L"err: `%lu` means: %s\r\n", err, handyDandyErrMsgFormatter( ).c_str( ) );
		return false;
		}
	return true;
	}

struct Directory_recursive_info {
	Directory_recursive_info(Directory_recursive_info&& in) = default;
	Directory_recursive_info() = default;
	Directory_recursive_info& operator=(const Directory_recursive_info& in) = default;
	std::uint64_t total_size = 0;
	std::uint64_t numItems = 0;
	};

struct Directory_ThreadPool_Folder_Context;

//template<typename T>
//struct std::experimental::coroutine_traits<std::future<T>> {
	//struct promise_type {
		//std::promise<T> promise;
		//std::future<T> get_return_object() {
			//promise.get_future();
			//}
		//auto initial_suspend() {
			//return std::experimental::suspend_never();
			//}
		//auto final_suspend() {
			//return std::experimental::suspend_never();
			//}
		//void return_value(T v) {
			//promise.set_value(v);
			//}
		//void set_exception(std::exception_ptr exc) {
			//promise.set_exception(exc);
			//}
		//};
//	};

struct Directory_ThreadPool_Context {
	Directory_ThreadPool_Context() = default;
	Directory_ThreadPool_Context(Directory_ThreadPool_Context&& in ) {
		//pool_raw = in.pool_raw;
		this_query_dir = std::move( in.this_query_dir );
		level = in.level;
		recursive_info = in.recursive_info;
		nt_dir_handle = in.nt_dir_handle;
		//CallBackEnviron = in.CallBackEnviron;
		//cleanupgroup = std::move(in.cleanupgroup);
		}
	Directory_ThreadPool_Context(const Directory_ThreadPool_Context& in ) {
		this_query_dir = in.this_query_dir;
		level = in.level;
		recursive_info = in.recursive_info;
		nt_dir_handle = in.nt_dir_handle;
		}
	PTP_POOL pool_raw; // Our pool
	std::wstring this_query_dir;
	unsigned int level;
	Directory_recursive_info recursive_info;
	HANDLE nt_dir_handle;
	TP_CALLBACK_ENVIRON CallBackEnviron;
	//unique_handle<PTP_CLEANUP_GROUP, cleanup_group_traits> cleanupgroup;

};

struct resume_background {
	bool await_ready() { return false; }
	void await_resume() { };
	void await_suspend(std::experimental::coroutine_handle<> handle) {
		const auto tp_lambda = [](PTP_CALLBACK_INSTANCE, void* context) {
			//operator() calls .resume()
			std::experimental::coroutine_handle<>::from_address(context)();
			};
		const BOOL submit_result = ::TrySubmitThreadpoolCallback(tp_lambda, handle.address(), nullptr);
		if (!submit_result) {
			wprintf_s( L"TrySubmitThreadpoolCallback failed! Error: %s\r\n", handyDandyErrMsgFormatter().c_str() );
			std::terminate();
			}		
		}
	};

bool await_ready(const std::future<Directory_recursive_info>& dir_future ) {
	return dir_future.valid();
	}

void await_suspend(std::future<Directory_recursive_info>& /*dir_future*/, std::experimental::coroutine_handle<> handle) {
	const auto tp_lambda = [](PTP_CALLBACK_INSTANCE, void* context) {
		//operator() calls .resume()
		std::experimental::coroutine_handle<>::from_address(context)();
		};
	const BOOL submit_result = ::TrySubmitThreadpoolCallback(tp_lambda, handle.address(), nullptr);
	if (!submit_result) {
		wprintf_s( L"TrySubmitThreadpoolCallback failed! Error: %s\r\n", handyDandyErrMsgFormatter().c_str() );
		std::terminate();
		}
	}

Directory_recursive_info await_resume( std::future<Directory_recursive_info>& dir_future ) {
	return dir_future.get();
	}

std::unique_ptr<wchar_t[]> build_level_str( unsigned int level ) {
	std::unique_ptr<wchar_t[]> level_str{ std::make_unique<wchar_t[]>( level + 1 )};
	::wmemset(level_str.get(), L'\t', level );
	level_str[level] = 0;
	return level_str;
	}

std::future<Directory_recursive_info> qDirRecursive( Directory_ThreadPool_Context Context ) {
	//co_await resume_background();


	//Directory_recursive_info recursive_info{ };
	Directory_recursive_info this_recursive_info{};
	

	HANDLE nt_dir_handle_raw;
	if ( !openDirectoryHandle( Context.this_query_dir, &nt_dir_handle_raw ) ) {
		co_return Directory_recursive_info();
		}

	unique_handle<HANDLE, file_traits> nt_dir_handle( nt_dir_handle_raw );



	const ULONG init_bufSize = ((sizeof( FILE_ID_BOTH_DIR_INFORMATION ) + (MAX_PATH * sizeof( wchar_t ))) * 100);

	std::unique_ptr<__declspec(align(8)) wchar_t[ ]> idInfo { std::make_unique<__declspec(align(8)) wchar_t[ ]>( init_bufSize ) };
	::wmemset( idInfo.get( ), 0, init_bufSize );


	std::vector<Directory_recursive_info> futureDirs;

	IO_STATUS_BLOCK iosb{ };

	//UNICODE_STRING _glob;

	NTSTATUS query_directory_result = STATUS_PENDING;

	if constexpr( writeToScreen ) {
		std::unique_ptr<wchar_t[]> level_str( build_level_str( Context.level));
		::wprintf_s( L"%sFiles in directory %s\r\n", level_str.get(), Context.this_query_dir.c_str( ) );
		}

	assert( init_bufSize > 1 );
	const NTSTATUS sBefore = query_directory_result;
	query_directory_result = ntdll.NtQueryDirectoryFile_f( nt_dir_handle.get(), NULL, NULL, NULL, &iosb, idInfo.get( ), init_bufSize, InfoClass, FALSE, NULL, TRUE );
	if ( (query_directory_result == STATUS_TIMEOUT) || (query_directory_result == STATUS_PENDING) ) {
		std::terminate( );
		}
	assert( NT_SUCCESS( query_directory_result ) );
	assert( query_directory_result != sBefore );
	assert( ::GetLastError( ) != ERROR_MORE_DATA );

	ULONG bufSize = init_bufSize;
	while ( query_directory_result == STATUS_BUFFER_OVERFLOW ) {
		idInfo.reset( );
		bufSize *= 2;
		idInfo = std::make_unique<__declspec(align(8)) wchar_t[ ]>( bufSize );
		query_directory_result = /*co_await?*/ ntdll.NtQueryDirectoryFile_f( nt_dir_handle.get(), NULL, NULL, NULL, &iosb, idInfo.get( ), bufSize, InfoClass, FALSE, NULL, TRUE );
		}
	assert( NT_SUCCESS( query_directory_result ) );
	//bool id_info_heap = false;
	const ULONG_PTR bufSizeWritten = iosb.Information;


	const ULONG_PTR count_records = bufSizeWritten / sizeof( THIS_FILE_INFORMATION_CLASS );
	const THIS_FILE_INFORMATION_CLASS* pFileInf = reinterpret_cast<PTHIS_FILE_INFORMATION_CLASS>(idInfo.get( ));

	std::vector<std::wstring> breadthDirs;
	
	//absolute guess
	breadthDirs.reserve(10);

	assert( pFileInf != NULL );
	while ( NT_SUCCESS( query_directory_result ) && (pFileInf != NULL) ) {

		assert( pFileInf->FileNameLength > 1 );
		if ( pFileInf->FileName[ 0 ] == L'.' && (pFileInf->FileName[ 1 ] == 0 || (pFileInf->FileName[ 1 ] == '.')) ) {
			//continue;
			goto nextItem;
			}

		this_recursive_info.total_size += pFileInf->AllocationSize.QuadPart;
		//const auto lores = GetCompressedFileSizeW( , ) 
		{
			std::unique_ptr<wchar_t[]> level_str( build_level_str( Context.level));
			displayInfo( pFileInf, Context.this_query_dir, level_str.get( ) );
		}
		++(this_recursive_info.numItems);
		if ( (pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) || writeToScreen ) {//I'd like to avoid building a null terminated string unless it is necessary

			PCWCHAR const end = pFileInf->FileName + (pFileInf->FileNameLength / sizeof( WCHAR ));

			if ( writeToScreen ) {
				std::vector<WCHAR> fNameVect;
				fNameVect.insert( fNameVect.end( ), pFileInf->FileName, end );
				fNameVect.emplace_back( L'\0' );
				PCWSTR const fNameChar = &(fNameVect[ 0 ]);
				fNameVect.reserve( FileDirectoryInformationFileNameRequiredBufferCountWithNull( pFileInf ) );
				std::unique_ptr<wchar_t[]> level_str( build_level_str( Context.level));
				writeCompressedFileSizeInfoToScreen( pFileInf, Context.this_query_dir, fNameChar, level_str.get( ) );
				}
			if ( pFileInf->FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				breadthDirs.emplace_back( std::wstring(pFileInf->FileName, (pFileInf->FileNameLength / sizeof( WCHAR ))) );
				}
			}

	nextItem:
		//stat = NtQueryDirectoryFile( hDir, NULL, NULL, NULL, &iosb, &idInfo[ 0 ], idInfo.size( ), FileIdBothDirectoryInformation, TRUE, NULL, FALSE );
		if ( writeToScreen ) {
			::wprintf_s( L"\t\tpFileInf: %p, pFileInf->NextEntryOffset: %lu, ( pFileInf + pFileInf->NextEntryOffset ): %p\r\n", pFileInf, pFileInf->NextEntryOffset, (pFileInf + pFileInf->NextEntryOffset) );
			}
		if ( pFileInf->NextEntryOffset != 0 ) {
			const std::uintptr_t address_of_next_entry = reinterpret_cast<std::uintptr_t>(pFileInf) + static_cast<std::uintptr_t>(pFileInf->NextEntryOffset);
			pFileInf = reinterpret_cast<PTHIS_FILE_INFORMATION_CLASS>(address_of_next_entry);
			}
		else {
			pFileInf = nullptr;
			}
		}
	//std::vector<Directory_ThreadPool_Context> subdir_tp_context(breadthDirs.size());
	//const size_t subdir_tp_context_byte_size = sizeof(Directory_ThreadPool_Context) * breadthDirs.size();
	

	//memset(subdir_tp_context.get(), 0, subdir_tp_context_byte_size);

	for ( rsize_t i = 0u; i < breadthDirs.size(); ++i ) {
		Directory_ThreadPool_Context this_subdir_tp_context{};
		this_subdir_tp_context.level = Context.level + 1;
		//this_subdir_tp_context.pool_raw = tp_context.pool_raw;
		this_subdir_tp_context.recursive_info = Directory_recursive_info{};
		if ( Context.this_query_dir[ Context.this_query_dir.length( ) - 1 ] != L'\\' ) {
			//breadthDirs.emplace_back( std::wstring( curDir ) + L'\\' + fNameChar + L'\\' );
			auto query = std::wstring( Context.this_query_dir + L'\\' + breadthDirs[i] + L'\\' );
			this_subdir_tp_context.this_query_dir = query;
			//qDirRecursive( Instance, this_subdir_tp_context, Work );
			futureDirs.emplace_back( co_await qDirRecursive( std::move( this_subdir_tp_context ) ) );

			//futureDirs.emplace_back( std::async( std::launch::async | std::launch::deferred, ListDirectory, query, writeToScreen, ntdll ) );
			}
		else {
			//breadthDirs.emplace_back( std::wstring( curDir ) + fNameChar + L'\\' );
			auto query = std::wstring( Context.this_query_dir + breadthDirs[i] + L'\\' );
			this_subdir_tp_context.this_query_dir = query;
			//qDirRecursive( Instance, this_subdir_tp_context, Work );
			futureDirs.emplace_back( co_await qDirRecursive( std::move( this_subdir_tp_context ) ) );

			//futureDirs.emplace_back( std::async( std::launch::async | std::launch::deferred, ListDirectory, query, writeToScreen, ntdll ) );
			}	
		}

	//::CloseThreadpoolCleanupGroupMembers( tp_context.cleanupgroup.get(), FALSE, NULL );

	for ( rsize_t i = 0u; i < breadthDirs.size(); ++i ) {
		const Directory_recursive_info recursive_info = std::move(futureDirs[i]);
		this_recursive_info.numItems += recursive_info.numItems;
		this_recursive_info.total_size += recursive_info.total_size;
		}
	co_return this_recursive_info;
	}

void stdRecurseFindFutures( const std::wstring raw_dir ) {
	const std::wstring dir = L"\\\\?\\" + raw_dir;
	Directory_ThreadPool_Context tp_context;
	tp_context.this_query_dir = dir;
	tp_context.level = 2u;
	tp_context.recursive_info = Directory_recursive_info{};
	//tp_context.pool_raw = nullptr;

	//UNICODE_STRING path = {};
	unicode_string_dynamic_memory_manager path(raw_dir);
	RTL_RELATIVE_NAME rtl_rel_name = {};
	PCWSTR ntpart = nullptr;
	////::RtlInitUnicodeString(&path, dir.c_str());
	const NTSTATUS conversion_result = ntdll.RtlDosPathNameToNtPathName_U_WithStatus_f(raw_dir.c_str(), &(path.managed), &ntpart, &rtl_rel_name);
	if ( !NT_SUCCESS( conversion_result ) ) {
		fwprintf_s( stderr, L"RtlDosPathNameToNtPathName_U_WithStatus failed!\r\n" );
		std::terminate();
		}
	else {
		fwprintf_s( stderr, L"RtlDosPathNameToNtPathName_U_WithStatus succeeded! ntpart: %s\r\n", ntpart );
		}


	//
	//const ULONG is_dos_device = ntdll.RtlIsDosDeviceName_U_f( raw_dir.c_str( ) );
	//if ( is_dos_device ) {
	//	fwprintf_s( stderr, L"'%s' (ntpart) is a DOS device!\r\n", ntpart );
	//	std::terminate();
	//	}
	//HANDLE nt_dir_handle = INVALID_HANDLE_VALUE;
	//OBJECT_ATTRIBUTES oa = {};
	//IO_STATUS_BLOCK iosb = {};
	//
	//InitializeObjectAttributes(&oa, &path, OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE , NULL, NULL);
	//
	//const NTSTATUS open_result = ntdll.NtOpenFile_f(&nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA, &oa, &iosb,FILE_SHARE_READ, FILE_DIRECTORY_FILE| FILE_NON_DIRECTORY_FILE);
	//const NTSTATUS open_result = ntdll.NtCreateFile_f( &nt_dir_handle, FILE_READ_ATTRIBUTES | FILE_READ_EA | FILE_LIST_DIRECTORY, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, FILE_DIRECTORY_FILE, NULL, NULL );
	//
	//
	//if ( !NT_SUCCESS( open_result ) ) {
		//fwprintf_s( stderr, L"NtOpenFile failed!\r\n" );
		//std::terminate();
		//}
	//NtQueryDirectory
	const NTSTATUS release_result = ntdll.RtlReleaseRelativeName_f(&rtl_rel_name);
	if(!NT_SUCCESS(release_result)) {
		fwprintf_s( stderr, L"RtlReleaseRelativeName_f failed!\r\n" );
		std::terminate();
		}

	// Create a custom, dedicated thread pool.
	 unique_handle<PTP_POOL, pool_traits> const pool_owner ( ::CreateThreadpool( NULL ) );
	if ( NULL == pool_owner.get() ) {
		::wprintf_s(L"CreateThreadpool failed. LastError: %u\n", ::GetLastError( ) );
		return;
		}

	tp_context.pool_raw = pool_owner.get();

	//::SetThreadpoolThreadMaximum(pool_owner.get(), 16u);

	//{
	//	void * context = nullptr;
	//	work w(::CreateThreadpoolWork(hard_work, context, nullptr));
	//	check_bool(w);

	//	::SubmitThreadpoolWork(w.get());
	//	bool cancel = false;
	//	WaitForThreadpoolWorkCallbacks(w.get(), cancel);
	//}

	//{
	//	void * context = nullptr;
	//	check_bool(::TrySubmitThreadpoolCallback(
	//	simple_work, context, nullptr));
	//}

		// Create a cleanup group for this thread pool.
	//tp_context.cleanupgroup.reset( ::CreateThreadpoolCleanupGroup( ) );
	//if ( NULL == tp_context.cleanupgroup.get() ) {
	//	::wprintf_s( L"CreateThreadpoolCleanupGroup failed. LastError: %u\n", ::GetLastError( ) );
	//	return;
	//	}


	// Associate the cleanup group with our thread pool.
	// Objects created with the same callback environment
	// as the cleanup group become members of the cleanup group.
	//::SetThreadpoolCallbackCleanupGroup( &(tp_context.CallBackEnviron), tp_context.cleanupgroup.get(), NULL );


	::InitializeThreadpoolEnvironment( &(tp_context.CallBackEnviron) );
	//PTP_WORK work = ::CreateThreadpoolWork(qDirRecursive, &tp_context, &(tp_context.CallBackEnviron));
	//::SubmitThreadpoolWork(work);
	//::WaitForThreadpoolWorkCallbacks(work, FALSE);
	//qDirRecursive( nullptr, &tp_context, nullptr );

	std::future<Directory_recursive_info> all_dirs_fut = qDirRecursive(std::move(tp_context));
	Directory_recursive_info all_dirs = all_dirs_fut.get();



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
	//	fwprintf_s( stderr, L"Closing handle nt_dir_handle (%p) failed!\r\n\tResult code: %li\r\n", nt_dir_handle, close_result );
	//	}

	//if (writeToScreen) {
		::wprintf_s(L"Total number of files: %I64u\r\n", all_dirs.numItems);
		::wprintf_s(L"Total size of files: %I64u\r\n", all_dirs.total_size);
		//}
	return;
	}


int wmain( int argc,  _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR*[ ] ) {
	if ( argc < 2 ) {
		::puts("Need more than 1 argument!\r\n");
		return ERROR_BAD_ARGUMENTS;
		}
	//for ( uint32_t i = 0; i < ( UINT32_MAX - 2 ); ++i) {
		stdRecurseFindFutures(std::wstring(argv[1]));
		//}
	::puts("---------------------\r\n");
	return 0;
	}
