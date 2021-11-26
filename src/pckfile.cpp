#include "pckfile.h"
#include <cstring>
#include <tuple>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <locale>
#include <codecvt>
#include "stringhelper.h"
#include <zlib.h>
#include "myfilesystem.h"
#include "pckitem.h"
#include "pckfileio.h"
#include "pckpendingitem.h"
#include "pckhelper.h"

class PckFile::PckFileImpl
{
public:
	PckFileImpl(PckFile* pck);
	void ReadHead();
	void ReadTail();
	void ReadIndexTable();
	uint32_t ReadIndex(_PckItemIndex* pindex);
	void WriteHead();
	void WriteTail();
	void WriteIndexTable();
	uint32_t WriteIndex(const _PckItemIndex& index);
	void AddPendingItem(std::unique_ptr<PckPendingItem>&& item);
	static void EnumDir(filesystem::path dir, filesystem::path base, std::function<void(std::string diskpath, std::string pckpath)>);
	void CalcIndexTableAddr();

	PckFile* m_pck;
	PckFileIO m_file;
	_PckHead m_head = { 0 };
	_PckTail m_tail = { 0 };
	std::vector<PckItem> m_items;
	uint64_t m_indextableaddr = 0;
	uint64_t m_indextablesize = 0;
	uint64_t m_totalsize = 0;
	uint64_t m_totalcompresssize = 0;

	// 事务相关
	std::vector<std::unique_ptr<PckPendingItem>> m_pendingitems;
	bool m_trans = false;
};

PckFile::PckFile()
{
	pImpl = std::make_unique<PckFileImpl>(this);
}

PckFile::~PckFile()
{

}

std::shared_ptr<PckFile> PckFile::Open(const std::string& filename, bool readonly)
{
	auto pck = std::shared_ptr<PckFile>(new PckFile());
	auto& p = pck->pImpl;
	p->m_file.Open(filename.c_str(), readonly);
	p->ReadHead();
	p->ReadTail();
	p->ReadIndexTable();
	return pck;
}

std::shared_ptr<PckFile> PckFile::Create(const std::string& filename, bool overwrite)
{
	auto pck = std::shared_ptr<PckFile>(new PckFile());
	auto& p = pck->pImpl;
	p->m_file.Create(filename.c_str(), overwrite);
	p->m_head.dwHeadCheckHead = PCK_HEAD_VERIFY;
	p->m_head.dwPckSize = sizeof(_PckHead) + sizeof(_PckTail);
	p->m_tail.dwIndexTableCheckHead = PCK_TAIL_VERIFY1;
	p->m_tail.dwIndexTableCheckTail = PCK_TAIL_VERIFY2;
	p->m_tail.dwVersion1 = p->m_tail.dwVersion2 = PCK_VERSION;
	memset(p->m_tail.szAdditionalInfo, 0, 256);
	strcpy(p->m_tail.szAdditionalInfo + 4, PCK_ADDITIONAL_INFO);
	strcat(p->m_tail.szAdditionalInfo + 4, PCK_ADDITIONAL_INFO_STSM);
	p->m_indextableaddr = sizeof(_PckHead);
	return pck;
}

uint32_t PckFile::GetFileCount() const noexcept
{
	return pImpl->m_tail.dwFileCount;
}

const PckItem& PckFile::GetSingleFileItem(const std::string& filename) const
{
	auto s = NormalizePckFileName(filename);
	for (auto i = pImpl->m_items.begin(); i != pImpl->m_items.end(); ++i)
	{
		std::string s1 = std::string(i->GetFileName());
		if (StringHelper::CompareIgnoreCase(s1, s) == 0)
		{
			return *i;
		}
	}
	throw std::runtime_error("找不到指定的文件");
}

const PckItem& PckFile::GetSingleFileItem(uint32_t i) const
{
	return pImpl->m_items[i];
}

std::vector<uint8_t> PckFile::GetSingleFileData(const std::string& filename)
{
	return GetSingleFileData(GetSingleFileItem(filename));
}

