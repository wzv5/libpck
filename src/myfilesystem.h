#pragma once

//#define USE_BOOST
#ifdef USE_BOOST
#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif

#include <cstdio>
#if defined(_WINDOWS) || defined(_WIN32)
#include <io.h>
#define ftruncate	_chsize_s
#define fileno		_fileno
#define ftell		_ftelli64
#define fseek		_fseeki64
#else
#define _FILE_OFFSET_BITS 64
#include <unistd.h>
// TODO: 测试Linux中64位文件偏移的兼容性
#endif

inline uint64_t MyGetFileSize(const char* filename)
{
	uint64_t ret = 0;
	auto f = fopen(filename, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		ret = ftell(f);
		fclose(f);
	}
	return ret;
}