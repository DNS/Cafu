/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_MODIFY_MODEL_HPP_INCLUDED
#define CAFU_COMMAND_MODIFY_MODEL_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "Models/AnimExpr.hpp"


class MapDocumentT;
class MapModelT;


class CommandModifyModelT : public CommandT
{
    public:

    CommandModifyModelT(MapDocumentT& MapDoc, MapModelT* Model, const wxString& ModelFileName,
        const wxString& CollisionModelFileName, const wxString& Label, float Scale,
        IntrusivePtrT<AnimExprStandardT> AnimExpr, float FrameTimeOff, float FrameTimeScale, bool Animated);

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&                    m_MapDoc;
    MapModelT*                       m_Model;
    const wxString                   m_NewModelFileName;
    const wxString                   m_OldModelFileName;
    const wxString                   m_NewCollModelFileName;
    const wxString                   m_OldCollModelFileName;
    const wxString                   m_NewLabel;
    const wxString                   m_OldLabel;
    float                            m_NewScale;
    float                            m_OldScale;
    IntrusivePtrT<AnimExprStandardT> m_NewAnimExpr;
    IntrusivePtrT<AnimExprStandardT> m_OldAnimExpr;
    float                            m_NewFrameTimeOff;
    float                            m_OldFrameTimeOff;
    float                            m_NewFrameTimeScale;
    float                            m_OldFrameTimeScale;
    bool                             m_NewAnimated;
    bool                             m_OldAnimated;
};

#endif
