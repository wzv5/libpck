#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <zlib.h>
#include "pckitem.h"

enum class PckPendingActionType {
	Add,
	Delete,
	Rename,
	Update
};

class PckPendingItem
{
public:
	PckPendingItem(PckPendingActionType action) : m_action(action) {}
	virtual ~PckPendingItem() { }

	PckPendingActionType GetType() { return m_action; }

	// 在用完后随即调用，释放临时数据，避免重建操作时内存溢出
	virtual void Release() = 0;

private:
	PckPendingActionType m_action;
};

class PckPendingItem_Add : public PckPendingItem
{
public:
	PckPendingItem_Add(const std::string& filename)
		: PckPendingItem(PckPendingActionType::Add)
		, m_filename(filename)
	{
	}

	const std::string& GetFileName() const noexcept	{ return m_filename; }
	virtual size_t GetDataSize() = 0;
	virtual const std::vector<uint8_t>& GetCompressData(int level = Z_DEFAULT_COMPRESSION) = 0;

	virtual void Release() override
	{
		std::string().swap(m_filename);
	}

private:
	std::string m_filename;
};

class PckPendingItem_AddBuffer : public PckPendingItem_Add
{
public:
	PckPendingItem_AddBuffer(const std::string& filename, const void* buf, size_t len)
		: PckPendingItem_Add(filename)
		, m_data((uint8_t*)buf, (uint8_t*)buf + len)
	{
	}

	virtual size_t GetDataSize() override
	{
		return m_data.size();
	}

	virtual const std::vector<uint8_t>& GetCompressData(int level = Z_DEFAULT_COMPRESSION) override
	{
		if (GetDataSize() < PCK_BEGINCOMPRESS_SIZE)
		{
			return m_data;
		}

		if (m_compressdata.empty())
		{
			auto len = compressBound(m_data.size());
			m_compressdata.resize(len);
			auto ret = compress2(m_compressdata.data(), &len, m_data.data(), m_data.size(), level);
			if (ret != Z_OK)
			{
				throw std::runtime_error("压缩数据失败");
			}
			m_compressdata.resize(len);
		}
		return m_compressdata;
	}

	virtual void Release() override
	{
		PckPendingItem_Add::Release();
		std::vector<uint8_t>().swap(m_data);
		std::vector<uint8_t>().swap(m_compressdata);
	}

private:
	std::vector<uint8_t> m_data;
	std::vector<uint8_t> m_compressdata;
};

class PckPendingItem_AddFile : public PckPendingItem_Add
{
public:
	PckPendingItem_AddFile(const std::string& filename, const std::string& diskfile)
		: PckPendingItem_Add(filename)
		, m_diskfile(diskfile)
	{
	}

	virtual size_t GetDataSize() override
	{
		if (m_data.empty())
		{
			std::ifstream f(m_diskfile.c_str(), std::ios::in | std::ios::binary);
			if (!f.is_open())
			{
				throw std::runtime_error("打开文件失败");
			}
			f.seekg(0, std::ios::end);
			auto len = f.tellg();
			f.seekg(0);
			m_data.resize(len);
			if (f.read((char*)m_data.data(), len).fail())
			{
				throw std::runtime_error("读取文件失败");
			}
		}
		return m_data.size();
	}

	virtual const std::vector<uint8_t>& GetCompressData(int level = Z_DEFAULT_COMPRESSION) override
	{
		if (GetDataSize() < PCK_BEGINCOMPRESS_SIZE)
		{
			return m_data;
		}

		if (m_compressdata.empty())
		{
			auto len = compressBound(m_data.size());
			m_compressdata.resize(len);
			auto ret = compress2(m_compressdata.data(), &len, m_data.data(), m_data.size(), level);
			if (ret != Z_OK)
			{
				throw std::runtime_error("压缩数据失败");
			}
			m_compressdata.resize(len);
		}
		return m_compressdata;
	}

	virtual void Release() override
	{
		PckPendingItem_Add::Release();
		std::string().swap(m_diskfile);
		std::vector<uint8_t>().swap(m_data);
		std::vector<uint8_t>().swap(m_compressdata);
	}

private:
	std::string m_diskfile;
	std::vector<uint8_t> m_data;
	std::vector<uint8_t> m_compressdata;
};

class PckPendingItem_AddPckItem : public PckPendingItem_Add
{
public:
	PckPendingItem_AddPckItem(const PckItem& item)
		: PckPendingItem_Add(item.GetFileName())
		, m_item(item)
	{
	}

