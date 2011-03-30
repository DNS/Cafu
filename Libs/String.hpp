/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#ifndef _CF_STRING_HPP_
#define _CF_STRING_HPP_

#include <string>


namespace cf
{
    /// The String namespace gathers auxiliary string functions that are not found in that standard library.
    namespace String
    {
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
    }
}

#endif
