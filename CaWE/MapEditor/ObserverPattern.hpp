/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_OBSERVER_PATTERN_HPP_INCLUDED
#define CAFU_OBSERVER_PATTERN_HPP_INCLUDED

/// \file
/// This file provides the classes for the Observer pattern as described in the book by the GoF.
/// Note that implementations of ObserverT normally maintain a pointer to the subject(s) that they observe,
/// e.g. in order to be able to redraw themselves also outside of and independent from the NotifySubjectChanged() method.
/// This however in turn creates problems when the life of the observer begins before or ends after the life of the observed subject(s).
/// In fact, it seems that the observers have to maintain lists of valid, observed subjects as the subjects maintain lists of observers.
/// Fortunately, these possibly tough problems can (and apparently must) be addressed by the concrete implementation classes of
/// observers and subjects, not by the base classes that the Observer pattern describes and provided herein.

#include "Math3D/BoundingBox.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include "wx/string.h"


namespace cf { namespace GameSys { class EntityT; } }
namespace cf { namespace TypeSys { class VarBaseT; } }
namespace MapEditor { class CompMapEntityT; }
class SubjectT;
class MapElementT;
class MapPrimitiveT;


//#####################################
//# New observer notifications hints. #
//#####################################
enum MapElemModDetailE
{
    MEMD_GENERIC,                   ///< Generic change of map elements (useful if the subject doesn't know what exactly has been changed).
    MEMD_TRANSFORM,                 ///< A map element has been transformed.
    MEMD_PRIMITIVE_PROPS_CHANGED,   ///< The properties of a map primitive have been modified.
    MEMD_SURFACE_INFO_CHANGED,      ///< The surface info of a map element has changed. Note that surface info changes also include the selection of faces.
    MEMD_ASSIGN_PRIM_TO_ENTITY,     ///< A map primitive has been assigned to another entity (the world or any custom entity).
    MEMD_VISIBILITY                 ///< The visibility of a map element has changed.
};

enum EntityModDetailE
{
    EMD_COMPONENTS,     ///< The set of components has changed (e.g. added, deleted, order changed).
    EMD_HIERARCHY       ///< The position of an entity in the entity hierarchy has changed.
};

enum MapDocOtherDetailT
{
    UPDATE_GRID,
    UPDATE_POINTFILE,
    UPDATE_GLOBALOPTIONS
};
//############################################
//# End of new observer notifications hints. #
//############################################


class ObserverT
{
    public:

    //###################################################################################################################################
    //# New observer notifications methods. These methods differ from the basic observer pattern from the GoF and are CaWE specific!    #
    //# Note that the new methods are NOT pure virtual since a concrete observer might not be interested in some of these notifications #
    //###################################################################################################################################

    /// Notifies the observer that some other detail than those specifically addressed below has changed.
    virtual void NotifySubjectChanged(SubjectT* Subject, MapDocOtherDetailT OtherDetail) { }

