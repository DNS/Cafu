/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_VAR_VISITORS_PROPERTIES_HPP_INCLUDED
#define CAFU_VAR_VISITORS_PROPERTIES_HPP_INCLUDED

#include "Variables.hpp"


namespace cf { namespace TypeSys { class TypeInfoT; } }
class CommandT;
class DocAdapterI;
class wxPGProperty;
class wxPropertyGridEvent;
class wxPropertyGridManager;


/// This visitor creates a wxPGProperty for the visited variable
/// and adds it to the given wxPropertyGridManager.
class VarVisitorAddPropT : public cf::TypeSys::VisitorT
{
    public:

    VarVisitorAddPropT(wxPropertyGridManager& PropMan, DocAdapterI& DocAdapter, const cf::TypeSys::TypeInfoT* TI);

    void visit(cf::TypeSys::VarT<float>& Var);
    void visit(cf::TypeSys::VarT<double>& Var);
    void visit(cf::TypeSys::VarT<int>& Var);
    void visit(cf::TypeSys::VarT<unsigned int>& Var);
    void visit(cf::TypeSys::VarT<uint16_t>& Var);
    void visit(cf::TypeSys::VarT<uint8_t>& Var);
    void visit(cf::TypeSys::VarT<bool>& Var);
    void visit(cf::TypeSys::VarT<std::string>& Var);
    void visit(cf::TypeSys::VarT<Vector2fT>& Var);
    void visit(cf::TypeSys::VarT<Vector3fT>& Var);
    void visit(cf::TypeSys::VarT<Vector3dT>& Var);
    void visit(cf::TypeSys::VarT<BoundingBox3dT>& Var);
    void visit(cf::TypeSys::VarArrayT<uint32_t>& Var);
    void visit(cf::TypeSys::VarArrayT<uint16_t>& Var);
    void visit(cf::TypeSys::VarArrayT<uint8_t>& Var);
    void visit(cf::TypeSys::VarArrayT<std::string>& Var);


    private:

    void SetHelpString(wxPGProperty* Prop, wxPGProperty* SourceProp = NULL) const;

    wxPropertyGridManager&        m_PropMan;
    DocAdapterI&                  m_DocAdapter;
    const cf::TypeSys::TypeInfoT* m_TI;
};


/// This visitor updates the value of the given property `Prop` to the value of the visited variable.
class VarVisitorUpdatePropT : public cf::TypeSys::VisitorConstT
{
    public:

    VarVisitorUpdatePropT(wxPGProperty& Prop);

    void visit(const cf::TypeSys::VarT<float>& Var);
    void visit(const cf::TypeSys::VarT<double>& Var);
    void visit(const cf::TypeSys::VarT<int>& Var);
    void visit(const cf::TypeSys::VarT<unsigned int>& Var);
    void visit(const cf::TypeSys::VarT<uint16_t>& Var);
    void visit(const cf::TypeSys::VarT<uint8_t>& Var);
    void visit(const cf::TypeSys::VarT<bool>& Var);
    void visit(const cf::TypeSys::VarT<std::string>& Var);
    void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
    void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
    void visit(const cf::TypeSys::VarT<Vector3dT>& Var);
    void visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var);
    void visit(const cf::TypeSys::VarArrayT<uint32_t>& Var);
    void visit(const cf::TypeSys::VarArrayT<uint16_t>& Var);
    void visit(const cf::TypeSys::VarArrayT<uint8_t>& Var);
    void visit(const cf::TypeSys::VarArrayT<std::string>& Var);


    private:

    wxPGProperty& m_Prop;
};


/// This visitor creates a command for updating the value of the visited variable according to
/// the given EVT_PG_CHANGING event.
class VarVisitorHandlePropChangingEventT : public cf::TypeSys::VisitorT
{
    public:

    VarVisitorHandlePropChangingEventT(wxPropertyGridEvent& Event, unsigned int Depth, DocAdapterI& DocAdapter);
    ~VarVisitorHandlePropChangingEventT();

    CommandT* TransferCommand();

    void visit(cf::TypeSys::VarT<float>& Var);
    void visit(cf::TypeSys::VarT<double>& Var);
    void visit(cf::TypeSys::VarT<int>& Var);
    void visit(cf::TypeSys::VarT<unsigned int>& Var);
    void visit(cf::TypeSys::VarT<uint16_t>& Var);
    void visit(cf::TypeSys::VarT<uint8_t>& Var);
    void visit(cf::TypeSys::VarT<bool>& Var);
    void visit(cf::TypeSys::VarT<std::string>& Var);
    void visit(cf::TypeSys::VarT<Vector2fT>& Var);
    void visit(cf::TypeSys::VarT<Vector3fT>& Var);
    void visit(cf::TypeSys::VarT<Vector3dT>& Var);
    void visit(cf::TypeSys::VarT<BoundingBox3dT>& Var);
    void visit(cf::TypeSys::VarArrayT<uint32_t>& Var);
    void visit(cf::TypeSys::VarArrayT<uint16_t>& Var);
    void visit(cf::TypeSys::VarArrayT<uint8_t>& Var);
    void visit(cf::TypeSys::VarArrayT<std::string>& Var);


    private:

    wxPropertyGridEvent& m_Event;
    const unsigned int   m_Depth;
    DocAdapterI&         m_DocAdapter;
    CommandT*            m_Command;
};

#endif
