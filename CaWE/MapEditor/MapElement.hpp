/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_ELEMENT_HPP_INCLUDED
#define CAFU_MAP_ELEMENT_HPP_INCLUDED

#include "Math3D/Angles.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"
#include "TypeSys.hpp"

#include "wx/colour.h"
#include "wx/gdicmn.h"
#include "wx/string.h"

#include <ostream>


namespace MapEditor { class CompMapEntityT; }
class GroupT;
class MapDocumentT;
class Renderer2DT;
class Renderer3DT;
class TextParserT;
class ViewWindow2DT;
template<class T> class Matrix4x4T;
typedef Matrix4x4T<float> Matrix4x4fT;


/// The TypeInfoTs of all MapElementT derived classes must register with this TypeInfoManT instance.
cf::TypeSys::TypeInfoManT& GetMapElemTIM();


/// An instance of this class encapsulates the transform-related state of a MapElementT.
class TrafoMementoT
{
    public:

    virtual ~TrafoMementoT() { }
};


/// This is the base class for all elements ("objects") that can exist in a map.
///
/// Generally, elements can exist stand-alone, without being assigned to a parent entity;
/// they are intended to be fully functional even without a parent entity.
/// Examples include newly created elements, elements in the clipboard, temporary copies (e.g. for preview
/// rendering in Selection tool), and elements inside commands (e.g. state before or after a transform).
/// For stand-alone elements, GetParent() returns `NULL`.
///
/// Usually though, elements are kept by an entity. The entity that an element is a part of can be learned
/// with the GetParent() method.
class MapElementT
{
    public:

    /// The default constructor.
    MapElementT();

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
    virtual ~MapElementT();


    virtual void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, bool IgnoreGroups);
    virtual void Save_cmap(std::ostream& OutFile, unsigned long ElemNr, const MapDocumentT& MapDoc) const;


    /// Returns the entity that this element is a part of, or `NULL` if the element has no parent entity.
    /// See the MapElementT class documentation for additional information.
    MapEditor::CompMapEntityT* GetParent() const { return m_Parent; }

    /// Sets the parent entity that is element is a part of.
    /// See the MapElementT class documentation for additional information.
    void SetParent(MapEditor::CompMapEntityT* Ent);


    /// Returns whether this element is currently selected in the map document.
    bool IsSelected() const { return m_IsSelected; }

    /// Sets the selection state of this element. As this should always be in sync with the map document,
    /// the map document is the only legitimate caller of this method (but there are some exceptions).
    void SetSelected(bool Selected=true) { m_IsSelected=Selected; }


    /// Returns whether this map element is (entirely or partially) translucent.
    /// Translucent map elements are typically implemented with "alpha blending" and require rendering in back-to-front order.
    /// @see EditorMaterialI::IsTranslucent()
    virtual bool IsTranslucent() const { return false; }

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
    virtual wxColour GetColor(bool ConsiderGroup=true) const = 0;

    virtual wxString GetDescription() const { return ""; }


    GroupT* GetGroup() const { return m_Group; }        ///< Returns NULL when this map element is in no group, or the pionter to the group it is a member of otherwise.
    void SetGroup(GroupT* Group) { m_Group = Group; }   ///< Sets the group this element is a member of (use NULL for "no group").

    /// Returns whether this map element is currently visible (in the 2D, 3D and other views).
    /// Note that the visibility does not depend on the visibility of the parent entity -- in CaWE, map elements are
    /// mostly independent of their parents (and thus entities can also be "half visible" if the user wishes so).
    bool IsVisible() const;

    /// Returns whether this map element can currently be selected (in the 2D, 3D and other views).
    /// Note that as with IsVisible(), the "can select" status is independent of that of the parent entity.
    bool CanSelect() const;

    /// Computes how the selection must be changed in order to toggle the given element when the element's entity
    /// and group memberships are taken into account.
    /// Unfortunately, the method cannot be `const`, because `this` map element is possibly added to one of the given
    /// arrays, whose elements are non-`const`.
    void GetToggleEffects(ArrayT<MapElementT*>& RemoveFromSel, ArrayT<MapElementT*>& AddToSel, bool AutoGroupEntities);


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

    /// Returns a memento that encapsulates the transform-related state of this element.
    /// The method saves all state in the memento that calls to the Trafo*() methods can possibly modify.
    virtual TrafoMementoT* GetTrafoState() const { return NULL; }

    /// Restores the transform-related state of this element from the given memento.
    /// The method restores all state from the memento that calls to the Trafo*() methods have possibly modified.
    virtual void RestoreTrafoState(const TrafoMementoT* TM) { }

    /// Translates this element by the given vector (in world-space).
    /// @param Delta           The offset by which to translate the element.
    /// @param LockTexCoords   Transform the texture-space along with the geometry.
    virtual void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) { }

    /// Rotates this element about the given reference point (in world-space).
    /// @param RefPoint        The reference point (origin) for the rotation.
    /// @param Angles          The rotation angles for the three axes.
    /// @param LockTexCoords   Transform the texture-space along with the geometry.
    virtual void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) { }

    /// Scales this element about the given reference point (in world-space).
    /// @param RefPoint        The reference point (origin) for the scale.
    /// @param Scale           The scale factors for the three axes.
    /// @param LockTexCoords   Transform the texture-space along with the geometry.
    /// @throws DivisionByZeroE, e.g. when Scale is too small and the element becomes degenerate (e.g. a brush with too small faces).
    virtual void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) { }

    /// Mirrors this element along the given mirror plane (in world-space).
    /// @param NormalAxis      The number of the axis along which the normal vector of the mirror plane points: 0, 1 or 2 for the x-, y- or z-axis respectively.
    /// @param Dist            The position of the mirror plane along its normal vector, where it intersects the NormalAxis.
    /// @param LockTexCoords   Transform the texture-space along with the geometry.
    /// Note that the mirroring is not necessarily "perfect", because for some elements like models or plants,
    /// only their point of origin can be mirrored, but not their mesh.
    virtual void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) { }

    /// Why does this method not replace all the other Trafo*() methods?
    /// This method is the most generic, allowing transformations that e.g. are non-orthogonal (like shears or non-uniform scales).
    /// This in turn conflicts with map primitives that can only store and deal with a restricted fixed set of transformations,
    /// e.g. an origin, a rotation and a uniform scale. These values cannot properly be re-computed from a general matrix with
    /// non-orthogonal basis vectors.
    /// @param Matrix          The matrix that describes the transform to be applied.
    /// @param LockTexCoords   Transform the texture-space along with the geometry.
    virtual void Transform(const Matrix4x4fT& Matrix, bool LockTexCoords) { }


    unsigned int GetFrameCount() const { return m_FrameCount; }
    void SetFrameCount(unsigned int FrameCount) { m_FrameCount=FrameCount; }


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    protected:

    MapEditor::CompMapEntityT* m_Parent;        ///< The entity that this element is a part of.
    bool                       m_IsSelected;    ///< Is this element currently selected in the map document?
    GroupT*                    m_Group;         ///< The group this element is in, NULL if in no group.
    unsigned int               m_FrameCount;    ///< The number of the frame in which this element was last rendered in a 3D view, used in order to avoid processing/rendering it twice.
};

#endif
