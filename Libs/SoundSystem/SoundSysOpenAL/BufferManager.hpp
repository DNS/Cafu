/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _SOUNDSYS_BUFFER_MANAGER_HPP_
#define _SOUNDSYS_BUFFER_MANAGER_HPP_

#include "../SoundShader.hpp"   // For LoadTypeE.
#include "Templates/Array.hpp"

#include <string>
#include <map>


class BufferT;


/// This class efficiently manages audio buffers by employing resource sharing whenever possible.
class BufferManagerT
{
    public:

    /// Returns the BufferManagerTs singleton instance.
    static BufferManagerT* GetInstance();

    /// Returns a buffer for the audio data in the specified file.
    /// The method can return a newly created or a previously existing buffer for resource sharing,
    /// and thus the buffer cannot directly be deleted by the user but must be released through ReleaseBuffer().
    /// @param AudioFile   Path and name of the file to return a buffer for.
    /// @param LoadType    The type of buffer that is to be returned (see SoundShaderT for more details).
    /// @param Is3DSound   Whether the buffer is intended for use as a 3-dimensional sound.
    /// @return The buffer for the specified file, or NULL if no such buffer could be created.
    BufferT* GetBuffer(const std::string& AudioFile, SoundShaderT::LoadTypeE LoadType, bool Is3DSound);

    /// Releases a buffer.
    /// Internally, the released buffer may or may not be completely deleted from memory, depending on its reference count.
    /// @param Buffer   The buffer to release.
    void ReleaseBuffer(BufferT* Buffer);

    /// Releases all buffers.
    void ReleaseAll();


    private:

    /// Private constructor for singleton pattern.
    BufferManagerT();

    /// Destructor. Deletes all created buffers.
    ~BufferManagerT();

    /// Deletes all buffers that have no references left and are thus unused.
    void CleanUp();

    ArrayT<BufferT*> m_Buffers;     ///< The set of buffers currently known to and managed by the buffer manager.
};

#endif
