#pragma once

#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;

#ifdef _WINDOWS
#include <io.h>
#define ftruncate	_chsize
#define fileno		_fileno
#else
#include <unistd.h>
#endif