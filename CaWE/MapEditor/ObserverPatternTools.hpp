/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/// @file
/// This file provides the classes for the Observer pattern as described in the book by the GoF.
/// These classes are specific to the tools. See the map document related observer classes for more details.
#ifndef CAFU_OBSERVER_PATTERN_TOOLS_HPP_INCLUDED
#define CAFU_OBSERVER_PATTERN_TOOLS_HPP_INCLUDED

#include "Templates/Array.hpp"


class ToolT;
class ToolsSubjectT;


/// An enumeration that determines the urgency with which observers should update themselves when their subject has changed.
enum ToolsUpdatePriorityT
{
    UPDATE_NOW,     ///< Update immediately, in real-time, before the function returns.
    UPDATE_SOON     ///< Update as soon as convenient, at the next opportunity.
};


class ToolsObserverT
{
    public:

    /// Notifies the observer that the current subject has been changed.
    /// @param Subject    The tools (tool manager) in which something has changed.
    /// @param Tool       The specific tool that has changed.
    /// @param Priority   The priority with which the update should be implemented.
    virtual void NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority) { }

    /// The virtual destructor.
    virtual ~ToolsObserverT() { }


    protected:

    /// The constructor. It is protected so that only derived classes can create instances of this class.
    ToolsObserverT() { }
};


class ToolsSubjectT
{
    public:

    /// Registers the observer Obs for notification on changes in this class.
    /// @param Obs   The observer that is to be registered.
    void RegisterObserver(ToolsObserverT* Obs);

    /// Unregisters the observer Obs from further notification on changes in this class.
    /// @param Obs   The observer that is to be unregistered.
    void UnregisterObserver(ToolsObserverT* Obs);

    /// Notifies all observers that something in the given tool changed so that they update themselves.
    /// @param Tool       The specific tool that has changed.
    /// @param Priority   The priority with which the update should be implemented.
    void UpdateAllObservers(ToolT* Tool, ToolsUpdatePriorityT Priority);

    /// The virtual destructor.
    virtual ~ToolsSubjectT();


    protected:

    /// The constructor. It is protected so that only derived classes can create instances of this class.
    ToolsSubjectT();


    private:

    /// The list of the observers that have registered for notification on updates of this class.
    ArrayT<ToolsObserverT*> m_Observers;
};

#endif
