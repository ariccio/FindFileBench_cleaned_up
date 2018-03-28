#pragma once

#include "stdafx.h"
#include "resource.h"


typedef NTSYSCALLAPI
NTSTATUS
(NTAPI*
NtQueryDirectoryFile_t) (
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan
    );

typedef struct _RTL_RELATIVE_NAME {
  UNICODE_STRING RelativeName;
  HANDLE         ContainingDirectory;
  void*          CurDirRef;
} RTL_RELATIVE_NAME, *PRTL_RELATIVE_NAME;

typedef NTSYSCALLAPI NTSTATUS (NTAPI *RTL_DOS_PATH_NAME_TO_NT_PATH_NAME_U_WITHSTATUS)(
	_In_opt_z_ PCWSTR DosPathName,
	_Out_ PUNICODE_STRING NtPathName,
	_Out_opt_ PCWSTR* NtFileNamePart,
	_Out_opt_ PRTL_RELATIVE_NAME RelativeName);

//RTL_DOS_PATH_NAME_TO_NT_PATH_NAME_U RtlDosPathNameToNtPathName_t;

typedef NTSYSCALLAPI
NTSTATUS
(NTAPI*
NtOpenFile_t) (
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions
    );

typedef NTSYSCALLAPI
NTSTATUS
(NTAPI*
NtCreateFile_t) (
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength
    );


typedef NTSYSCALLAPI
NTSTATUS
(NTAPI*
NtClose_t) (
    _In_ _Post_ptr_invalid_ HANDLE Handle
    );

typedef NTSYSCALLAPI ULONG
(NTAPI* RtlIsDosDeviceName_U_t) (
	_In_ PCWSTR Name);


_Success_(1)
_Unchanged_(Destination->MaximumLength)
_Unchanged_(Destination->Buffer)
_When_(_Old_(Destination->Length) + Source->Length <= Destination->MaximumLength,
    _At_(Destination->Length,
         _Post_equal_to_(_Old_(Destination->Length) + Source->Length))
    _At_(return, _Out_range_(==, 0)))
_When_(_Old_(Destination->Length) + Source->Length > Destination->MaximumLength,
    _Unchanged_(Destination->Length)
    _At_(return, _Out_range_(<, 0)))
typedef NTSYSAPI
NTSTATUS
(NTAPI*
RtlAppendUnicodeStringToString_t) (
    _Inout_ PUNICODE_STRING Destination,
    _In_ PCUNICODE_STRING Source
    );

_Success_(1)
_Unchanged_(Destination->MaximumLength)
_Unchanged_(Destination->Buffer)
_When_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR) <= Destination->MaximumLength,
    _At_(Destination->Length,
         _Post_equal_to_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR)))
    _At_(return, _Out_range_(==, 0)))
_When_(_Old_(Destination->Length) + _String_length_(Source) * sizeof(WCHAR) > Destination->MaximumLength,
    _Unchanged_(Destination->Length)
    _At_(return, _Out_range_(<, 0)))
typedef NTSYSAPI
NTSTATUS
(NTAPI*
RtlAppendUnicodeToString_t) (
    _Inout_ PUNICODE_STRING Destination,
    _In_opt_z_ PCWSTR Source
    );


struct NtdllWrap {
	HMODULE ntdll = nullptr;
	//FARPROC ntQueryDirectoryFuncPtr = nullptr;

	NtQueryDirectoryFile_t NtQueryDirectoryFile_f;
	RTL_DOS_PATH_NAME_TO_NT_PATH_NAME_U_WITHSTATUS RtlDosPathNameToNtPathName_U_WithStatus_f;
	RtlIsDosDeviceName_U_t RtlIsDosDeviceName_U_f;
	RtlAppendUnicodeStringToString_t RtlAppendUnicodeStringToString_f;
	RtlAppendUnicodeToString_t RtlAppendUnicodeToString_f;
	NtOpenFile_t NtOpenFile_f;
	NtCreateFile_t NtCreateFile_f;
	NtClose_t NtClose_f;
	NtdllWrap( );
	NtdllWrap( NtdllWrap& in ) = delete;
	};
