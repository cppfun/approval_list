
#include "stdafx.h"
#include <Strsafe.h>

#include "UTFConv.h"
#ifdef UTF8TEST
#include <stdio.h>
#endif

#ifdef UTF8TEST
void utf8toWStr(WStr& dest, const Str& src){
	dest.clear();
	wchar_t w = 0;
	int bytes = 0;
	wchar_t err = L'�';
	for (size_t i = 0; i < src.size(); i++){
		unsigned char c = (unsigned char)src[i];
		if (c <= 0x7f){//first byte
			if (bytes){
				dest.push_back(err);
				bytes = 0;
			}
			dest.push_back((wchar_t)c);
		}
		else if (c <= 0xbf){//second/third/etc byte
			if (bytes){
				w = ((w << 6)|(c & 0x3f));
				bytes--;
				if (bytes == 0)
					dest.push_back(w);
			}
			else
				dest.push_back(err);
		}
		else if (c <= 0xdf){//2byte sequence start
			bytes = 1;
			w = c & 0x1f;
		}
		else if (c <= 0xef){//3byte sequence start
			bytes = 2;
			w = c & 0x0f;
		}
		else if (c <= 0xf7){//3byte sequence start
			bytes = 3;
			w = c & 0x07;
		}
		else{
			dest.push_back(err);
			bytes = 0;
		}
	}
	if (bytes)
		dest.push_back(err);
}

