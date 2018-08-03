// pcktool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <experimental/filesystem>
#include "../include/pckfile.h"
#include "../include/pckitem.h"
#include "../src/stringhelper.h"
#include "../include/pcktree.h"

namespace filesystem = std::experimental::filesystem;

void PrintHelp(const char* s);
void PrintProgress(int i, int t);
bool ExtractAll(const char* pckname);
bool ExtractSingle(const char* pckname, const char* filename);
bool ExtractList(const char* pckname, const char* excludelist, const char* keeplist);
bool CompressDir(const char* pckname, const char* dirname);
bool ListAll(const char* pckname);
bool ListTree(const char* pckname);
bool AddFile(const char* pckname, const char* diskfilename, const char* pckfilename);
bool DeleteFile(const char* pckname, const char* pckfilename);
bool ReBuild(const char* pckname, const char* outname);

#define HELPSTR "{0} 2018.08.03\n" \
"\n" \
"解压所有文件：\n" \
"{0} -x input.pck\n" \
"\n" \
"解压单个文件：\n" \
"{0} -x input.pck filename\n" \
"\n" \
"根据列表解压文件：\n" \
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
"树状列出所有文件：\n" \
"{0} -t input.pck\n" \
"\n" \
"添加或更新文件：\n" \
"{0} -a input.pck diskfilename pckfilename\n" \
"\n" \
"删除文件：\n" \
"{0} -d input.pck pckfilename\n" \
"\n" \
"重建：\n" \
"{0} -r input.pck output.pck\n" \
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
		else if (argc == 4)
		{
			ret = ExtractSingle(argv[2], argv[3]);
		}
		else if (argc == 5)
		{
			ret = ExtractList(argv[2], argv[3], argv[4]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数\n");
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
			fprintf(stderr, "错误：无效参数\n");
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
			fprintf(stderr, "错误：无效参数\n");
		}
	}
	else if (strcmp("-t", argv[1]) == 0)
	{
		if (argc == 3)
		{
			ret = ListTree(argv[2]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数\n");
		}
	}
	else if (strcmp("-a", argv[1]) == 0)
	{
		if (argc == 5)
		{
			ret = AddFile(argv[2], argv[3], argv[4]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数\n");
		}
	}
	else if (strcmp("-d", argv[1]) == 0)
	{
		if (argc == 4)
		{
			ret = DeleteFile(argv[2], argv[3]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数\n");
		}
	}
	else if (strcmp("-r", argv[1]) == 0)
	{
		if (argc == 4)
		{
			ret = ReBuild(argv[2], argv[3]);
		}
		else
		{
			fprintf(stderr, "错误：无效参数\n");
		}
	}
	else if (strcmp("-h", argv[1]) == 0 || strcmp("-?", argv[1]) == 0)
	{
		PrintHelp(argv[0]);
	}
	else
	{
		fprintf(stderr, "错误：无效参数\n");
	}
	return !ret;
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
	auto s = StringHelper::FormatString("\r%d / %d", i, t);
	printf(s.c_str());
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
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}

bool ExtractSingle(const char* pckname, const char* filename)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname);
		// 先尝试获取文件对象，判断文件是否存在
		pck->GetSingleFileItem(filename);
		pck->Extract_if("./", [&](const PckItem& item) {
			return strcmp(item.GetFileName(), filename) == 0;
		});
		printf("完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
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
		fprintf(stderr, "操作失败：%s\n", e.what());
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
		fprintf(stderr, "操作失败：%s\n", e.what());
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
		for (auto i = pck->begin(); i != pck->end(); ++i)
		{
			printf(i->GetFileName());
			printf("\n");
		}
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}

void _listtree(PckTreeItem item, int level = 0)
{
	for (size_t i = 0; i < level; i++)
	{
		printf("\t");
	}
	printf(item.FileName.c_str());
	printf("\n");
	level++;
	for each (auto i in item.Items)
	{
		if (i.second.IsDirectory)
		{
			_listtree(i.second, level);
		}
		else
		{
			for (size_t i = 0; i < level; i++)
			{
				printf("\t");
			}
			printf(i.second.FileName.c_str());
			printf("\n");
		}
	}
}

bool ListTree(const char* pckname)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname);
		printf("文件数：%d\n", pck->GetFileCount());
		printf("================\n");
		auto tree = PckTree::BuildTree(pck);
		for each (auto& i in tree)
		{
			_listtree(i.second);
		}
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}

bool AddFile(const char* pckname, const char* diskfilename, const char* pckfilename)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname, false);
		pck->AddItem(diskfilename, pckfilename);
		printf("完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}

bool DeleteFile(const char* pckname, const char* pckfilename)
{
	bool ret = false;
	try
	{
		auto pck = PckFile::Open(pckname, false);
		try
		{
			auto& item = pck->GetSingleFileItem(pckfilename);
		}
		catch (const std::exception&)
		{
			printf("找不到指定文件\n");
			return true;
		}
		pck->DeleteItem(pck->GetSingleFileItem(pckfilename));
		printf("完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}

bool ReBuild(const char* pckname, const char* outname)
{
	bool ret = false;
	try
	{
		PckFile::ReBuild(pckname, outname, true, [](auto i, auto t) {
			PrintProgress(i, t);
			return true;
		});
		printf("\n完成！\n");
		ret = true;
	}
	catch (const std::exception& e)
	{
		fprintf(stderr, "操作失败：%s\n", e.what());
	}
	return ret;
}