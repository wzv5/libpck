#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\libpck2\pckfile.h"
#include "..\libpck2\pckitem.h"
#include <sstream>
#include "../libpck2/stringhelper.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{		
	TEST_CLASS(Pck核心功能)
	{
		shared_ptr<PckFile> pck;
	public:
		Pck核心功能()
		{
			pck = PckFile::Open("Z:\\gfx.pck");
		}

		TEST_METHOD(获得文件数)
		{
			stringstream ss;
			ss << "文件数：" << pck->GetFileCount() << endl;
			Logger::WriteMessage(ss.str().c_str());
		}

		TEST_METHOD(获得文件对象)
		{
			auto& item1 = pck->GetSingleFileItem(0);
			auto& item2 = pck->GetSingleFileItem(item1.GetFileName());
			Assert::IsTrue(&item1 == &item2);
		}

		TEST_METHOD(统计信息)
		{
			stringstream ss;
			size_t s1, s2, s3, s4, s5;
			s1 = pck->GetTotalCipherDataSize();
			s2 = pck->GetTotalDataSize();
			s3 = pck->GetRedundancySize();
			s4 = pck->GetFileSize();
			s5 = pck->GetIndexTableSize();
			ss << "文件大小：" << s4 / 1024 / 1024 << " MB" << endl;
			ss << "数据区：" << s1 / 1024 / 1024 << " MB" << endl;
			ss << "解压后：" << s2 / 1024 / 1024 << " MB" << endl;
			ss << "索引表：" << s5 / 1024 << " KB" << endl;
			ss << "冗余数据：" << s3 / 1024 / 1024 << " MB" << endl;
			ss << "压缩率：" << (double)s1 / s2 << endl;
			ss << "冗余率：" << (double)s3 / s4 << endl;
			Logger::WriteMessage(ss.str().c_str());
		}

		TEST_METHOD(解压)
		{
			pck->Extract("Z:\\");
		}
	};

	TEST_CLASS(辅助函数)
	{
	public:
#define TESTSTRA "1234啊不错的abcd"
#define TESTSTRW L"1234啊不错的abcd"
		TEST_METHOD(编码转换_A2W)
		{
			auto loc = std::locale("chinese-simplified_china.936");
			std::string s = TESTSTRA;
			Assert::AreEqual(StringHelper::A2W(s, loc).c_str(), TESTSTRW);
		}

		TEST_METHOD(编码转换_W2A)
		{
			auto loc = std::locale("zh-CN");
			std::wstring s = TESTSTRW;
			Assert::AreEqual(StringHelper::W2A(s, loc).c_str(), TESTSTRA);
		}
	};
}