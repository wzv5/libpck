#pragma once

#include <memory>
#include <fstream>
#include <tuple>
#include <vector>
#include <functional>
#include "pckdef.h"

class PckItem;

class PckFile : public std::enable_shared_from_this<PckFile>
{
private:
	// 私有构造函数，禁止显式创建对象，必须使用下面的静态方法
	PckFile();

public:
	typedef std::vector<PckItem>::const_iterator PckItemIterator;

	//******************************
	// 构造
	//******************************
	// 打开现有文件，返回shared_ptr，失败抛出异常
	static std::shared_ptr<PckFile> Open(const std::string& filename, bool readonly = true);
	// 创建一个空白的对象，返回shared_ptr
	static std::shared_ptr<PckFile> New() { throw std::runtime_error("未实现此方法"); }

	//******************************
	// 读取
	//******************************
	// 获取文件数量
	uint32_t GetFileCount() const noexcept;

	// 获取指定文件对象，成功返回文件对象，失败抛出异常
	const PckItem& GetSingleFileItem(const std::string& filename) const;
	const PckItem& GetSingleFileItem(uint32_t index) const;

	// 获取指定文件数据，成功vector包装的数据，失败抛出异常
	std::vector<uint8_t> GetSingleFileData(const std::string& filename);
	std::vector<uint8_t> GetSingleFileData(const PckItem& item);
	std::vector<uint8_t> GetSingleFileCipherData(const PckItem& item);

	//******************************
	// 遍历
	//******************************
	PckItemIterator begin() const noexcept;
	PckItemIterator end() const noexcept;
	size_t size() const noexcept;
	const PckItem& operator[](size_t index) const;
	const PckItem& operator[](const std::string& filename) const;

	//******************************
	// 保存
	//******************************
	void SaveToFile(const std::string& filename = "") { throw std::runtime_error("未实现此方法"); }
	std::vector<uint8_t> SaveToBuffer() { throw std::runtime_error("未实现此方法"); }

	//******************************
	// 修改
	//******************************
	bool DeleteFile(const PckItem& item) { throw std::runtime_error("未实现此方法"); }
	bool AddFile(void* buf, size_t len, const std::string& filename) { throw std::runtime_error("未实现此方法"); }
	bool AddFile(const PckItem& item) { throw std::runtime_error("未实现此方法"); }
	bool RenameFile(const PckItem& item, const std::string& newname) { throw std::runtime_error("未实现此方法"); }

	//******************************
	// 解压
	//******************************
	// 解压整个文件包，会自动使用多线程解压，线程数为CPU核心数，失败抛出异常
	uint32_t Extract(const std::string& directory, std::function<bool(uint32_t index, uint32_t total)> callback = {});
	// 解压符合条件的文件，返回解压的文件数，失败抛出异常
	uint32_t Extract_if(const std::string& directory,
		std::function<bool(const PckItem& item)> fn,
		std::function<bool(uint32_t index, uint32_t total)> callback = {});

	//******************************
	// 压缩
	//******************************
	// TODO

	//******************************
	// 重建
	//******************************
	// TODO

	//******************************
	// 统计信息
	//******************************
	size_t GetFileSize() const noexcept;
	size_t GetTotalDataSize() const noexcept;
	size_t GetTotalCipherDataSize() const noexcept;
	size_t GetRedundancySize() const noexcept;
	size_t GetIndexTableSize() const noexcept;

private:
	class PckFileImpl;
	std::unique_ptr<PckFileImpl> pImpl;
};