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

#include "Model_cmdl.hpp"
#include "String.hpp"

// #if defined(_WIN32) && defined(_MSC_VER)
//     // Turn off warning C4355: 'this' : used in base member initializer list.
//     // This is required here before the inclusion of the dae.h header, because the warning is triggered there.
//     #pragma warning(disable:4355)
// #endif
// #include <dae.h>

#include <cassert>
#include <stdio.h>


static std::string ReplaceExtension(const std::string& FileName)
{
    const size_t pos=FileName.rfind('.');

    // Note that this works even if pos==std::string::npos, because that is also the default value of the ctor parameter in the next line.
    return std::string(FileName, 0, pos)+".cmdl";
}


ModelCaMdlT::ModelCaMdlT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    // : ModelMd5T(FileName, true /* Yes, the base class should use the tangent-space given by us. */)
    : m_FileName(ReplaceExtension(FileName))
{
    if (cf::String::EndsWith(FileName, "dae") || cf::String::EndsWith(FileName, "collada"))
    {
        // DAE dae;
        ;
    }

    // Load the .cmdl file.
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelCaMdlT::GetFileName() const
{
    return m_FileName;
}


void ModelCaMdlT::Draw(int /*SequenceNr*/, float /*FrameNr*/, float /*LodDist*/, const ModelT* /*SubModel*/) const
{
}


bool ModelCaMdlT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    return false;
}


void ModelCaMdlT::Print() const
{
    printf("\nThis is cmdl model. FileName: \"%s\"\n", m_FileName.c_str());
}


int ModelCaMdlT::GetNrOfSequences() const
{
    return 1;
}


const float* ModelCaMdlT::GetSequenceBB(int /*SequenceNr*/, float /*FrameNr*/) const
{
    static float BB[6];

    /* TODO....
    BoundingBox3T<double> TotalBB(GeomObjects[0].BB);

    for (unsigned long GONr=0; GONr<GeomObjects.Size(); GONr++)
    {
        TotalBB.Insert(GeomObjects[GONr].BB.Min);
        TotalBB.Insert(GeomObjects[GONr].BB.Max);
    }

    BB[0]=float(TotalBB.Min.x);
    BB[1]=float(TotalBB.Min.y);
    BB[2]=float(TotalBB.Min.z);

    BB[3]=float(TotalBB.Max.x);
    BB[4]=float(TotalBB.Max.y);
    BB[5]=float(TotalBB.Max.z); */

    return BB;
}


// float ModelCaMdlT::GetNrOfFrames(int /*SequenceNr*/) const
// {
//     return 0.0;
// }


float ModelCaMdlT::AdvanceFrameNr(int /*SequenceNr*/, float /*FrameNr*/, float /*DeltaTime*/, bool /*Loop*/) const
{
    return 0.0;
}
