/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_BASE_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_BASE_HPP_INCLUDED

#include "Variables.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Templates/Pointer.hpp"

// This macro is introduced by some header (gtk?) under Linux...
#undef CurrentTime


namespace cf { namespace ClipSys { class ClipModelT; } }
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }
namespace cf { namespace TypeSys { class MethsDocT; } }
namespace cf { namespace TypeSys { class VarsDocT; } }

struct lua_State;
struct luaL_Reg;


namespace cf
{
    namespace GameSys
    {
        class ApproxBaseT;
        class EntityT;

        /// This is the base class for the components that an entity is composed/aggregated of.
        ///
        /// Components are the basic building blocks of an entity: their composition defines
        /// the properties, the behaviour, and thus virtually every aspect of the entity.
        ///
        /// Components can exist in two invariants:
        ///   - Stand-alone, independent and not a part of any entity.
        ///   - Normally, as an active part of a entity.
        ///
        /// Stand-alone components typically occur when they're newly instantiated, for example
        /// when they are loaded from disk, when they are instantiated in scripts, or when they
        /// are kept in the clipboard or managed in the Undo/Redo system of the Map Editor.
        /// Newly created, copied or cloned components are initially stand-alone.
        ///
        /// A component becomes a part of an entity via the EntityT::AddComponent() method.
        /// The entity then knows the component, because it hosts it, and reversely, the
        /// component then knows the parent entity that it is a component of.
        class ComponentBaseT : public RefCountedT
        {
            public:

            /// The constructor.
            /// The newly created component is initially not a part of any entity.
            ComponentBaseT();

            /// The copy constructor.
            /// The newly copied component is initially not a part of any entity, even if the source component was.
            /// @param Comp   The component to create a copy of.
            ComponentBaseT(const ComponentBaseT& Comp);

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this component without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete type is known.
            /// The newly cloned component is initially not a part of any entity, even if the source component was.
            virtual ComponentBaseT* Clone() const;

            /// The virtual destructor.
            virtual ~ComponentBaseT();


            /// Returns the name of this component.
            virtual const char* GetName() const { return "Base"; }

            /// Returns the parent entity that contains this component,
            /// or `NULL` if this component is currently not a part of any entity.
            EntityT* GetEntity() const { return m_Entity; }

            /// Returns the variable manager that keeps generic references to our member variables,
            /// providing a simple kind of "reflection" or "type introspection" feature.
            TypeSys::VarManT& GetMemberVars() { return m_MemberVars; }

            /// Sets the member variable with the given name to the given value.
            /// The purpose of this method is to allow C++ code an easier access to the `m_MemberVars`,
            /// especially when initializing newly created components (e.g. in the Map Editor).
            /// (Alternatively, most derived classes had to provide some `SetXY()` methods on their own,
            ///  and thereby clutter their own public interfaces with quasi unimportant methods,
            ///  or the user code had to deal cumbersomely with the TypeSys::VarManT instance itself.)
            template<class T> void SetMember(const char* Name, const T& Value)
            {
                TypeSys::VarT<T>* v = dynamic_cast<TypeSys::VarT<T>*>(m_MemberVars.Find(Name));
                assert(v);
                v->Set(Value);
            }

            /// Returns the interpolators that have been registered with this component.
            ArrayT<ApproxBaseT*>& GetInterpolators() { return m_ClientApprox; }

            /// Registers the member variable with the given name for interpolation over client
            /// frames in order to bridge the larger intervals between server frames.
            /// This method only works with variables whose related type is `float`, `double`,
            /// `Vector2fT`, `Vector3fT` or `Vector3dT`.
            bool InitClientApprox(const char* VarName);

            /// Writes the current state of this component into the given stream.
            /// This method is called to send the state of the component over the network, to save it to disk,
            /// or to store it in the clipboard.
            /// The implementation calls DoSerialize() that derived classes can override to add their own data.
            ///
            /// @param Stream
            ///   The stream to write the state data to.
            void Serialize(cf::Network::OutStreamT& Stream) const;

            /// Reads the state of this component from the given stream, and updates the component accordingly.
            /// This method is called after the state of the component has been received over the network,
            /// has been loaded from disk, has been read from the clipboard, or must be "reset" for the purpose
            /// of (re-)prediction.
            /// The implementation calls DoDeserialize() that derived classes can override to read their own data,
            /// or to run any post-deserialization code.
            ///
            /// @param Stream
            ///   The stream to read the state data from.
            ///
            /// @param IsIniting
            ///   Used to indicate that the call is part of the construction / first-time initialization of the component.
            ///   The implementation will use this to not wrongly process the event counters, interpolation, etc.
            void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting);