	virtual size_t GetDataSize() override
	{
		return m_item.GetDataSize();
	}

	virtual const std::vector<uint8_t>& GetCompressData(int level = Z_DEFAULT_COMPRESSION) override
	{
		if (m_compressdata.empty())
		{
			m_compressdata = m_item.GetCompressData();
		}
		return m_compressdata;
	}

	virtual void Release() override
	{
		PckPendingItem_Add::Release();
		std::vector<uint8_t>().swap(m_compressdata);
	}

private:
	PckItem m_item;
	std::vector<uint8_t> m_compressdata;
};

class PckPendingItem_Delete : public PckPendingItem
{
public:
	PckPendingItem_Delete(const PckItem& item)
		: PckPendingItem(PckPendingActionType::Delete)
		, m_item(item)
	{
	}

	const PckItem& GetItem() const noexcept
	{
		return m_item;
	}

	virtual void Release() override
	{

	}
private:
	const PckItem& m_item;
};

class PckPendingItem_Rename : public PckPendingItem
{
public:
	PckPendingItem_Rename(const PckItem& item, const std::string& newname)
		: PckPendingItem(PckPendingActionType::Rename)
		, m_item(item)
		, m_name(newname)
	{
	}

	const PckItem& GetItem() const noexcept
	{
		return m_item;
	}

	const std::string& GetNewFileName() const noexcept
	{
		return m_name;
	}

	virtual void Release() override
	{
		std::string().swap(m_name);
	}

private:
	const PckItem& m_item;
	std::string m_name;
};

class PckPendingItem_Update : public PckPendingItem
{
public:
	PckPendingItem_Update(const PckItem& item)
		: PckPendingItem(PckPendingActionType::Update)
		, m_item(item)
	{
	}

	const PckItem& GetItem() const noexcept
	{
		return m_item;
	}

	virtual const std::vector<uint8_t>& GetData() = 0;
	size_t GetDataSize() { return GetData().size(); }
	const std::vector<uint8_t>& GetCompressData(int level = Z_DEFAULT_COMPRESSION)
	{
		if (GetDataSize() < PCK_BEGINCOMPRESS_SIZE)
		{
			return GetData();
		}

		if (m_compressdata.empty())
		{
			auto& data = GetData();
			auto len = compressBound(data.size());
			m_compressdata.resize(len);
			auto ret = compress2(m_compressdata.data(), &len, data.data(), data.size(), level);
			if (ret != Z_OK)
			{
				throw std::runtime_error("压缩数据失败");
			}
			m_compressdata.resize(len);
		}
		return m_compressdata;
	}

	virtual void Release() override
	{
		std::vector<uint8_t>().swap(m_compressdata);
	}

private:
	const PckItem& m_item;
	std::vector<uint8_t> m_compressdata;
};

class PckPendingItem_UpdateBuffer : public PckPendingItem_Update
{
public:
	PckPendingItem_UpdateBuffer(const PckItem& item, const void* buf, size_t len)
		: PckPendingItem_Update(item)
		, m_data((uint8_t*)buf, (uint8_t*)buf + len)
	{
	}

	virtual const std::vector<uint8_t>& GetData() override
	{
		return m_data;
	}

	virtual void Release() override
	{
		PckPendingItem_Update::Release();
		std::vector<uint8_t>().swap(m_data);
	}

private:
	std::vector<uint8_t> m_data;
};

class PckPendingItem_UpdateFile : public PckPendingItem_Update
{
public:
	PckPendingItem_UpdateFile(const PckItem& item, const std::string& filename)
		: PckPendingItem_Update(item)
		, m_filename(filename)
	{
	}

	virtual const std::vector<uint8_t>& GetData() override
	{
		if (m_data.empty())
		{
			std::ifstream f(m_filename.c_str(), std::ios::in | std::ios::binary);
			if (!f.is_open())
			{
				throw std::runtime_error("打开文件失败");
			}
			f.seekg(0, std::ios::end);
			auto len = f.tellg();
			f.seekg(0);
			m_data.resize(len);
			if (f.read((char*)m_data.data(), len).fail())
			{
				throw std::runtime_error("读取文件失败");
			}
		}
		return m_data;
	}

	virtual void Release() override
	{
		PckPendingItem_Update::Release();
		std::string().swap(m_filename);
		std::vector<uint8_t>().swap(m_data);
	}

private:
	std::string m_filename;
	std::vector<uint8_t> m_data;
};