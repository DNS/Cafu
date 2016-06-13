/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SOUNDSYS_BUFFER_MANAGER_HPP_INCLUDED
#define CAFU_SOUNDSYS_BUFFER_MANAGER_HPP_INCLUDED

#include "../SoundShader.hpp" // For LoadTypeE.

#include "Templates/Array.hpp"

#include <string>
#include <map>


class BufferT;
class StaticBufferT;
class StreamingBufferT;


/// The buffer manager provides the means to create sound buffers from filenames.
/// Sound buffers are managed in a way that they are reused if a buffer is needed multiple times.
class BufferManagerT
{
    public:

    /// Get the global instance of the buffer manager.
    static BufferManagerT* GetInstance();

    /// Interface to get the sound buffer of a specific audio file.
    /// If the buffer has not yet been created it is created in this method. Otherwise the already exisiting buffer is returned.
    /// @param AudioFile Relative Path to the audio file from which the buffer should be created.
    /// @param LoadType What kind of buffer should be created (see SoundShaderT for more details).
    /// @param Is3DSound If the buffer should be loaded as a 3 dimensional sound.
    /// @return The created buffer or NULL if no buffer could be created from the specified audio file.
    BufferT* GetBuffer(const std::string& AudioFile, SoundShaderT::LoadTypeE LoadType, bool Is3DSound);

    /// Deletes all buffers that are currently unused (have no references).
    void CleanUp();

    /// Release a buffer.
    /// The buffer is also completely destroyed if there are no references on it after the release.
    /// @param Buffer The buffer to release.
    void ReleaseBuffer(BufferT* Buffer);

    /// Releases all buffers.
    void ReleaseAll();


    private:

    std::map<std::string, StaticBufferT*> StaticBuffers;    ///< Array of all static buffers that have been created.
    ArrayT<StreamingBufferT*>             StreamingBuffers; ///< Array of all streaming buffers.

    /// Private constructor for singleton pattern.
    BufferManagerT();

    /// Destructor. Deletes all created buffers.
    ~BufferManagerT();
};

#endif
