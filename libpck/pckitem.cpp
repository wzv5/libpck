#include "pckitem.h"
#include "pckfile.h"

const char* PckItem::GetFileName() const
{
	return m_index.szFilename;
}

uint32_t PckItem::GetDataSize() const
{
	return m_index.dwFileDataSize;
}

uint32_t PckItem::GetCompressDataSize() const
{
	return m_index.dwFileCompressDataSize;
}

std::vector<uint8_t> PckItem::GetCompressData() const
{
	if (auto p = m_pck.lock())
	{
		return p->GetSingleFileCompressData(*this);
	}
	else
	{
		throw std::runtime_error("PckFile对象已释放");
	}
}

std::vector<uint8_t> PckItem::GetData() const
{
	if (auto p = m_pck.lock())
	{
		return p->GetSingleFileData(*this);
	}
	else
	{
		throw std::runtime_error("PckFile对象已释放");
	}
}
