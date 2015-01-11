/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

            /// Returns the clip model of this component, NULL for none.
            const cf::ClipSys::ClipModelT* GetClipModel() const { return m_ClipModel; }

            // Base class overrides.
            ComponentCollisionModelT* Clone() const;
            const char* GetName() const { return "CollisionModel"; }
            void UpdateDependencies(EntityT* Entity);
            unsigned int GetEditorColor() const { return 0xAAAAAA; }
            BoundingBox3fT GetCollisionBB() const override;


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

            void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) /*override*/;
            void DoServerFrame(float t);
            void CleanUp();
            void UpdateClipModel();

            TypeSys::VarT<std::string>          m_CollMdlName;      ///< The file name of the collision model.
            std::string                         m_PrevName;         ///< The previous file name, used to detect changes in `m_CollMdlName`.
            const cf::ClipSys::CollisionModelT* m_CollisionModel;   ///< The collision model of this component, NULL for none.

            cf::ClipSys::ClipModelT*            m_ClipModel;        ///< The clip model of this component, NULL for none.
            Vector3fT                           m_ClipPrevOrigin;
            cf::math::QuaternionfT              m_ClipPrevQuat;
        };
    }
}

#endif
