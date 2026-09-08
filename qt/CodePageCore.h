#pragma once
unsigned CodePageFromCharSet(unsigned characterSet, unsigned documentCodePage) {
	if (documentCodePage == SC_CP_UTF8) {
		return SC_CP_UTF8;
	}
	switch (characterSet) {
	case SC_CHARSET_ANSI: return 1252;
	case SC_CHARSET_DEFAULT: return documentCodePage ? documentCodePage : 1252;
	case SC_CHARSET_BALTIC: return 1257;
	case SC_CHARSET_CHINESEBIG5: return 950;
	case SC_CHARSET_EASTEUROPE: return 1250;
	case SC_CHARSET_GB2312: return 936;
	case SC_CHARSET_GREEK: return 1253;
	case SC_CHARSET_HANGUL: return 949;
	case SC_CHARSET_MAC: return 10000;
	case SC_CHARSET_OEM: return 437;
	case SC_CHARSET_RUSSIAN: return 1251;
	case SC_CHARSET_SHIFTJIS: return 932;
	case SC_CHARSET_TURKISH: return 1254;
	case SC_CHARSET_JOHAB: return 1361;
	case SC_CHARSET_HEBREW: return 1255;
	case SC_CHARSET_ARABIC: return 1256;
	case SC_CHARSET_VIETNAMESE: return 1258;
	case SC_CHARSET_THAI: return 874;
	case SC_CHARSET_8859_15: return 28605;
		// Not supported
	case SC_CHARSET_CYRILLIC: return documentCodePage;
	case SC_CHARSET_SYMBOL: return documentCodePage;
	}
	return documentCodePage;
}
