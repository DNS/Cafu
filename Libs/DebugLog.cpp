/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DebugLog.hpp"

#include <fstream>
#include <map>
#include <stdarg.h>
#include <string>


class LogManagerT
{
    public:

    LogManagerT()
        : m_MapCatToStream()
    {
    }

    ~LogManagerT()
    {
        // Properly close all open log files.
        for (std::map<std::string, std::ofstream*>::const_iterator It=m_MapCatToStream.begin(); It!=m_MapCatToStream.end(); ++It)
        {
            delete It->second;
        }

        m_MapCatToStream.clear();
    }

    std::ofstream& GetLogStream(const std::string& Category)
    {
        std::map<std::string, std::ofstream*>::const_iterator It=m_MapCatToStream.find(Category);

        if (It!=m_MapCatToStream.end()) return *(It->second);

        std::ofstream* NewStream=new std::ofstream((Category+".log").c_str());
        m_MapCatToStream[Category]=NewStream;
        return *NewStream;
    }


    private:

    LogManagerT(const LogManagerT&);            ///< Use of the Copy Constructor    is not allowed.
    void operator = (const LogManagerT&);       ///< Use of the Assignment Operator is not allowed.

    std::map<std::string, std::ofstream*> m_MapCatToStream;
};


static LogManagerT LogMan;


void cf::LogDebugBasic(const char* Category, const char* FileName, const int LineNr, const char* FormatString, ...)
{
    // Implement arbitrary log filters here.
    // if (std::string(Category)!="net") return;

    std::ofstream& OutStream=LogMan.GetLogStream(Category);

    if (OutStream.bad()) return;
    if (!FormatString) return;

    va_list ArgList;
    char    Buffer[1024];

    va_start(ArgList, FormatString);
        vsnprintf(Buffer, 1024-1, FormatString, ArgList);
    va_end(ArgList);
    Buffer[1024-1]=0;

    // TODO: Use e.g.   sprintf(Buffer2, "%.30s(%4i): ", FileName, LineNr);   for fixed-width formatting the beginning of the line...
 // OutStream << current_time;
    OutStream << FileName;
    OutStream << "(" << LineNr << "): ";
    OutStream << Buffer;
    OutStream << "\n";

    // Make sure that the whole output is in the file, even after program interruptions/crashes/asserts.
    OutStream.flush();
}
