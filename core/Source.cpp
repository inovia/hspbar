#include "windows.h"
#include "atlstr.h"

#include "BarcodeFormat.h"
#include "DecodeHints.h"
#include "ReadBarcode.h"
#include "TextUtfEncoding.h"

#include <map>
using namespace std;
using namespace ZXing;

typedef struct _BAR_RESULT
{
	int /*DecodeStatus*/ status;
	int /*BarcodeFormat*/ format;

	int posTopLeftX;
	int posTopLeftY;
	
	int posTopRightX;
	int posTopRightY;

	int posBottomLeftX;
	int posBottomLeftY;

	int posBottomRightX;
	int posBottomRightY;

	double orientation;

	// バイナリ配列
	int size;
	BYTE* rawBytes;

	// テキストにしたもの
	wchar_t* text;

} BAR_RESULT, *PBAR_RESULT, **PPBAR_RESULT;

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

int bar_Init( bool tryHarder, bool tryRotate, BarcodeFormat fmt)
{
	auto pDecodeHints = new DecodeHints();

	pDecodeHints->setTryHarder( tryHarder);
	pDecodeHints->setTryRotate( tryRotate);
	pDecodeHints->setFormats( fmt);

	static int nSeqIdx = -1;
	g_pDecodeHintsMap[++nSeqIdx] = pDecodeHints;
	return nSeqIdx;
}

// ImageFormat::Lum は グレースケール

int bar_Read(int nIdx, ImageFormat fmt, const uint8_t* pBuf, int nWidth, int nHeight, int nStride, PBAR_RESULT pRet)
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

	auto img = ImageView(pBuf, nWidth, nHeight, fmt, nStride);
	auto result = ReadBarcode(img, *pDecodeHints);
	if ( result.isValid()) 
	{
		pRet->format = static_cast<int>(result.format());
		pRet->status = static_cast<int>(result.status());

		pRet->posTopLeftX = result.position().topLeft().x;
		pRet->posTopLeftY = result.position().topLeft().y;

		pRet->posTopRightX = result.position().topRight().x;
		pRet->posTopRightY = result.position().topRight().y;

		pRet->posBottomLeftX = result.position().bottomLeft().x;
		pRet->posBottomLeftY = result.position().bottomLeft().y;

		pRet->posBottomRightX = result.position().bottomRight().x;
		pRet->posBottomRightY = result.position().bottomRight().y;

		pRet->orientation = result.position().orientation();

		pRet->size = result.numBits() / 8;
		pRet->rawBytes = new BYTE[pRet->size];
		::memcpy( pRet->rawBytes, result.rawBytes().data(), pRet->size );

		pRet->text = new wchar_t[ result.text().length() + 1];
		::wcscpy( pRet->text, result.text().c_str());
	}

	return -4;
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
