/*
 *  unicode.c
 *
 *  $Id$
 *
 *  ODBC unicode functions
 *
 *  The iODBC driver manager.
 *
 *  Copyright (C) 1996-2022 OpenLink Software <iodbc@openlinksw.com>
 *  All Rights Reserved.
 *
 *  This software is released under the terms of either of the following
 *  licenses:
 *
 *      - GNU Library General Public License (see LICENSE.LGPL)
 *      - The BSD License (see LICENSE.BSD).
 *
 *  Note that the only valid version of the LGPL license as far as this
 *  project is concerned is the original GNU Library General Public License
 *  Version 2, dated June 1991.
 *
 *  While not mandated by the BSD license, any patches you make to the
 *  iODBC source code may be contributed back into the iODBC project
 *  at your discretion. Contributions will benefit the Open Source and
 *  Data Access community as a whole. Submissions may be made at:
 *
 *      http://www.iodbc.org
 *
 *
 *  GNU Library Generic Public License Version 2
 *  ============================================
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; only
 *  Version 2 of the License dated June 1991.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  The BSD License
 *  ===============
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. Neither the name of OpenLink Software Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OPENLINK OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define UNICODE

#include <iodbc.h>

#include <isql.h>
#include <isqlext.h>
#include <isqltypes.h>

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#if _MSC_VER < 1300
#include <ansiapi.h>
#endif
#include <stringapiset.h>
#endif

#include "unicode.h"


#ifndef UNICHAR_DEFINED
#define UNICHAR_DEFINED
typedef int unichar;				/*!< 31-bit unicode values, negative ones are invalid */
#endif

#define UNICHAR_EOD		((unichar)(-2))	/*!< End of source buffer reached, no data to convert */
#define UNICHAR_NO_DATA		((unichar)(-3))	/*!< Source buffer is too short, but nonempty (contains part of a char) */
#define UNICHAR_NO_ROOM		((unichar)(-4))	/*!< Target buffer is too short */
#define UNICHAR_BAD_ENCODING	((unichar)(-5))	/*!< Invalid character decoded from invalid string */
#define UNICHAR_OUT_OF_WCHAR	((unichar)(-6))	/*!< The encoded data are valid but the encoded character is out of 16-bit range and will not fit 2-byte wchar_t. */

typedef const char *__constcharptr;

static unichar eh_decode_char__UTF16BE (__constcharptr *src_begin_ptr,
	const char *src_buf_end, ...);
static char *eh_encode_char__UTF16BE (unichar char_to_put, char *tgt_buf,
	char *tgt_buf_end, ...);
static unichar eh_decode_char__UTF16LE (__constcharptr *src_begin_ptr,
	const char *src_buf_end, ...);
static char *eh_encode_char__UTF16LE (unichar char_to_put, char *tgt_buf,
	char *tgt_buf_end, ...);

static size_t _calc_len_for_utf8 (IODBC_CHARSET charset, void * str,
	int size);
static size_t _wcxtoutf8 (IODBC_CHARSET charset, void * wstr, char * ustr,
	int size);
static size_t _wcxntoutf8 (IODBC_CHARSET charset, void * wstr, char *ustr,
	int wlen, int size, int * converted);
static size_t _utf8towcx (IODBC_CHARSET charset, char * ustr, void * wstr,
	int size);

static size_t _WCSLEN(IODBC_CHARSET charset, void *str);

static size_t dm_UWtoA(wchar_t *src, int ilen, char *dest, int olen);
static size_t dm_AtoUW(char *src, int ilen, wchar_t *dest, size_t olen);

#ifndef MAX
# define MAX(X,Y)	(X > Y ? X : Y)
# define MIN(X,Y)	(X < Y ? X : Y)
#endif


#if !defined(HAVE_WCSLEN)
size_t
wcslen (const wchar_t * wcs)
{
  size_t len = 0;

  while (*wcs++ != L'\0')
    len++;

  return len;
}
#endif


#if !defined(HAVE_WCSCPY)
wchar_t *
wcscpy (wchar_t * wcd, const wchar_t * wcs)
{
  wchar_t *dst = wcd;

  while ((*dst++ = *wcs++) != L'\0')
    ;

  return wcd;
}
#endif


#if !defined (HAVE_WCSNCPY)
wchar_t *
wcsncpy (wchar_t * wcd, const wchar_t * wcs, size_t n)
{
  wchar_t *dst = wcd;
  size_t len = 0;

  while ( len < n && (*dst++ = *wcs++) != L'\0')
    len++;

  for (; len < n; len++)
    *dst++ = L'\0';

  return wcd;
}
#endif

#if !defined(HAVE_WCSCHR)
wchar_t* wcschr(const wchar_t *wcs, const wchar_t wc)
{
  do
    if(*wcs == wc)
      return (wchar_t*) wcs;
  while(*wcs++ != L'\0');
 
  return NULL;
}
#endif

#if !defined(HAVE_WCSCAT)
wchar_t* wcscat(wchar_t *dest, const wchar_t *src)
{
  wchar_t *s1 = dest;
  const wchar_t *s2 = src;
  wchar_t c;
  
  do
    c = *s1 ++;
  while(c != L'\0');

  s1 -= 2;
  
  do
    {
      c = *s2 ++;
      *++s1 = c;
    }
  while(c != L'\0');

  return dest;
}
#endif

#if !defined(HAVE_WCSCMP)
int wcscmp (const wchar_t* s1, const wchar_t* s2)
{
  wchar_t c1, c2;
  
  if (s1 == s2)
    return 0;

  do
    {
      c1 = *s1++;
      c2 = *s2++;
      if(c1 == L'\0')
        break;
    }
  while (c1 == c2);

  return c1 - c2;
}
#endif


#if !defined(HAVE_TOWLOWER)

#if (defined (__APPLE__) && !(defined (NO_FRAMEWORKS) || defined (_LP64)))

#include <Carbon/Carbon.h>

wchar_t
towlower (wchar_t wc)
{
  CFMutableStringRef strRef = CFStringCreateMutable (NULL, 0);
  UniChar c = (UniChar) wc;
  wchar_t wcs;

  CFStringAppendCharacters (strRef, &c, 1);
  CFStringLowercase (strRef, NULL);
  wcs = CFStringGetCharacterAtIndex (strRef, 0);
  CFRelease (strRef);

  return wcs;
}

#else

/* Use dummy function */
wchar_t
towlower (wchar_t wc)
{
  return wc;
}

#endif /* __APPLE__ */
#endif /* !HAVE_TOWLOWER */


#if !defined(HAVE_WCSNCASECMP)
int wcsncasecmp (const wchar_t* s1, const wchar_t* s2, size_t n)
{
  wchar_t c1, c2;
  
  if (s1 == s2 || n ==0)
    return 0;

  do
    {
      c1 = towlower(*s1++);
      c2 = towlower(*s2++);
      if(c1 == L'\0' || c1 != c2)
        return c1 - c2;
    } while (--n > 0);

  return c1 - c2;
}
#endif


/* UTF-16BE */

static unichar
eh_decode_char__UTF16BE (__constcharptr *src_begin_ptr, const char *src_buf_end, ...)
{
/* As is in RFC 2781...
   U' = yyyyyyyyyyxxxxxxxxxx
   W1 = 110110yyyyyyyyyy
   W2 = 110111xxxxxxxxxx
*/
  unsigned char *src_begin = (unsigned char *)(src_begin_ptr[0]);
  unsigned char hi, lo, hiaddon, loaddon;
  unichar acc /* W1 */, accaddon /* W2 */;
  if (src_begin >= (unsigned char *)src_buf_end)
    return UNICHAR_EOD;
  if (src_begin+1 >= (unsigned char *)src_buf_end)
    return UNICHAR_NO_DATA;
  hi = src_begin[0];
  lo = src_begin[1];
  acc = (hi << 8) | lo;
  if (0xFFFE == acc)
    return UNICHAR_BAD_ENCODING; /* Maybe UTF16LE ? */
  switch (acc & 0xFC00)
    {
      case 0xD800:
	if (src_begin+3 >= (unsigned char *)src_buf_end)
	  return UNICHAR_NO_DATA;
	hiaddon = src_begin[2];
	loaddon = src_begin[3];
	accaddon = (hiaddon << 8) | loaddon;
	if (0xDC00 != (accaddon & 0xFC00))
	  return UNICHAR_BAD_ENCODING; /* No low-half after hi-half ? */
	src_begin_ptr[0] += 4;
	return 0x10000 + (((acc & 0x3FF) << 10) | (accaddon & 0x3FF));
      case 0xDC00:
	return UNICHAR_BAD_ENCODING; /* Low-half first ? */
      default:
	src_begin_ptr[0] += 2;
	return acc;
    }
}


