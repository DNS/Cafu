/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_MODELEDITOR_UPDATE_UV_COORDS_HPP_INCLUDED
#define CAFU_MODELEDITOR_UPDATE_UV_COORDS_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Math3D/Vector3.hpp"


class AnimPoseT;


namespace ModelEditor
{
    class ModelDocumentT;

    class CommandUpdateUVCoordsT : public CommandT
    {
        public:

        struct CoordT
        {
            bool operator == (const CoordT& C) { return u==C.u && v==C.v; }
            bool operator != (const CoordT& C) { return u!=C.u || v!=C.v; }

            float u;
            float v;
        };

        CommandUpdateUVCoordsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, const AnimPoseT& Pose, const Vector3fT& u, const Vector3fT& v);
        CommandUpdateUVCoordsT(ModelDocumentT* ModelDoc, unsigned int MeshNr, const ArrayT<CoordT>& NewUVs);

        // CommandT implementation.
        bool Do();
        void Undo();
        wxString GetName() const;


        private:

        ArrayT<CoordT> GetNewUVs(const AnimPoseT& Pose, const Vector3fT& u, const Vector3fT& v) const;
        ArrayT<CoordT> GetOldUVs() const;

        ModelDocumentT*      m_ModelDoc;
        const unsigned int   m_MeshNr;

        const ArrayT<CoordT> m_NewUVs;
        const ArrayT<CoordT> m_OldUVs;
    };
}

#endif
