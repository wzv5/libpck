#include "pcktree.h"
#include "pckfile.h"
#include "pckitem.h"
#include "myfilesystem.h"
#include "stringhelper.h"

std::map<std::string, PckTreeItem> PckTree::BuildTree(std::shared_ptr<PckFile>& pck)
{
	std::map<std::string, PckTreeItem> tree;

	for (auto i = pck->begin(); i != pck->end(); ++i)
	{
		// 拆分路径，并把路径转换为小写
		filesystem::path fullpath = i->GetFileName();
		std::vector<std::string> splitpath;
		for (auto j = fullpath.begin(); j != fullpath.end(); ++j)
		{
			splitpath.emplace_back(StringHelper::ToLower_Copy(j->string()));
		}

		// 每次前进到一个新的文件项目，都把当前节点重置到根节点
		auto currentnode = &tree;
		// 记录上次的节点
		PckTreeItem* parent = nullptr;
		// 重新组合当前所在的完整目录
		std::string dir = "";

		// 遍历拆分后的路径
		for (size_t j = 0; j < splitpath.size() - 1; ++j)
		{
			dir.append(splitpath[j]);
			dir.append("\\");
			auto& item = (*currentnode)[splitpath[j]];
			item.FileName = splitpath[j];
			item.FullPath = dir.substr(0, dir.size() - 1);
			item.Parent = parent;
			item.PckItem = nullptr;
			item.IsDirectory = true;

			// 把当前项目保存为parent
			parent = &item;
			// 前进到子节点中
			currentnode = &item.Items;
		}
		auto& item = (*currentnode)[splitpath[splitpath.size() - 1]];
		item.FileName = fullpath.filename().string();
		item.FullPath = i->GetFileName();
		item.Parent = parent;
		item.PckItem = &(*i);
	}

	return tree;
}