static char *
eh_encode_char__UTF16BE (unichar char_to_put, char *tgt_buf, char *tgt_buf_end, ...)
{
  if (char_to_put < 0)
    return tgt_buf;
  if (char_to_put & ~0xFFFF)
    {
      if (tgt_buf+4 > tgt_buf_end)
	return (char *)UNICHAR_NO_ROOM;
      char_to_put -= 0x10000;
      tgt_buf[0] = (unsigned char)(0xD8 | ((char_to_put >> 18) & 0x03));
      tgt_buf[1] = (unsigned char)((char_to_put >> 10) & 0xFF);
      tgt_buf[2] = (unsigned char)(0xDC | ((char_to_put >> 8) & 0x03));
      tgt_buf[3] = (unsigned char)(char_to_put & 0xFF);
      return tgt_buf+4;
    }
  if (0xD800 == (char_to_put & 0xF800))
    return tgt_buf;
  if (tgt_buf+2 > tgt_buf_end)
    return (char *)UNICHAR_NO_ROOM;
  tgt_buf[0] = (unsigned char)(char_to_put >> 8);
  tgt_buf[1] = (unsigned char)(char_to_put & 0xFF);
  return tgt_buf+2;
}


/* UTF-16LE */

static unichar
eh_decode_char__UTF16LE (__constcharptr *src_begin_ptr, const char *src_buf_end, ...)
{
/* As is in RFC 2781...
   U' = yyyyyyyyyyxxxxxxxxxx
   W1 = 110110yyyyyyyyyy
   W2 = 110111xxxxxxxxxx
*/
  unsigned char *src_begin = (unsigned char *)(src_begin_ptr[0]);
  unsigned char hi, lo, hiaddon, loaddon;
  unichar acc /* W1 */, accaddon /* W2 */;
  if (src_begin >= (unsigned char *)src_buf_end)
    return UNICHAR_EOD;
  if (src_begin+1 >= (unsigned char *)src_buf_end)
    return UNICHAR_NO_DATA;
  hi = src_begin[1];
  lo = src_begin[0];
  acc = (hi << 8) | lo;
  if (0xFFFE == acc)
    return UNICHAR_BAD_ENCODING; /* Maybe UTF16BE ? */
  switch (acc & 0xFC00)
    {
      case 0xD800:
	if (src_begin+3 >= (unsigned char *)src_buf_end)
	  return UNICHAR_NO_DATA;
	hiaddon = src_begin[3];
	loaddon = src_begin[2];
	accaddon = (hiaddon << 8) | loaddon;
	if (0xDC00 != (accaddon & 0xFC00))
	  return UNICHAR_BAD_ENCODING; /* No low-half after hi-half ? */
	src_begin_ptr[0] += 4;
	return 0x10000 + (((acc & 0x3FF) << 10) | (accaddon & 0x3FF));
      case 0xDC00:
	return UNICHAR_BAD_ENCODING; /* Low-half first ? */
      default:
	src_begin_ptr[0] += 2;
	return acc;
    }
}


static char *
eh_encode_char__UTF16LE (unichar char_to_put, char *tgt_buf, char *tgt_buf_end, ...)
{
  if (char_to_put < 0)
    return tgt_buf;
  if (char_to_put & ~0xFFFF)
    {
      if (tgt_buf+4 > tgt_buf_end)
	return (char *)UNICHAR_NO_ROOM;
      char_to_put -= 0x10000;
      tgt_buf[1] = (unsigned char)(0xD8 | ((char_to_put >> 18) & 0x03));
      tgt_buf[0] = (unsigned char)((char_to_put >> 10) & 0xFF);
      tgt_buf[3] = (unsigned char)(0xDC | ((char_to_put >> 8) & 0x03));
      tgt_buf[2] = (unsigned char)(char_to_put & 0xFF);
      return tgt_buf+4;
    }
  if (0xD800 == (char_to_put & 0xF800))
    return tgt_buf;
  if (tgt_buf+2 > tgt_buf_end)
    return (char *)UNICHAR_NO_ROOM;
  tgt_buf[1] = (unsigned char)(char_to_put >> 8);
  tgt_buf[0] = (unsigned char)(char_to_put & 0xFF);
  return tgt_buf+2;
}



SQLCHAR *
dm_SQL_W2A (SQLWCHAR * inStr, int size)
{
  SQLCHAR *outStr = NULL;
  size_t len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
   len = wcslen (inStr);
  else if (size < 0)
   return NULL;
  else
    len = (size_t)size;

# ifdef WIN32
  if ((outStr = (SQLCHAR *) calloc (len * UTF8_MAX_CHAR_LEN + 1, 1)) != NULL)
    {
      if (len > 0)
	OPL_W2A (inStr, outStr, len);
      outStr[len] = '\0';
    }
#else
  if ((outStr = (SQLCHAR *) calloc (len * MB_CUR_MAX + 1, 1)) != NULL)
    {
      if (len > 0)
        dm_UWtoA(inStr, len, outStr, len * MB_CUR_MAX);
    }
#endif

  return outStr;
}


SQLWCHAR *
dm_SQL_A2W (SQLCHAR * inStr, int size)
{
  SQLWCHAR *outStr = NULL;
  size_t len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
    len = strlen ((char *) inStr);
  else if (size < 0)
    return NULL;
  else
    len = (size_t)size;

  if ((outStr = (SQLWCHAR *) calloc (len + 1, sizeof (SQLWCHAR))) != NULL)
    {
      if (len > 0)
        dm_AtoUW(inStr, len, outStr, len);
    }

  return outStr;
}


