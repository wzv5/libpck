#pragma once

// C语言接口，用于跨编程语言调用

#include <cstdint>

typedef void* PckFile_c;
typedef const void* PckItem_c;
typedef bool(__stdcall *ProcessCallback_c)(uint32_t index, uint32_t total);

PckFile_c __stdcall Pck_Open(const char* filename, bool readonly = true);
PckFile_c __stdcall Pck_Create(const char* filename, bool overwrite = false);
void __stdcall Pck_Release(PckFile_c pck);
bool __stdcall Pck_GetLastError();
const char* __stdcall Pck_GetLastErrorMessage();

uint32_t __stdcall Pck_GetFileCount(PckFile_c pck);
PckItem_c __stdcall Pck_GetFileItem_name(PckFile_c pck, const char* filename);
PckItem_c __stdcall Pck_GetFileItem_index(PckFile_c pck, uint32_t index);

const char* __stdcall Pck_Item_GetFileName(PckItem_c item);
uint32_t __stdcall Pck_Item_GetFileDataSize(PckItem_c item);
bool __stdcall Pck_Item_GetFileData(PckItem_c item, void* buf);
uint32_t __stdcall Pck_Item_GetFileCompressDataSize(PckItem_c item);
bool __stdcall Pck_Item_GetFileCompressData(PckItem_c item, void* buf);

bool __stdcall Pck_AddItem_buf(PckFile_c pck, const void* buf, uint32_t len, const char* filename);
bool __stdcall Pck_AddItem_item(PckFile_c pck, PckItem_c item);
bool __stdcall Pck_AddItem_file(PckFile_c pck, const char* diskfilename, const char* pckfilename);
bool __stdcall Pck_DeleteItem(PckFile_c pck, PckItem_c item);
bool __stdcall Pck_RenameItem(PckFile_c pck, PckItem_c item, const char* newname);
bool __stdcall Pck_UpdateItem_buf(PckFile_c pck, PckItem_c item, const void* buf, uint32_t len);
bool __stdcall Pck_UpdateItem_file(PckFile_c pck, PckItem_c item, const char* diskfilename);

bool __stdcall Pck_Extract(PckFile_c pck, const char* dir, ProcessCallback_c callback = NULL);
bool __stdcall Pck_Extract_if(PckFile_c pck, const char* dir, bool(__stdcall *fn)(PckItem_c), ProcessCallback_c callback = NULL);

uint32_t __stdcall Pck_GetFileSize(PckFile_c pck);
uint32_t __stdcall Pck_GetTotalDataSize(PckFile_c pck);
uint32_t __stdcall Pck_GetTotalCompressDataSize(PckFile_c pck);
uint32_t __stdcall Pck_GetRedundancySize(PckFile_c pck);
uint32_t __stdcall Pck_GetIndexTableSize(PckFile_c pck);

bool __stdcall Pck_BeginTransaction(PckFile_c pck);
bool __stdcall Pck_CancelTransaction(PckFile_c pck);
bool __stdcall Pck_CommitTransaction(PckFile_c pck, ProcessCallback_c callback = NULL);

bool __stdcall Pck_CreateFromDirectory(const char* filename, const char* dir, bool usedirname = true, bool overwrite = false, ProcessCallback_c callback = NULL);
bool __stdcall Pck_ReBuild(const char* filename, const char* newname, bool overwrite = false, ProcessCallback_c callback = NULL);
