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
/*** Model Proxy ***/
/*******************/

#ifndef _MODEL_PROXY_HPP_
#define _MODEL_PROXY_HPP_

#include "Model.hpp"


/// This class represents a model proxy that behaves like a real ModelT.
/// It implements (the factory of) the FLYWEIGHT design pattern, using reference counting.
/// As such, it is very lightweight to copy, and ressources are saved as each "real" ModelT only ever exists once.
/// (The real ModelTs are indexed/discriminated by the result of their GetName() function.)
/// The constructor also decides which concrete model class is represented by this proxy.
class ModelProxyT : public ModelT
{
    public:

    /// The constructor (creates a dummy model).
    ModelProxyT();

    /// The constructor.
    ModelProxyT(const std::string& FileName);

    /// The destructor.
    ~ModelProxyT();

    /// The copy constructor.
    ModelProxyT(const ModelProxyT& Other);

    /// The assignment operator.
    ModelProxyT& operator = (const ModelProxyT& Other);

    /// A method for obtaining access to the real model.
    const ModelT* GetRealModel() const;


    // The ModelT interface.
    const std::string& GetFileName() const;
    void               Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel=0 /*NULL*/) const;
    bool               GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const;
    void               Print() const;
    int                GetNrOfSequences() const;
    const float*       GetSequenceBB(int SequenceNr, float FrameNr) const;
 // float              GetNrOfFrames(int SequenceNr) const;
    float              AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop=true) const;


    private:

    unsigned long PoolIndex; ///< Index into the pool of real ModelTs.
};

#endif
