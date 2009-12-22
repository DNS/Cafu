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

#ifndef _MAP_ELEMENT_HPP_
#define _MAP_ELEMENT_HPP_

#include "Group.hpp"

#include "Math3D/Angles.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Templates/Array.hpp"
#include "TypeSys.hpp"

#include "wx/string.h"

#include <ostream>


class MapElementT;
class Renderer2DT;
class Renderer3DT;
class TextParserT;
class ViewWindow2DT;
class MatrixT;


/// The TypeInfoTs of all MapElementT derived classes must register with this TypeInfoManT instance.
cf::TypeSys::TypeInfoManT& GetMapElemTIM();


class MapElementT
{
    public:

    /// The default constructor.
    MapElementT(const wxColour& Color);

    /// The copy constructor for copying a map element.
    ///
    /// @param Elem   The element to copy-construct this element from.
    ///
    /// Contrary to Elem, the new MapElementT instance is:
    ///   - never selected (no matter if Elem was selected),
    ///   - not in a group (and thus visible).
    /// It is up to the caller to assign selection state and group membership if desired.
    /// The new element is always constructed with a deep-copy (that is, the children of Elem are copied recursively).
    MapElementT(const MapElementT& Elem);

    /// The virtual destructor.
    virtual ~MapElementT() { }


    /// The virtual copy constructor.
    /// Creates a copy of this element that is of the *same* class as the original, even when called
    /// via a base class pointer (the caller doesn't even need to know the exact derived class).
    virtual MapElementT* Clone() const=0;

    /// Assigns the given element to this element.
    ///
    /// @param Elem   The element that is to be assigned to this element.
    ///
    /// Elem must be of the exact same type as this element (e.g. as obtained by the Clone() method),
    /// or else the assignment will silently succeed only partially (or not at all),
    /// without explicit notice of the failure (except for built-in debug asserts).
    ///
    /// Why did we not override operator = instead?
    /// Having a virtual assignment operator is highly confusing and typically doesn't work as expected.
    /// See http://www.icu-project.org/docs/papers/cpp_report/the_assignment_operator_revisited.html for details.
    /// Among the many problems, note that the different semantics between this method (it just does a "best try")
    /// and a true assignment operator (which makes the left object computationally equivalent to the right)
    /// is the biggest one.
    virtual void Assign(const MapElementT* Elem);


