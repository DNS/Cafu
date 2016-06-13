/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "OrthoBspTree.hpp"
#include "MapElement.hpp"

#include "wx/log.h"


OrthoBspTreeT::NodeT::NodeT(const BoundingBox3fT& BB)
    : m_PlaneType(NONE),
      m_PlaneDist(0.0f),
      m_Parent(NULL),
      // m_Children(),
      m_Elems(),
      m_BB(BB)
{
    m_Children[0]=NULL;
    m_Children[1]=NULL;
}


OrthoBspTreeT::NodeT::~NodeT()
{
    delete m_Children[0];
    delete m_Children[1];
}


unsigned long OrthoBspTreeT::NodeT::GetNumNodes() const
{
    const NodeT*  Node =this;
    unsigned long Count=0;

    while (true)
    {
        Count++;
        if (Node->m_PlaneType==NONE) break;

        Count+=Node->m_Children[1]->GetNumNodes();
        Node=Node->m_Children[0];
    }

    return Count;
}


bool OrthoBspTreeT::NodeT::DetermineSplitPlane()
{
    const float MIN_NODE_SIZE=512.0f;

    PlaneTypeE PlaneTypes_SBLE[3]={ ALONG_X, ALONG_Y, ALONG_Z };
    Vector3fT  NodeBBSize_SBLE   =m_BB.Max-m_BB.Min;

    // SBLE: "Sorted by largest extent"  :-)
    for (int i=0; i<2; i++)
    {
        if (NodeBBSize_SBLE[i] < NodeBBSize_SBLE[i+1])
        {
            std::swap(PlaneTypes_SBLE[i], PlaneTypes_SBLE[i+1]);
            std::swap(NodeBBSize_SBLE[i], NodeBBSize_SBLE[i+1]);

            i=-1;
        }
    }

    // A split of a node that is normally too small for further splits is forced anyway if there are more than 100 map elements inside it.
    const bool ForceSplit=(NodeBBSize_SBLE[0]<MIN_NODE_SIZE && m_Elems.Size()>100);

    // Try to find an axis-aligned split plane.
    // This is achieved by trying all the bounding-box planes of the contents of this node AND ALL ANCESTOR nodes.
    // For best results, planes that are orthogonal to the axis with the largest extent are tried first.
    for (int i=0; i<3; i++)
    {
        const PlaneTypeE CurrentPlaneType=PlaneTypes_SBLE[i];   // Currently considered plane type. Ideally we never consider PlaneTypes_SBLE[1] and PlaneTypes_SBLE[2].
        float            BestOffset      =NodeBBSize_SBLE[i];   // Two times the distance from the center of our m_BB.

        // If the node has reached the minimum size (and is not forcibly split), then stop: We've reached a leaf.
        if (!ForceSplit && BestOffset<MIN_NODE_SIZE) break;

        // Consider the planes of the sides of the bounding-boxes of all map elements.
        for (const NodeT* Ancestor=this; Ancestor!=NULL; Ancestor=Ancestor->m_Parent)
        {
            for (unsigned long ElemNr=0; ElemNr<Ancestor->m_Elems.Size(); ElemNr++)
            {
                const BoundingBox3fT ElemBB=Ancestor->m_Elems[ElemNr]->GetBB();

                for (unsigned int Side=0; Side<2; Side++)
                {
                    const float CurrentPlaneDist=(Side==0) ? ElemBB.Min[CurrentPlaneType] : ElemBB.Max[CurrentPlaneType];

                    // The split plane must intersect our bounding-box per definition.
                    if (CurrentPlaneDist >= m_BB.Max[CurrentPlaneType] || CurrentPlaneDist <= m_BB.Min[CurrentPlaneType]) continue;

                    // Offset is (two times) the distance of the plane from the center of m_BB.
                    const float CurrentOffset=fabs((m_BB.Max[CurrentPlaneType]-CurrentPlaneDist) - (CurrentPlaneDist-m_BB.Min[CurrentPlaneType]));

                    if (CurrentOffset<BestOffset)
                    {
                        BestOffset =CurrentOffset;
                        m_PlaneType=CurrentPlaneType;
                        m_PlaneDist=CurrentPlaneDist;
                    }
                }
            }
        }

        // Check if we were able to choose a split plane orthogonal to the current (largest extent) axis.
        if (BestOffset<NodeBBSize_SBLE[i])
        {
            // If the split was forced due to too many polygons in the node, we accept the found plane unconditionally.
            if (ForceSplit) return true;

            // In the normal case, the plane is OK if it isn't too close to the sides of our m_BB.
            // (If too close, skip it and rather try planes types along the axes with lesser extents instead.)
            if ((m_BB.Max[CurrentPlaneType]-m_PlaneDist > MIN_NODE_SIZE*0.5f) &&
                (m_PlaneDist-m_BB.Min[CurrentPlaneType] > MIN_NODE_SIZE*0.5f)) return true;
        }
    }

    // No split plane was found.
    m_PlaneType=NONE;
    m_PlaneDist=0.0f;
    return false;
}


bool OrthoBspTreeT::NodeT::IntersectsAllChildren(const BoundingBox3fT& BB) const
{
    if (m_PlaneType!=NONE)
    {
        if (BB.Min[m_PlaneType] >= m_PlaneDist) return false;
        if (BB.Max[m_PlaneType] <= m_PlaneDist) return false;

        if (!m_Children[0]->IntersectsAllChildren(BB)) return false;
        if (!m_Children[1]->IntersectsAllChildren(BB)) return false;
    }

    return true;
}


