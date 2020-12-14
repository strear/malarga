#ifndef RNCRTDEF_H
#define RNCRTDEF_H

#include <algorithm>
#include <climits>
#include <cstring>
#include <ctime>

//#define RN_NO_CRT_CHECKS

#ifndef _MSC_VER

#define RN_NO_CRT_CHECKS
#define localtime_s(tm, t) localtime_r(t, tm)
#define strtok_s(str, delim, env) strtok_r(str, delim, env)
#define wcstok_s(str, delim, env) wcstok(str, delim, env)

#endif

#ifndef RN_NO_CRT_CHECKS

#define mbstowcs(wcs, mbs, n) mbstowcs_s(nullptr, wcs, sizeof(wcs) / sizeof(*wcs), mbs, n) 
#define wcsncpy(dst, src, c) wcsncpy_s(dst, sizeof(dst) / sizeof(*dst), src, c)
#define snprintf(s, c, format, ...) _snprintf_s(s, c, c-1, format, __VA_ARGS__)
#define swprintf(s, c, format, ...) swprintf_s(s, c, format, __VA_ARGS__)
#define wcscpy(dst, src) wcscpy_s(dst, sizeof(dst) / sizeof(*dst), src)
#define strcat(dst, src) strcat_s(dst, sizeof(dst) / sizeof(*dst), src)
#define wcscat(dst, src) wcscat_s(dst, sizeof(dst) / sizeof(*dst), src)

#else

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#define _snprintf_s(s, n, c, format, ...) snprintf(s, n-1, format, __VA_ARGS__)
#define mbstowcs_s(r, wcs, c, mbs, n) mbstowcs(wcs, mbs, n) 
#define wcsncpy_s(dst, n, src, c) wcsncpy(dst, src, c)
#define wcscpy_s(dst, s, src) wcscpy(dst, src)
#define wcscat_s(dst, n, src) wcscat(dst, src)

#endif

#endif // RNCRTDEF_H