int
dm_StrCopyOut2_A2W (
  SQLCHAR	* inStr,
  SQLWCHAR	* outStr,
  SQLSMALLINT	  size,
  WORD		* result)
{
  size_t length;

  if (!inStr)
    return -1;

  length = strlen ((char *) inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  if (size >= length + 1)
    {
      if (length > 0)
        length = dm_AtoUW(inStr, length, outStr, length);

      outStr[length] = L'\0';
      return 0;
    }
  if (size > 0)
    {
      length = dm_AtoUW(inStr, length, outStr, size);
      outStr[length] = L'\0';
    }
  return -1;
}

int
dm_StrCopyOut2_W2A (
  SQLWCHAR	* inStr,
  SQLCHAR	* outStr,
  SQLSMALLINT	  size,
  WORD		* result)
{
  size_t length;

  if (!inStr)
    return -1;

  length = wcslen (inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  if (size >= length + 1)
    {
      if (length > 0)
        length = dm_UWtoA(inStr, length, outStr, length);

      outStr[length] = '\0';
      return 0;
    }
  if (size > 0)
    {
      length = dm_UWtoA(inStr, length, outStr, size);
      outStr[length] = '\0';
    }
  return -1;
}


SQLWCHAR *
dm_strcpy_A2W (SQLWCHAR * destStr, SQLCHAR * sourStr)
{
  size_t length;

  if (!sourStr || !destStr)
    return destStr;

  length = strlen ((char *) sourStr);

  if (length > 0)
    length = dm_AtoUW(sourStr, length, destStr, length);

  destStr[length] = L'\0';
  return destStr;
}


SQLCHAR *
dm_strcpy_W2A (SQLCHAR * destStr, SQLWCHAR * sourStr)
{
  size_t length;

  if (!sourStr || !destStr)
    return destStr;

  length = wcslen (sourStr);

  if (length > 0)
    length = dm_UWtoA(sourStr, length, destStr, length);

  destStr[length] = '\0';
  return destStr;
}


/* encode */
#define LEN_FOR_UTF8(Char, Len)		        \
  if (Char < 0x80)                              \
    {                                           \
      Len = 1;                                  \
    }                                           \
  else if (Char < 0x800)                        \
    {                                           \
      Len = 2;                                  \
    }                                           \
  else if (Char < 0x10000)                      \
    {                                           \
      Len = 3;                                  \
    }                                           \
  else if (Char < 0x110000)                     \
    {                                           \
      Len = 4;                                  \
    }                                           \
  else                                          \
    {                                           \
      Len = 1;                                  \
    }

#define CONV_TO_UTF8(Char, Len, First)		\
  if (Char < 0x80)                              \
    {                                           \
      Len = 1;                                  \
      First = 0;                                \
    }                                           \
  else if (Char < 0x800)                        \
    {                                           \
      Len = 2;                                  \
      First = 0xC0;                             \
    }                                           \
  else if (Char < 0x10000)                      \
    {                                           \
      Len = 3;                                  \
      First = 0xE0;                             \
    }                                           \
  else if (Char < 0x110000)                     \
    {                                           \
      Len = 4;                                  \
      First = 0xf0;                             \
    }                                           \
  else                                          \
    {                                           \
      Len = 1;                                  \
      First = 0;                                \
      Char = '?';                               \
    }


/* decode */
#define UTF8_COMPUTE(Char, Mask, Len)					      \
  if (Char < 128)							      \
    {									      \
      Len = 1;								      \
      Mask = 0x7f;							      \
    }									      \
  else if ((Char & 0xe0) == 0xc0)					      \
    {									      \
      Len = 2;								      \
      Mask = 0x1f;							      \
    }									      \
  else if ((Char & 0xf0) == 0xe0)					      \
    {									      \
      Len = 3;								      \
      Mask = 0x0f;							      \
    }									      \
  else if ((Char & 0xf8) == 0xf0)					      \
    {									      \
      Len = 4;								      \
      Mask = 0x07;							      \
    }									      \
  else									      \
    Len = -1;



static size_t
_WCHARSIZE(IODBC_CHARSET charset)
{
  switch(charset)
    {
    case CP_UTF8: return 1;
    case CP_UTF16: return sizeof(ucs2_t);
    case CP_UCS4:
    default:
    	return sizeof(ucs4_t);
    }
}


static size_t
_WCHARSIZE_ALLOC(IODBC_CHARSET charset)
{
  switch(charset)
    {
    case CP_UTF8: return UTF8_MAX_CHAR_LEN;
    case CP_UTF16: return sizeof(ucs2_t) * 2;
    case CP_UCS4:
    default:
    	return sizeof(ucs4_t);
    }
}


static size_t
_utf16_calc_len_for_utf8 (ucs2_t *str, int size)
{
  size_t len = 0;
  ucs4_t wc;
  char *us = (char *) str;
  char *us_end;
  size_t utf8_len = 0;

  if (!str)
    return len;

  us_end = (char *)(str + size);

  while (size > 0)
    {
#ifdef WORDS_BIGENDIAN
      wc = (ucs4_t) eh_decode_char__UTF16BE((__constcharptr *) &us, us_end);
#else
      wc = (ucs4_t) eh_decode_char__UTF16LE((__constcharptr *) &us, us_end);
#endif
      if (wc == UNICHAR_EOD || wc == UNICHAR_NO_DATA || wc == UNICHAR_BAD_ENCODING)
        break;

      LEN_FOR_UTF8(wc, utf8_len);
      len += utf8_len;

      size--;
    }

  return len;
}

static size_t
_ucs4_calc_len_for_utf8 (ucs4_t *str, int size)
{
  size_t len = 0;
  ucs4_t wc;
  size_t utf8_len = 0;

  if (!str)
    return len;

  while (size > 0)
    {
      wc = *str;

      LEN_FOR_UTF8(wc, utf8_len);
      len += utf8_len;

      str++;
      size--;
    }

  return len;
}


static size_t
_calc_len_for_utf8 (IODBC_CHARSET charset, void * str, int size)
{
  if (!str)
    return 0;

  if (size == SQL_NTS)
    size = _WCSLEN (charset, str);

  if (charset == CP_UTF16)
    return _utf16_calc_len_for_utf8 ((ucs2_t*)str, size);
  else
    return _ucs4_calc_len_for_utf8 ((ucs4_t*)str, size);
}



static size_t
utf8_len (SQLCHAR * p, int size)
{
  size_t len = 0;

  if (!*p)
    return 0;

  if (size == SQL_NTS)
    while (*p)
      {
	for (p++; (*p & 0xC0) == 0x80; p++)
	  ;
	len++;
      }
  else
    while (size > 0)
      {
	for (p++, size--; (size > 0) && ((*p & 0xC0) == 0x80); p++, size--)
	  ;
	len++;
      }
  return len;
}


/*
 *  size      - size of buffer for output utf8 string in bytes
 *  return    - length of output utf8 string
 */
static size_t
wcstoutf8 (wchar_t *wstr, char *ustr, int size_bytes)
{
  return _wcxtoutf8 (CP_DEF, (void *)wstr, ustr, size_bytes);
}

static size_t
_utf16ntoutf8 (ucs2_t *wstr, char *ustr, int wlen, int size_bytes, int *converted)
{
  int len;
  ucs4_t wc;
  int first;
  int i;
  int count = 0;
  char *us0 = (char *) wstr;
  char *us = (char *) wstr;
  char *us_end;
  int _converted = 0;

  if (!wstr)
    return 0;

  us_end = (char *)(wstr + wlen);

  while(_converted < wlen && count < size_bytes)
    {
#ifdef WORDS_BIGENDIAN
      wc = (ucs4_t) eh_decode_char__UTF16BE((__constcharptr *) &us, us_end);
#else
      wc = (ucs4_t) eh_decode_char__UTF16LE((__constcharptr *) &us, us_end);
#endif
      if (wc == UNICHAR_EOD || wc == UNICHAR_NO_DATA || wc == UNICHAR_BAD_ENCODING)
        break;

      CONV_TO_UTF8(wc, len, first);

      if (size_bytes - count < len)
	{
	  if (converted)
	    *converted = _converted;
	  return count;
	}

      for (i = len - 1; i > 0; --i)
	{
	  ustr[i] = (wc & 0x3f) | 0x80;
	  wc >>= 6;
	}
      ustr[0] = wc | first;

      ustr += len;
      count += len;
      _converted = (us - us0) / sizeof(ucs2_t);
    }

  if (converted)
    *converted = _converted;

  return count;
}


/*
 *  wlen      - length of input *wstr string in symbols(for utf8 in bytes)
 *  size     - size of buffer ( *ustr string) in bytes
 *  converted - number of converted symbols from *wstr(for utf8 in bytes)
 *
 *  Return    - length of output utf8 string
 */
static size_t
_wcxntoutf8 (
  IODBC_CHARSET   charset,
  void		* wstr,
  char		* ustr,
  int		  wlen,
  int		  size_bytes,
  int	 	* converted)
{
  int len;
  ucs4_t c;
  int first;
  int i;
  int count = 0;
  int _converted = 0;
  ucs4_t *u4str = (ucs4_t *)wstr;
  wchar_t *sstr = (wchar_t *)wstr;

  if (!wstr)
    return 0;

  if (charset == CP_UTF8)
    {
      unsigned char *u8str = (unsigned char*)wstr;
      int mask;

      while (_converted < wlen && count < size_bytes)
        {
          UTF8_COMPUTE(*u8str, mask, len);

          if (size_bytes - count < len)
	    {
	      if (converted)
	        *converted = _converted;
	      return count;
	    }

          for (i = 0; i < len; i++)
	    *ustr++ = *u8str++;

	  count += len;
          _converted+=len;
        }

      if (converted)
        *converted = _converted;

      return count;
    }
  else if (charset == CP_UTF16)
    {
      return _utf16ntoutf8 ((ucs2_t *) wstr, ustr, wlen, size_bytes, converted);
    }
  else
    {
      while (_converted < wlen && count < size_bytes)
        {
          switch(charset)
            {
            case CP_UCS4: c = (ucs4_t)*u4str; break;
            default:      c = (ucs4_t)*sstr;  break;
            }

          CONV_TO_UTF8(c, len, first);

          if (size_bytes - count < len)
	    {
	      if (converted)
	        *converted = _converted;
	      return count;
	    }

          for (i = len - 1; i > 0; --i)
	    {
	      ustr[i] = (c & 0x3f) | 0x80;
	      c >>= 6;
	    }
          ustr[0] = c | first;

          ustr += len;
          count += len;
          _converted++;

          switch(charset)
            {
            case CP_UCS4: u4str++; break;
            default:      sstr++;  break;
            }
        }

      if (converted)
        *converted = _converted;

      return count;
    }
}


/*
 *  size      - size of buffer for output utf8 string in bytes
 *  return    - length of output utf8 string in bytes
 */
static size_t
_wcxtoutf8 (IODBC_CHARSET charset, void * wstr, char * ustr, int size_bytes)
{
  if (!wstr)
    return 0;

  return _wcxntoutf8 (charset, wstr, ustr, _WCSLEN(charset, wstr), size_bytes, NULL);
}


/*
 *  wlen      - length of input *wstr string in symbols
 *  size     - size of buffer ( *ustr string) in bytes
 *  converted - number of converted symbols from *wstr
 *
 *  Return    - length of output utf8 string
 */
static size_t
wcsntoutf8 (wchar_t *wstr, char *ustr, int wlen, int size,
    int *converted)
{
  if (!wstr)
    return 0;

  return _wcxntoutf8 (CP_DEF, (void*)wstr, ustr, wlen, size, converted);
}



static SQLCHAR *
strdup_WtoU8 (SQLWCHAR * str)
{
  SQLCHAR *ret;
  int len;

  if (!str)
    return NULL;

  len = _calc_len_for_utf8 (CP_DEF, str, SQL_NTS);
  if ((ret = (SQLCHAR *) malloc (len + 1)) == NULL)
    return NULL;

  len = wcstoutf8 (str, (char *)ret, len);
  ret[len] = '\0';

  return ret;
}


/*
 *  size      - size of buffer for output string in symbols (SQLWCHAR)
 *  return    - length of output SQLWCHAR string
 */
static size_t
utf8towcs (char *ustr, SQLWCHAR *wstr, int size)
{
  return _utf8towcx (CP_DEF, ustr, (void *)wstr, size);
}


/*
 *  ulen      - length of input *ustr string in bytes
 *  size      - size of buffer ( *wstr string) in symbols
 *  converted - number of converted bytes from *ustr
 *
 *  Return    - length of output wcs string
 */
static size_t
_utf8ntoutf16 (
  char		* ustr,
  ucs2_t	* wstr,
  int		  ulen,
  int		  size,
  int		* converted)
{
  int i;
  int mask = 0;
  int len;
  SQLCHAR c;
  ucs4_t wc;
  int count = 0;
  int _converted = 0;
  char *rc;
  char *us = (char*) wstr;
  char *us_end = (char*) (wstr + size);

  if (!ustr)
    return 0;

  while ((_converted < ulen) && (count < size))
    {
      c = *ustr;
      UTF8_COMPUTE (c, mask, len);
      if ((len == -1) || (_converted + len > ulen))
	{
	  if (converted)
	    *converted = _converted;
	  return count;
	}

      wc = c & mask;
      for (i = 1; i < len; i++)
	{
	  if ((ustr[i] & 0xC0) != 0x80)
	    {
	      if (converted)
		*converted = _converted;
	      return count;
	    }
	  wc <<= 6;
	  wc |= (ustr[i] & 0x3F);
	}

#ifdef WORDS_BIGENDIAN
      rc = eh_encode_char__UTF16BE ((unichar)wc, us, us_end);
#else
      rc = eh_encode_char__UTF16LE ((unichar)wc, us, us_end);
#endif
      if ((char *)UNICHAR_NO_ROOM == rc)
        break;

      count += (rc - us) / sizeof(ucs2_t);
      us = rc;
      ustr += len;
      _converted += len;
    }
  if (converted)
    *converted = _converted;
  return count;
}


/*
 *  ulen      - length of input *ustr string in bytes
 *  size      - size of buffer ( *wstr string) in symbols
 *  converted - number of converted bytes from *ustr
 *
 *  Return    - length of output wcs string
 */
static size_t
_utf8ntowcx (
  IODBC_CHARSET   charset,
  char		* ustr,
  void		* wstr,
  int		  ulen,
  int		  size,
  int		* converted)
{
  int  i;
  int  mask = 0;
  int  len;
  unsigned char c;
  ucs4_t wc;
  int  count = 0;
  int  _converted = 0;
  ucs4_t *u4str = (ucs4_t *)wstr;
  char *u8str = (char*)wstr;

  if (!ustr)
    return 0;

  if (charset == CP_UTF16)
    return _utf8ntoutf16 (ustr, wstr, ulen, size, converted);

  while ((_converted < ulen) && (count < size))
    {
      c = (unsigned char)*ustr;
      UTF8_COMPUTE (c, mask, len);

      if ((len == -1) || (_converted + len > ulen))
	{
	  if (converted)
	    *converted = _converted;
	  return count;
	}

      if (charset == CP_UTF8)
        {
          for (i = 0; i < len; i++)
	    *u8str++ = *ustr++;

	  count += len;
          _converted += len;
        }
      else /* CP_UCS4 */
        {
          wc = c & mask;
          for (i = 1; i < len; i++)
	    {
	      if ((ustr[i] & 0xC0) != 0x80)
	        {
	          if (converted)
		    *converted = _converted;
	          return count;
	        }
	      wc <<= 6;
	      wc |= (ustr[i] & 0x3F);
	    }

          *u4str = (ucs4_t)wc;
          u4str++;

          ustr += len;
          count++;
          _converted += len;
        }
    }

  if (converted)
    *converted = _converted;
  return count;
}

/*
 *  size      - size of buffer for output string in symbols
 *  return    - length of output SQLWCHAR string in bytes
 */
static size_t
_utf8towcx (IODBC_CHARSET charset, char * ustr, void * wstr, int size)
{
  if (!ustr)
    return 0;

  return _utf8ntowcx (charset, ustr, wstr, strlen(ustr), size, NULL);
}


/*
 *  ulen      - length of input *ustr string in bytes
 *  size      - size of buffer ( *wstr string) in symbols
 *  converted - number of converted bytes from *ustr
 *
 *  Return    - length of output wcs string
 */
static size_t
utf8ntowcs (
  SQLCHAR	* ustr,
  SQLWCHAR	* wstr,
  int		  ulen,
  int		  size,
  int		* converted)
{
  int i;
  int mask = 0;
  int len;
  SQLCHAR c;
  SQLWCHAR wc;
  int count = 0;
  int _converted = 0;

  if (!ustr)
    return 0;

  while ((_converted < ulen) && (count < size))
    {
      c = (SQLCHAR) *ustr;
      UTF8_COMPUTE (c, mask, len);
      if ((len == -1) || (_converted + len > ulen))
	{
	  if (converted)
	    *converted = _converted;
	  return count;
	}

      wc = c & mask;
      for (i = 1; i < len; i++)
	{
	  if ((ustr[i] & 0xC0) != 0x80)
	    {
	      if (converted)
		*converted = _converted;
	      return count;
	    }
	  wc <<= 6;
	  wc |= (ustr[i] & 0x3F);
	}
      *wstr = wc;
      ustr += len;
      wstr++;
      count++;
      _converted += len;
    }
  if (converted)
    *converted = _converted;
  return count;
}


static SQLWCHAR *
strdup_U8toW (SQLCHAR * str)
{
  SQLWCHAR *ret;
  int len;

  if (!str)
    return NULL;

  len = utf8_len (str, SQL_NTS);
  if ((ret = (SQLWCHAR *) malloc ((len + 1) * sizeof (SQLWCHAR))) == NULL)
    return NULL;

  len = utf8towcs ((char *)str, ret, len);
  ret[len] = L'\0';

  return ret;
}


SQLCHAR *
dm_SQL_WtoU8 (SQLWCHAR * inStr, int size)
{
  SQLCHAR *outStr = NULL;
  int len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
    {
      outStr = strdup_WtoU8 (inStr);
    }
  else
    {
      len = _calc_len_for_utf8 (CP_DEF, inStr, size);
      if ((outStr = (SQLCHAR *) malloc (len + 1)) != NULL)
	{
	  len = wcsntoutf8 (inStr, (char*)outStr, size, len, NULL);
	  outStr[len] = '\0';
	}
    }

  return outStr;
}


SQLWCHAR *
dm_SQL_U8toW (SQLCHAR * inStr, int size)
{
  SQLWCHAR *outStr = NULL;
  int len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
    {
      outStr = strdup_U8toW (inStr);
    }
  else
    {
      len = utf8_len (inStr, size);
      if ((outStr = (SQLWCHAR *) calloc (len + 1, sizeof (SQLWCHAR))) != NULL)
	utf8ntowcs (inStr, outStr, size, len, NULL);
    }

  return outStr;
}


int
dm_StrCopyOut2_U8toW (
  SQLCHAR	* inStr,
  SQLWCHAR	* outStr,
  int		  size,
  WORD		* result)
{
  int length;

  if (!inStr)
    return -1;

  length = utf8_len (inStr, SQL_NTS);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  if (size >= length + 1)
    {
      length = utf8towcs ((char *)inStr, outStr, size);
      outStr[length] = L'\0';
      return 0;
    }
  if (size > 0)
    {
      length = utf8towcs ((char *)inStr, outStr, size - 1);
      outStr[length] = L'\0';
    }
  return -1;
}


static size_t
_WCSLEN(IODBC_CHARSET charset, void *str)
{
  size_t len = 0;
  ucs4_t *u4str = (ucs4_t *)str;
  ucs2_t *u2str = (ucs2_t *)str;

  if (!str)
    return 0;

  switch(charset)
    {
    case CP_UTF8:
      return utf8_len((SQLCHAR *)str, SQL_NTS);
    case CP_UTF16:
      while (*u2str++ != 0)
        len++;
      break;
    case CP_UCS4:
      while (*u4str++ != 0)
        len++;
      break;
    }
  return len;
}



/*
 * ilen -  length of inStr in SQL Ind format
 * olen - size of outStr buffer in symbols
 * return length of copied data in symbols
 */
static size_t
dm_U2toU4(ucs2_t *inStr, int ilen, ucs4_t *outStr, int olen)
{
  size_t n = 0;
  char *us = (char *) inStr;
  char *us_end = (char *)(inStr + ilen);
  ucs4_t wc;
  int count = 0;

  while(n < ilen)
    {
#ifdef WORDS_BIGENDIAN
      wc = (ucs4_t) eh_decode_char__UTF16BE((__constcharptr *) &us, us_end);
#else
      wc = (ucs4_t) eh_decode_char__UTF16LE((__constcharptr *) &us, us_end);
#endif
      if (wc == UNICHAR_EOD || wc == UNICHAR_NO_DATA || wc == UNICHAR_BAD_ENCODING)
        break;

      if (count + 1 > olen)
        break;

      *outStr = wc;
      n++;
      outStr++;
      count ++;
    }
  return count;
}



static size_t
dm_U4toU2(ucs4_t *inStr, int ilen, ucs2_t *outStr, int olen)
{
  size_t n = 0;
  char *rc;
  char *us = (char*) outStr;
  char *us_end = (char*) (outStr + olen);

  while (n < ilen && us < us_end)
    {
#ifdef WORDS_BIGENDIAN
      rc = eh_encode_char__UTF16BE (*inStr, us, us_end);
#else
      rc = eh_encode_char__UTF16LE (*inStr, us, us_end);
#endif
      if ((char *)UNICHAR_NO_ROOM == rc)
        break;

      us = rc;
      n++;
      inStr++;

      if (!*inStr)
	break;
    }
  return (us - (char *)outStr) / sizeof(ucs2_t);
}



static size_t
dm_AtoU2(char *src, int ilen, ucs2_t *dest, int olen)
{
  size_t n = 0;
  wchar_t wc;
  char *rc;
  char *us = (char*) dest;
  char *us_end = (char*)(dest + olen);
  mbstate_t st;

  memset (&st, 0, sizeof (st));

  while (n < ilen && us < us_end)
    {
      size_t sz = mbrtowc (&wc, src, ilen - n, &st);

      if (((long) sz) > 0)
        {
          n += sz - 1;
          src += sz - 1;
        }
      else if (((long) sz) < 0)
        {
          wc = 0xFFFD;
        }

#ifdef WORDS_BIGENDIAN
      rc = eh_encode_char__UTF16BE (wc, us, us_end);
#else
      rc = eh_encode_char__UTF16LE (wc, us, us_end);
#endif
      if ((char *)UNICHAR_NO_ROOM == rc)
        break;

      us = rc;
      n++;
      if (!*src)
	break;
      src++;
    }
  return (us - (char *)dest) / sizeof(ucs2_t);
}


static size_t
dm_AtoU4(char *src, int ilen, ucs4_t *dest, size_t olen)
{
  size_t n = 0;
  wchar_t wc;
  ucs4_t *us = dest;
  int count = 0;
  mbstate_t st;
  
  memset (&st, 0, sizeof (st));

  while (n < ilen && count < olen)
    {
      size_t sz = mbrtowc (&wc, src, ilen - n, &st);

      if (((long) sz) > 0)
        {
          n += sz - 1;
          src += sz - 1;
        }
      else if (((long) sz) < 0)
        {
          wc = 0xFFFD;
        }

      *us = wc;
      n++;
      us++;
      count++;
      if (!*src)
	break;
      src++;
    }
  return count;
}


static size_t
dm_AtoUW(char *src, int ilen, wchar_t *dest, size_t olen)
{
  size_t n = 0;
  wchar_t wc;
  ucs4_t *us = dest;
  int count = 0;
  mbstate_t st;
  
  memset (&st, 0, sizeof (st));

  while (n < ilen && count < olen)
    {
      size_t sz = mbrtowc (&wc, src, ilen - n, &st);

      if (((long) sz) > 0)
        {
          n += sz - 1;
          src += sz - 1;
        }
      else if (((long) sz) < 0)
        {
          wc = 0xFFFD;
        }

      *us = wc;
      n++;
      us++;
      count++;
      if (!*src)
	break;
      src++;
    }
  return count;
}


static size_t
dm_U2toA(ucs2_t *src, int ilen, char *dest, int olen)
{
  size_t n = 0;
  char *us = (char *) src;
  char *us_end = (char*)(src + ilen);
  wchar_t wc;
  mbstate_t st;

  if (!*src)
    return 0;

  while (n < olen)
    {
      char temp[MB_CUR_MAX];
      size_t sz, sz_written = 0;
#ifdef WORDS_BIGENDIAN
      wc = eh_decode_char__UTF16BE((__constcharptr *) &us, us_end);
#else
      wc = eh_decode_char__UTF16LE((__constcharptr *) &us, us_end);
#endif
      if (wc == UNICHAR_EOD || wc == UNICHAR_NO_DATA || wc == UNICHAR_BAD_ENCODING)
        break;

      memset (&st, 0, sizeof (st));
  
      sz = wcrtomb (temp, wc, &st);
      if (((long) sz) > 0)
	{
	  if (sz > olen - n)
	    break;

	  memcpy (dest, temp, sz);
	  n += sz - 1;
	  dest += sz - 1;
	}
      else
	*dest = '?';

      n++;
      dest++;
    }
  return n;
}


static size_t
dm_U4toA(ucs4_t *src, int ilen, char *dest, int olen)
{
  int n = 0;
  wchar_t wc;
  int count = 0;
  mbstate_t st;

  if (!*src)
    return 0;

  while (n < ilen && count < olen)
    {
      char temp[MB_CUR_MAX];
      size_t sz, sz_written = 0;

      memset (&st, 0, sizeof (st));

      wc = *src;
      sz = wcrtomb (temp, wc, &st);

      if (((long) sz) > 0)
	{
	  if (sz > olen - count)
	    break;

	  memcpy (dest, temp, sz);
	  count += sz - 1;
	  dest += sz - 1;
	}
      else
	*dest = '?';

      src++;
      count++;
      n++;
      dest++;
    }
  return n;
}



static size_t
dm_UWtoA(wchar_t *src, int ilen, char *dest, int olen)
{
  int n = 0;
  wchar_t wc;
  int count = 0;
  mbstate_t st;

  if (!*src)
    return 0;

  while (n < ilen && count < olen)
    {
      char temp[MB_CUR_MAX];
      size_t sz, sz_written = 0;

      memset (&st, 0, sizeof (st));

      wc = *src;
      sz = wcrtomb (temp, wc, &st);

      if (((long) sz) > 0)
	{
	  if (sz > olen - count)
	    break;

	  memcpy (dest, temp, sz);
	  count += sz - 1;
	  dest += sz - 1;
	}
      else
	*dest = '?';

      src++;
      count++;
      n++;
      dest++;
    }
  return n;
}



/*
 * len -  length of inStr in SQL Ind format
 * size - size of outStr buffer in bytes
 * return length of copied data in bytes
 */
int
dm_conv_W2W(void *inStr, int len, void *outStr, int size,
	IODBC_CHARSET icharset, IODBC_CHARSET ocharset)
{
  int count = 0;
  int o_wchar_size = _WCHARSIZE(ocharset);

  size /= o_wchar_size;

  if (icharset == CP_UTF8)
    {
      if (len == SQL_NTS)
        len = strlen((char*)inStr);

      count = _utf8ntowcx(ocharset, (char*)inStr, outStr, len, size, NULL);
      return count * o_wchar_size;
    }
  else if (ocharset == CP_UTF8)
    {
      if (len == SQL_NTS)
        len = (icharset==CP_UTF8)?strlen((char*)inStr):_WCSLEN(icharset, inStr);

      return _wcxntoutf8(icharset, inStr, (char*)outStr, len, size, NULL);
    }
  else
    {
      if (len == SQL_NTS)
        len = _WCSLEN(icharset, inStr);

      if (icharset == CP_UTF16)
        {
          if (ocharset == CP_UCS4)
            {
              count = dm_U2toU4((ucs2_t *)inStr, len, (ucs4_t *)outStr, size);
              return count * o_wchar_size;
            }
          else
            {
              ucs2_t *u2i = (ucs2_t *) inStr;
              ucs2_t *u2o = (ucs2_t *) outStr;
              while(len > 0 && count < size)
                {
                  *u2o = *u2i;
                   u2o++;
                   u2i++;
                   len--;
                   count++;
                }
              return count * o_wchar_size;
            }
        }
      else /* CP_UCS4 */
        {
          if (ocharset == CP_UTF16)
            {
              count = dm_U4toU2((ucs4_t *)inStr, len, (ucs2_t *)outStr, size);
              return count * o_wchar_size;
            }
          else
            {
              ucs4_t *u4i = (ucs4_t *) inStr;
              ucs4_t *u4o = (ucs4_t *) outStr;
              while(len > 0 && count < size)
                {
                  *u4o = *u4i;
                   u4o++;
                   u4i++;
                   len--;
                   count++;
                }
              return count * o_wchar_size;
            }
        }
    }
}


/*
 * len -  length of inStr in SQL Ind format
 * size - size of buffer for output string in bytes
 * return length of copied data in bytes
 */
int
dm_conv_W2A(void *inStr, int inLen, char *outStr, int size,
	IODBC_CHARSET charset)
{
  SQLWCHAR wc;
  int count = 0;

  if (inLen == SQL_NTS)
    {
      inLen = (charset == CP_UTF8) ? strlen((char*)inStr)
                                   : _WCSLEN(charset, inStr);
    }

  if (size > 0)
    {
      if (charset == CP_UTF8)
        {
          SQLCHAR *u8 = (SQLCHAR *)inStr;
          SQLCHAR c;
          int len, mask, i;
          char temp[MB_CUR_MAX];
          mbstate_t st;
          size_t rc;

          while((c = *u8) && size > 0 && inLen > 0)
            {
              UTF8_COMPUTE (c, mask, len);
              if (len == -1)
                return count;

              wc = c & mask;
              for(i = 1; i < len; i++)
                {
                  if ((u8[i] & 0xC0) != 0x80)
                    return count;
                  wc <<= 6;
                  wc |= (u8[i] & 0x3F);
                }

              memset (&st, 0, sizeof (st));
              rc = wcrtomb (temp, wc, &st);
              if (((ssize_t)rc) > 0)
                {
                  rc = MIN(rc, MB_CUR_MAX);
                  if (rc > size)
                    break;

                  memcpy(outStr, temp, rc);
                  size -= rc -1;
                  outStr += rc - 1;
                  count += rc - 1;
                }
              else if (rc == 0)
                {
                  *outStr = 0;
                }
              else
                {
                  *outStr = '?';
                }

              size --;
              outStr ++;
              count ++;

              u8 += len;
              inLen -= len;
            }
        }
      else if (charset == CP_UTF16)
        {
          count = dm_U2toA((ucs2_t *)inStr, inLen, outStr, size);
        }
      else if (charset == CP_UCS4)
        {
          count = dm_U4toA((ucs4_t *)inStr, inLen, outStr, size);
        }
    }
  return count;
}


/*
 * len -  length of inStr in SQL Ind format
 * size - size of buffer for output string in bytes
 * return length of copied data in bytes
 */
int
dm_conv_A2W(char *inStr, int inLen, void *outStr, int size,
	IODBC_CHARSET charset)
{
  SQLWCHAR wc;
  int count = 0;
  int o_wchar_size = _WCHARSIZE(charset);

  if (inLen == SQL_NTS)
    inLen = strlen(inStr);

  if (size > 0)
    {
      if (charset == CP_UTF8)
        {
          SQLCHAR *u8 = (SQLCHAR*)outStr;
          int len,first,i;
          mbstate_t st;

          memset (&st, 0, sizeof (st));
          
          while(*inStr && size>0 && inLen > 0)
            {
              size_t rc;
              rc = mbrtowc (&wc, inStr, (size_t)inLen, &st);
              if (((ssize_t)rc) > 0)
                {
                  inLen -= rc - 1;
                  inStr += rc - 1;
                }
              else if (((ssize_t)rc) < 0)
                {
                  wc = 0xFFFD;
                }

              CONV_TO_UTF8(wc, len, first);
              for(i = len-1; i > 0; --i)
                {
                  u8[i] = (wc & 0x3F) | 0x80;
                  wc >>= 6;
                }
              u8[0] = wc | first;
              u8 += len;
              size -= len;
              count += len;
              inStr++;
              inLen--;
            }
        }
      else if (charset == CP_UTF16)
        {
          count = dm_AtoU2(inStr, inLen, (ucs2_t *)outStr, size/o_wchar_size);
          count *= o_wchar_size;
        }
      else if (charset == CP_UCS4)
        {
          count = dm_AtoU4(inStr, inLen, (ucs4_t *)outStr, size/o_wchar_size);
          count *= o_wchar_size;
        }
    }
  return count;
}


static void
_SetWCharAt(IODBC_CHARSET charset, void *str, int pos, int ch)
{
  ucs4_t *u4 = (ucs4_t *)str;
  ucs2_t *u2 = (ucs2_t *)str;

  if (!str)
    return;

  switch(charset)
    {
    case CP_UTF8:
      {
        int i=0;
        SQLCHAR *u8str = (SQLCHAR*)str;
        while(i < pos)
        {
          int mask, len;
          UTF8_COMPUTE(*u8str, mask, len);
          if (len == -1)
            break;
          u8str += len;
          i++;
        }
        *u8str = (SQLCHAR)ch;
      }
      break;
    case CP_UTF16: u2[pos] = (ucs4_t)ch; break;
    case CP_UCS4: u4[pos] = (ucs4_t)ch; break;
    }
}


void
DM_strcpy_U8toW (DM_CONV *conv, void *dest, SQLCHAR *sour)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  int len;

  if (!sour)
    return;

  if (charset == CP_UTF16 || charset == CP_UCS4)
    {
      len = utf8_len(sour, SQL_NTS) * _WCHARSIZE(charset);
      _utf8towcx(charset, (char *)sour, dest, len);
    }
  else
    strcpy(dest, (char *)sour);
}


size_t
DRV_WCHARSIZE(DM_CONV *conv)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return _WCHARSIZE(charset);
}


