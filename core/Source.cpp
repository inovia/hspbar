#include "windows.h"
#include "atlstr.h"

#include "BarcodeFormat.h"
#include "DecodeHints.h"
#include "ReadBarcode.h"
#include "TextUtfEncoding.h"

#include "Header.h"

#include <map>
using namespace std;
using namespace ZXing;

// ------------------------------------------------------------------------------
// UTF8 <- -> UTF-16 文字コード変換
// https://www.codeproject.com/Articles/26134/UTF16-to-UTF8-to-UTF16-simple-CString-based-conver
// ------------------------------------------------------------------------------
static CStringA UTF16toUTF8(const CStringW& utf16)
{
	CStringA utf8;
	int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, 0, 0);
	if (len > 1)
	{
		char *ptr = utf8.GetBuffer(len - 1);
		if (ptr) WideCharToMultiByte(CP_UTF8, 0, utf16, -1, ptr, len, 0, 0);
		utf8.ReleaseBuffer();
	}
	return utf8;
}

static CStringW UTF8toUTF16(const CStringA& utf8)
{
	CStringW utf16;
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len > 1)
	{
		wchar_t *ptr = utf16.GetBuffer(len - 1);
		if (ptr) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, ptr, len);
		utf16.ReleaseBuffer();
	}
	return utf16;
}
// ------------------------------------------------------------------------------

// HSP 64bit版がポインタでアクセスできないため、こういう仕様になってます…
// (32bit版は問題ないんですけどね。)
static map<int, DecodeHints* > g_pDecodeHintsMap;

static DecodeHints* GetDecodeHints(int nSmartIdx)
{
	if ( g_pDecodeHintsMap.count( nSmartIdx))
	{
		return g_pDecodeHintsMap[nSmartIdx];
	}
	return nullptr;
}


static void CopyResult(const Result& in, PBAR_RESULT& out)
{
	// 構造体へ詰める
	out->format = static_cast<int>(in.format());
	out->status = static_cast<int>(in.status());

	out->posTopLeftX = in.position().topLeft().x;
	out->posTopLeftY = in.position().topLeft().y;

	out->posTopRightX = in.position().topRight().x;
	out->posTopRightY = in.position().topRight().y;

	out->posBottomLeftX = in.position().bottomLeft().x;
	out->posBottomLeftY = in.position().bottomLeft().y;

	out->posBottomRightX = in.position().bottomRight().x;
	out->posBottomRightY = in.position().bottomRight().y;

	out->orientation = in.position().orientation();

	out->size = in.numBits() / 8;
	if (0 < out->size)
	{
		out->rawBytes = new BYTE[out->size];
		::memcpy(out->rawBytes, in.rawBytes().data(), out->size);
	}
	else
	{
		out->rawBytes = nullptr;
	}

	if (0 < in.text().length())
	{
		out->text = new wchar_t[in.text().length() + 1];
		::wcscpy(out->text, in.text().c_str());
	}
	else
	{
		out->text = nullptr;
	}
}


int bar_Init(BOOL tryHarder, BOOL tryRotate, int fmt)
{
	auto pDecodeHints = new DecodeHints();

	pDecodeHints->setTryHarder( (tryHarder != FALSE));
	pDecodeHints->setTryRotate( (tryRotate != FALSE));
	pDecodeHints->setFormats( static_cast<BarcodeFormat>(fmt));

	static int nSeqIdx = -1;
	g_pDecodeHintsMap[++nSeqIdx] = pDecodeHints;
	return nSeqIdx;
}

// ImageFormat::Lum は グレースケール

int bar_Read(int nIdx, int fmt, const uint8_t* pBuf, int nWidth, int nHeight, int nStride, PBAR_RESULT pRet)
{
	auto pDecodeHints = GetDecodeHints(nIdx);
	if ( pDecodeHints == nullptr)
	{
		return -1;
	}

	if ( pBuf == nullptr)
	{
		return -2;
	}

	if ( pRet == nullptr)
	{
		return -3;
	}

	auto img = ImageView(pBuf, nWidth, nHeight, static_cast<ImageFormat>(fmt), nStride);
	auto result = ReadBarcode(img, *pDecodeHints);
	if ( result.isValid()) 
	{
		CopyResult(result, pRet);
		return 1;
	}

	return 0;
}

char* bar_GetTextA(PBAR_RESULT pRet)
{
	if ( pRet != nullptr && pRet->text != nullptr)
	{
		static CStringA str = pRet->text;
		return str.GetBuffer();
	}

	return nullptr;
}

// HSPInt64.dll なしで文字列を取り扱う用
// var, var(nullptr), int なので 64bitランタイムでも扱える
int bar_CopyTextU8(PBAR_RESULT pRet, char* pDest, int nDestSize)
{
	if ( pRet == nullptr)
	{
		return -1;
	}

	if ( pRet->text == nullptr)
	{
		return -2;
	}

	const auto& pStr = bar_GetTextU8( pRet);
	int nSrcSize = ::strlen( pStr) + 1;
	if ( pDest == nullptr)
	{
		// 必要なバッファサイズを返す
		return nSrcSize;
	}

	if ( nDestSize < nSrcSize )
	{
		return -3;
	}

	::memcpy( pDest, pStr, nSrcSize);

	return nSrcSize;
}

char* bar_GetTextU8(PBAR_RESULT pRet)
{
	if ( pRet != nullptr && pRet->text != nullptr)
	{
		static CStringA str = UTF16toUTF8(pRet->text);
		return str.GetBuffer();
	}

	return nullptr;
}

wchar_t* bar_GetTextU16(PBAR_RESULT pRet)
{
	if ( pRet != nullptr)
	{
		return pRet->text;
	}

	return nullptr;
}

int bar_GetSize()
{
	return sizeof(BAR_RESULT);
}

int bar_Free(PBAR_RESULT pRet)
{
	if ( pRet == nullptr)
	{
		return -1;
	}

	if ( pRet->rawBytes != nullptr)
	{
		delete[] pRet->rawBytes;
		pRet->rawBytes = nullptr;
	}

	if ( pRet->text != nullptr)
	{
		delete[] pRet->text;
		pRet->text = nullptr;
	}

	return 0;
}

int bar_UnInit(int nIdx)
{
	auto pDecodeHints = GetDecodeHints( nIdx);
	if ( pDecodeHints == nullptr)
	{
		return -1;
	}

	// 管理マップから削除
	g_pDecodeHintsMap.erase( nIdx);

	// 実態を削除
	delete pDecodeHints;

	return 0;
}
