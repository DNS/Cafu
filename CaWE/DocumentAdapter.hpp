/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
