#include "pckitem.h"
#include "pckfile.h"

const char* PckItem::GetFileName() const
{
	return m_index.szFilename;
}

uint32_t PckItem::GetDataSize() const
{
	return m_index.dwFileClearDataSize;
}

uint32_t PckItem::GetCipherDataSize() const
{
	return m_index.dwFileCipherDataSize;
}

std::vector<uint8_t> PckItem::GetCipherData() const
{
	if (auto p = m_pck.lock())
	{
		return p->GetSingleFileCipherData(*this);
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
