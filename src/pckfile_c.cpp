#include "pckfile.h"
#include "pckfile_c.h"
#include "pckitem.h"

struct _PckPtrHolder
{
	std::shared_ptr<PckFile> ptr;
};

// 使用线程局部存储来避免多线程冲突
thread_local bool _PckLastError = false;
thread_local char _PckLastErrorBuffer[1024];

#define PCK_SETLASTERROR()											\
	strcpy(_PckLastErrorBuffer, e.what());							\
	_PckLastError = true;											\
	return 0;

#define PCK_RESETLASTERROR()										\
	memset(_PckLastErrorBuffer, 0, sizeof(_PckLastErrorBuffer));	\
	_PckLastError = false;

#define PCK_GETPTR()												\
	std::shared_ptr<PckFile>& p = ((_PckPtrHolder*)pck)->ptr;

PckFile_c __stdcall Pck_Open(const char* filename, bool readonly)
{
	PCK_RESETLASTERROR();
	try
	{
		auto pck = PckFile::Open(filename, readonly);
		auto p = new _PckPtrHolder();
		pck.swap(p->ptr);
		return p;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

PckFile_c __stdcall Pck_Create(const char* filename, bool overwrite)
{
	PCK_RESETLASTERROR();
	try
	{
		auto pck = PckFile::Create(filename, overwrite);
		auto p = new _PckPtrHolder();
		pck.swap(p->ptr);
		return p;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

void __stdcall Pck_Release(PckFile_c pck)
{
	delete (_PckPtrHolder*)pck;
}

bool __stdcall Pck_GetLastError()
{
	return _PckLastError;
}

const char* __stdcall Pck_GetLastErrorMessage()
{
	return _PckLastErrorBuffer;
}

uint32_t __stdcall Pck_GetFileCount(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetFileCount();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

PckItem_c __stdcall Pck_GetFileItem_name(PckFile_c pck, const char* filename)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return &(p->GetSingleFileItem(filename));
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

PckItem_c __stdcall Pck_GetFileItem_index(PckFile_c pck, uint32_t index)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return &(p->GetSingleFileItem(index));
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

const char* __stdcall Pck_Item_GetFileName(PckItem_c item)
{
	PCK_RESETLASTERROR();
	try
	{
		auto p = (PckItem*)item;
		return p->GetFileName();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_Item_GetFileDataSize(PckItem_c item)
{
	PCK_RESETLASTERROR();
	try
	{
		auto p = (PckItem*)item;
		return p->GetDataSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_Item_GetFileData(PckItem_c item, void* buf)
{
	PCK_RESETLASTERROR();
	try
	{
		auto p = (PckItem*)item;
		auto data = p->GetData();
		memcpy(buf, data.data(), data.size());
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_Item_GetFileCompressDataSize(PckItem_c item)
{
	PCK_RESETLASTERROR();
	try
	{
		auto p = (PckItem*)item;
		return p->GetCompressDataSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_Item_GetFileCompressData(PckItem_c item, void* buf)
{
	PCK_RESETLASTERROR();
	try
	{
		auto p = (PckItem*)item;
		auto data = p->GetCompressData();
		memcpy(buf, data.data(), data.size());
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_AddItem_buf(PckFile_c pck, const void* buf, uint32_t len, const char* filename)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->AddItem(buf, len, filename);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_AddItem_item(PckFile_c pck, PckItem_c item)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		auto pitem = (PckItem*)item;
		p->AddItem(*pitem);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_AddItem_file(PckFile_c pck, const char* diskfilename, const char* pckfilename)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->AddItem(diskfilename, pckfilename);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_DeleteItem(PckFile_c pck, PckItem_c item)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		auto pitem = (PckItem*)item;
		p->DeleteItem(*pitem);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_RenameItem(PckFile_c pck, PckItem_c item, const char* newname)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		auto pitem = (PckItem*)item;
		p->RenameItem(*pitem, newname);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_UpdateItem_buf(PckFile_c pck, PckItem_c item, const void* buf, uint32_t len)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		auto pitem = (PckItem*)item;
		p->UpdateItem(*pitem, buf, len);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_UpdateItem_file(PckFile_c pck, PckItem_c item, const char* diskfilename)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		auto pitem = (PckItem*)item;
		p->UpdateItem(*pitem, diskfilename);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_Extract(PckFile_c pck, const char* dir, ProcessCallback_c callback)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->Extract(dir, callback ? [&callback](auto i, auto t) { return callback(i, t); } : std::function<bool(uint32_t, uint32_t)>());
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_Extract_if(PckFile_c pck, const char* dir, bool(__stdcall *fn)(PckItem_c), ProcessCallback_c callback)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->Extract_if(dir,
			fn ? [&fn](auto i) { return fn(&i); } : std::function<bool(const PckItem&)>(),
			callback ? [&callback](auto i, auto t) { return callback(i, t); } : std::function<bool(uint32_t, uint32_t)>()
		);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_GetFileSize(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetFileSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_GetTotalDataSize(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetTotalDataSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_GetTotalCompressDataSize(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetTotalCompressDataSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_GetRedundancySize(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetRedundancySize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

uint32_t __stdcall Pck_GetIndexTableSize(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		return p->GetIndexTableSize();
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_BeginTransaction(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->BeginTransaction();
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_CancelTransaction(PckFile_c pck)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->CancelTransaction();
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_CommitTransaction(PckFile_c pck, ProcessCallback_c callback)
{
	PCK_RESETLASTERROR();
	try
	{
		PCK_GETPTR();
		p->CommitTransaction(callback ? [&callback](auto i, auto t) { return callback(i, t); } : std::function<bool(uint32_t, uint32_t)>());
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_CreateFromDirectory(const char* filename, const char* dir, bool usedirname, bool overwrite, ProcessCallback_c callback)
{
	PCK_RESETLASTERROR();
	try
	{
		PckFile::CreateFromDirectory(filename, dir, usedirname, overwrite, 
			callback ? [&callback](auto i, auto t) { return callback(i, t); } : std::function<bool(uint32_t, uint32_t)>()
		);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}

bool __stdcall Pck_ReBuild(const char* filename, const char* newname, bool overwrite, ProcessCallback_c callback)
{
	PCK_RESETLASTERROR();
	try
	{
		PckFile::ReBuild(filename, newname, overwrite,
			callback ? [&callback](auto i, auto t) { return callback(i, t); } : std::function<bool(uint32_t, uint32_t)>()
		);
		return true;
	}
	catch (const std::exception& e)
	{
		PCK_SETLASTERROR();
	}
}