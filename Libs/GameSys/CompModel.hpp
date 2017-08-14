/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_MODEL_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_MODEL_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf { namespace GuiSys { class GuiImplT; } }
class AnimPoseT;
class CafuModelT;


namespace cf
{
    namespace GameSys
    {
        /// This component adds a 3D model to its entity.
        /// Models can be used to add geometric detail to a map. Some models also have ready-made
        /// "GUI fixtures" where scripted GUIs can be attached that players can interact with.
        /// Use the CaWE Model Editor in order to import mesh and animation data for models, and
        /// to prepare them for use in game maps.
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
            ComponentModelT* Clone() const override;
            const char* GetName() const override { return "Model"; }
            void UpdateDependencies(EntityT* Entity) override;
            unsigned int GetEditorColor() const override { return 0x00FFFF; }
            BoundingBox3fT GetEditorBB() const override;
            BoundingBox3fT GetCullingBB() const override;
            bool Render(bool FirstPersonView, float LodDist) const override;
            void DoSerialize(cf::Network::OutStreamT& Stream) const override;
            void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) override;
            void DoServerFrame(float t) override;
            void DoClientFrame(float t) override;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetNumAnims(lua_State* LuaState);
            static int GetNumSkins(lua_State* LuaState);
            static int GetGui(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::MethsDocT DocCallbacks[];
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
                std::string GetExtraMessage() const override { return m_ExtraMsg; }
                void Serialize(cf::Network::OutStreamT& Stream) const override;  ///< See base class documentation for details!
                void Deserialize(cf::Network::InStreamT& Stream) override;
                void Set(const std::string& v) override;


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
                void Set(const int& v) override;
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const override;


                private:

                void SetAnimNr(int AnimNr);

                ComponentModelT& m_Comp;    ///< The parent ComponentModelT that contains this variable.
            };


            /// A variable of type int, specifically for model skin numbers.
            class VarModelSkinNrT : public TypeSys::VarT<int>
            {
                public:

                VarModelSkinNrT(const char* Name, const int& Value, const char* Flags[], ComponentModelT& Comp);
                VarModelSkinNrT(const VarModelSkinNrT& Var, ComponentModelT& Comp);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const override;


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
                void Set(const std::string& v) override;


                private:

                ComponentModelT& m_Comp;    ///< The parent ComponentModelT that contains this variable.
            };


            void FillMemberVars();                          ///< A helper method for the constructors.
            void ReInit(std::string* ErrorMsg=NULL);        ///< A helper method.

            TypeSys::VarT<bool>                     m_ModelShow;    ///< Whether the model is currently shown.
            VarModelNameT                           m_ModelName;    ///< The file name of the model.
            VarModelAnimNrT                         m_ModelAnimNr;  ///< The animation sequence number of the model.
            VarModelSkinNrT                         m_ModelSkinNr;  ///< The skin used for rendering the model.
            TypeSys::VarT<float>                    m_ModelScale;   ///< The scale factor applied to the model coordinates when converted to world space.
            VarGuiNameT                             m_GuiName;      ///< The file name of the GUI to be used with the models GUI fixtures (if there are any).
         // TypeSys::VarT<bool>                     m_CastShadows;  ///< Does this model cast shadows when lit by a dynamic light source?
            TypeSys::VarT<bool>                     m_IsSubmodel;   ///< Is this model a submodel of another model?
            TypeSys::VarT<bool>                     m_Is1stPerson;  ///< Is this a 1st-person view model? If `true`, the model is rendered if the world is rendered from *this* entity's perspective. If `false`, the model is rendered when seen from the outside, i.e. in everybody else's view. The default is `false`, because `true` is normally only used with the human player's 1st-person carried weapon models.

            const CafuModelT*                       m_Model;        ///< The model instance, updated by changes to m_ModelName.
            mutable AnimPoseT*                      m_Pose;         ///< The pose of the model, updated by changes to m_ModelAnimNr.
            mutable IntrusivePtrT<GuiSys::GuiImplT> m_Gui;          ///< The GUI instance, updated by changes to m_GuiName.
        };
    }
}

#endif
