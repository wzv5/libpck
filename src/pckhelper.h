#pragma once

#include <string>
#include <vector>
#include <algorithm>

// 把文件名变为pck内的标准格式
inline std::string NormalizePckFileName(const std::string& filename)
{
	std::vector<char> buf(filename.begin(), filename.end());
	
	// 把所有 '/' 替换为 '\'
	std::replace_if(buf.begin(), buf.end(), [](auto i) { return i == '/'; }, '\\');
	
	// 把多个 '\' 替换为单个
	buf.resize(std::distance(buf.begin(), std::unique(buf.begin(), buf.end(), [](auto i, auto j) { return i == j && i == '\\'; })));

	std::string ret(buf.begin(), buf.end());
	ret.erase(0, ret.find_first_not_of(" \r\n\t\\"));
	ret.erase(ret.find_last_not_of(" \r\n\t\\") + 1);

	if (ret.size() > 255)
		throw std::runtime_error("文件名长度超出限制");

	return std::move(ret);
}
