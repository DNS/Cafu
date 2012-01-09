/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _OBSERVER_PATTERN_HPP_
#define _OBSERVER_PATTERN_HPP_

/// \file
/// This file provides the classes for the Observer pattern as described in the book by the GoF.
/// Note that implementations of ObserverT normally maintain a pointer to the subject(s) that they observe,
/// e.g. in order to be able to redraw themselves also outside of and independent from the NotifySubjectChanged() method.
/// This however in turn creates problems when the life of the observer begins before or ends after the life of the observed subject(s).
/// In fact, it seems that the observers have to maintain lists of valid, observed subjects as the subjects maintain lists of observers.
/// Fortunately, these possibly tough problems can (and apparently must) be addressed by the concrete implementation classes of
/// observers and subjects, not by the base classes that the Observer pattern describes and provided herein.

#include "Templates/Array.hpp"
#include "Math3D/BoundingBox.hpp"

#include "wx/string.h"


class SubjectT;
class MapElementT;


//#####################################
//# New observer notifications hints. #
//#####################################
enum MapElemModDetailE
{
    MEMD_GENERIC,                   ///< Generic change of map elements (useful if the subject doesn't know what exactly has been changed).
 // MEMD_ENTITY_PROPERTY,
    MEMD_ENTITY_PROPERTY_CREATED,   ///< A new property has been created for an entity.
    MEMD_ENTITY_PROPERTY_DELETED,   ///< A property has been deleted from an entity.
    MEMD_ENTITY_PROPERTY_MODIFIED,  ///< An entities property has been modified.
    MEMD_ENTITY_CLASS_CHANGED,      ///< An entities class has changed.
    MEMD_TRANSFORM,                 ///< A map element has been transformed.
    MEMD_PRIMITIVE_PROPS_CHANGED,   ///< The properties of a map primitve have been modified.
    MEMD_SURFACE_INFO_CHANGED,      ///< The surface info of a map element has changed. Note that surface info changes also include the selection of faces.
    MEMD_MORPH,                     ///< The vertices of a map primitive have been transformed. This is actually UNUSED now that \c CommandMorphT has been removed (r447, 2011-12-20).
    MEMD_ASSIGN_PRIM_TO_ENTITY,     ///< A map primitive has been assigned to another entity (the world or any custom entity).
    MEMD_VISIBILITY                 ///< The visibility of a map element has changed.
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

    /// Notifies the observer that one or more map elements have been created.
    /// @param Subject The map document in which the elements have been created.
    /// @param MapElements List of created map elements.
    virtual void NotifySubjectChanged_Created(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements) { }

    /// Notifies the observer that one or more map elements have been deleted.
    /// @param Subject The map document in which the elements have been deleted.
    /// @param MapElements List of deleted map elements.
    virtual void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements) { }

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
    ///                      Can be one of MEMD_TRANSFORM, MEMD_PRIMITIVE_PROPS_CHANGED and MEMD_MORPH.
    ///                      In the case of MEMD_PRIMITIVE_PROPS_CHANGED one can assume that
    ///                      only map primitives (no entities) are in the MapElements array.
    /// @param OldBounds     Holds the bounds of each objects BEFORE the modification (has the same size as MapElements).
    virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds) { }

    /// Since this overload is used exclusively for entities, one can assume that all map elements are entities.
    /// @param Subject       The map document in which the elements have been modified.
    /// @param MapElements   List of modified map elements.
    /// @param Detail        Information about what has been modified:
    ///                      Can be one of MEMD_ENTITY_PROPERTY_CREATED, MEMD_ENTITY_PROPERTY_DELETED, MEMD_ENTITY_PROPERTY_MODIFIED or MEMD_ENTITY_CLASS_CHANGED.
    /// @param Key           Holds the keyname of the changed property or an empty string if Detail is MEMD_ENTITY_CLASS_CHANGED.
    /// @todo move MEMD_ENTITY_CLASS_CHANGED into general modification method.
    virtual void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key) { }
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
    virtual void RegisterObserver(ObserverT* Obs);

    /// Unregisters the observer Obs from further notification on updates of this class.
    /// \param Obs   The observer that is to be unregistered.
    virtual void UnregisterObserver(ObserverT* Obs);


    //###################################################################################################################################
    //# New observer notifications methods. These methods differ from the basic observer pattern from the GoF and are CaWE specific!    #
    //# For detailed comments on the parameters, see the equivalent methods in ObserverT                                                #
    //###################################################################################################################################
    virtual void UpdateAllObservers(MapDocOtherDetailT OtherDetail);
    virtual void UpdateAllObservers_SelectionChanged(const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    virtual void UpdateAllObservers_GroupsChanged();
    virtual void UpdateAllObservers_Created(const ArrayT<MapElementT*>& MapElements);
    virtual void UpdateAllObservers_Deleted(const ArrayT<MapElementT*>& MapElements);
    virtual void UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    virtual void UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    virtual void UpdateAllObservers_Modified(const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key);
    //############################################
    //# End of new observer notification methods #
    //############################################


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
