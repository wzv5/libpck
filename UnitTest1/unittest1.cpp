#include "stdafx.h"
#include "CppUnitTest.h"
#include <sstream>
#include "../include/pckfile.h"
#include "../include/pckitem.h"
#include "../include/pckfile_c.h"
#include "../src/stringhelper.h"
#include "../include/pcktree.h"
#include <Windows.h>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace UnitTest1
{	
	TEST_MODULE_INITIALIZE(ModuleInitialize)
	{
		system("copy /y test.pck test2.pck");
	}

	TEST_CLASS(Pck核心功能)
	{
		shared_ptr<PckFile> pck;
	public:
		Pck核心功能()
		{
			pck = PckFile::Open("test.pck");
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

		TEST_METHOD(获得文件对象_不规范的文件名格式)
		{
			auto& item1 = pck->GetSingleFileItem("//abc\\\\//123.txt");
			Assert::AreEqual("abc\\123.txt", item1.GetFileName());
		}

		TEST_METHOD(统计信息)
		{
			stringstream ss;
			uint64_t s1, s2, s3, s4, s5;
			s1 = pck->GetTotalCompressDataSize();
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
			pck->Extract("./temp");
		}

		TEST_METHOD(构建文件树)
		{
			auto tree = PckTree::BuildTree(pck);
		}

		TEST_METHOD(文件是否存在)
		{
			Assert::IsTrue(pck->FileExists("abc\\123.txt"));
			Assert::IsFalse(pck->FileExists("xxxxx"));
		}
	};

	TEST_CLASS(新建PCK)
	{
		shared_ptr<PckFile> pck;
	public:
		新建PCK()
		{
			pck = PckFile::Create("new.pck", true);
		}

		TEST_METHOD(添加文件)
		{
			pck->AddItem("abc123", 6, "abc/123.txt");
			pck->AddItem(nullptr, 0, "abc/del.txt");
		}
	};

	TEST_CLASS(修改PCK)
	{
		shared_ptr<PckFile> pck;
	public:
		修改PCK()
		{
			pck = PckFile::Open("test2.pck", false);
		}

		TEST_METHOD(添加文件)
		{
			stringstream ss;
			ss << "添加文件前：" << endl;
			ss << "文件数：" << pck->GetFileCount() << endl;
			ss << "文件大小：" << pck->GetFileSize() << endl;
			ss << "数据量：" << pck->GetTotalCompressDataSize() << endl;
			ss << "冗余：" << pck->GetRedundancySize() << endl;
			ss << "索引表：" << pck->GetIndexTableSize() << endl;

			pck->AddItem("abc123", 6, "abc/123.txt");

			ss << "添加文件后：" << endl;
			ss << "文件数：" << pck->GetFileCount() << endl;
			ss << "文件大小：" << pck->GetFileSize() << endl;
			ss << "数据量：" << pck->GetTotalCompressDataSize() << endl;
			ss << "冗余：" << pck->GetRedundancySize() << endl;
			ss << "索引表：" << pck->GetIndexTableSize() << endl;

			Logger::WriteMessage(ss.str().c_str());
			auto data = pck->GetSingleFileItem("abc/123.txt").GetData();
			data.push_back('\0');
			Assert::AreEqual("abc123", (char*)data.data());
		}

		TEST_METHOD(删除文件)
		{
			pck->DeleteItem((*pck)["abc/del.txt"]);
		}
	};

	/*
	TEST_CLASS(创建PCK)
	{
	public:
		TEST_METHOD(重建)
		{
			//PckFile::ReBuild("E:\\诛仙3\\element\\gfx.pck", "Z:\\new.pck", true);
			Pck_ReBuild("E:\\诛仙3\\element\\gfx.pck", "Z:\\new.pck", true);
		}

		TEST_METHOD(从目录创建)
		{
			PckFile::CreateFromDirectory("Z:\\test.pck", "Z:\\npm", false, true, [](auto i, auto t) {
				stringstream ss;
				ss << i << " / " << t << endl;
				Logger::WriteMessage(ss.str().c_str());
				return true;
			});
		}
	};
	*/

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