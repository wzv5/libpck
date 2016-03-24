#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "pckdef.h"

class PckFile;

class PckItem
{
	friend class PckFile;

public:
	PckItem() = default;

	const char* GetFileName() const;
	uint32_t GetDataSize() const;
	uint32_t GetCipherDataSize() const;

	// 以下两个方法不推荐使用，效率略低，尽可能使用PckFile类中的方法
	std::vector<uint8_t> GetCipherData() const;
	std::vector<uint8_t> GetData() const;

private:
	_PckItemIndex m_index;
	std::weak_ptr<PckFile> m_pck;  // 使用弱引用指向父PckFile对象，避免循环引用
};