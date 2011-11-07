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

#ifndef _CACLIENTWORLD_HPP_
#define _CACLIENTWORLD_HPP_

#include "../Both/Ca3DEWorld.hpp"


struct EntityStateT;
struct PlayerCommandT;
class  NetDataT;


class FrameT
{
    public:

    bool          IsValid;
    unsigned long ServerFrameNr;
    unsigned long DeltaFrameNr;

    ArrayT<unsigned long> EntityIDsInPVS;


    FrameT() : IsValid(false)
    {
    }
};


class CaClientWorldT
{
    public:

    // Constructor
    CaClientWorldT(const char* FileName, ModelManagerT& ModelMan, WorldT::ProgressFunctionT ProgressFunction, unsigned long OurEntityID_) /*throw (WorldT::LoadErrorT)*/;
   ~CaClientWorldT();

    // Worker functions
    unsigned long GetOurEntityID() const { return OurEntityID; }  // AUFLÖSEN!?
    void ReadEntityBaseLineMessage(NetDataT& InData);
    unsigned long ReadServerFrameMessage(NetDataT& InData);

    bool OurEntity_Repredict(unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr);
    void OurEntity_Predict(const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr);
    const EntityStateT* OurEntity_GetState(bool PredictedState);

    void ComputeBFSPath(const VectorT& Start, const VectorT& End);
    void Draw(float FrameTime, const EntityStateT* DrawState);


    private:

    CaClientWorldT(const CaClientWorldT&);      // Use of the Copy    Constructor is not allowed.
    void operator = (const CaClientWorldT&);    // Use of the Assignment Operator is not allowed.


    Ca3DEWorldT*          Ca3DEWorld;
    EntityManagerT&       EntityManager;

    unsigned long         OurEntityID;

    unsigned long         ServerFrameNr;    // Erhalte mit Frames[ServerFrameNr & (MAX_FRAMES-1)] das Frame zur ServerFrameNr!
    const char            MAX_FRAMES;
    ArrayT<FrameT>        Frames;

    ArrayT<unsigned long> BFS_Tree;
    ArrayT<VectorT>       BFS_TreePoints;
    unsigned long         BFS_EndLeafNr;
};

#endif