size_t
DM_WCHARSIZE(DM_CONV *conv)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return _WCHARSIZE(charset);
}


size_t
DRV_WCHARSIZE_ALLOC(DM_CONV *conv)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return _WCHARSIZE_ALLOC(charset);
}


size_t
DM_WCHARSIZE_ALLOC(DM_CONV *conv)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return _WCHARSIZE_ALLOC(charset);
}

void *
DM_A2W(DM_CONV *conv, SQLCHAR * inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  SQLWCHAR *outStr = NULL;
  ssize_t len;

  if (size == SQL_NTS)
    len = strlen((char *) inStr);
  else
    len = size;

  if (len < 0)
    return NULL;

  outStr = (SQLWCHAR *) calloc(len + 1, DM_WCHARSIZE_ALLOC(conv));
  if (!outStr)
    return NULL;

  dm_conv_A2W((char *)inStr, size, outStr, len*DM_WCHARSIZE_ALLOC(conv), charset);
  return outStr;
}


static SQLCHAR *
__W2A(IODBC_CHARSET charset, void * inStr, int size)
{
  SQLCHAR *outStr = NULL;
  ssize_t len;

  if (size == SQL_NTS)
    len = _WCSLEN(charset, inStr);
  else
    len = size;

  if (len < 0)
    return NULL;

  outStr = (SQLCHAR *) calloc (len * MB_CUR_MAX + 1, 1);
  if (!outStr)
    return NULL;

  dm_conv_W2A(inStr, size, (char *)outStr, len, charset);
  return outStr;
}

