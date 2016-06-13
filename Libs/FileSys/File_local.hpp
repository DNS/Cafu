/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILE_LOCAL_HPP_INCLUDED
#define CAFU_FILESYS_FILE_LOCAL_HPP_INCLUDED

#include <fstream>

#include "File.hpp"


namespace cf
{
    namespace FileSys
    {
        class LocalInFileT : public InFileI
        {
            public:

            /// Constructor.
            LocalInFileT(const std::string& BaseName_, const std::string& FullName_);

            // Implement all the (pure) virtual methods of the FileI and InFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            uint32_t           Read(char* Buffer, uint32_t Size);
            uint64_t           GetSize() const;


            private:

            std::string           BaseName;     ///< The relative name of the file, agnostic of the local path.
            std::string           FullName;     ///< The BaseName prepended by the local path.
            mutable std::ifstream ifs;          ///< The input file stream.
        };


        class LocalOutFileT : public OutFileI
        {
            public:

            /// Constructor.
            LocalOutFileT(const std::string& BaseName_, const std::string& FullName_);

            // Implement all the (pure) virtual methods of the FileI and OutFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            void               Write(const char* Buffer, uint32_t Size);


            private:

            std::string           BaseName;     ///< The relative name of the file, agnostic of the local path.
            std::string           FullName;     ///< The BaseName prepended by the local path.
            mutable std::ofstream ofs;          ///< The output file stream.
        };
    }
}

#endif
