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

/******************/
/*** dlod Model ***/
/******************/

#include <stdio.h>
#include "Model_dlod.hpp"
#include "TextParser/TextParser.hpp"


ModelDlodT::ModelDlodT(const std::string& FileName_) /*throw (ModelT::LoadError)*/
    : FileName(FileName_)
{
    TextParserT         TP(FileName.c_str());
    ArrayT<std::string> LodModelNames;

    try
    {
        while (!TP.IsAtEOF())
        {
            LodModelNames .PushBack(TP.GetNextToken());
            LodStartRanges.PushBack(TP.GetNextTokenAsFloat());
            LodEndRanges  .PushBack(TP.GetNextTokenAsFloat());
        }
    }
    catch (const TextParserT::ParseError&)
    {
        throw ModelT::LoadError();
    }

    if (LodModelNames.Size()==0 ||
        LodModelNames.Size()!=LodStartRanges.Size() ||
        LodModelNames.Size()!=LodEndRanges.Size()) throw ModelT::LoadError();

    for (unsigned long LodModelNr=0; LodModelNr<LodModelNames.Size(); LodModelNr++)
    {
        // The LodModelNames are specified relative to the parent file name,
        // thus extract the path portion from FileName and prepend it to the LodModelNames.
        size_t i=FileName.length();

        while (i>0 && FileName[i-1]!='/' && FileName[i-1]!='\\') i--;

        LodModels.PushBack(ModelProxyT(std::string(FileName, 0, i)+LodModelNames[LodModelNr]));
    }
}


/***********************************************/
/*** Implementation of the ModelT interface. ***/
/***********************************************/

const std::string& ModelDlodT::GetFileName() const
{
    return FileName;
}


void ModelDlodT::Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel) const
{
    for (unsigned long LodModelNr=0; LodModelNr<LodModels.Size(); LodModelNr++)
        if (LodDist>=LodStartRanges[LodModelNr] && LodDist<LodEndRanges[LodModelNr])
        {
            LodModels[LodModelNr].Draw(SequenceNr, FrameNr, LodDist, SubModel);
            break;
        }
}


bool ModelDlodT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    for (unsigned long LodModelNr=0; LodModelNr<LodModels.Size(); LodModelNr++)
        if (LodDist>=LodStartRanges[LodModelNr] && LodDist<LodEndRanges[LodModelNr])
            return LodModels[LodModelNr].GetGuiPlane(SequenceNr, FrameNr, LodDist, GuiOrigin, GuiAxisX, GuiAxisY);

    return false;
}


void ModelDlodT::Print() const
{
    printf("\nThis is a discrete-steps level-of-detail model. FileName: \"%s\"\n", FileName.c_str());

    for (unsigned long LodModelNr=0; LodModelNr<LodModels.Size(); LodModelNr++)
        printf("%-50s %10.1f %10.1f\n", LodModels[LodModelNr].GetFileName().c_str(), LodStartRanges[LodModelNr], LodEndRanges[LodModelNr]);
}


int ModelDlodT::GetNrOfSequences() const
{
    return LodModels[0].GetNrOfSequences();
}


BoundingBox3fT ModelDlodT::GetBB(int SequenceNr, float FrameNr) const
{
    return LodModels[0].GetBB(SequenceNr, FrameNr);
}


bool ModelDlodT::TraceRay(int SequenceNr, float FrameNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const
{
    // for (unsigned long LodModelNr=0; LodModelNr<LodModels.Size(); LodModelNr++)
    //     if (LodDist>=LodStartRanges[LodModelNr] && LodDist<LodEndRanges[LodModelNr])
    //         return LodModels[LodModelNr].TraceRay(SequenceNr, FrameNr, RayOrigin, RayDir, Result);
    //
    // return false;
    return LodModels[0].TraceRay(SequenceNr, FrameNr, RayOrigin, RayDir, Result);
}


// float ModelDlodT::GetNrOfFrames(int /*SequenceNr*/) const
// {
//     return LodModels[0].GetNrOfFrames(SequenceNr);
// }


float ModelDlodT::AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop) const
{
    return LodModels[0].AdvanceFrameNr(SequenceNr, FrameNr, DeltaTime, Loop);
}
