/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_PATTERN_HPP_INCLUDED
#define CAFU_COMMAND_PATTERN_HPP_INCLUDED

/// \file
/// This file provides the classes for the Command pattern as described in the book by the GoF.

#include "wx/wx.h"
#include "Templates/Array.hpp"


/// This class represents a general command for implementing modifications to the applications document.
/// Commands are usually kept in an undo/redo history in order to provide the user with undo and redo.
///
/// The use of commands - especially with an undo/redo history - implies several inherent "rules and considerations"
/// that must be followed and taken into account for a successful implementation.
/// - It is obvious that when undo/redo is desired, documents can ONLY be modified by a closed series of commands
///   that can be played forth and back in its respective, proper order only.
/// - VERY IMPORTANT: The time between the constructor and calling Do() is problematic, and should be null:
///   there should be NO code between the construction of a command and the related call to Do().
///   This is especially problematic when commands are (improperly) queued, e.g. while macro commands are being constructed.
///   As keeping the ctor empty is usually quasi impossible (we normally want to use it to "configure" the command), this
///   implies that we must make sure that the command is run and submitted to the undo/redo history IMMEDIATELY after construction.
///   As a help, both macro commands and the undo/redo history are able to accept commands whose Do() has already been run by the caller.
class CommandT
{
    public:

    /// The constructor.
    /// @param ShowInHistory If the command should be visible in the history.
    /// @param SuggestsSave  If the command should suggest that the document has to be saved
    ///                      if it is closed and the command has been executed without a
    ///                      following save.
    CommandT(bool ShowInHistory=true, bool SuggestsSave=true);

    /// The virtual destructor.
    virtual ~CommandT() {}

    /// This method executes the command.
    /// @returns true if the command succeeded, false if it failed.
    virtual bool Do()=0;

    /// This method un-does the command.
    virtual void Undo()=0;

    /// Returns the name (a description) of the command.
    virtual wxString GetName() const=0;

    /// Whether the command should be shown in the undo/redo history.
    /// Commands not shown are still part of the history but are not undoable/redoable
    /// by the user. Instead they are silently undone/redone when the preceding command
    /// is undone/redone.
    bool ShowInHistory() const { return m_ShowInHistory; }

    /// Whether the command suggests to save the document when its closed and hasn't
    /// been saved between the command execution and the closing (selection changes
    /// for example don't suggest to save the document since no important changes
    /// have been made).
    bool SuggestsSave() const { return m_SuggestsSave; }

    /// Whether the command has been executed.
    bool IsDone() const { return m_Done; }

    /// Returns the commands unique ID.
    unsigned long GetID() const { return m_ID; }


    protected:

    bool                m_Done;          ///< Whether the command has been executed.
    const bool          m_ShowInHistory; ///< Whether the command should have an entry in the undo/redo history.
    const bool          m_SuggestsSave;  ///< Whether the command suggests saving of the document on close.
    const unsigned long m_ID;            ///< The commands unique ID.


    private:

    /// Use of the Copy Constructor is not allowed. See "svn log -r 174" and the top of "svn cat -r 174 DocumentCommands.hpp" for a discussion.
    CommandT(const CommandT&);

    /// Use of the Assignment Operator is not allowed. See "svn log -r 174" and the top of "svn cat -r 174 DocumentCommands.hpp" for a discussion.
    void operator = (const CommandT&);
};


class CommandMacroT : public CommandT
{
    public:

    CommandMacroT(const ArrayT<CommandT*>& SubCommands, const wxString& Name="Macro Command", bool ShowInHistory=true, bool SuggestsSave=true);
    ~CommandMacroT();

    // Implement the CommandT interface.
    virtual bool     Do();
    virtual void     Undo();
    virtual wxString GetName() const;


    private:

    const ArrayT<CommandT*> m_SubCommands;  ///< The sub-commands that this macro is composed of.
    const wxString          m_Name;
};

#endif
