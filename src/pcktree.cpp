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
		// ���·��������·��ת��ΪСд
		filesystem::path fullpath = i->GetFileName();
		std::vector<std::string> splitpath;
		for (auto j = fullpath.begin(); j != fullpath.end(); ++j)
		{
			splitpath.emplace_back(StringHelper::ToLower_Copy(j->string()));
		}

		// ÿ��ǰ����һ���µ��ļ���Ŀ�����ѵ�ǰ�ڵ����õ����ڵ�
		auto currentnode = &tree;
		// ��¼�ϴεĽڵ�
		PckTreeItem* parent = nullptr;
		// ������ϵ�ǰ���ڵ�����Ŀ¼
		std::string dir = "";

		// ������ֺ��·��
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

			// �ѵ�ǰ��Ŀ����Ϊparent
			parent = &item;
			// ǰ�����ӽڵ���
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