#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <stdexcept>
#include <mutex>
#include "pckdef.h"
#include "myfilesystem.h"

class PckFileIO
{
public:
	PckFileIO() = default;
	virtual ~PckFileIO()
	{
		Close();
	}

	// 打开已有的文件，如果文件不存在则打开失败
	void Open(const char* filename, bool readonly = true)
	{
		Close();
		_setfilename(filename);
		m_readonly = readonly;
		const char* mode = readonly ? "rb" : "rb+";
		try
		{
			_openpck(mode);
			if (filesystem::exists(m_pkxname))
			{
				// 如果pkx文件存在，则继续打开pkx文件，并以pkx文件的打开结果为准
				_openpkx(mode);
			}
		}
		catch (...)
		{
			Close();
			std::rethrow_exception(std::current_exception());
		}
	}

	// 创建新文件，如果文件存在且不允许重写则失败
	void Create(const char* filename, bool overwrite = false)
	{
		Close();
		_setfilename(filename);
		try
		{
			if (filesystem::exists(m_pckname) && !overwrite)
			{
				throw std::runtime_error("创建文件失败，文件已存在，且不允许重写");
			}
			// 先删除现有的文件
			remove(m_pckname.c_str());
			remove(m_pkxname.c_str());
			_openpck("wb");
			m_readonly = false;
		}
		catch (...)
		{
			Close();
			std::rethrow_exception(std::current_exception());
		}
	}

	bool IsOpen()
	{
		return m_pckfile != NULL;
	}

	void Close()
	{
		if (m_pckfile)
		{
			fflush(m_pckfile);
			fclose(m_pckfile);
		}
		if (m_pkxfile)
		{
			fflush(m_pkxfile);
			fclose(m_pkxfile);
		}
		m_pckfile = nullptr;
		m_pkxfile = nullptr;
		m_pcksize = 0;
		m_pkxsize = 0;
		m_pos = 0;
		m_haspkx = false;
		m_inpkx = false;
		m_readonly = true;
		std::string().swap(m_pckname);
		std::string().swap(m_pkxname);
	}

	bool HasPkx()
	{
		return m_haspkx;
	}

	uint64_t Size()
	{
		return m_pcksize + m_pkxsize;
	}

	void Seek(uint64_t pos)
	{
		if (pos < m_pcksize)
		{
			// 位于pck文件中
			if (fseek(m_pckfile, pos, SEEK_SET))
			{
				throw std::runtime_error("设置文件读写指针失败");
			}
			m_pos = pos;
			m_inpkx = false;
		}
		else
		{
			if (m_haspkx)
			{
				// 位于pkx文件中
				if (fseek(m_pkxfile, pos - m_pcksize, SEEK_SET))
				{
					throw std::runtime_error("设置文件读写指针失败");
				}
				if (pos - m_pcksize > m_pkxsize)
				{
					m_pkxsize = pos - m_pcksize;
				}
				m_pos = pos;
				m_inpkx = true;
			}
			else
			{
				// 位于pck文件外，且没有pkx文件
				if (pos < PCK_MAX_SIZE)
				{
					// 没有超过pck文件的最大尺寸
					if (fseek(m_pckfile, pos, SEEK_SET))
					{
						throw std::runtime_error("设置文件读写指针失败");
					}
					m_pos = pos;
					m_inpkx = false;
					m_pcksize = pos;
				}
				else
				{
					// 超过了pck文件的最大尺寸，且不是只读模式，则创建pkx文件
					if (!m_readonly)
					{
						_openpkx("wb");
					}
					else
					{
						// 如果为只读模式，则失败
						throw std::runtime_error("设置文件读写指针失败");
					}
					// 创建pkx文件成功，重新seek
					return Seek(pos);
				}
			}
		}
	}

	void Seek(uint64_t off, int way)
	{
		uint64_t pos = 0;
		switch (way)
		{
		case SEEK_CUR:
			pos = m_pos + off;
			break;
		case SEEK_END:
			pos = Size() + off;
			break;
		case SEEK_SET:
			pos = off;
			break;
		default:
			throw std::runtime_error("设置文件读写指针失败");
		}
		Seek(pos);
	}

	void Read(void* buf, uint32_t len)
	{
		if (m_inpkx)
		{
			// 全部数据都在pkx文件中
			uint32_t nread = fread(buf, 1, len, m_pkxfile);
			m_pos += nread;
			if (nread != len)
			{
				throw std::runtime_error("读取文件失败");
			}
		}
		else
		{
			uint32_t nread = fread(buf, 1, len, m_pckfile);
			m_pos += nread;
			if (nread == len)
			{
				if (m_pos == m_pcksize)
				{
					// 如果当前读写指针在pck文件末尾，需要重新seek，以便处理pkx文件
					Seek(m_pos);
				}
			}
			else
			{
				// 数据跨越pck和pkx
				if (m_haspkx)
					m_inpkx = true;
				else
					throw std::runtime_error("读取文件失败");
				Read((char*)buf + nread, len - nread);
			}
		}
	}