            /// Calls the given Lua method of this component.
            /// This method is analogous to UniScriptStateT::CallMethod(), see there for details.
            /// @param MethodName     The name of the Lua method to call.
            /// @param NumExtraArgs   The number of extra arguments that have been pushed on the stack.
            /// @param Signature      See UniScriptStateT::Call() for details.
            bool CallLuaMethod(const char* MethodName, int NumExtraArgs, const char* Signature="", ...);


            /// This method is called whenever something "external" to this component has changed:
            ///   - if the parent entity has changed, because this component was added to or removed from it,
            ///   - if other components in the parent entity have changed.
            /// The component can use the opportunity to search the entity for "sibling" components
            /// that it depends on, and store direct pointers to them.
            /// Note however that dependencies among components must not be cyclic, or else the deletion
            /// of an entity will leave a memory leak.
            /// @param Entity   The parent entity that contains this component, or `NULL` to indicate that this component is removed from the entity that it used to be a part of.
            virtual void UpdateDependencies(EntityT* Entity);

            /// Returns a color that the Map Editor can use to render the representation of this component's entity.
            /// The Map Editor may use the color of an entity's first component as returned by this method to render
            /// the visual representation of the entity.
            virtual unsigned int GetEditorColor() const { return 0xDC1EDC; }    // (220, 30, 220)

