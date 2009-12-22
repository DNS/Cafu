#ifndef MPG123_CAFU_H
#define MPG123_CAFU_H

#include <stdlib.h>
#include <sys/types.h>

#ifdef _WIN32
typedef long ssize_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
#endif

#define MPG123_NO_CONFIGURE
#include "mpg123.h.in" /* Yes, .h.in; we include the configure template! */

// #ifdef __cplusplus
// extern "C" {
// #endif
//
// 	// Wrapper around mpg123_open that supports path names with unicode characters.
// 	EXPORT int mpg123_topen(mpg123_handle *fr, const _TCHAR *path);
// 	EXPORT int mpg123_tclose(mpg123_handle *fr);
//
// #ifdef __cplusplus
// }
// #endif

#endif
