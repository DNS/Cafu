/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************************************************/
/*** Small and Portable Utility Library (Header) ***/
/***************************************************/

#ifndef CAFU_UTIL_HPP_INCLUDED
#define CAFU_UTIL_HPP_INCLUDED

#ifndef _WIN32
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _getch getchar
typedef long long int __int64;
#endif


/// A platform independent timer class that allows to measure the time passed since
/// its construction or the last measuring point.
class TimerT
{
    private:

    double  Frequency;
    __int64 TimeStamp1st;
    __int64 TimeStampOld;


    public:

    /// The constructor. It initializes the timer.
    TimerT();

    /// This function returns the time elapsed since the function was last called.
    /// The return value is the elapsed time in seconds.
    /// The very first call of this function returns always 0.
    double GetSecondsSinceLastCall();

    /// This function returns the time elapsed since the timer was constructed.
    /// The return value is the elapsed time in seconds.
    double GetSecondsSinceCtor() const;
};


// Given a string of the form "path/filename.extension" (where individual parts may be missing),
// this functions strips off the file extension, including the '.' character.
// This is done by inserting a '\0' character in the right place.
// void StripExt(char* PathFileExt);

// Given a string of the form "path/filename.extension" (where individual parts may be missing),
// this functions strips off the file name, the file extension, and the possibly preceding '/' or '\\' character.
// This is done by inserting a '\0' character in the right place.
// WARNING: This is pretty problematic when no '/' or '\\' was given!
// void StripFileNameAndExt(char* PathFileExt);

#endif
