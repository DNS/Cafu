/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODELEDITOR_TRANSFORM_JOINT_HPP_INCLUDED
#define CAFU_MODELEDITOR_TRANSFORM_JOINT_HPP_INCLUDED

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
