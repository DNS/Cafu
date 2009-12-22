// This is our Cafu-specific config file that has been created manually from a hand-mix of
// mpg123/ports/MSVC++\config.h and mpg123/src/config.h.in


// ************************************************************
// First the statements that are universally true
// (all supported platforms: Windows and Linux, 32 and 64 bit).
// ************************************************************

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1


/* Define if frame index should be used. */
#define FRAME_INDEX 1

/* Define if gapless playback is enabled. */
#define GAPLESS 1

/* Size of the frame index seek table. */
#define INDEX_SIZE 1000

/* Use float as the type for real numbers (also set at our command-line). */
#define REAL_IS_FLOAT


// ************************************************************
// Now the platform-specific refinements.
// ************************************************************

#ifdef _WIN32
#define WIN32
#define strcasecmp _strcmpi
#define strncasecmp _strnicmp
#define strdup _strdup
#else
#define HAVE_STDINT_H 1
#endif