SQLCHAR *
DM_W2A(DM_CONV *conv, void * inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return __W2A(charset, inStr, size);
}

SQLCHAR *
DRV_W2A(DM_CONV *conv, void * inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return __W2A(charset, inStr, size);
}

void
DM_SetWCharAt(DM_CONV *conv, void *str, int pos, int ch)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  _SetWCharAt(charset, str, pos, ch);
}


void
DRV_SetWCharAt(DM_CONV *conv, void *str, int pos, int ch)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  _SetWCharAt(charset, str, pos, ch);
}


SQLWCHAR
DM_GetWCharAt(DM_CONV *conv, void *str, int pos)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  ucs4_t *u4 = (ucs4_t *)str;
  ucs2_t *u2 = (ucs2_t *)str;

  if (!str)
    return 0;

  switch(charset)
    {
    case CP_UTF8:
      {
        int mask, len, i=0;
        SQLWCHAR wc = 0;
        SQLCHAR *u8str = (SQLCHAR*)str;
        while(i < pos)
        {
          UTF8_COMPUTE(*u8str, mask, len);
          if (len == -1)
            break;
          u8str += len;
          i++;
        }
        UTF8_COMPUTE(*u8str, mask, len);
        wc = (*u8str)&mask;
        for(i=1; i < len; i++)
          {
            if ((u8str[i] & 0xC0) != 0x80)
              return 0;
            wc <<= 6;
            wc |= (u8str[i] & 0x3F);
          }
        return wc;
      }
    case CP_UTF16: return (SQLWCHAR)u2[pos];
    case CP_UCS4:
    default:
    	return (SQLWCHAR)u4[pos];
    }
}


