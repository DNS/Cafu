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

#ifndef CAFU_DOCUMENT_ADAPTER_HPP_INCLUDED
#define CAFU_DOCUMENT_ADAPTER_HPP_INCLUDED

#include "Templates/Array.hpp"


class EditorMaterialI;
class MapDocumentT;
namespace cf { namespace TypeSys { class VarBaseT; } }
namespace GuiEditor { class GuiDocumentT; }
namespace ModelEditor { class ModelDocumentT; }


/// This class provides a common interface to the documents of the map, GUI or model editor.
/// Using this interface, it is possible to have shared code (like the material browser,
/// parts of the component system, and some commands) that can access a document without
/// knowing whether it deals with (and does its work for) a map, a GUI or a model document.
/// In summary, the shared code is nicely separated from the editor documents.
class DocAdapterI
{
    public:

    virtual const ArrayT<EditorMaterialI*>& GetMaterials() const=0;
    virtual void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const=0;
    virtual void OnMarkMaterial(EditorMaterialI* Mat) const=0;
    virtual void OnReplaceMaterial(EditorMaterialI* Mat) const=0;
    virtual void UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var)=0;
};


class MapDocAdapterT : public DocAdapterI
{
    public:

    MapDocAdapterT(MapDocumentT& MapDoc);

    const ArrayT<EditorMaterialI*>& GetMaterials() const;
    void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const;
    void OnMarkMaterial(EditorMaterialI* Mat) const;
    void OnReplaceMaterial(EditorMaterialI* Mat) const;
    void UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var);


    private:

    MapDocumentT& m_MapDoc;
};


class GuiDocAdapterT : public DocAdapterI
{
    public:

    GuiDocAdapterT(GuiEditor::GuiDocumentT& GuiDoc);

    const ArrayT<EditorMaterialI*>& GetMaterials() const;
    void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const;
    void OnMarkMaterial(EditorMaterialI* Mat) const;
    void OnReplaceMaterial(EditorMaterialI* Mat) const;
    void UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var);


    private:

    GuiEditor::GuiDocumentT& m_GuiDoc;
};


class ModelDocAdapterT : public DocAdapterI
{
    public:

    ModelDocAdapterT(ModelEditor::ModelDocumentT& ModelDoc);

    const ArrayT<EditorMaterialI*>& GetMaterials() const;
    void GetUsedMaterials(ArrayT<EditorMaterialI*>& UsedMaterials) const;
    void OnMarkMaterial(EditorMaterialI* Mat) const;
    void OnReplaceMaterial(EditorMaterialI* Mat) const;
    void UpdateAllObservers_VarChanged(const cf::TypeSys::VarBaseT& Var);


    private:

    ModelEditor::ModelDocumentT& m_ModelDoc;
};

#endif
