#pragma once

#include <cstdint>

#define MAX_PATH_PCK				256
#define PCK_HEAD_SIZE				12
#define PCK_INDEX_SIZE				276
#define PCK_TAIL_SIZE				290
#define	PCK_BEGINCOMPRESS_SIZE		20
#define PCK_MAX_SIZE				0x7FFFFF00

#define	PCK_VERSION					0x20002
#define	PCK_HEAD_VERIFY1			0x4DCA23EF
#define	PCK_HEAD_VERIFY2			0x56A089B7

#define	PCK_TAIL_VERIFY1			0xFDFDFEEE
#define	PCK_ADDR_MASK				0xA8937462
#define	PCK_TAIL_VERIFY2			0xF00DBEEF

#define	PCK_INDEX_MASK1				0xA8937462
#define	PCK_INDEX_MASK2				0xF1A43653

#define	PCK_ADDITIONAL_INFO			"Angelica File Package"
#define	PCK_ADDITIONAL_INFO_STSM	", Perfect World Co. Ltd. 2002~2008. All Rights Reserved."

#pragma pack(1)
// 12 bytes
struct _PckHead
{
	uint32_t dwHeadCheckHead;
	uint32_t dwPckSize;
	uint32_t dwHeadCheckTail;
};

// 280 bytes
struct _PckTail
{
	uint32_t dwIndexTableCheckHead;
	uint32_t dwVersion1;
	uint32_t dwIndexValue;
	char szAdditionalInfo[MAX_PATH_PCK];
	uint32_t dwIndexTableCheckTail;
	uint32_t dwFileCount;
	uint32_t dwVersion2;
};

// 276 bytes
struct _PckItemIndex
{
	char szFilename[MAX_PATH_PCK];
	uint32_t dwUnknown1;
	uint32_t dwAddressOffset;
	uint32_t dwFileDataSize;
	uint32_t dwFileCompressDataSize;
	uint32_t dwUnknown2;
};
#pragma pack()