            /// Returns a bounding-box that the Map Editor can use to render the representation of this component's
            /// entity and for related hit tests in the 2D and 3D views after mouse clicks.
            /// The Map Editor may use the bounding-box of an entity's first component as returned by this method to
            /// render the visual representation of the entity.
            ///
            /// Note that the returned bounding-box is often *smaller* than the bounding-box returned by
            /// GetCullingBB(), e.g. for light sources (whose radius and thus their indirect effects on other objects
            /// it may not cover), for trees (whose trunk it usually covers, but maybe not their crown), or for models
            /// (that, when animated, may break the limits of the static bounding-box).
            ///
            /// On the other hand, the returned bounding-box may also be *larger* than the bounding-box returned by
            /// GetCullingBB(), e.g. for models that are not initialized. Such models would be "invisible" in the 2D
            /// and 3D views of the Map Editor if we didn't return "dummy" bounding-boxes for them so that users can
            /// see and work with them.
            ///
            /// The returned bounding-box is in local entity space and is always initialized (`IsInited() == true`).
            virtual BoundingBox3fT GetEditorBB() const { return BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)); }

            /// This method returns a bounding-box that encloses the visual representation of this component.
            /// It is used to determine if the entity is in the view-frustum of a camera, how large a region must be
            /// updated in the 2D views of a Map Editor, if the entity is in the potentially-visibility-set (PVS) of
            /// another entity, and similar purposes.
            ///
            /// The returned bounding-box is in local space, i.e. typically centered around the origin (0, 0, 0).
            /// If the component doesn't have a visual representation, the returned bounding-box may be uninitialized
            /// (`!IsInited()`). Also see EntityT::GetCullingBB() for additional details.
            virtual BoundingBox3fT GetCullingBB() const { return BoundingBox3fT(); }

            /// This method returns the clip model of this component, if any.
            virtual const cf::ClipSys::ClipModelT* GetClipModel() { return NULL; }

            /// Initializes any resources that may be needed on the client or server ahead of time.
            /// The goal is to avoid in-game disruptions on frame-rate caused by lazily loaded assets.
            virtual void PreCache() { }

            /// This method implements the graphical output of this component.
            /// @param FirstPersonView   If the world is rendered from the perspective of this component's entity.
            /// @param LodDist           The distance of the viewer entity to this component's entity.
            /// @returns `true` if "something" was rendered, `false` otherwise (in this case the Map Editor may choose
            ///     to render another visual representation of this component's entity).
            virtual bool Render(bool FirstPersonView, float LodDist) const { return false; }

            /// This method provides an opportunity for another render pass.
            virtual void PostRender(bool FirstPersonView) { }

            /// This method is called after all entities and their components have been loaded.
            ///
            /// It is called only once when the static part of world initializatzion is complete, i.e. after the initial
            /// values of all entities and their components have been set.
            /// Components can override this method in order act / do something / add custom behaviour at that time.
            ///
            /// For example, a choice component can use it to set the associated text component to the initial
            /// selection, a script component can forward it to the script by calling a related script function,
            /// a component that for backwards-compatibility supports reading old variables can convert to new ones, etc.
            ///
            /// @param OnlyStatic   `true` if only the loading of static data is desired, e.g.
            ///     when the world is instantiated in the Map Editor, `false` if also
            ///     user-defined scripts with custom, initial behaviour should be loaded.
            ///     Also see WorldT::InitFlagsT::InitFlag_OnlyStatic for related information.
            virtual void OnPostLoad(bool OnlyStatic) { }

            /// Advances the component one frame (one "clock-tick") on the server.
            /// It typically updates all game-relevant state that is sync'ed over the network to all
            /// connected game clients.
            ///
            /// Note that the implementation calls DoServerFrame() that derived classes can override to
            /// implement their own custom behaviour.
            ///
            /// @param t   The time in seconds since the last server frame.
            void OnServerFrame(float t);

            /// Advances the component one frame (one "clock-tick") on the client.
            /// It typically updates eye-candy that is *not* sync'ed over the network.
            /// (State that is sync'ed over the network must be updated in OnServerFrame() instead.)
            ///
            /// Note that the implementation calls DoClientFrame() that derived classes can override to
            /// implement their own custom behaviour.
            ///
            /// @param t   The time in seconds since the last client frame.
            void OnClientFrame(float t);


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int Get(lua_State* LuaState);
            static int Set(lua_State* LuaState);
            static int GetExtraMessage(lua_State* LuaState);
            static int Interpolate(lua_State* LuaState);
            static int GetEntity(lua_State* LuaState);
            static int InitClientApprox(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::MethsDocT DocCallbacks[];


            private:

            /// A helper structure for interpolations.
            struct InterpolationT
            {
                cf::TypeSys::VarBaseT* Var;         ///< The variable whose value is being interpolated.
                unsigned int           Suffix;      ///< If the variable is composed of several values, this is the index of the one being interpolated.
                float                  StartValue;  ///< Start value of the interpolation.
                float                  EndValue;    ///< End value of the interpolation.
                float                  CurrentTime; ///< Current time between 0 and TotalTime.
                float                  TotalTime;   ///< Duration of the interpolation.

                float GetCurrentValue() const { return StartValue + (EndValue-StartValue)*CurrentTime/TotalTime; }
            };


            /// Use of the Assignment Operator is not allowed (the method is declared, but left undefined).
            void operator = (const ComponentBaseT&);

            /// Derived classes override this method in order to write additional state data into the given stream.
            /// The method itself is automatically called from Serialize(), see Serialize() for more details.
            ///
            /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
            /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
            virtual void DoSerialize(cf::Network::OutStreamT& Stream) const { }

            /// Derived classes override this method in order to read additional state data from the given stream,
            /// or to run any post-deserialization code.
            /// The method itself is automatically called from Deserialize(), see Deserialize() for more details.
            ///
            /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
            /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
            virtual void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) { }

            /// Derived classes override this method in order to implement the real work proposed by OnServerFrame(),
            /// which explicitly calls this method for this purpose.
            ///
            /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
            /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
            virtual void DoServerFrame(float t) { }

            /// Derived classes override this method in order to implement the real work proposed by OnClientFrame(),
            /// which explicitly calls this method for this purpose.
            ///
            /// (This follows the "Non-Virtual Interface Idiom" as described by Scott Meyers in
            /// "Effective C++, 3rd Edition", item 35 ("Consider alternatives to virtual functions.").)
            virtual void DoClientFrame(float t) { }


            EntityT*                m_Entity;           ///< The parent entity that contains this component, or `NULL` if this component is currently not a part of any entity.
            TypeSys::VarManT        m_MemberVars;       ///< The variable manager that keeps generic references to our member variables.
            ArrayT<InterpolationT*> m_PendingInterp;    ///< The currently pending interpolations. These are usually set by scripts, in order to e.g. transfer a lift from A to B.
            ArrayT<ApproxBaseT*>    m_ClientApprox;     ///< The interpolators that advance values over client frames in order to bridge the larger intervals between server frames.
        };
    }
}

#endif
