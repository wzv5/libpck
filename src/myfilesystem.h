#pragma once

//#define USE_BOOST
#ifdef USE_BOOST
#include <boost/filesystem.hpp>
namespace filesystem = boost::filesystem;
#else
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#endif

#if defined(_WINDOWS) || defined(_WIN32)
#include <io.h>
#define ftruncate	_chsize
#define fileno		_fileno
#else
#include <unistd.h>
#endif