static void *
_WCSCPY(IODBC_CHARSET charset, void *dest, void *sour)
{
  ucs4_t *u4dst = (ucs4_t *)dest;
  ucs2_t *u2dst = (ucs2_t *)dest;
  ucs4_t *u4src = (ucs4_t *)sour;
  ucs2_t *u2src = (ucs2_t *)sour;

  switch(charset)
    {
    case CP_UTF8:
      strcpy((char*)dest, (char*)sour);
      break;
    case CP_UTF16:
      while ((*u2dst++ = *u2src++) != 0)
        ;
      *u2dst = 0;
      break;
    case CP_UCS4:
      while ((*u4dst++ = *u4src++) != 0)
        ;
      *u4dst = 0;
      break;
    }
  return dest;
}


static void *
_WCSNCPY(IODBC_CHARSET charset, void *dest, void *sour, size_t count)
{
  size_t len = 0;
  ucs4_t *u4dst = (ucs4_t *)dest;
  ucs2_t *u2dst = (ucs2_t *)dest;
  ucs4_t *u4src = (ucs4_t *)sour;
  ucs2_t *u2src = (ucs2_t *)sour;

  switch(charset)
    {
    case CP_UTF8:
      strncpy((char*)dest, (char*)sour, count);
      break;
    case CP_UTF16:
      while (len < count && *u2src != 0)
        {
          *u2dst++ = *u2src++;
          len++;
        }

      if (len < count)
        *u2dst = 0;
      break;
    case CP_UCS4:
      while (len < count && *u4src != 0) 
        {
          *u4dst++ = *u4src++;
          len++;
        }

      if (len < count)
        *u4dst = 0;
      break;
    }
  return dest;
}


