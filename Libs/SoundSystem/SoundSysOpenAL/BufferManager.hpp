/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
