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

/*******************/
/*** Dummy Model ***/
/*******************/

#ifndef _MODEL_DUMMY_HPP_
#define _MODEL_DUMMY_HPP_

#include "Model.hpp"


/// This class implements a dummy model.
class ModelDummyT : public ModelT
{
    public:

    /// The constructor.
    ModelDummyT(const std::string& FileName_);

    // The ModelT interface.
    const std::string& GetFileName() const;
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=NULL) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    int                GetNrOfSequences() const;
    const float*       GetSequenceBB(int SequenceNr, float FrameNr) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;


    private:

    const std::string FileName;
};

#endif
