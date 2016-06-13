/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_BUFFER_MANAGER_HPP_INCLUDED
#define CAFU_SOUNDSYS_BUFFER_MANAGER_HPP_INCLUDED

#include "../SoundShader.hpp"   // For LoadTypeE.
#include "Templates/Array.hpp"

#include <string>


class BufferT;


/// This class efficiently manages audio buffers by employing resource sharing whenever possible.
class BufferManagerT
{
    public:

    /// Returns the BufferManagerTs singleton instance.
    static BufferManagerT* GetInstance();

    /// This method obtains a BufferT instance for the specified resource (a file or a capture device).
    /// When the user code is done with the returned buffer, it must call ReleaseBuffer() in order to release the buffer.
    /// (The implementation can return a newly created or a reference-counted BufferT instance for resource sharing.)
    ///
    /// @param ResName     The name of the resource (file or capture device) that the requested buffer is for (i.e. created from).
    /// @param ForceMono   Whether the data from the resource should be reduced to a single channel before use (mono output).
    /// @param LoadType    The type of buffer that should handle the resource (see SoundShaderT for more details).
    ///
    /// @returns a BufferT instance for the specified resource. Throws an exception of type std::runtime_error on failure.
    BufferT* GetBuffer(const std::string& ResName, bool ForceMono, SoundShaderT::LoadTypeE LoadType);

    /// Updates all buffers.
    void UpdateAll();

    /// Releases a buffer.
    /// Internally, the released buffer may or may not be completely deleted from memory, depending on its reference count.
    /// @param Buffer   The buffer to release.
    void ReleaseBuffer(BufferT* Buffer);

    /// Releases all buffers.
    void ReleaseAll();


    private:

    /// Private constructor for singleton pattern.
    BufferManagerT();

    /// The destructor.
    ~BufferManagerT();

    /// Deletes all buffers that have no references left and are thus unused.
    void CleanUp();

    ArrayT<BufferT*> m_Buffers;     ///< The set of buffers currently known to and managed by the buffer manager.
};

#endif
