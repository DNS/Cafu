/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_FILESYS_FILESYSTEM_HPP_INCLUDED
#define CAFU_FILESYS_FILESYSTEM_HPP_INCLUDED

#include <string>


namespace cf
{
    namespace FileSys
    {
        class InFileI;


        /// A class for exceptions of the FileSystemT class.
        /// This is mostly used when new FileSystemT objects are to be created, but the creation is not possible,
        /// for example when a zip archive file cannot be opened, a local path doesn't exist, etc.
        class FileSystemExceptionT : public std::exception
        {
            public:

            enum ReasonT
            {
                Generic,
                InitFailure
            };


            /// The constructor.
            FileSystemExceptionT(ReasonT R=Generic) : Reason(R) { }

            /// The reason for the exception.
            ReasonT Reason;
        };


        class FileSystemT
        {
            public:

            /// The virtual destructor, so that derived classes can safely be deleted via a FileSystemT (base class) pointer.
            virtual ~FileSystemT() { }

            /// Opens the file with the given name for reading.
            /// @param FileName   The name of the file to be opened within this file system.
            /// @returns The pointer to the file or NULL if the file could not be opened.
            virtual InFileI* OpenRead(const std::string& FileName)=0;
        };
    }
}

#endif