    /// Notifies the observer that the selection in the current subject has been changed.
    /// @param Subject The map document in which the selection has been changed.
    /// @param OldSelection Array of the previously selected objects.
    /// @param NewSelection Array of the new selected objects.
    virtual void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection) { }

    /// Notifies the observer that the groups in the current subject have been changed (new group added, group deleted, visibility changed, anything).
    /// @param Subject The map document in which the group inventory has been changed.
    virtual void NotifySubjectChanged_Groups(SubjectT* Subject) { }

    /// Notifies the observer that one or more entities have been created.
    /// @param Subject    The map document in which the entities have been created.
    /// @param Entities   List of created entities.
    virtual void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) { }

    /// Notifies the observer that one or more map primitives have been created.
    /// @param Subject      The map document in which the primitives have been created.
    /// @param Primitives   List of created map primitives.
    virtual void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) { }

    /// Notifies the observer that one or more entities have been deleted.
    /// @param Subject    The map document in which the entities have been deleted.
    /// @param Entities   List of deleted entities.
    virtual void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities) { }

    /// Notifies the observer that one or more map primitives have been deleted.
    /// @param Subject      The map document in which the primitives have been deleted.
    /// @param Primitives   List of deleted map primitives.
    virtual void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives) { }

    /// @name Modification notification method and overloads.
    /// These methods notify the observer that one or more map elements have been modified.
    /// The first 3 parameters are common to all of these methods since they are the basic information needed to communicate
    /// an object modification.
    //@{

    /// @param Subject       The map document in which the elements have been modified.
    /// @param MapElements   List of modified map elements.
    /// @param Detail        Information about what has been modified:
    ///                      Can be one of MEMD_PRIMITIVE_PROPS_CHANGED, MEMD_GENERIC, MEMD_ASSIGN_PRIM_TO_ENTITY and MEMD_MATERIAL_CHANGED.
    virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail) { }

    /// @param Subject       The map document in which the elements have been modified.
    /// @param MapElements   List of modified map elements.
    /// @param Detail        Information about what has been modified:
    ///                      Can be MEMD_TRANSFORM or MEMD_PRIMITIVE_PROPS_CHANGED.
    ///                      In the case of MEMD_PRIMITIVE_PROPS_CHANGED one can assume that
    ///                      only map primitives (no entities) are in the MapElements array.
    /// @param OldBounds     Holds the bounds of each objects BEFORE the modification (has the same size as MapElements).
    virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds) { }

    /// @param Subject    The map document in which the entities have been modified.
    /// @param Entities   List of modified map entities.
    /// @param Detail     Information about what has been modified.
    virtual void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail) { }

    /// Notifies the observer that a variable has changed.
    /// @param Subject   The map document in which a variable has changed.
    /// @param Var       The variable whose value has changed.
    virtual void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var) { }
    //@}

    //############################################
    //# End of new observer notification methods #
    //############################################


    /// This method is called whenever a subject is about the be destroyed (and become unavailable).
    /// \param dyingSubject   The subject that is being destroyed.
    virtual void NotifySubjectDies(SubjectT* dyingSubject)=0;

    /// The virtual destructor.
    virtual ~ObserverT();


    protected:

    /// The constructor. It is protected so that only derived classes can create instances of this class.
    ObserverT();
};


class SubjectT
{
    public:

    /// Registers the observer Obs for notification on updates of this class.
    /// \param Obs   The observer that is to be registered.
    void RegisterObserver(ObserverT* Obs);

    /// Unregisters the observer Obs from further notification on updates of this class.
    /// \param Obs   The observer that is to be unregistered.
    void UnregisterObserver(ObserverT* Obs);


    //###################################################################################################################################
    //# New observer notifications methods. These methods differ from the basic observer pattern from the GoF and are CaWE specific!    #
    //# For detailed comments on the parameters, see the equivalent methods in ObserverT                                                #
    //###################################################################################################################################
    void UpdateAllObservers(MapDocOtherDetailT OtherDetail);
    void UpdateAllObservers_SelectionChanged(const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void UpdateAllObservers_GroupsChanged();
    void UpdateAllObservers_Created(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void UpdateAllObservers_Created(const ArrayT<MapPrimitiveT*>& Primitives);
    void UpdateAllObservers_Deleted(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entites);
    void UpdateAllObservers_Deleted(const ArrayT<MapPrimitiveT*>& Primitives);
    void UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void UpdateAllObservers_EntChanged(const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail);
    void UpdateAllObservers_EntChanged(const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities, EntityModDetailE Detail);
    void UpdateAllObservers_EntChanged(IntrusivePtrT<cf::GameSys::EntityT> Entity, EntityModDetailE Detail);
    void UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var);
    void UpdateAllObservers_SubjectDies();

    /// The virtual destructor.
    virtual ~SubjectT();


    protected:

    /// The constructor. It is protected so that only derived classes can create instances of this class.
    SubjectT();


    private:

    /// The list of the observers that have registered for notification on updates of this class.
    ArrayT<ObserverT*> m_Observers;
};

#endif
