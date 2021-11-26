#pragma once

// C语言接口，用于跨编程语言调用

#ifndef STDCALL
#ifdef WIN32
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>

typedef void* PckFile_c;
typedef const void* PckItem_c;
typedef bool(STDCALL *ProcessCallback_c)(uint32_t index, uint32_t total);

PckFile_c STDCALL Pck_Open(const char* filename, bool readonly = true);
PckFile_c STDCALL Pck_Create(const char* filename, bool overwrite = false);
void STDCALL Pck_Release(PckFile_c pck);
bool STDCALL Pck_GetLastError();
const char* STDCALL Pck_GetLastErrorMessage();

uint32_t STDCALL Pck_GetFileCount(PckFile_c pck);
PckItem_c STDCALL Pck_GetFileItem_name(PckFile_c pck, const char* filename);
PckItem_c STDCALL Pck_GetFileItem_index(PckFile_c pck, uint32_t index);

const char* STDCALL Pck_Item_GetFileName(PckItem_c item);
uint32_t STDCALL Pck_Item_GetFileDataSize(PckItem_c item);
bool STDCALL Pck_Item_GetFileData(PckItem_c item, void* buf);
uint32_t STDCALL Pck_Item_GetFileCompressDataSize(PckItem_c item);
bool STDCALL Pck_Item_GetFileCompressData(PckItem_c item, void* buf);
bool STDCALL Pck_FileExists(PckFile_c pck, const char* filename);

bool STDCALL Pck_AddItem_buf(PckFile_c pck, const void* buf, uint32_t len, const char* filename);
bool STDCALL Pck_AddItem_item(PckFile_c pck, PckItem_c item);
bool STDCALL Pck_AddItem_file(PckFile_c pck, const char* diskfilename, const char* pckfilename);
bool STDCALL Pck_DeleteItem(PckFile_c pck, PckItem_c item);
bool STDCALL Pck_RenameItem(PckFile_c pck, PckItem_c item, const char* newname);
bool STDCALL Pck_UpdateItem_buf(PckFile_c pck, PckItem_c item, const void* buf, uint32_t len);
bool STDCALL Pck_UpdateItem_file(PckFile_c pck, PckItem_c item, const char* diskfilename);
bool STDCALL Pck_DeleteDirectory(PckFile_c pck, const char* dirname);

bool STDCALL Pck_Extract(PckFile_c pck, const char* dir, ProcessCallback_c callback = NULL);
bool STDCALL Pck_Extract_if(PckFile_c pck, const char* dir, bool(STDCALL *fn)(PckItem_c), ProcessCallback_c callback = NULL);

uint64_t STDCALL Pck_GetFileSize(PckFile_c pck);
uint64_t STDCALL Pck_GetTotalDataSize(PckFile_c pck);
uint64_t STDCALL Pck_GetTotalCompressDataSize(PckFile_c pck);
uint64_t STDCALL Pck_GetRedundancySize(PckFile_c pck);
uint64_t STDCALL Pck_GetIndexTableSize(PckFile_c pck);

bool STDCALL Pck_BeginTransaction(PckFile_c pck);
bool STDCALL Pck_CancelTransaction(PckFile_c pck);
bool STDCALL Pck_CommitTransaction(PckFile_c pck, ProcessCallback_c callback = NULL);

bool STDCALL Pck_CreateFromDirectory(const char* filename, const char* dir, bool usedirname = true, bool overwrite = false, ProcessCallback_c callback = NULL);
bool STDCALL Pck_ReBuild(const char* filename, const char* newname, bool overwrite = false, ProcessCallback_c callback = NULL);

#ifdef __cplusplus
    }
#endif