	void Write(const void* buf, uint32_t len)
	{
		if (m_inpkx)
		{
			// 在pkx文件中写入
			auto nwrite = fwrite(buf, 1, len, m_pkxfile);
			m_pos += nwrite;
			if (nwrite != len)
			{
				throw std::runtime_error("写入文件失败");
			}
			// 更新文件尺寸
			m_pkxsize = _getfilesize(m_pkxfile);
		}
		else
		{
			if (m_pos + len < PCK_MAX_SIZE)
			{
				// 如果pck文件还可以存下
				auto nwrite = fwrite(buf, 1, len, m_pckfile);
				m_pos += nwrite;
				if (nwrite != len)
				{
					throw std::runtime_error("写入文件失败");
				}
				// 更新文件尺寸
				m_pcksize = _getfilesize(m_pckfile);
			}
			else
			{
				auto len1 = PCK_MAX_SIZE - m_pos;
				auto len2 = len - len1;
				auto nwrite = fwrite(buf, 1, len1, m_pckfile);
				m_pos += nwrite;
				if (nwrite != len1)
				{
					throw std::runtime_error("写入文件失败");
				}
				// 更新文件尺寸
				m_pcksize = _getfilesize(m_pckfile);
				// 需要重新调用下seek，处理pkx文件
				Seek(m_pos);
				return Write((char*)buf + len1, len2);
			}
		}
	}

	void SetSize(uint64_t len)
	{
		if (m_readonly)
		{
			throw std::runtime_error("设置文件大小失败");
		}

		Seek(len);

		if (len < PCK_MAX_SIZE)
		{
			if (ftruncate(fileno(m_pckfile), len) != 0)
			{
				throw std::runtime_error("设置文件大小失败");
			}
			if (m_haspkx)
			{
				remove(m_pkxname.c_str());
				m_haspkx = false;
			}
			m_pcksize = _getfilesize(m_pckfile);
			m_pkxsize = 0;
		}
		else
		{
			if (ftruncate(fileno(m_pckfile), PCK_MAX_SIZE) != 0 || ftruncate(fileno(m_pkxfile), len - PCK_MAX_SIZE) != 0)
			{
				throw std::runtime_error("设置文件大小失败");
			}
			m_pcksize = _getfilesize(m_pckfile);
			m_pkxsize = _getfilesize(m_pkxfile);
		}

		Seek(0);
	}

	std::mutex& GetMutex()
	{
		return m_mutex;
	}

private:
	PckFileIO(const PckFileIO& a) = delete;
	void operator=(const PckFileIO& a) = delete;
	
	void _openpck(const char* mode)
	{
		m_pckfile = fopen(m_pckname.c_str(), mode);
		if (!m_pckfile)
		{
			throw std::runtime_error("打开PCK文件失败");
		}
		m_pcksize = _getfilesize(m_pckfile);
		if (m_pcksize > PCK_MAX_SIZE)
		{
			throw std::runtime_error("PCK文件尺寸超过最大值");
		}
	}
	void _openpkx(const char* mode)
	{
		m_pkxfile = fopen(m_pkxname.c_str(), mode);
		if (!m_pkxfile)
		{
			throw std::runtime_error("打开PKX文件失败");
		}
		m_pkxsize = _getfilesize(m_pkxfile);
		m_haspkx = true;
	}
	std::string _getpkxfilename(const char* filename)
	{
		std::string filename2 = filename;
		filename2[filename2.size() - 2] = 'k';
		filename2[filename2.size() - 1] = 'x';
		return std::move(filename2);
	}
	uint64_t _getfilesize(FILE* f)
	{
		uint64_t ret;
		auto pos = ftell(f);
		fflush(f);
		fseek(f, 0, SEEK_END);
		ret = ftell(f);
		fseek(f, pos, SEEK_SET);
		return ret;
	}
	void _setfilename(const char* filename)
	{
		m_pckname = filename;
		m_pkxname = _getpkxfilename(filename);
	}

	std::string m_pckname = "";
	std::string m_pkxname = "";
	FILE* m_pckfile = nullptr;
	FILE* m_pkxfile = nullptr;
	bool m_haspkx = false;
	bool m_inpkx = false;
	bool m_readonly = true;
	uint64_t m_pcksize = 0;
	uint64_t m_pkxsize = 0;
	uint64_t m_pos = 0;
	std::mutex m_mutex;
};