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

/*****************************/
/*** Net Message Constants ***/
/*****************************/


// Dieses Header-File definiert die Konstanten für die Kommunikation zwischen Server und Client.
// Die 0 wurde absichtlich freigehalten, um sie später evtl. als ErrorFlag verwenden zu können.

// 'connection-less' Message-Types vom Client zum Server
const char CS0_NoOperation         =1;
const char CS0_Ping                =2;
const char CS0_Connect             =3;
const char CS0_Info                =4;
const char CS0_RemoteConsoleCommand=5;  ///< A message consisting of a password and a command string that is to be executed by the server console interpreter.

// 'connection-less' Message-Types vom Server zum Client
const char SC0_ACK           =1;
const char SC0_NACK          =2;
const char SC0_RccReply      =3;        ///< String reply to a CS0_RemoteConsoleCommand message.

// 'connection-established' Message-Types vom Client zum Server
const char CS1_PlayerCommand =1;
const char CS1_Disconnect    =2;
const char CS1_SayToAll      =3;
const char CS1_WorldInfoACK  =4;
const char CS1_FrameInfoACK  =5;

// 'connection-established' Message-Types vom Server zum Client
const char SC1_WorldInfo     =1;
const char SC1_EntityBaseLine=2;
const char SC1_FrameInfo     =3;
const char SC1_EntityUpdate  =4;
const char SC1_DropClient    =5;
const char SC1_ChatMsg       =6;