void *
DM_WCSCPY(DM_CONV *conv, void *dest, void *sour)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return _WCSCPY(charset, dest, sour);
}


void *
DM_WCSNCPY(DM_CONV *conv, void *dest, void *sour, size_t count)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return _WCSNCPY(charset, dest, sour, count);
}


void *
DRV_WCSNCPY(DM_CONV *conv, void *dest, void *sour, size_t count)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return _WCSNCPY(charset, dest, sour, count);
}


size_t
DM_WCSLEN(DM_CONV *conv, void *str)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return _WCSLEN(charset, str);
}


size_t
DRV_WCSLEN(DM_CONV *conv, void *str)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return _WCSLEN(charset, str);
}


static SQLCHAR *
__WtoU8(IODBC_CHARSET charset, void *inStr, int size)
{
  SQLCHAR *outStr = NULL;
  int len;

  if (inStr == NULL)
    return NULL;

  len = _calc_len_for_utf8(charset, inStr, size);
  if (!(outStr = (SQLCHAR *) calloc (len + 1, sizeof(char))))
    return NULL;

  if (size == SQL_NTS)
    _wcxtoutf8 (charset, inStr, (char *)outStr, len);
  else
    _wcxntoutf8 (charset, inStr, (char *)outStr, size, len, NULL);

  return outStr;
}


SQLCHAR *
DM_WtoU8(DM_CONV *conv, void *inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  return __WtoU8(charset, inStr, size);
}


SQLCHAR *
DRV_WtoU8(DM_CONV *conv, void *inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->drv_cp : CP_DEF;
  return __WtoU8(charset, inStr, size);
}


void *
DM_U8toW(DM_CONV *conv, SQLCHAR *inStr, int size)
{
  IODBC_CHARSET charset = (conv) ? conv->dm_cp : CP_DEF;
  void *outStr = NULL;
  int  len = 0;

  if (inStr == NULL)
    return NULL;

  len = utf8_len (inStr, size);
  outStr = (void *) calloc (len + 1, _WCHARSIZE_ALLOC(charset));

  if (size == SQL_NTS)
    _utf8towcx(charset, (char *)inStr, outStr, len);
  else
    _utf8ntowcx(charset, (char *)inStr, outStr, size, len, NULL);

  return outStr;
}


/*  drv => dm
 *
 * size - size of outStr in bytes
 * result - length in symbols
 * copied - count of copied in bytes
 */
