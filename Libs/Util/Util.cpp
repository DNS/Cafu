/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************************************************/
/*** Small and Portable Utility Library (Code) ***/
/*************************************************/

#include "Util.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif


TimerT::TimerT()
{
#ifdef _WIN32
    __int64 Frequency64;

    QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency64 );
    QueryPerformanceCounter  ((LARGE_INTEGER*)&TimeStamp1st);

    Frequency   =double(Frequency64);
    TimeStampOld=TimeStamp1st;
#else
    timeval tv;

    gettimeofday(&tv, 0);

    Frequency   =1000000.0;
    TimeStamp1st=(__int64)(tv.tv_sec)*(__int64)(1000000) + (__int64)(tv.tv_usec);
    TimeStampOld=TimeStamp1st;
#endif
}


double TimerT::GetSecondsSinceLastCall()
{
    __int64 TimeStampNew;
    __int64 Diff;

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&TimeStampNew); // Get the new timestamp.

    Diff        =TimeStampNew-TimeStampOld;                 // Calculate the difference between the old and the new timestamp.
    TimeStampOld=TimeStampNew;                              // The new timestamp becomes the old timestamp.

    return double(Diff)/Frequency;                          // Return the difference in seconds.
#else
    timeval tv;

    gettimeofday(&tv, 0);                                   // Get the new timestamp.

    TimeStampNew=(__int64)(tv.tv_sec)*(__int64)(1000000) + (__int64)(tv.tv_usec);
    Diff        =TimeStampNew-TimeStampOld;                 // Calculate the difference between the old and the new timestamp.
    TimeStampOld=TimeStampNew;                              // The new timestamp becomes the old timestamp.

    return double(Diff)/Frequency;                          // Return the difference in seconds.
#endif
}


double TimerT::GetSecondsSinceCtor() const
{
    __int64 TimeStamp;

#ifdef _WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&TimeStamp);
#else
    timeval tv;

    gettimeofday(&tv, 0);
    TimeStamp=(__int64)(tv.tv_sec)*(__int64)(1000000) + (__int64)(tv.tv_usec);
#endif

    return double(TimeStamp-TimeStamp1st)/Frequency;
}


/* void StripExt(char* PathFileExt)
{
    int i=strlen(PathFileExt)-1;

    while (i>=0)
    {
        if (PathFileExt[i]=='.'                        ) { PathFileExt[i]=0; break; }
        if (PathFileExt[i]=='/' || PathFileExt[i]=='\\') break;
        i--;
    }
}


void StripFileNameAndExt(char* PathFileExt)
{
    int i=strlen(PathFileExt)-1;

    while (i>=0)
    {
        if (PathFileExt[i]=='/' || PathFileExt[i]=='\\') { PathFileExt[i]=0; break; }
        i--;
    }

    if (i<0) PathFileExt[0]=0;  // If no path was given (and thus no '/' or '\\' to overwrite), do the best we can do.
} */
