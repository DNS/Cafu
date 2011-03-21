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

#ifndef _CASERVERWORLD_HPP_
#define _CASERVERWORLD_HPP_

#include "../Both/Ca3DEWorld.hpp"
#include "../../Games/PlayerCommand.hpp"


class NetDataT;


class CaServerWorldT
{
    public:

    // Erstellt eine neue ServerWorld anhand des World-Files 'FileName', wobei 'FileName' den kompletten (wenn auch relativen) Pfad und Namen enth�lt.
    CaServerWorldT(const char* FileName, Ca3DEWorldT* Ca3DEWorld_);

    // Der Destruktor.
    ~CaServerWorldT();

    // F�gt einen neuen HumanPlayer-Entity zum N�CHSTEN Frame in die World ein (idR nach Client-Join oder World-Change),
    // NICHT ins aktuelle (bzgl. der BaseLineFrameNr). Ziel: Erreiche gleiches Verhalten wie z.B. das des MonsterMakers.
    // Gibt bei Erfolg die ID des neuen Entities zur�ck, sonst 0xFFFFFFFF.
    unsigned long InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr);

    // Entfernt den (HumanPlayer-)Entity mit der ID 'HumanPlayerEntityID' aus der World.
    void RemoveHumanPlayerEntity(unsigned long HumanPlayerEntityID);

    // Informiert den (HumanPlayer-)Entity mit der ID 'HumanPlayerEntityID' �ber das 'PlayerCommand' (zur Verarbeitung beim n�chsten 'Think()en'.
    void NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand);

    // Falls es neue Entities (und damit neue BaseLine-Messages) gibt, die j�nger sind als 'OldBaseLineFrameNr',
    // schreibe entsprechende BaseLine-Messages nach 'OutDatas'.
    unsigned long WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    // Schreibt eine komplette Delta-Update-Message (FrameInfo+EntityUpdates) nach 'OutData'.
    // 'ClientEntityID' ist dabei die ID des Entities, dessen PVS zur Bestimmung der sichtbaren Entities herangezogen wird,
    // 'ClientFrameNr' die Nummer des Frames/Zustands, den der Client zuletzt best�tigt hat,
    // 'ClientOldStatesPVSEntityIDs' die von dieser Funktion gewarteteten PVS-Informationen vorangegangener Zust�nde und
    // 'ClientCurrentStateIndex' der (ebenfalls von dieser Funktion gewartete) Index in die PVS-Informationen.
    void WriteClientDeltaUpdateMessages(unsigned long ClientEntityID, unsigned long ClientFrameNr, ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs, unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const;

    // �berf�hrt die World �ber die Zeit 'FrameTime' in den n�chsten Zustand.
    void Think(float FrameTime);


    private:

    CaServerWorldT(const CaServerWorldT&);      ///< Use of the Copy Constructor    is not allowed.
    void operator = (const CaServerWorldT&);    ///< Use of the Assignment Operator is not allowed.

    Ca3DEWorldT*    Ca3DEWorld;     // Hier findet die Action statt (ESInterface bezieht sich auf die Ca3DEWorld)
    EntityManagerT& EntityManager;  // Decoration Pattern
    unsigned long   ServerFrameNr;  // Nummer des aktuellen Frames/Zustands
};

#endif
