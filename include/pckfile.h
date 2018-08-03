#pragma once

#include <memory>
#include <fstream>
#include <tuple>
#include <vector>
#include <functional>
#include "pckdef.h"

class PckItem;

// 为保证运行效率，整个类都是非线程安全的！多线程操作请自己加锁！
class PckFile : public std::enable_shared_from_this<PckFile>
{
private:
	// 私有构造函数，禁止显式创建对象，必须使用下面的静态方法
	PckFile();

public:
	typedef std::vector<PckItem>::const_iterator PckItemIterator;
	typedef std::function<bool(uint32_t index, uint32_t total)> ProcessCallback;

	virtual ~PckFile();

	//******************************
	// 构造
	//******************************
	// 打开现有文件，返回shared_ptr，失败抛出异常
	static std::shared_ptr<PckFile> Open(const std::string& filename, bool readonly = true);
	// 创建一个空白的对象，返回shared_ptr
	static std::shared_ptr<PckFile> Create(const std::string& filename, bool overwrite = false);

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
	std::vector<uint8_t> GetSingleFileCompressData(const PckItem& item);

	//文件是否存在
	bool FileExists(const std::string& filename) const;


	//******************************
	// 遍历
	//******************************
	PckItemIterator begin() const noexcept;
	PckItemIterator end() const noexcept;
	size_t size() const noexcept;
	const PckItem& operator[](size_t index) const;
	const PckItem& operator[](const std::string& filename) const;

	//******************************
	// 修改
	//******************************
	void AddItem(const void* buf, uint32_t len, const std::string& filename);
	void AddItem(const PckItem& item);
	void AddItem(const std::string& diskfilename, const std::string& pckfilename);
	void DeleteItem(const PckItem& item);
	void RenameItem(const PckItem& item, const std::string& newname);
	void UpdateItem(const PckItem& item, const void* buf, uint32_t len);
	void UpdateItem(const PckItem& item, const std::string& diskfilename);

	//******************************
	// 解压
	//******************************
	// 解压整个文件包，会自动使用多线程解压，线程数为CPU核心数，失败抛出异常
	uint32_t Extract(const std::string& directory, ProcessCallback callback = {});
	// 解压符合条件的文件，返回解压的文件数，失败抛出异常
	uint32_t Extract_if(const std::string& directory,
		std::function<bool(const PckItem& item)> fn,
		ProcessCallback callback = {});

	//******************************
	// 统计信息
	//******************************
	uint64_t GetFileSize() const noexcept;
	uint64_t GetTotalDataSize() const noexcept;
	uint64_t GetTotalCompressDataSize() const noexcept;
	uint64_t GetRedundancySize() const noexcept;
	uint64_t GetIndexTableSize() const noexcept;

	//******************************
	// 事务（默认状态下为关闭事务，立即执行）
	//******************************
	// 开始事务，之后的增删改操作会暂时缓存，直到提交事务
	void BeginTransaction() noexcept;
	// 取消事务，清除未提交的事务，并禁用事务，之后的增删改操作会立即进行
	void CancelTransaction() noexcept;
	// 提交事务，实际写入文件，失败抛出异常
	void CommitTransaction(ProcessCallback callback = {});

	//******************************
	// 压缩（静态）
	//******************************
	// 从指定目录创建pck文件，参数3指定是否使用参数2的目录名作为根目录名
	static void CreateFromDirectory(const std::string& filename, const std::string& dir, bool usedirname = true, bool overwrite = false, ProcessCallback callback = {});
	// 重建文件包，在冗余数据量过大时使用
	static void ReBuild(const std::string& filename, const std::string& newname, bool overwrite = false, ProcessCallback callback = {});

private:
	class PckFileImpl;
	std::unique_ptr<PckFileImpl> pImpl;
};