int
dm_StrCopyOut2_A2W_d2m (DM_CONV *conv, SQLCHAR *inStr,
		void *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET o_charset = (conv) ? conv->dm_cp : CP_DEF;
  int length, count;
  int ret = 0;

  if (!inStr)
    return -1;

  length = strlen ((char *) inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size -= DM_WCHARSIZE(conv);

  if (size <= 0)
    return -1;

  count = dm_conv_A2W((char *)inStr, SQL_NTS, outStr, size, o_charset);

  if (o_charset == CP_UTF16 || o_charset == CP_UCS4)
    _SetWCharAt(o_charset, outStr, count/_WCHARSIZE(o_charset), 0);
  else
    *(char*)(outStr + count) = 0;

  if (_WCSLEN(o_charset, outStr) < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}


/* drv => dm */
int
dm_StrCopyOut2_W2A_d2m (DM_CONV *conv, void *inStr,
		SQLCHAR *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET i_charset = (conv) ? conv->drv_cp : CP_DEF;
  int length, count;
  int ret = 0;

  if (!inStr)
    return -1;

  length = _WCSLEN(i_charset, inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size--;

  if (size < 0)
    return -1;

  count = dm_conv_W2A(inStr, SQL_NTS, (char *)outStr, size, i_charset);
  outStr[count] = '\0';

  if (count < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}


/* dm => drv */
int
dm_StrCopyOut2_W2A_m2d (DM_CONV *conv, void *inStr,
		SQLCHAR *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET i_charset = (conv) ? conv->dm_cp : CP_DEF;
  int length, count;
  int ret = 0;

  if (!inStr)
    return -1;

  length = _WCSLEN(i_charset, inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size--;

  if (size < 0)
    return -1;

  count = dm_conv_W2A(inStr, SQL_NTS, (char *)outStr, size, i_charset);
  outStr[count] = '\0';

  if (count < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}


/* drv => dm
 *
 * size - size of outStr in bytes
 * result - length in symbols
 * copied - count of copied in bytes
*/
int
dm_StrCopyOut2_U8toW_d2m (DM_CONV *conv, SQLCHAR *inStr,
		void *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET o_charset = (conv) ? conv->dm_cp : CP_DEF;
  int length;
  int ret = 0;
  int count = 0;

  if (!inStr)
    return -1;

  length = utf8_len ((SQLCHAR *) inStr, SQL_NTS);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size -= _WCHARSIZE(o_charset);

  if (size < 0)
    return -1;

  count = dm_conv_W2W(inStr, SQL_NTS, outStr, size, CP_UTF8, o_charset);

  if (o_charset == CP_UTF16 || o_charset == CP_UCS4)
    _SetWCharAt(o_charset, outStr, count/_WCHARSIZE(o_charset), 0);
  else
    *(char*)(outStr + count) = 0;

  if (_WCSLEN(o_charset, outStr) < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}



/* drv => dm
 *
 * size - size of outStr in bytes
 * result - length in symbols
 * copied - count of copied in bytes
*/
int
dm_StrCopyOut2_W2W_d2m (DM_CONV *conv, void *inStr,
		void *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET o_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET i_charset = (conv) ? conv->drv_cp : CP_DEF;
  int length;
  int ret = 0;
  int count = 0;

  if (!inStr)
    return -1;

  length = _WCSLEN(i_charset, inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size -= _WCHARSIZE(o_charset);

  if (size <= 0)
    return -1;

  count = dm_conv_W2W(inStr, SQL_NTS, outStr, size, i_charset, o_charset);

  if (o_charset == CP_UTF16 || o_charset == CP_UCS4)
    _SetWCharAt(o_charset, outStr, count/_WCHARSIZE(o_charset), 0);
  else
    *(char*)(outStr + count) = 0;

  if (_WCSLEN(o_charset, outStr) < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}


/* dm => drv
 *
 * size - size of outStr in bytes
 * result - length in symbols
 * copied - count of copied in bytes
*/
int
dm_StrCopyOut2_W2W_m2d (DM_CONV *conv, void *inStr,
		void *outStr, int size, SQLSMALLINT *result, int *copied)
{
  IODBC_CHARSET o_charset = (conv) ? conv->drv_cp : CP_DEF;
  IODBC_CHARSET i_charset = (conv) ? conv->dm_cp : CP_DEF;
  int length;
  int ret = 0;
  int count = 0;

  if (!inStr)
    return -1;

  length = _WCSLEN(i_charset, inStr);

  if (result)
    *result = (SQLSMALLINT) length;

  if (!outStr)
    return 0;

  size -= _WCHARSIZE(o_charset);

  if (size <= 0)
    return -1;

  count = dm_conv_W2W(inStr, SQL_NTS, outStr, size, i_charset, o_charset);

  if (o_charset == CP_UTF16 || o_charset == CP_UCS4)
    _SetWCharAt(o_charset, outStr, count/_WCHARSIZE(o_charset), 0);
  else
    *(char*)(outStr + count) = 0;

  if (_WCSLEN(o_charset, outStr) < length)
    ret = -1;

  if (copied)
    *copied = count;

  return ret;
}


/* drv => dm */
void *
conv_text_d2m(DM_CONV *conv, void *inStr, int size, CONV_DIRECT direct)
{
  IODBC_CHARSET m_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET d_charset = (conv) ? conv->drv_cp : CP_DEF;
  void *outStr = NULL;
  int len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
    {
      if (direct == CD_W2A || direct == CD_W2W)
        len = DRV_WCSLEN(conv, inStr);
      else
        len = strlen ((char *)inStr);
    }
  else
    len = size;

  if (len < 0)
    return NULL;

  if (direct == CD_W2A)
    outStr = calloc (len * MB_CUR_MAX + 1, 1);
  else
    outStr = calloc(len + 1, DM_WCHARSIZE_ALLOC(conv));

  if (outStr)
    {
      if (direct == CD_A2W)
        {
          dm_conv_A2W((char *)inStr, size, outStr,
		len*DM_WCHARSIZE_ALLOC(conv),m_charset);
        }
      else if (direct == CD_W2A)
        {
          dm_conv_W2A(inStr, size, (char *)outStr, len, d_charset);
        }
      else /* CD_W2W */
        {
          dm_conv_W2W(inStr, size, outStr, len*DM_WCHARSIZE_ALLOC(conv),
		d_charset, m_charset);
        }
    }

  return outStr;
}



/* dm => drv */
void *
conv_text_m2d(DM_CONV *conv, void *inStr, int size, CONV_DIRECT direct)
{
  IODBC_CHARSET m_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET d_charset = (conv) ? conv->drv_cp : CP_DEF;
  void *outStr = NULL;
  int len;

  if (inStr == NULL)
    return NULL;

  if (size == SQL_NTS)
    {
      if (direct == CD_W2A || direct == CD_W2W)
        len = DM_WCSLEN(conv, inStr);
      else
        len = strlen ((char *)inStr);
    }
  else
    len = size;

  if (len < 0)
    return NULL;

  if (direct == CD_W2A)
    outStr = calloc (len * MB_CUR_MAX + 1, 1);
  else
    outStr = calloc(len + 1, DRV_WCHARSIZE_ALLOC(conv));

  if (outStr)
    {
      if (direct == CD_A2W)
        {
          dm_conv_A2W((char *)inStr, size, outStr,
		len*DRV_WCHARSIZE_ALLOC(conv), d_charset);
        }
      else if (direct == CD_W2A)
        {
          dm_conv_W2A(inStr, size, (char *)outStr, len, m_charset);
        }
      else /* CD_W2W */
        {
          dm_conv_W2W(inStr, size, outStr, len*DRV_WCHARSIZE_ALLOC(conv),
			m_charset, d_charset);
        }
    }

  return outStr;
}


void *
conv_text_m2d_W2W(DM_CONV *conv, void *inStr, SQLLEN size, SQLLEN *copied)
{
  IODBC_CHARSET m_charset = (conv) ? conv->dm_cp : CP_DEF;
  IODBC_CHARSET d_charset = (conv) ? conv->drv_cp : CP_DEF;
  void *outStr = NULL;
  int len;
  int rc;

  if (inStr == NULL)
    return NULL;

  len = size / DM_WCHARSIZE(conv);

  if (len < 0)
    return NULL;

  outStr = calloc(len + 1, DRV_WCHARSIZE_ALLOC(conv));

  if (outStr)
    {
      rc = dm_conv_W2W(inStr, len, outStr, len*DRV_WCHARSIZE_ALLOC(conv),
		m_charset, d_charset);
      if (copied)
        *copied = rc;
    }

  return outStr;
}