void wstrToUtf8(Str& dest, const WStr& src){
	dest.clear();
	for (size_t i = 0; i < src.size(); i++){
		wchar_t w = src[i];
		if (w <= 0x7f)
			dest.push_back((char)w);
		else if (w <= 0x7ff){
			dest.push_back(0xc0 | ((w >> 6)& 0x1f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else if (w <= 0xffff){
			dest.push_back(0xe0 | ((w >> 12)& 0x0f));
			dest.push_back(0x80| ((w >> 6) & 0x3f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else if (w <= 0x10ffff){
			dest.push_back(0xf0 | ((w >> 18)& 0x07));
			dest.push_back(0x80| ((w >> 12) & 0x3f));
			dest.push_back(0x80| ((w >> 6) & 0x3f));
			dest.push_back(0x80| (w & 0x3f));
		}
		else
			dest.push_back('?');
	}
}

Str wstrToUtf8(const WStr& str){
	Str result;
	wstrToUtf8(result, str);
	return result;
}

WStr utf8toWStr(const Str& str){
	WStr result;
	utf8toWStr(result, str);
	return result;
}

std::ostream& operator<<(std::ostream& f, const WStr& s){
	Str s1;
	wstrToUtf8(s1, s);
	f << s1;
	return f;
}

std::istream& operator>>(std::istream& f, WStr& s){
	Str s1;
	f >> s1;
	utf8toWStr(s, s1);
	return f;
}


bool utf8test(){
	WStr w1;
	//for (wchar_t c = 1; c <= 0x10ffff; c++){
	for (wchar_t c = 0x100000; c <= 0x100002; c++){
		w1 += c;	
	}
	Str s = wstrToUtf8(w1);
	WStr w2 = utf8toWStr(s);
	bool result = true;
	if (w1.length() != w2.length()){
		printf("length differs\n");
		//std::cout << "length differs" << std::endl;
		result = false;
	}

	printf("w1: %S\ns: %s\nw2: %S\n", w1.c_str(), s.c_str(), w2.c_str());

	for (size_t i = 0; i < w1.size(); i++)
		if (w1[i] != w2[i]){
			result = false;
			printf("character at pos %x differs (expected %.8x got %.8x)\n", i, w1[i], w2[i]);
			//std::cout << "character at pos " << i  << " differs" << std::endl;
			break;
		}

		if (!result){
			printf("utf8 dump: \n");
			for (size_t i = 0; i < s.size(); i++)
				printf("%2x ", (unsigned char)s[i]);
		}

		return result;
}

int main(int argc, char** argv){
	std::wstring ws(L"фыва");
	std::string s("фыва");
	std::cout << ws << s << std::endl;
	std::cout << wstrToUtf8(utf8toWStr("фыва")) << std::endl;
	if (utf8test())
		std::cout << "utf8Test succesful" << std::endl;
	else 
		std::cout << "utf8Test failed" << std::endl;
	return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// *** Routines to convert between Unicode UTF-8 and Unicode UTF-16 ***
//
// By Giovanni Dicanio <giovanni.dicanio AT gmail.com>
//
// Last update: 2010, January 2nd
//
//
// These routines use ::MultiByteToWideChar and ::WideCharToMultiByte
// Win32 API functions to convert between Unicode UTF-8 and UTF-16.
//
// UTF-16 strings are stored in instances of CStringW.
// UTF-8 strings are stored in instances of CStringA.
//
// On error, the conversion routines use AtlThrow to signal the
// error condition.
//
// If input string pointers are NULL, empty strings are returned.
//
//
// Prefixes used in these routines:
// --------------------------------
//
//  - cch  : count of characters (CHAR's or WCHAR's)
//  - cb   : count of bytes
//  - psz  : pointer to a NUL-terminated string (CHAR* or WCHAR*)
//  - str  : instance of CString(A/W) class
//
//
//
// Useful Web References:
// ----------------------
//
// WideCharToMultiByte Function
// http://msdn.microsoft.com/en-us/library/dd374130.aspx
//
// MultiByteToWideChar Function
// http://msdn.microsoft.com/en-us/library/dd319072.aspx
//
// AtlThrow
// http://msdn.microsoft.com/en-us/library/z325eyx0.aspx
//
//
// Developed on VC9 (Visual Studio 2008 SP1)
//
//
//////////////////////////////////////////////////////////////////////////////

namespace UTF8Util
{
	//----------------------------------------------------------------------------
	// FUNCTION: ConvertUTF8ToUTF16
	// DESC: Converts Unicode UTF-8 text to Unicode UTF-16 (Windows default).
	//----------------------------------------------------------------------------
	CStringW ConvertUTF8ToUTF16( __in const CHAR * pszTextUTF8 )

	{
		//
		// Special case of NULL or empty input string
		//
		if ( (pszTextUTF8 == NULL) || (*pszTextUTF8 == '\0') )
		{
			// Return empty string
			return L"";
		}
		//
		// Consider CHAR's count corresponding to total input string length,
		// including end-of-string (\0) character
		//
		const size_t cchUTF8Max = INT_MAX - 1;
		size_t cchUTF8;
		HRESULT hr = ::StringCchLengthA( pszTextUTF8, cchUTF8Max, &cchUTF8 );
		if ( FAILED( hr ) )
		{
			AtlThrow( hr );
		}
		// Consider also terminating \0
		++cchUTF8;
		// Convert to 'int' for use with MultiByteToWideChar API
		int cbUTF8 = static_cast<int>( cchUTF8 );
		//
		// Get size of destination UTF-16 buffer, in WCHAR's
		//
		int cchUTF16 = ::MultiByteToWideChar(
			CP_UTF8,                // convert from UTF-8
			MB_ERR_INVALID_CHARS,   // error on invalid chars
			pszTextUTF8,            // source UTF-8 string
			cbUTF8,                 // total length of source UTF-8 string,
			// in CHAR's (= bytes), including end-of-string \0
			NULL,                   // unused - no conversion done in this step
			0                       // request size of destination buffer, in WCHAR's
			);

		DWORD dwErr = GetLastError();
		ATLASSERT( cchUTF16 != 0 );
		if ( cchUTF16 == 0 )
		{
			AtlThrowLastWin32();
		}
		//
		// Allocate destination buffer to store UTF-16 string
		//
		CStringW strUTF16;
		WCHAR * pszUTF16 = strUTF16.GetBuffer( cchUTF16 );
		//
		// Do the conversion from UTF-8 to UTF-16
		//
		int result = ::MultiByteToWideChar(
			CP_UTF8,                // convert from UTF-8
			MB_ERR_INVALID_CHARS,   // error on invalid chars
			pszTextUTF8,            // source UTF-8 string
			cbUTF8,                 // total length of source UTF-8 string,
			// in CHAR's (= bytes), including end-of-string \0
			pszUTF16,               // destination buffer
			cchUTF16                // size of destination buffer, in WCHAR's
			);
		ATLASSERT( result != 0 );
		if ( result == 0 )
		{
			AtlThrowLastWin32();
		}
		// Release internal CString buffer
		strUTF16.ReleaseBuffer();
		// Return resulting UTF16 string
		return strUTF16;
	}
	//----------------------------------------------------------------------------
	// FUNCTION: ConvertUTF16ToUTF8
	// DESC: Converts Unicode UTF-16 (Windows default) text to Unicode UTF-8.
	//----------------------------------------------------------------------------
	CStringA ConvertUTF16ToUTF8( __in const WCHAR * pszTextUTF16 )
	{
		//
		// Special case of NULL or empty input string
		//
		if ( (pszTextUTF16 == NULL) || (*pszTextUTF16 == L'\0') )
		{
			// Return empty string
			return "";
		}
		//
		// Consider WCHAR's count corresponding to total input string length,
		// including end-of-string (L'\0') character.
		//
		const size_t cchUTF16Max = INT_MAX - 1;
		size_t cchUTF16;
		HRESULT hr = ::StringCchLengthW( pszTextUTF16, cchUTF16Max, &cchUTF16 );
		if ( FAILED( hr ) )
		{
			AtlThrow( hr );
		}
		// Consider also terminating \0
		++cchUTF16;
		//
		// WC_ERR_INVALID_CHARS flag is set to fail if invalid input character
		// is encountered.
		// This flag is supported on Windows Vista and later.
		// Don't use it on Windows XP and previous.
		//

#if (WINVER >= 0x0600)
		DWORD dwConversionFlags = WC_ERR_INVALID_CHARS;
#else
		DWORD dwConversionFlags = 0;
#endif
		//
		// Get size of destination UTF-8 buffer, in CHAR's (= bytes)
		//
		int cbUTF8 = ::WideCharToMultiByte(
			CP_UTF8,                // convert to UTF-8
			dwConversionFlags,      // specify conversion behavior
			pszTextUTF16,           // source UTF-16 string
			static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
			// including end-of-string \0
			NULL,                   // unused - no conversion required in this step
			0,                      // request buffer size
			NULL, NULL              // unused
			);
		ATLASSERT( cbUTF8 != 0 );
		if ( cbUTF8 == 0 )
		{
			AtlThrowLastWin32();
		}
		//
		// Allocate destination buffer for UTF-8 string
		//
		CStringA strUTF8;
		int cchUTF8 = cbUTF8; // sizeof(CHAR) = 1 byte
		CHAR * pszUTF8 = strUTF8.GetBuffer( cchUTF8 );
		//
		// Do the conversion from UTF-16 to UTF-8
		//
		int result = ::WideCharToMultiByte(
			CP_UTF8,                // convert to UTF-8
			dwConversionFlags,      // specify conversion behavior
			pszTextUTF16,           // source UTF-16 string
			static_cast<int>( cchUTF16 ),   // total source string length, in WCHAR's,
			// including end-of-string \0
			pszUTF8,                // destination buffer
			cbUTF8,                 // destination buffer size, in bytes
			NULL, NULL              // unused
			); 
		ATLASSERT( result != 0 );
		if ( result == 0 )
		{
			AtlThrowLastWin32();
		}
		// Release internal CString buffer
		strUTF8.ReleaseBuffer();
		// Return resulting UTF-8 string
		return strUTF8;
	}

	// 65001 is utf-8.
	wchar_t *CodePageToUnicode(int codePage, const char *src)
	{
		if (!src) return 0;
		int srcLen = strlen(src);
		if (!srcLen)
		{
			wchar_t *w = new wchar_t[1];
			w[0] = 0;
			return w;
		}

		int requiredSize = MultiByteToWideChar(codePage,
			0,
			src,srcLen,0,0);

		if (!requiredSize)
		{
			return 0;
		}

		wchar_t *w = new wchar_t[requiredSize+1];
		w[requiredSize] = 0;

		int retval = MultiByteToWideChar(codePage,
			0,
			src,srcLen,w,requiredSize);
		if (!retval)
		{
			delete [] w;
			return 0;
		}

		return w;
	}

	char *UnicodeToCodePage(int codePage, const wchar_t *src)
	{
		if (!src) return 0;
		int srcLen = wcslen(src);
		if (!srcLen)
		{
			char *x = new char[1];
			x[0] = '\0';
			return x;
		}

		int requiredSize = WideCharToMultiByte(codePage,
			0,
			src,srcLen,0,0,0,0);

		if (!requiredSize)
		{
			return 0;
		}

		char *x = new char[requiredSize+1];
		x[requiredSize] = 0;

		int retval = WideCharToMultiByte(codePage,
			0,
			src,srcLen,x,requiredSize,0,0);
		if (!retval)
		{
			delete [] x;
			return 0;
		}

		return x;
	}


} // namespace UTF8Util

 