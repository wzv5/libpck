#include "pckfile.h"
#include <cstring>
#include <tuple>
#include <mutex>
#include <thread>
#include <atomic>
#include <future>
#include <experimental/filesystem>
#include <locale>
#include <codecvt>
#include "stringhelper.h"
#include <zlib.h>
#include "pckitem.h"

namespace fs = std::experimental::filesystem;

class PckFile::PckFileImpl
{
public:
	PckFileImpl(PckFile* pck);
	void ReadHead();
	void ReadTail();
	void ReadIndexTable();
	int ReadIndex(_PckItemIndex* pindex);
	void FSeek(std::ios::pos_type pos);
	void FSeek(std::ios::pos_type off, std::ios::seekdir way);
	void FRead(void* buf, size_t len);

	PckFile* m_pck;
	std::string m_filename;
	std::fstream m_stream[2];
	std::size_t m_filesize1;
	bool m_inpkx;
	bool m_haspkx;
	_PckHead m_head;
	_PckTail m_tail;
	std::tuple<size_t, size_t> m_indexRange;
	std::vector<PckItem> m_items;
	std::mutex m_streamlock;
	size_t m_totalsize;
	size_t m_totalciphersize;
};

PckFile::PckFile()
{
	pImpl = make_unique<PckFileImpl>(this);
}

std::shared_ptr<PckFile> PckFile::Open(const std::string& filename, bool readonly)
{
	auto pck = std::shared_ptr<PckFile>(new PckFile());
	auto& p = pck->pImpl;
	p->m_filename = filename;
	auto mode = std::ios::in | std::ios::binary;
	if (!readonly) mode |= std::ios::out;
	p->m_stream[0].open(filename, mode);
	if (!p->m_stream[0].is_open())
	{
		throw std::runtime_error("打开文件失败");
	}
	p->m_stream[0].seekg(0, std::ios::end);
	p->m_filesize1 = (size_t)p->m_stream[0].tellg();
	p->m_stream[0].seekg(0);
	std::string filename2 = filename;
	filename2[filename2.size() - 2] = 'k';
	filename2[filename2.size() - 1] = 'x';
	if (fs::exists(filename2))
	{
		p->m_haspkx = true;
		p->m_stream[1].open(filename2, mode);
		if (!p->m_stream[1].is_open())
		{
			throw std::runtime_error("打开文件失败");
		}
	}
	p->ReadHead();
	p->ReadTail();
	p->ReadIndexTable();
	return std::move(pck);
}

uint32_t PckFile::GetFileCount() const noexcept
{
	return pImpl->m_tail.dwFileCount;
}

