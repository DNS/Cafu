/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _MAP_HELPER_MODEL_HPP_
#define _MAP_HELPER_MODEL_HPP_

#include "MapHelper.hpp"
#include "Models/Model_proxy.hpp"
#include "Util/Util.hpp"


class HelperInfoT;
class MapEntityT;
class Renderer2DT;
class Renderer3DT;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }


class MapHelperModelT : public MapHelperT
{
    public:

    /// The constructor.
    MapHelperModelT(const MapEntityT* ParentEntity, const HelperInfoT* HelperInfo);

    /// The copy constructor for copying a model.
    /// @param Model   The model to copy-construct this model from.
    MapHelperModelT(const MapHelperModelT& Model);


    // Implementations and overrides for base class methods.
    MapHelperModelT* Clone() const;
    void             Assign(const MapElementT* Elem);

    wxString GetDescription() const { return "model"; }
    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    void UpdateModelCache() const;
    int  GetSequenceNr() const;

    const HelperInfoT*  m_HelperInfo;       ///< The HelperInfoT instance that caused the instantiation of this helper.
    mutable ModelProxyT m_ModelProxy;       ///< Our model (proxy).
    mutable float       m_ModelFrameNr;     ///< The frame number of the sequence to render this model in.
    mutable TimerT      m_Timer;
};

#endif
