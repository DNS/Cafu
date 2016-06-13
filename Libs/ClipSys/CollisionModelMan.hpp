/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_COLLISION_MODEL_MANAGER_HPP_INCLUDED
#define CAFU_CLIPSYS_COLLISION_MODEL_MANAGER_HPP_INCLUDED

#include <string>

#include "Math3D/Vector3.hpp"
#include "Math3D/BoundingBox.hpp"


class MaterialT;


namespace cf
{
    namespace SceneGraph { namespace aux { class PoolT; } }


    namespace ClipSys
    {
        class CollisionModelT;


        /// This class provides (an interface to) the creation, management and destruction of collision models.
        /// It employs reference counting for collision models that are loaded from file.
        /// The interface is specified as an ABC in order to be able to share it across exe/dll boundaries.
        ///
        /// The important feature is that it allows the flexible, care-free handling of collision models:
        /// no matter where a model "comes from" (loaded file, file stream, bounding box, ...), it is created
        /// and deleted in a uniform manner, which in turn is an important feature for the game (entity) code.
        /// The const-ness of the returned pointers is obviously a consequence of the instance-sharing implied
        /// by the reference counting: You can inspect, but not mutate/modify shared objects.
        class CollModelManI
        {
            public:

            /// The virtual destructor.
            virtual ~CollModelManI() { }

            /// Loads a collision model from the file specified by FileName.
            /// If the collision model has been loaded before, reference counting is employed.
            /// @param FileName   The name of the file to load the collision model from.
            /// @returns (a pointer to) the collision model instance that has been loaded from the given file, or NULL on failure.
            virtual const CollisionModelT* GetCM(const std::string& FileName) = 0;

            // /// Loads a collision model from the specified instream.
            // /// @param InFile   The instream to load the collision model from.
            // /// @param Pool     Pool of vectors and strings used multiple times in InFile.
            // /// @returns the loaded collision model instance.
            // /// TODO: Es wäre SUPER wenn wir in InFile irgendwie einen unique Name o. Hash speichern könnten!!!!
            // ///       (Denn Sv und Cl laden ja unabh. voneinander dasselbe .cw world file. Ein gemeinsamer Hash würde eine einzige Instanz sharen.)
            // virtual const CollisionModelT* GetCM(std::istream& InFile, SceneGraph::aux::PoolT& Pool, const ArrayT<CollisionModelStaticT::TerrainRefT>& Terrains)=0;

            /// Creates a collision model from the given explicit mesh.
            ///
            /// @param Width      The width of the mesh.
            /// @param Height     The height of the mesh.
            /// @param Mesh       The vertices of the mesh.
            /// @param Material   The material reported in collision results when a trace hit.
            /// @param MIN_NODE_SIZE   The minimum size (side length) that a node should not fall below.
            ///
            /// @returns the matching collision model instance.
            virtual const CollisionModelT* GetCM(unsigned long Width, unsigned long Height, const ArrayT<Vector3dT>& Mesh, MaterialT* Material, const double MIN_NODE_SIZE) = 0;

            /// Creates a collision model from the given (axis-aligned) bounding-box.
            /// @param BB   The bounding box to create a collision model for.
            /// @param Material   The material to use for the collision model.
            /// @returns the desired collision model.
            virtual const CollisionModelT* GetCM(const BoundingBox3T<double>& BB, MaterialT* Material) = 0;

            /// Creates another collision model from a given collision model.
            /// a) If the given collision model has been created with other methods of this CollModelManI earlier,
            ///    the implementation will simply increase the related reference counter and return the same pointer.
            /// b) If the given collision model has been created "externally" with some other means (e.g. direct instantiation
            ///    of a concrete collision model class), the CollModelManI will create a record about it with a reference count of 1,
            ///    assume that the original instance is not freed/deleted by the caller as long as it has a non-zero reference count,
            ///    and will not attempt to delete the instance when the count finally drops to 0.
            /// @param CollisionModel   The collision model to create another collision model from.
            /// @returns a matching collision model instance.
            virtual const CollisionModelT* GetCM(const CollisionModelT* CollisionModel) = 0;


            /// Returns the file name the given collision model has been loaded from (using the GetCM(const std::string& FileName) method).
            /// @param CollisionModel   The collision model for which return the associated file name.
            /// @returns the file name of the collision model, or "" (the empty string) if the model was created by another method.
            virtual const std::string& GetFileName(const CollisionModelT* CollisionModel) const = 0;

            /// Frees the given collision model (taking reference counting into account if necessary).
            /// @param CollisionModel   The collision model to be freed.
            virtual void FreeCM(const CollisionModelT* CollisionModel) = 0;

            /// Returns the number of unique, physical collision model instances managed by this class.
            virtual unsigned long GetUniqueCMCount() const = 0;
        };


        /// A global pointer to an implementation of the CollModelManI interface.
        ///
        /// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the ClipSys library).
        /// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
        ///     cf::ClipSys::CollModelManI* cf::FileSys::CollModelMan=NULL;
        /// or else the module will not link successfully due to an undefined symbol.
        ///
        /// Exe files will then want to reset this pointer to an instance of a CollModelManImplT during their initialization
        /// e.g. by code like:   cf::ClipSys::CollModelMan=new cf::ClipSys::CollModelManImplT;
        /// Note that the CollModelManImplT ctor may require that other interfaces (e.g. the Console) have been inited first.
        ///
        /// Dlls typically get one of their init functions called immediately after they have been loaded.
        /// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its CollModelMan variable.
        extern CollModelManI* CollModelMan;
    }
}

#endif
