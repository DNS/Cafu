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

#include <stdio.h>
#include "Model_dummy.hpp"


ModelDummyT::ModelDummyT(const std::string& FileName_)
    : FileName(FileName_)
{
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelDummyT::GetFileName() const
{
    return FileName;
}


void ModelDummyT::Draw(int /*SequenceNr*/, float /*FrameNr*/, float /*LodDist*/, const ModelT* /*SubModel*/) const
{
}


bool ModelDummyT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    return false;
}


void ModelDummyT::Print() const
{
    printf("A dummy model. FileName: \"%s\"\n", FileName.c_str());
}


int ModelDummyT::GetNrOfSequences() const
{
    return 1;
}


const float* ModelDummyT::GetSequenceBB(int /*SequenceNr*/, float /*FrameNr*/) const
{
    static float BB[6];

    return BB;
}


// float ModelDummyT::GetNrOfFrames(int /*SequenceNr*/) const
// {
//     return 0.0;
// }


float ModelDummyT::AdvanceFrameNr(int /*SequenceNr*/, float /*FrameNr*/, float /*DeltaTime*/, bool /*Loop*/) const
{
    return 0.0;
}
