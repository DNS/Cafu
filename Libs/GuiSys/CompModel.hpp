/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_COMPONENT_MODEL_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_MODEL_HPP_INCLUDED

#include "CompBase.hpp"


class AnimPoseT;
class CafuModelT;


namespace cf
{
    namespace GuiSys
    {
        /// This component adds a 3D model to its window.
        class ComponentModelT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentModelT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentModelT(const ComponentModelT& Comp);

            /// The destructor.
            ~ComponentModelT();

            // Base class overrides.
            ComponentModelT* Clone() const;
            const char* GetName() const { return "Model"; }
            void UpdateDependencies(WindowT* Window);
            void Render() const;
            void OnClockTickEvent(float t);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetNumAnims(lua_State* LuaState);
            static int SetAnim(lua_State* LuaState);
            static int GetNumSkins(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            /// A variable of type std::string, specifically for model file names. It updates the related
            /// model instance in the parent ComponentModelT whenever a new model file name is set.
            class VarModelNameT : public TypeSys::VarT<std::string>
            {
                public:

                VarModelNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentModelT& Comp);
                VarModelNameT(const VarModelNameT& Var, ComponentModelT& Comp);

                // Base class overrides.
                std::string GetExtraMessage() const { return m_ExtraMsg; }
                void Serialize(cf::Network::OutStreamT& Stream) const;  ///< See base class documentation for details!
                void Deserialize(cf::Network::InStreamT& Stream);
                void Set(const std::string& v);


                private:

                ComponentModelT& m_Comp;        ///< The parent ComponentModelT that contains this variable.
                std::string      m_ExtraMsg;    ///< If the model could not be loaded in Set(), this message contains details about the problem.
            };


            /// A variable of type int, specifically for model animation sequence numbers. It updates the
            /// related model pose in the parent ComponentModelT whenever a new sequence number is set.
            class VarModelAnimNrT : public TypeSys::VarT<int>
            {
                public:

                VarModelAnimNrT(const char* Name, const int& Value, const char* Flags[], ComponentModelT& Comp);
                VarModelAnimNrT(const VarModelAnimNrT& Var, ComponentModelT& Comp);

                // Base class overrides.
                void Set(const int& v);
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;


                private:

                ComponentModelT& m_Comp;    ///< The parent ComponentModelT that contains this variable.
            };


            /// A variable of type int, specifically for model skin numbers.
            class VarModelSkinNrT : public TypeSys::VarT<int>
            {
                public:

                VarModelSkinNrT(const char* Name, const int& Value, const char* Flags[], ComponentModelT& Comp);
                VarModelSkinNrT(const VarModelSkinNrT& Var, ComponentModelT& Comp);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;


                private:

                ComponentModelT& m_Comp;    ///< The parent ComponentModelT that contains this variable.
            };


            void FillMemberVars();                                                ///< A helper method for the constructors.
            std::string SetModel(const std::string& FileName, std::string& Msg);  ///< m_ModelName::Set() delegates here.
            int SetAnimNr(int AnimNr, float BlendTime, bool ForceLoop);           ///< m_ModelAnimNr::Set() and SetAnim(LuaState) delegate here.

            VarModelNameT            m_ModelName;   ///< The file name of the model.
            VarModelAnimNrT          m_ModelAnimNr; ///< The animation sequence number of the model.
            VarModelSkinNrT          m_ModelSkinNr; ///< The skin used for rendering the model.
            TypeSys::VarT<Vector3fT> m_ModelPos;    ///< The position of the model in world space.
            TypeSys::VarT<float>     m_ModelScale;  ///< The scale factor applied to the model coordinates when converted to world space.
            TypeSys::VarT<Vector3fT> m_ModelAngles; ///< The angles around the axes that determine the orientation of the model in world space.
            TypeSys::VarT<Vector3fT> m_CameraPos;   ///< The position of the camera in world space.

            const CafuModelT*        m_Model;       ///< The model instance, updated by changes to m_ModelName.
            AnimPoseT*               m_Pose;        ///< The pose of the model.
        };
    }
}

#endif
