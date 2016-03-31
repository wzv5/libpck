#pragma once

#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;

#if defined(_WINDOWS) || defined(_WIN32)
#include <io.h>
#define ftruncate	_chsize
#define fileno		_fileno
#else
#include <unistd.h>
#endif