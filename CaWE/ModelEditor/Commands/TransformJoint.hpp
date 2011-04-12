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

#ifndef _MODELEDITOR_TRANSFORM_JOINT_HPP_
#define _MODELEDITOR_TRANSFORM_JOINT_HPP_

#include "../../CommandPattern.hpp"
#include "Math3D/Vector3.hpp"


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandTransformJointT : public CommandT
    {
        public:

        CommandTransformJointT(ModelDocumentT* ModelDoc, unsigned int JointNr, char Type, const Vector3fT& v);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        Vector3fT& GetModelVec();

        ModelDocumentT*    m_ModelDoc;
        const unsigned int m_JointNr;
        const char         m_Type;
        const Vector3fT    m_NewVec;
        const Vector3fT    m_OldVec;
    };
}

#endif