void OrthoBspTreeT::NodeT::FindMismatches(ArrayT<MapElementT*>& Mismatches) const
{
    const NodeT* Node=this;

    while (true)
    {
        for (unsigned long ElemNr=0; ElemNr<Node->m_Elems.Size(); ElemNr++)
        {
            MapElementT*         Elem  =Node->m_Elems[ElemNr];
            const BoundingBox3fT ElemBB=Elem->GetBB();

            // If ElemBB intersects (or touches) the bounding-box of the node as well as all of the nodes children,
            // the map element is in the right place and there is no need to update it.
            // We must also allow "touches", because some ElemBBs have zero volume (e.g. those from "flat" Bezier patches).
            if (ElemBB.IntersectsOrTouches(Node->m_BB) && Node->IntersectsAllChildren(ElemBB)) continue;

            // Otherwise, Elem is a mismatch. Add it to the Mismatches list if not already there.
            if (Mismatches.Find(Elem)==-1) Mismatches.PushBack(Elem);
        }

        // If Node is a leaf node, we're done.
        if (Node->m_PlaneType==NONE) break;

        // Recurse into the children.
        Node->m_Children[1]->FindMismatches(Mismatches);
        Node=Node->m_Children[0];
    }
}


void OrthoBspTreeT::NodeT::Insert(MapElementT* Elem)
{
    NodeT*               Node  =this;
    const BoundingBox3fT ElemBB=Elem->GetBB();

    while (Node->m_PlaneType!=NONE && !Node->IntersectsAllChildren(ElemBB))
    {
        if (ElemBB.Min[Node->m_PlaneType] >= Node->m_PlaneDist)
        {
            Node=Node->m_Children[0];
        }
        else if (ElemBB.Max[Node->m_PlaneType] <= Node->m_PlaneDist)
        {
            Node=Node->m_Children[1];
        }
        else
        {
            Node->m_Children[1]->Insert(Elem);
            Node=Node->m_Children[0];
        }
    }

    wxASSERT(Node->m_Elems.Find(Elem)==-1);
    Node->m_Elems.PushBack(Elem);
}


void OrthoBspTreeT::NodeT::Remove(MapElementT* Elem)
{
    NodeT* Node=this;

    while (true)
    {
        // Remove Elem from the Node->m_Elems array.
        for (unsigned long ElemNr=0; ElemNr<Node->m_Elems.Size(); ElemNr++)
            if (Node->m_Elems[ElemNr]==Elem)
            {
                Node->m_Elems.RemoveAt(ElemNr);
                ElemNr--;
            }

        // If Node is a leaf node, we're done.
        if (Node->m_PlaneType==NONE) break;

        // Recurse into the children.
        Node->m_Children[1]->Remove(Elem);
        Node=Node->m_Children[0];
    }
}


OrthoBspTreeT::OrthoBspTreeT(const ArrayT<MapElementT*>& Elems, const BoundingBox3fT& BB)
    : m_RootNode(new NodeT(BB))
{
    m_RootNode->m_Elems=Elems;
    BuildTree(m_RootNode);

    wxLogDebug("Created OrthoBspTreeT with %lu elements in %lu nodes.", Elems.Size(), m_RootNode->GetNumNodes());
    wxASSERT(Update()==0);
}


OrthoBspTreeT::~OrthoBspTreeT()
{
    delete m_RootNode;
}


unsigned long OrthoBspTreeT::Update()
{
    static ArrayT<MapElementT*> Mismatches;

    Mismatches.Overwrite();
    m_RootNode->FindMismatches(Mismatches);

    for (unsigned long ElemNr=0; ElemNr<Mismatches.Size(); ElemNr++)
    {
        m_RootNode->Remove(Mismatches[ElemNr]);

        // TODO: Insert with creating new splitplanes, if necessary?
        m_RootNode->Insert(Mismatches[ElemNr]);
    }

    return Mismatches.Size();
}


void OrthoBspTreeT::BuildTree(NodeT* Node)
{
    if (!Node->DetermineSplitPlane()) return;

    // Create a front child for Node.
    BoundingBox3fT FrontBB=Node->m_BB;
    FrontBB.Min[Node->m_PlaneType]=Node->m_PlaneDist;

    NodeT* FrontNode=new NodeT(FrontBB);
    FrontNode->m_Parent   =Node;
    FrontNode->m_PlaneType=NodeT::NONE;
    Node->m_Children[0]=FrontNode;

    // Create a back child for Node.
    BoundingBox3fT BackBB=Node->m_BB;
    BackBB.Max[Node->m_PlaneType]=Node->m_PlaneDist;

    NodeT* BackNode=new NodeT(BackBB);
    BackNode->m_Parent   =Node;
    BackNode->m_PlaneType=NodeT::NONE;
    Node->m_Children[1]=BackNode;

    // Insert the elements in Node at their proper place into the tree.
    for (NodeT* Ancestor=Node; Ancestor!=NULL; Ancestor=Ancestor->m_Parent)
    {
        for (unsigned long ElemNr=0; ElemNr<Ancestor->m_Elems.Size(); ElemNr++)
        {
            MapElementT* Elem=Ancestor->m_Elems[ElemNr];

            if (!Ancestor->IntersectsAllChildren(Elem->GetBB()))
            {
                Ancestor->Insert(Elem);
                Ancestor->m_Elems.RemoveAt(ElemNr);
                ElemNr--;
            }
        }
    }

    // Recurse.
    BuildTree(FrontNode);
    BuildTree(BackNode);
}
