/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_COLLISION_MODEL_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_COLLISION_MODEL_HPP_INCLUDED

#include "CompBase.hpp"
#include "Math3D/Quaternion.hpp"


namespace cf { namespace ClipSys { class ClipModelT; } }
namespace cf { namespace ClipSys { class CollisionModelT; } }


namespace cf
{
    namespace GameSys
    {
        /// This component adds a collision model to its entity.
        class ComponentCollisionModelT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentCollisionModelT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentCollisionModelT(const ComponentCollisionModelT& Comp);

            /// The destructor.
            ~ComponentCollisionModelT();

            // Sets the given bounding-box as the collision model.
            // Instead of loading a collision model from a file, user code can call this method
            // to set a bounding-box with the given dimensions as the collision model.
            void SetBoundingBox(const BoundingBox3dT& BB, const char* MatName);


            // Base class overrides.
            ComponentCollisionModelT* Clone() const;
            const char* GetName() const { return "CollisionModel"; }
            void UpdateDependencies(EntityT* Entity);
            unsigned int GetEditorColor() const { return 0xAAAAAA; }
            const cf::ClipSys::ClipModelT* GetClipModel() override { UpdateClipModel(); return m_ClipModel; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int SetBoundingBox(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) override;
            void DoServerFrame(float t) override;
            void CleanUp();
            void UpdateClipModel();

            TypeSys::VarT<std::string>          m_CollMdlName;      ///< The file name of the collision model.
            std::string                         m_PrevName;         ///< The previous file name, used to detect changes in `m_CollMdlName`.
            const cf::ClipSys::CollisionModelT* m_CollisionModel;   ///< The collision model of this component, NULL for none.
            TypeSys::VarT<bool>                 m_IgnoreOrient;     ///< If true, the orientation of the entity does not affect the orientation of the collision model.

            cf::ClipSys::ClipModelT*            m_ClipModel;        ///< The clip model of this component, NULL for none.
            Vector3fT                           m_ClipPrevOrigin;
            cf::math::QuaternionfT              m_ClipPrevQuat;
        };
    }
}

#endif
