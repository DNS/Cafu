/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILE_MEMORY_HPP_INCLUDED
#define CAFU_FILESYS_FILE_MEMORY_HPP_INCLUDED

#include "File.hpp"
#include "Templates/Array.hpp"


namespace cf
{
    namespace FileSys
    {
        class MemoryInFileT : public InFileI
        {
            public:

            /// Constructor. Creates an empty memory file for reading.
            /// Note that because the file is empty, the code that created the file should use the GetBuffer() method to set the initial contents.
            MemoryInFileT(const std::string& FileName_);

            /// Returns a reference to the buffer that keeps the contents of this file.
            ArrayT<char>& GetBuffer() { return Buffer; }

            /// Returns a const reference to the buffer that keeps the contents of this file.
            const ArrayT<char>& GetBuffer() const { return Buffer; }


            // Implement all the (pure) virtual methods of the FileI and InFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            uint32_t           Read(char* Buffer, uint32_t Size);
            uint64_t           GetSize() const;


            private:

            std::string   FileName;     ///< The name of the file.
            ArrayT<char>  Buffer;       ///< The contents of the file.
            unsigned long ReadPos;      ///< The position from which the next read operation starts.
        };


        /* class MemoryOutFileT : public OutFileI
        {
            public:

            /// Constructor.
            MemoryOutFileT(const std::string& FileName_);

            // Implement all the (pure) virtual methods of the FileI and OutFileI interfaces.
            bool               IsOpen() const;
            const std::string& GetBaseName() const;
            const std::string& GetFullName() const;
            uint64_t           GetPos() const;
            bool               Seek(int32_t Offset, SeekFromT SeekFrom);
            void               Write(const char* Buffer, uint32_t Size);


            private:

            // ...
        }; */
    }
}

#endif
