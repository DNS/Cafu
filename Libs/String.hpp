/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_STRING_HPP_INCLUDED
#define CAFU_STRING_HPP_INCLUDED

#include <string>


namespace cf
{
    /// The String namespace gathers auxiliary string functions that are not found in that standard library.
    namespace String
    {
        inline bool startsWith(const std::string& s, const std::string& prefix)
        {
            return s.compare(0, prefix.length(), prefix) == 0;
        }

        /// Returns whether the given string \c String ends with the given \c Suffix.
        inline bool EndsWith(const std::string& String, const std::string& Suffix)
        {
            const size_t StringLength=String.length();
            const size_t SuffixLength=Suffix.length();

            if (StringLength<SuffixLength) return false;

            std::string StringSuffix=&(String.c_str()[StringLength-SuffixLength]);

            return StringSuffix==Suffix;
        }

        /// Assumes that the given string \c s is a filename, removes the extension, if any, and returns the rest.
        inline std::string StripExt(std::string s)
        {
            const size_t PosDot=s.find_last_of('.');
            const size_t PosSep=s.find_last_of("/\\");

            if (PosDot==std::string::npos) return s;                    // "." not found in s?
            if (PosSep!=std::string::npos && PosDot<PosSep) return s;   // Last "." found before last "/"?

            s.erase(PosDot);
            return s;
        }

        /// Assumes that the given string \c s is a filename of pattern "path/filename.ext" and returns the path portion.
        inline std::string GetPath(std::string s)
        {
            const size_t PosSep=s.find_last_of("/\\");

            if (PosSep==std::string::npos) return "";   // "/" not found in s?

            s.erase(PosSep);
            return s;
        }

        /// Replaces in \c s all occurrences of \c search by \c replace, and returns the new string.
        inline std::string Replace(std::string s, const std::string& search, const std::string& replace)
        {
            const size_t len_search =search.length(); if (len_search==0) return s;
            const size_t len_replace=replace.length();

            size_t pos=s.find(search);

            while (pos!=std::string::npos)
            {
                s.replace(pos, len_search, replace);
                pos=s.find(search, pos+len_replace);
            }

            return s;
        }

        /// Modifies the given string `s` as necessary to turn it into a valid Lua 5.2 identifier and returns the result.
        inline std::string ToLuaIdentifier(std::string s)
        {
            // Must not be empty.
            if (s == "")
                return "var";

            // Must not start with a digit.
            if (isdigit(s[0]))
                s = "var_" + s;

            // Must only consist of locale-independent letters, digits and underscores.
            for (size_t i = 0; i < s.length(); i++)
            {
                if (isalnum(s[i]) && s[i] < 128) continue;
                if (s[i] == '_') continue;

                s[i] = '_';
            }

            // Must not be one of the reserved keywords.
            const char* Keywords[] =
            {
                "and", "break", "do", "else", "elseif", "end",
                "false", "for", "function", "goto", "if", "in",
                "local", "nil", "not", "or", "repeat", "return",
                "then", "true", "until", "while", NULL
            };

            for (unsigned int i = 0; Keywords[i]; i++)
                if (s == Keywords[i])
                {
                    s = "var_" + s;
                    break;
                }

            // Must not be a variable name that is reserved for Lua.
            if (s[0] == '_')
            {
                size_t i;

                for (i = 1; i < s.length(); i++)
                    if (!isupper(s[i]))
                        break;

                if (i >= s.length())
                    s = "var" + s;
            }

            return s;
        }
    }
}

#endif