    virtual void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc);
    virtual void Save_cmap(std::ostream& OutFile, unsigned long ElemNr, const MapDocumentT& MapDoc) const;


    /// Returns whether this element is currently selected in the map document.
    bool IsSelected() const { return m_IsSelected; }

    /// Sets the selection state of this element. As this should always be in sync with the map document,
    /// the map document is the only legitimate caller of this method (but there are some exceptions).
    void SetSelected(bool Selected=true) { m_IsSelected=Selected; }


    /// This method returns the "inherent" color of this map element.
    /// The returned color should be used for rendering the map element whenever no better
    /// (e.g. texture-mapped) alternative is available.
    ///
    /// @param ConsiderGroup   Whether the map elements group color should be taken into account.
    /// @returns the "inherent" color of this map element.
    ///   When the map element is in a group and ConsiderGroup is true, the group color is returned.
    ///   When the map element is an entity, the color of the entity class is returned.
    ///   Otherwise (the element is a primitive), when the map element is part of an entity, the entity color
    ///   is returned. Finally (it's a map primitive that is in the world), its native color is returned.
    virtual wxColour GetColor(bool ConsiderGroup=true) const;

    /// Returns whether this map element is (entirely or partially) translucent.
    /// Translucent map elements are typically implemented with "alpha blending" and require rendering in back-to-front order.
    /// @see EditorMaterialI::IsTranslucent()
    virtual bool IsTranslucent() const { return false; }

    virtual wxString GetDescription() const { return ""; }


    GroupT* GetGroup() const { return m_Group; }    ///< Returns NULL when this map element is in no group, or the pionter to the group it is a member of otherwise.
    void SetGroup(GroupT* Group) { m_Group=Group; } ///< Sets the group this element is a member of (use NULL for "no group").
    bool IsVisible() const { return !m_Group || m_Group->IsVisible; }   ///< Returns whether this map element is visible (in the 2D, 3D and other views). Note that the visibility does not depend on the visibility of the parent entity - in CaWE, map elements are mostly independent of their parents (and thus entities can also be "half visible" if the user wishes so).


    /// This is periodically called in order to have the element advance its internal clock by t seconds.
    /// The typical use case is with elements that represent models for updating the current frame in their animation sequence.
    virtual void AdvanceTime(float t) { }

    virtual void Render2D(Renderer2DT& Renderer) const { }
    virtual void Render3D(Renderer3DT& Renderer) const { }

    /// Returns the spatial bounding-box of this map element.
    virtual BoundingBox3fT GetBB() const=0;

    /// Traces a ray against this map element, and returns whether it was hit.
    /// The ray for the trace is defined by RayOrigin + RayDir*Fraction, where Fraction is a scalar >= 0.
    /// If a hit was detected, the Fraction is returned. Hit brushes return the number of the hit face as well.
    /// This method has been implemented mainly for "picking", that is, left-click selection in the 3D views
    /// (it makes sure that also objects that "clip nothing" in the engine can be picked), but it can also be used for any other purpose.
    ///
    /// @param RayOrigin   The point in world space where the ray starts.
    /// @param RayDir      A unit vector in world space that describes the direction the ray extends to.
    /// @param Fraction    On hit, the scalar along RayDir at which the hit occurred is returned here.
    /// @param FaceNr      If this map element is a brush and it was hit, the number of the hit face is returned here.
    ///
    /// @returns true if the ray hit this map element, false otherwise. Additional hit data (i.e. Fraction and FaceNr)
    ///          is returned via reference paramaters.
    virtual bool TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const;

    /// This method determines if this map element is intersected/affected by the specified disc in ViewWin.
    /// The disc for the test is defined by the given center pixel and the given radius.
    /// For example, the caller can learn by the result of this method whether the map element should respond to a mouse-click
    /// at the same pixel. Therefore, this method can be considered as the 2D analogue of the TraceRay() method.
    virtual bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;

    /// Translates this element by the given vector.
    /// @param Delta   The offset by which to translate the element.
    virtual void TrafoMove(const Vector3fT& Delta) { }

    /// Rotates this element about the given reference point.
    /// @param RefPoint   The reference point (origin) for the rotation.
    /// @param Angles     The rotation angles for the three axes.
    virtual void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles) { }

    /// Scales this element about the given reference point.
    /// @param RefPoint   The reference point (origin) for the scale.
    /// @param Scale      The scale factors for the three axes.
    /// @throws DivisionByZeroE, e.g. when Scale is too small and the element becomes degenerate (e.g. a brush with too small faces).
    virtual void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale) { }

    /// Mirrors this element along the given mirror plane.
    /// @param NormalAxis   The number of the axis along which the normal vector of the mirror plane points: 0, 1 or 2 for the x-, y- or z-axis respectively.
    /// @param Dist         The position of the mirror plane along its normal vector, where it intersects the NormalAxis.
    /// Note that the mirroring is not necessarily "perfect", because for some elements like models or plants,
    /// only their point of origin can be mirrored, but not their mesh.
    virtual void TrafoMirror(unsigned int NormalAxis, float Dist) { }

    /// Why does this method not replace all the other Trafo*() methods?
    /// This method is the most generic, allowing transformations that e.g. are non-orthogonal (like shears or non-uniform scales).
    /// This in turn conflicts with map primitives that can only store and deal with a restricted fixed set of transformations,
    /// e.g. an origin, a rotation and a uniform scale. These values cannot properly be re-computed from a general matrix with
    /// non-orthogonal basis vectors.
    virtual void Transform(const MatrixT& Matrix) { }


    unsigned int GetFrameCount() const { return m_FrameCount; }
    void SetFrameCount(unsigned int FrameCount) { m_FrameCount=FrameCount; }


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    bool         m_IsSelected;  ///< Is this element currently selected in the map document?
    wxColour     m_Color;       ///< The color of this element.
    GroupT*      m_Group;       ///< The group this element is in, NULL if in no group.
    unsigned int m_FrameCount;  ///< The number of the frame in which this element was last rendered in a 3D view, used in order to avoid processing/rendering it twice.
};

#endif
