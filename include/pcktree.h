#pragma once

#include <memory>
#include <map>
#include <string>

class PckFile;
class PckItem;

class PckTreeItem
{
public:
	std::string FullPath;
	std::string FileName;
	bool IsDirectory = false;
	PckItem const* Item = nullptr;
	PckTreeItem* Parent = nullptr;
	std::map<std::string, PckTreeItem> Items;
};

class PckTree
{

public:
	static std::map<std::string, PckTreeItem> BuildTree(std::shared_ptr<PckFile>& pck);

private:
	PckTree() = default;
};