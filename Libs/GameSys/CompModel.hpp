/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_GAMESYS_COMPONENT_MODEL_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_MODEL_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf { class UniScriptStateT; }
namespace cf { namespace GuiSys { class GuiImplT; } }
class AnimPoseT;
class CafuModelT;


namespace cf
{
    namespace GameSys
    {
        /// This component adds a 3D model to its entity.
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

            /// Returns the current pose of this model (or `NULL` if there is no pose (yet)).
            AnimPoseT* GetPose() const;

            /// Returns the GUI instance of this model, if it has one (or `NULL` otherwise).
            IntrusivePtrT<cf::GuiSys::GuiImplT> GetGui() const;

            // Base class overrides.
            ComponentModelT* Clone() const;
            const char* GetName() const { return "Model"; }
            void UpdateDependencies(EntityT* Entity);
            BoundingBox3fT GetEditorBB() const;
            void Render(float LodDist) const;
            void DoServerFrame(float t);
            void DoClientFrame(float t);


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

                void SetAnimNr(int AnimNr, float BlendTime, bool ForceLoop);

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


            /// A variable of type std::string, specifically for GUI file names. It updates the related
            /// GUI instance in the parent ComponentModelT whenever a new GUI file name is set.
            class VarGuiNameT : public TypeSys::VarT<std::string>
            {
                public:

                VarGuiNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentModelT& Comp);
                VarGuiNameT(const VarGuiNameT& Var, ComponentModelT& Comp);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentModelT& m_Comp;    ///< The parent ComponentModelT that contains this variable.
            };


            void FillMemberVars();                          ///< A helper method for the constructors.
            void ReInit(std::string* ErrorMsg=NULL);        ///< A helper method.

            TypeSys::VarT<bool>                         m_ModelShow;    ///< Whether the model is currently shown.
            VarModelNameT                               m_ModelName;    ///< The file name of the model.
            VarModelAnimNrT                             m_ModelAnimNr;  ///< The animation sequence number of the model.
            VarModelSkinNrT                             m_ModelSkinNr;  ///< The skin used for rendering the model.
            TypeSys::VarT<float>                        m_ModelScale;   ///< The scale factor applied to the model coordinates when converted to world space.
            VarGuiNameT                                 m_GuiName;      ///< The file name of the GUI to be used with the models GUI fixtures (if there are any).

            const CafuModelT*                           m_Model;        ///< The model instance, updated by changes to m_ModelName.
            mutable AnimPoseT*                          m_Pose;         ///< The pose of the model, updated by changes to m_ModelAnimNr.
            mutable cf::UniScriptStateT*                m_ScriptState;  ///< The script state that the m_Gui is bound to and lives in.
            mutable IntrusivePtrT<cf::GuiSys::GuiImplT> m_Gui;          ///< The GUI instance, updated by changes to m_GuiName.
        };
    }
}

#endif
