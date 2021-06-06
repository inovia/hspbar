#pragma once

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

static CStringA UTF16toUTF8(const CStringW& utf16);
static CStringW UTF8toUTF16(const CStringA& utf8);
static DecodeHints* GetDecodeHints(int nSmartIdx);
static void CopyResult(const Result& in, PBAR_RESULT& out);

int bar_Init(bool tryHarder, bool tryRotate, int fmt);
int bar_Read(int nIdx, int fmt, const uint8_t* pBuf, int nWidth, int nHeight, int nStride, PBAR_RESULT pRet);
char* bar_GetTextA(PBAR_RESULT pRet);
char* bar_GetTextU8(PBAR_RESULT pRet);
wchar_t* bar_GetTextU16(PBAR_RESULT pRet);
int bar_GetSize();
int bar_Free(PBAR_RESULT pRet);
int bar_UnInit(int nIdx);
