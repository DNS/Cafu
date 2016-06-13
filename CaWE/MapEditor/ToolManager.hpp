/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_MANAGER_HPP_INCLUDED
#define CAFU_TOOL_MANAGER_HPP_INCLUDED

#include "ObserverPatternTools.hpp"
#include "Templates/Array.hpp"


class MapDocumentT;
class ToolT;
class wxWindow;
namespace cf { namespace TypeSys { class TypeInfoT; } }


class ToolManagerT : public ToolsSubjectT
{
    public:

    /// The constructor.
    ToolManagerT(MapDocumentT& MapDoc, wxWindow* ParentToolOptions);

    /// The destructor.
    ~ToolManagerT();


    /// Returns the array of all tools registered with and known to the tool manager.
    const ArrayT<ToolT*>& GetTools() const { return m_Tools; }

    /// Returns the tool instance matching the given type.
    /// NULL is returned when Type is not a type of a valid tool.
    ToolT* GetTool(const cf::TypeSys::TypeInfoT& Type);

    /// Returns the currently active tool.
    ToolT* GetActiveTool() const { return m_ActiveTool; }

    /// Returns the type of the currently active tool.
    /// This method is just a convenience method that saves the caller the explicit check for NULL when calling GetActiveTool().
    const cf::TypeSys::TypeInfoT* GetActiveToolType() const;

    /// Sets the tool of the given type as the currently active tool.
    void SetActiveTool(const cf::TypeSys::TypeInfoT* Type);


    private:

    ToolManagerT(const ToolManagerT&);          ///< Use of the Copy Constructor    is not allowed.
    void operator = (const ToolManagerT&);      ///< Use of the Assignment Operator is not allowed.


    MapDocumentT&  m_MapDoc;        ///< The MapDocumentT whose tool manager we are.
    ArrayT<ToolT*> m_Tools;         ///< The list of all tools available for editing the map document.
    ToolT*         m_ActiveTool;    ///< One of the tools in m_Tools, this is the currently active one. NULL when no tool is currently active.
};

#endif
