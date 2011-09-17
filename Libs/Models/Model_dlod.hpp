/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _MODEL_DLOD_HPP_
#define _MODEL_DLOD_HPP_

#include "Templates/Array.hpp"
#include "Model.hpp"
#include "Model_proxy.hpp"


/// This class implements a dLoD (discrete level-of-detail) model.
class ModelDlodT : public ModelT
{
    public:

    /// The constructor.
    ModelDlodT(const std::string& FileName_) /*throw (ModelT::LoadError)*/;

    // The ModelT interface.
    const std::string& GetFileName() const;
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=NULL) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    unsigned int       GetNrOfSequences() const;
    BoundingBox3fT     GetBB(int SequenceNr, float FrameNr) const;
    bool               TraceRay(int SequenceNr, float FrameNr, int SkinNr, const Vector3fT& RayOrigin, const Vector3fT& RayDir, TraceResultT& Result) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;


    private:

    const std::string   FileName;
    ArrayT<ModelProxyT> LodModels;
    ArrayT<float>       LodStartRanges;
    ArrayT<float>       LodEndRanges;
};

#endif