const PckItem& PckFile::GetSingleFileItem(const std::string& filename) const
{
	using namespace StringHelper;
	for (auto i = pImpl->m_items.begin(); i != pImpl->m_items.end(); ++i)
	{
		std::string s1 = std::string(i->GetFileName());
		ReplaceAll<std::string>(s1, "/", "\\");

		std::string s2 = std::string(filename);
		ReplaceAll<std::string>(s2, "/", "\\");
		Trim(s1);
		Trim(s2);
		if (CompareIgnoreCase(s1, s2) == 0)
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
	auto cipherdata = GetSingleFileCipherData(item);
	std::vector<uint8_t> buf(index.dwFileClearDataSize);
	if (/*cipherdata.size() > PCK_BEGINCOMPRESS_SIZE || */index.dwFileCipherDataSize != index.dwFileClearDataSize)
	{
		uLongf destLen = index.dwFileClearDataSize;
		auto ret = uncompress((Bytef*)buf.data(), &destLen, (Bytef*)cipherdata.data(), (uLongf)cipherdata.size());
		if (ret != Z_OK)
		{
			throw std::runtime_error("解压数据失败");
		}
	}
	else
	{
		memcpy(buf.data(), cipherdata.data(), cipherdata.size());
	}
	return std::move(buf);
}

std::vector<uint8_t> PckFile::GetSingleFileCipherData(const PckItem& item)
{
	std::lock_guard<std::mutex> lock(pImpl->m_streamlock);
	std::vector<uint8_t> buf;
	size_t len = item.m_index.dwFileCipherDataSize;
	buf.resize(len);
	pImpl->FSeek(item.m_index.dwAddressOffset);
	pImpl->FRead(buf.data(), len);
	return std::move(buf);
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

uint32_t PckFile::Extract(const std::string& directory, std::function<bool(uint32_t index, uint32_t total)> callback)
{
	// fn参数传null，则默认解压所有，不传一个真正的函数是为了提高效率，避免多余的跳转
	return Extract_if(directory, {}, callback);
}

uint32_t PckFile::Extract_if(const std::string& directory,
	std::function<bool(const PckItem& item)> fn,
	std::function<bool(uint32_t index, uint32_t total)> callback)
{
	// 由于pck内部保存的文件名使用gbk编码，linux可能乱码
	// 如果当前系统不支持GBK编码，这行将抛出异常
	std::locale loc;
	try
	{
		loc = std::locale("zh_CN.GBK");  // linux格式
	}
	catch (...)
	{
		try
		{
			loc = std::locale("chinese-simplified_china.936");  // Windows格式
		}
		catch (...)
		{
			throw std::runtime_error("当前系统不支持GBK编码");
		}
		
	}

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
	fs::path dir = fs::system_complete(directory);

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
						fs::path p = dir;
						p /= StringHelper::A2W(item.GetFileName(), loc).c_str();
						fs::create_directories(p.parent_path());  // 如果目录创建失败，将抛出异常
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
	while (index < total)
	{
		if (callback && !callback(index, total))
		{
			// 退出点1：用户手动取消
			stopflag = true;
			for (auto& to : threads)
				to.t.join();
			return nfiles;
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

size_t PckFile::GetFileSize() const noexcept
{
	return pImpl->m_head.dwPckSize;
}

size_t PckFile::GetTotalDataSize() const noexcept
{
	return pImpl->m_totalsize;
}

size_t PckFile::GetTotalCipherDataSize() const noexcept
{
	return pImpl->m_totalciphersize;
}

size_t PckFile::GetIndexTableSize() const noexcept
{
	return std::get<1>(pImpl->m_indexRange) - std::get<0>(pImpl->m_indexRange);
}

size_t PckFile::GetRedundancySize() const noexcept
{
	return pImpl->m_head.dwPckSize - sizeof(_PckHead) - sizeof(_PckTail)
		- GetIndexTableSize()
		- pImpl->m_totalciphersize;
}

//**************************
// PckFileImpl
//**************************
PckFile::PckFileImpl::PckFileImpl(PckFile* pck) : 
	m_pck(pck),
	m_totalsize(0),
	m_totalciphersize(0),
	m_haspkx(false),
	m_inpkx(false)
{
}

void PckFile::PckFileImpl::ReadHead()
{
	FSeek(0);
	FRead(&m_head, sizeof(_PckHead));
	if (m_head.dwHeadCheckHead != PCK_HEAD_VERIFY1 || m_head.dwHeadCheckTail != PCK_HEAD_VERIFY2)
	{
		throw std::runtime_error("文件头校验失败");
	}
}

void PckFile::PckFileImpl::ReadTail()
{
	FSeek(m_head.dwPckSize - 280);
	FRead(&m_tail, sizeof(_PckTail));
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
	size_t addr = m_tail.dwIndexValue ^ PCK_ADDR_MASK;
	FSeek(addr);
	m_items.resize(m_tail.dwFileCount);
	auto pthis = m_pck->shared_from_this();
	auto addr2 = addr;
	for (size_t i = 0; i < m_tail.dwFileCount; i++)
	{
		m_items[i].m_pck = pthis;
		addr2 += ReadIndex(&m_items[i].m_index);
		m_totalsize += m_items[i].m_index.dwFileClearDataSize;
		m_totalciphersize += m_items[i].m_index.dwFileCipherDataSize;
	}
	m_indexRange = std::make_tuple(addr, addr2);
}

int PckFile::PckFileImpl::ReadIndex(_PckItemIndex* pindex)
{
	uint32_t check1, check2, len1, len2;
	FRead(&check1, 4);
	FRead(&check2, 4);
	len1 = check1 ^ PCK_INDEX_MASK1;
	len2 = check2 ^ PCK_INDEX_MASK2;
	if (len1 != len2)
	{
		throw std::runtime_error("文件索引的长度错误");
	}
	if (len1 == PCK_INDEX_SIZE)
	{
		FRead(pindex, PCK_INDEX_SIZE);
	}
	else
	{
		std::vector<char> buf(len1);
		FRead(buf.data(), len1);
		uLongf destLen = PCK_INDEX_SIZE;
		auto ret = uncompress((Bytef*)pindex, &destLen, (Bytef*)buf.data(), len1);
		if (ret != Z_OK)
		{
			throw std::runtime_error("解压文件索引失败");
		}
	}
	return len1;
}

void PckFile::PckFileImpl::FSeek(std::ios::pos_type pos)
{
	if ((size_t)pos >= m_filesize1 && !m_haspkx)
	{
		throw std::runtime_error("设置文件读写指针失败");
	}
	if ((size_t)pos < m_filesize1)
	{
		m_inpkx = false;
		if (m_stream[0].seekg(pos).fail())
		{
			throw std::runtime_error("设置文件读写指针失败");
		}
	}
	else
	{
		m_inpkx = true;
		if (m_stream[1].seekg((size_t)pos - m_filesize1).fail())
		{
			throw std::runtime_error("设置文件读写指针失败");
		}
	}
}

void PckFile::PckFileImpl::FRead(void* buf, size_t len)
{
	if (m_inpkx)
	{
		// 数据全部位于pkx文件中
		if (m_stream[1].read((char*)buf, len).fail())
		{
			throw std::runtime_error("读取文件失败");
		}
	}
	else
	{
		auto& f = m_stream[0].read((char*)buf, len);
		size_t nread = (size_t)f.gcount();
		if (nread < len)
		{
			if (!m_haspkx)
			{
				// 不存在pkx文件，且没有读取到指定长度的数据，抛出异常
				throw std::runtime_error("读取文件失败");
			}
			// 数据跨域pck与pkx文件，继续从pkx文件中读取剩余部分
			m_inpkx = true;
			FRead((char*)buf + nread, len - nread);
		}
	}
}