std::vector<uint8_t> PckFile::GetSingleFileData(const PckItem& item)
{
	const _PckItemIndex& index = item.m_index;
	auto compressdata = GetSingleFileCompressData(item);
	std::vector<uint8_t> buf(index.dwFileDataSize);
	uLongf destLen = index.dwFileDataSize;
	auto ret = uncompress((Bytef*)buf.data(), &destLen, (Bytef*)compressdata.data(), (uLongf)compressdata.size());
	if (ret != Z_OK)
	{
		if (index.dwFileCompressDataSize != index.dwFileDataSize)
		{
			throw std::runtime_error("解压数据失败");
		}
		else
		{
			memcpy(buf.data(), compressdata.data(), compressdata.size());
		}
	}
	return buf;
}

std::vector<uint8_t> PckFile::GetSingleFileCompressData(const PckItem& item)
{
	std::lock_guard<std::mutex> lock(pImpl->m_file.GetMutex());
	std::vector<uint8_t> buf;
	auto len = item.m_index.dwFileCompressDataSize;
	buf.resize(len);
	pImpl->m_file.Seek(item.m_index.dwAddressOffset);
	pImpl->m_file.Read(buf.data(), len);
	return buf;
}

bool PckFile::FileExists(const std::string& filename) const noexcept
{
	try
	{
		GetSingleFileItem(filename);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

PckFile::PckItemIterator PckFile::begin() const noexcept
{
	return pImpl->m_items.cbegin();
}

PckFile::PckItemIterator PckFile::end() const noexcept
{
	return pImpl->m_items.cend();
}

size_t PckFile::size() const noexcept
{
	return pImpl->m_items.size();
}

const PckItem& PckFile::operator[](size_t index) const
{
	return pImpl->m_items[index];
}

const PckItem& PckFile::operator[](const std::string& filename) const
{
	return GetSingleFileItem(filename);
}

uint32_t PckFile::Extract(const std::string& directory, ProcessCallback callback)
{
	// fn参数传null，则默认解压所有，不传一个真正的函数是为了提高效率，避免多余的跳转
	return Extract_if(directory, {}, callback);
}

uint32_t PckFile::Extract_if(const std::string& directory,
	std::function<bool(const PckItem& item)> fn,
	ProcessCallback callback)
{
	// 由于pck内部保存的文件名使用gbk编码，linux可能乱码
	// 如果当前系统不支持GBK编码，这行将抛出异常
	std::locale loc = StringHelper::GetGBKLocale();

	// 线程数，等于CPU线程数
	uint32_t nthread = std::thread::hardware_concurrency();
	if (nthread == 0) nthread = 1;
	// 当前进行的索引
	std::atomic<uint32_t> index(0);
	// 总文件数
	uint32_t total = GetFileCount();
	// 线程对象
	struct threadobj
	{
		std::thread t;
		std::promise<void> p;
		std::future<void> f;

		threadobj()
		{
			f = p.get_future();
		}
	};
	std::vector<threadobj> threads(nthread);
	// 停止标志，当线程检测到此标志时应该立即停止
	bool stopflag = false;
	// 符合条件的文件数
	std::atomic<uint32_t> nfiles(0);
	// 目录的绝对路径
	filesystem::path dir = filesystem::absolute(directory);

	for (size_t i = 0; i < nthread; ++i)
	{
		threads[i].t = std::thread([&](PckFile* pck, std::promise<void>& p) {
			try
			{
				while (!stopflag)
				{
					uint32_t n = index++;
					if (n >= total) return;
					auto& item = (*pck)[n];
					if (!fn || fn(item))
					{
						auto data = pck->GetSingleFileData(item);
						filesystem::path p = dir;
						p /= StringHelper::A2W(item.GetFileName(), loc).c_str();
						filesystem::create_directories(p.parent_path());  // 如果目录创建失败，将抛出异常
						std::ofstream f(p.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
						if (f.write((const char*)data.data(), data.size()).fail())
						{
							throw std::runtime_error("写出文件失败");
						}
						++nfiles;
					}
				}
			}
			catch (...)
			{
				// 通过promise设置异常，主线程可接收到这个异常
				p.set_exception(std::current_exception());
			}

		}, this, std::ref(threads[i].p));
	}
	while (index <= total)
	{
		if (callback && !callback(index, total))
		{
			// 退出点1：用户手动取消
			stopflag = true;
			for (auto& to : threads)
				to.t.join();
			throw std::runtime_error("用户手动取消");
		}
		try
		{
			for (auto& t : threads)
			{
				if (t.f.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
				{
					// 如果一个线程抛出异常，get()将抛出这个异常
					t.f.get();
				}
			}
		}
		catch (...)
		{
			// 有线程异常退出，通知并等待其他线程退出
			stopflag = true;
			for (auto& to : threads)
				to.t.join();
			// 退出点2：有线程抛出异常
			std::rethrow_exception(std::current_exception());
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
	for (auto& to : threads)
		to.t.join();
	return nfiles;
}

uint64_t PckFile::GetFileSize() const noexcept
{
	return pImpl->m_head.dwPckSize;
}

uint64_t PckFile::GetTotalDataSize() const noexcept
{
	return pImpl->m_totalsize;
}

uint64_t PckFile::GetTotalCompressDataSize() const noexcept
{
	return pImpl->m_totalcompresssize;
}

uint64_t PckFile::GetIndexTableSize() const noexcept
{
	return pImpl->m_indextablesize;
}

uint64_t PckFile::GetRedundancySize() const noexcept
{
	return pImpl->m_head.dwPckSize - sizeof(_PckHead) - sizeof(_PckTail)
		- pImpl->m_indextablesize
		- pImpl->m_totalcompresssize;
}

void PckFile::BeginTransaction() noexcept
{
	pImpl->m_pendingitems.clear();
	pImpl->m_trans = true;
}

void PckFile::CancelTransaction() noexcept
{
	pImpl->m_pendingitems.clear();
	pImpl->m_trans = false;
}

void PckFile::CommitTransaction(ProcessCallback callback)
{
	if (!pImpl->m_trans)
	{
		pImpl->m_pendingitems.clear();
		return;
	}

	auto total = pImpl->m_pendingitems.size();
	for (size_t i = 0; i < total; ++i)
	{
		auto& p = pImpl->m_pendingitems[i];
		if (callback && !callback(i, total))
		{
			throw std::runtime_error("用户手动取消");
		}
		auto t = p->GetType();
		if (t == PckPendingActionType::Add)
		{
			auto p1 = (PckPendingItem_Add*)p.get();
			auto& compressdata = p1->GetCompressData();
			auto datasize = p1->GetDataSize();
			auto& name = p1->GetFileName();
			pImpl->m_file.Seek(pImpl->m_indextableaddr);
			pImpl->m_file.Write(compressdata.data(), compressdata.size());
			

			auto item = PckItem();
			item.m_pck = shared_from_this();
			item.m_index.dwAddressOffset = pImpl->m_indextableaddr;
			item.m_index.dwFileCompressDataSize = compressdata.size();
			item.m_index.dwFileDataSize = datasize;
			memset(item.m_index.szFilename, 0, 256);
			strcpy(item.m_index.szFilename, p1->GetFileName().c_str());
			pImpl->m_items.emplace_back(std::move(item));
			
			pImpl->m_indextableaddr += compressdata.size();
			pImpl->m_totalcompresssize += compressdata.size();
			pImpl->m_totalsize += datasize;
		}
		else if (t == PckPendingActionType::Delete)
		{
			auto p1 = (PckPendingItem_Delete*)p.get();
			auto& item = GetSingleFileItem(p1->GetFileName());

			pImpl->m_totalcompresssize -= item.GetCompressDataSize();
			pImpl->m_totalsize -= item.GetDataSize();

			auto iter = std::remove(pImpl->m_items.begin(), pImpl->m_items.end(), item);
			pImpl->m_items.erase(iter);
		}
		else if (t == PckPendingActionType::Rename)
		{
			auto p1 = (PckPendingItem_Rename*)p.get();
			auto& item = const_cast<PckItem&>(p1->GetItem());
			memset(item.m_index.szFilename, 0, 256);
			strcpy(item.m_index.szFilename, p1->GetNewFileName().c_str());
		}
		else if (t == PckPendingActionType::Update)
		{
			auto p1 = (PckPendingItem_Update*)p.get();
			auto& compressdata = p1->GetCompressData();
			auto& data = p1->GetData();
			auto& item = const_cast<PckItem&>(p1->GetItem());

			// 先在统计信息中减去旧数据的大小
			pImpl->m_totalcompresssize -= item.GetCompressDataSize();
			pImpl->m_totalsize -= item.GetDataSize();

			if (compressdata.size() > item.GetCompressDataSize())
			{
				// 如果新数据量大于旧数据量，则在文件末尾追加新数据
				pImpl->m_file.Seek(pImpl->m_indextableaddr);
				pImpl->m_file.Write(compressdata.data(), compressdata.size());

				item.m_index.dwAddressOffset = pImpl->m_indextableaddr;
				item.m_index.dwFileCompressDataSize = compressdata.size();
				item.m_index.dwFileDataSize = data.size();

				pImpl->m_indextableaddr += compressdata.size();
			}
			else
			{
				// 如果新数据量小于等于旧数据量，则直接覆盖之前的，避免产生多余的冗余数据量
				pImpl->m_file.Seek(item.m_index.dwAddressOffset);
				pImpl->m_file.Write(compressdata.data(), compressdata.size());

				item.m_index.dwFileCompressDataSize = compressdata.size();
				item.m_index.dwFileDataSize = data.size();
			}

			// 更新统计信息
			pImpl->m_totalcompresssize += item.GetCompressDataSize();
			pImpl->m_totalsize += item.GetDataSize();
		}
		p->Release();
	}

	// 写出顺序是重要的！
	pImpl->CalcIndexTableAddr();
	pImpl->WriteIndexTable();
	pImpl->WriteHead();
	pImpl->WriteTail();
	
	// 修正文件尺寸
	pImpl->m_file.SetSize(pImpl->m_head.dwPckSize);

	pImpl->m_pendingitems.clear();
	pImpl->m_trans = false;
}

void PckFile::AddItem(const void* buf, uint32_t len, const std::string& filename)
{
	auto s = NormalizePckFileName(filename);
	try
	{
		UpdateItem(GetSingleFileItem(s), buf, len);
	}
	catch (...)
	{
		pImpl->AddPendingItem(std::make_unique<PckPendingItem_AddBuffer>(s, buf, len));
	}
}

void PckFile::AddItem(const PckItem& item)
{
	if (item.m_pck.lock().get() == this)
	{
		// 如果传入的文件对象为当前pck文件中的，直接跳过
		return;
	}
	pImpl->AddPendingItem(std::make_unique<PckPendingItem_AddPckItem>(item));
}

void PckFile::AddItem(const std::string& diskfilename, const std::string& pckfilename)
{
	if (MyGetFileSize(diskfilename.c_str()) > PCK_MAX_ITEM_SIZE)
	{
		throw new std::runtime_error("目标文件过大");
	}
	auto s = NormalizePckFileName(pckfilename);
	try
	{
		UpdateItem(GetSingleFileItem(s), diskfilename);
	}
	catch (...)
	{
		pImpl->AddPendingItem(std::make_unique<PckPendingItem_AddFile>(s, diskfilename));
	}
}

void PckFile::DeleteItem(const PckItem& item)
{
	pImpl->AddPendingItem(std::make_unique<PckPendingItem_Delete>(item));
}

void PckFile::RenameItem(const PckItem& item, const std::string& newname)
{
	if (FileExists(newname))
	{
		throw new std::runtime_error("存在同名文件");
	}
	pImpl->AddPendingItem(std::make_unique<PckPendingItem_Rename>(item, NormalizePckFileName(newname)));
}

void PckFile::UpdateItem(const PckItem& item, const void* buf, uint32_t len)
{
	pImpl->AddPendingItem(std::make_unique<PckPendingItem_UpdateBuffer>(item, buf, len));
}

void PckFile::UpdateItem(const PckItem& item, const std::string& diskfilename)
{
	if (MyGetFileSize(diskfilename.c_str()) > PCK_MAX_ITEM_SIZE)
	{
		throw new std::runtime_error("目标文件过大");
	}
	pImpl->AddPendingItem(std::make_unique<PckPendingItem_UpdateFile>(item, diskfilename));
}

int PckFile::DeleteDirectory(const std::string& dirname)
{
	auto dir = NormalizePckFileName(dirname);
	dir.append("\\");
	int n = 0;
	for (auto i = this->begin(); i != this->end(); ++i)
	{
		if (StringHelper::CompareIgnoreCase(std::string(i->GetFileName()).substr(0, dir.size()), dir) == 0)
		{
			pImpl->AddPendingItem(std::make_unique<PckPendingItem_Delete>(*i));
			++n;
		}
	}
	return n;
}

void PckFile::CreateFromDirectory(const std::string& filename, const std::string& dir, bool usedirname, bool overwrite, ProcessCallback callback)
{
	auto pck = PckFile::Create(filename, overwrite);
	auto basedir = filesystem::absolute(dir + "/");
	filesystem::path rootname = usedirname ? basedir.parent_path().filename() : "";
	pck->BeginTransaction();
	PckFileImpl::EnumDir(basedir, rootname, [&](std::string diskpath, std::string pckpath) {
		pck->AddItem(diskpath, pckpath);
	});
	pck->CommitTransaction(callback);
}

void PckFile::ReBuild(const std::string& filename, const std::string& newname, bool overwrite, ProcessCallback callback)
{
	auto pck = PckFile::Open(filename);
	auto pcknew = PckFile::Create(newname, overwrite);

	// 去重
	{
		auto& items = pck->pImpl->m_items;
		std::sort(items.begin(), items.end(), [](const PckItem& left, const PckItem& right) {
			return StringHelper::CompareIgnoreCase(std::string(left.GetFileName()), std::string(right.GetFileName())) < 0;
		});
		auto it = std::unique(items.begin(), items.end(), [](const PckItem& left, const PckItem& right) {
			return StringHelper::CompareIgnoreCase(std::string(left.GetFileName()), std::string(right.GetFileName())) == 0;
		});
		items.erase(it, items.end());
	}

	pcknew->BeginTransaction();
	for (auto& i : pck->pImpl->m_items)
	{
		pcknew->AddItem(i);
	}
	pcknew->CommitTransaction(callback);
}

//************************
// PckFileImpl
//************************
#pragma region PckFileImpl

PckFile::PckFileImpl::PckFileImpl(PckFile* pck) : 
	m_pck(pck)
{
}

void PckFile::PckFileImpl::ReadHead()
{
	m_file.Seek(0);
	m_file.Read(&m_head, sizeof(_PckHead));
	if (m_head.dwHeadCheckHead != PCK_HEAD_VERIFY)
	{
		throw std::runtime_error("文件头校验失败");
	}
}

void PckFile::PckFileImpl::ReadTail()
{
	m_file.Seek(m_head.dwPckSize - sizeof(_PckTail));
	m_file.Read(&m_tail, sizeof(_PckTail));
	if (m_tail.dwIndexTableCheckHead != PCK_TAIL_VERIFY1 || m_tail.dwIndexTableCheckTail != PCK_TAIL_VERIFY2)
	{
		throw std::runtime_error("文件尾校验失败");
	}
	if (m_tail.dwVersion1 != PCK_VERSION || m_tail.dwVersion2 != PCK_VERSION)
	{
		throw std::runtime_error("文件版本不支持");
	}
}

void PckFile::PckFileImpl::ReadIndexTable()
{
	m_indextableaddr = m_tail.dwIndexValue ^ PCK_ADDR_MASK;
	m_file.Seek(m_indextableaddr);
	m_items.resize(m_tail.dwFileCount);
	auto pthis = m_pck->shared_from_this();
	m_indextablesize = 0;
	for (size_t i = 0; i < m_tail.dwFileCount; i++)
	{
		m_items[i].m_pck = pthis;
		m_indextablesize += ReadIndex(&m_items[i].m_index);
		m_totalsize += m_items[i].m_index.dwFileDataSize;
		m_totalcompresssize += m_items[i].m_index.dwFileCompressDataSize;
	}
}

uint32_t PckFile::PckFileImpl::ReadIndex(_PckItemIndex* pindex)
{
	uint32_t check1, check2, len1, len2;
	m_file.Read(&check1, 4);
	m_file.Read(&check2, 4);
	len1 = check1 ^ PCK_INDEX_MASK1;
	len2 = check2 ^ PCK_INDEX_MASK2;
	if (len1 != len2)
	{
		throw std::runtime_error("文件索引的长度错误");
	}
	std::vector<char> buf(len1);
	m_file.Read(buf.data(), len1);
	uLongf destLen = sizeof(_PckItemIndex);
	auto ret = uncompress((Bytef*)pindex, &destLen, (Bytef*)buf.data(), len1);
	if (ret != Z_OK)
	{
		if (len1 != sizeof(_PckItemIndex))
		{
			throw std::runtime_error("解压文件索引失败");
		}
		else
		{
			memcpy(pindex, buf.data(), len1);
		}
	}
	auto s = NormalizePckFileName(pindex->szFilename);
	memset(pindex->szFilename, 0, 256);
	strcpy(pindex->szFilename, s.c_str());
	return len1 + 8;
}

void PckFile::PckFileImpl::WriteHead()
{
	m_head.dwPckSize = m_indextableaddr + m_indextablesize + sizeof(_PckTail);
	m_file.Seek(0);
	m_file.Write(&m_head, sizeof(_PckHead));
}

void PckFile::PckFileImpl::WriteTail()
{
	m_tail.dwFileCount = m_items.size();
	m_tail.dwIndexValue = m_indextableaddr ^ PCK_ADDR_MASK;
	m_file.Seek(m_head.dwPckSize - sizeof(_PckTail));
	m_file.Write(&m_tail, sizeof(_PckTail));
}

void PckFile::PckFileImpl::WriteIndexTable()
{
	m_file.Seek(m_indextableaddr);
	m_indextablesize = 0;
	for (auto& i : m_items)
	{
		m_indextablesize += WriteIndex(i.m_index);
	}
}

uint32_t PckFile::PckFileImpl::WriteIndex(const _PckItemIndex& index)
{
	auto len = compressBound(sizeof(index));
	std::vector<uint8_t> buf(len);
	auto ret = compress2(buf.data(), &len, (const Bytef*)&index, sizeof(index), Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
	{
		throw std::runtime_error("压缩数据失败");
	}
	buf.resize(len);
	uint32_t len1 = len ^ PCK_INDEX_MASK1;
	uint32_t len2 = len ^ PCK_INDEX_MASK2;
	m_file.Write(&len1, 4);
	m_file.Write(&len2, 4);
	m_file.Write(buf.data(), len);
	return len + 8;
}

void PckFile::PckFileImpl::AddPendingItem(std::unique_ptr<PckPendingItem>&& item)
{
	auto trans = m_trans;
	if (!trans)
	{
		m_pck->BeginTransaction();
	}
	m_pendingitems.emplace_back(std::move(item));
	if (!trans)
	{
		m_pck->CommitTransaction();
	}
}

void PckFile::PckFileImpl::EnumDir(filesystem::path dir, filesystem::path base, std::function<void(std::string, std::string)> callback)
{
	filesystem::directory_iterator dir_end;
	for (auto i = filesystem::directory_iterator(dir); i != dir_end; ++i)
	{
		auto status = i->status();
		if (filesystem::is_regular_file(status))
		{
			auto diskpath = StringHelper::T2A(i->path().c_str());
			auto pckpath = StringHelper::T2A((base / i->path().filename()).c_str());
			callback(diskpath, pckpath);
		}
		else if (filesystem::is_directory(status))
		{
			EnumDir(*i, base / i->path().filename(), callback);
		}
	}
}

// 计算索引表（即数据区末尾）的偏移
// 在删除数据末尾的文件时，可以回收这些空间
void PckFile::PckFileImpl::CalcIndexTableAddr()
{
	for (auto& item : m_items)
	{
		auto addr = item.m_index.dwAddressOffset + item.m_index.dwFileCompressDataSize;
		if (addr > m_indextableaddr)
		{
			m_indextableaddr = addr;
		}
	}
}
#pragma endregion