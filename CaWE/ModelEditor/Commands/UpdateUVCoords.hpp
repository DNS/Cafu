/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
