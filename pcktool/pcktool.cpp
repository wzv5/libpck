// pcktool.cpp : 定义控制台应用程序的入口点。
//

#pragma warning(disable: 4018)
#pragma warning(disable: 4267)

#include "stdafx.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <experimental/filesystem>
#include "../include/pckfile.h"
#include "../include/pckitem.h"
#include "../include/stringhelper.h"

namespace filesystem = std::experimental::filesystem;

void PrintHelp(const char* s);
void PrintProgress(int i, int t);
bool ExtractAll(const char* pckname);
bool ExtractList(const char* pckname, const char* excludelist, const char* keeplist);
bool CompressDir(const char* pckname, const char* dirname);
bool ListAll(const char* pckname);

#define HELPSTR "{0} 2017.04.08\n" \
"\n" \
"解压所有文件：\n" \
"{0} -x input.pck\n" \
"\n" \
"解压指定文件：\n" \
"{0} -x input.pck 排除列表.txt 保留列表.txt\n" \
"如果以“\\”结尾，则为目录，否则为文件。\n" \
"如果行首为“#”，则该行为注释，不作为列表内容。\n" \
"\n" \
"压缩：\n" \
"{0} -c output.pck inputdir\n" \
"\n" \
"列出所有文件：\n" \
"{0} -l input.pck\n" \
"\n" \
"感谢 stsm/liqf/李秋枫 开源的WinPCK！\n"

int main(int argc, char* argv[])
{
	bool ret = false;

	if (argc < 3)
	{
		PrintHelp(argv[0]);
	}
	else if (strcmp("-x", argv[1]) == 0)
	{
		if (argc == 3)
		{
			ret = ExtractAll(argv[2]);
		}
		else if (argc == 5)
		{
			ret = ExtractList(argv[2], argv[3], argv[4]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数");
		}
	}
	else if (strcmp("-c", argv[1]) == 0)
	{
		if (argc == 4)
		{
			ret = CompressDir(argv[2], argv[3]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数");
		}
	}
	else if (strcmp("-l", argv[1]) == 0)
	{
		if (argc == 3)
		{
			ret = ListAll(argv[2]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数");
		}
	}
	else if (strcmp("-h", argv[1]) == 0 || strcmp("-?", argv[1]) == 0)
	{
		PrintHelp(argv[0]);
	}
	else
	{
		fprintf(stderr, "错误：无效参数");
	}

    return ret;
}

void PrintHelp(const char* s)
{
	filesystem::path p = s;
	std::string helpstr = HELPSTR;
	auto f = StringHelper::ReplaceAll<std::string>(helpstr, "{0}", p.filename().string());
	printf(f.c_str());
}

void PrintProgress(int i, int t)
{
	static int lastlen = 0;
	for (size_t i = 0; i < lastlen; i++)
	{
		printf("\b");
	}
	auto s = StringHelper::FormatString("%d / %d", i, t);
	printf(s.c_str());
	lastlen = s.size();
}

bool ExtractAll(const char* pckname)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname);
		pck->Extract("./", [](auto i, auto t) {
			PrintProgress(i, t);
			return true;
		});
		printf("\n完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "解压失败：%s\n", e.what());
	}
	return ret;
}

void _loadlistfile(const char* filename, std::unordered_set<std::string>& files, std::vector<std::string>& dirs)
{
	ifstream f(filename);
	if (!f.is_open())
		throw std::runtime_error("打开列表文件失败");
	std::string line;
	do
	{
		std::getline(f, line);
		StringHelper::Trim(line);
		StringHelper::ReplaceAll<std::string>(line, "/", "\\");
		line = StringHelper::ToLower_Copy(line);
		if (line.empty())
			continue;
		else if (line.substr(0, 1) == "#")
			continue;
		else if (line.substr(line.size() - 1) == "\\")
			dirs.push_back(line);
		else
			files.insert(line);
	} while (!f.eof());
}

bool _fileindirs(std::string& file, std::vector<std::string>& dirs)
{
	for (auto& i : dirs)
	{
		if (file.substr(0, i.size()) == i)
			return true;
	}
	return false;
}

bool ExtractList(const char* pckname, const char* excludelist, const char* keeplist)
{
	bool ret = false;
	std::unordered_set<std::string> keepfile;
	std::unordered_set<std::string> excludefile;
	std::vector<std::string> keepdir;
	std::vector<std::string> excludedir;

	try
	{
		_loadlistfile(excludelist, excludefile, excludedir);
		_loadlistfile(keeplist, keepfile, keepdir);
		auto pck = PckFile::Open(pckname);
		pck->Extract_if("./", [&](const PckItem& item) {
			std::string name = item.GetFileName();
			name = StringHelper::ToLower_Copy(name);
			if (keepfile.find(name) != keepfile.end() || _fileindirs(name, keepdir))
			{
				return true;
			}
			else if (excludefile.find(name) == excludefile.end() && !_fileindirs(name, excludedir))
			{
				return true;
			}
			return false;
		}, [](auto i, auto t) {
			PrintProgress(i, t);
			return true;
		});
		printf("\n完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "解压失败：%s\n", e.what());
	}
	return ret;
}

bool CompressDir(const char* pckname, const char* dirname)
{
	bool ret = false;
	try
	{
		PckFile::CreateFromDirectory(pckname, dirname, true, true, [](auto i, auto t) {
			PrintProgress(i, t);
			return true;
		});
		printf("\n完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "压缩失败：%s\n", e.what());
	}
	return ret;
}

bool ListAll(const char* pckname)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname);
		printf("文件数：%d\n", pck->GetFileCount());
		printf("================\n");
		for (size_t i = 0; i < pck->GetFileCount(); i++)
		{
			printf(pck->GetSingleFileItem(i).GetFileName());
			printf("\n");
		}
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "列出文件失败：%s\n", e.what());
	}
	return ret;
}