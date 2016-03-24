#include <cstdio>
#include "pckfile.h"
#include "pckitem.h"

int main()
{
	auto pck = PckFile::Open("/Z/interfaces.pck");
	printf("文件数：%d\n============\n", pck->GetFileCount());
	pck->Extract("/home/wangzheng/pckaaa", [](auto i, auto t) {
		printf("%d / %d\n", i, t);
	});
}
