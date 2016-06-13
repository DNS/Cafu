/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***********************************/
/*** Compute the Draw Structures ***/
/***********************************/


// Determines at what index number the vertex 'A' is in 'Vertices'.
// If 'A' is not found at all, it is inserted into the 'Vertices' array.
unsigned long FindVertexIndex(ArrayT<VectorT>& Vertices, const VectorT& A)
{
    // Existiert schon so ein Vertex?
    unsigned long VertexNr;

    for (VertexNr=0; VertexNr<Vertices.Size(); VertexNr++)
        if (Vertices[VertexNr].IsEqual(A, MapT::RoundEpsilon)) break;

    // Falls nicht, einen neuen Vertex 'A' anhängen.
    if (VertexNr==Vertices.Size()) Vertices.PushBack(A);

    return VertexNr;
}


void BspTreeBuilderT::ComputeDrawStructures()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Compute Draw Structures ***", GetTimeSinceProgramStart()));

    // Lege zuerst eine Kopie 'DrawFaces' der 'Faces' an,
    // und ergänze darin die zur T-Junction-Entfernung notw. Vertices.
    ArrayT< Polygon3T<double> >     DrawFaces;
    ArrayT< BoundingBox3T<double> > FaceBB;

    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++)
    {
        DrawFaces.PushBack(FaceChildren[FaceNr]->Polygon);
        FaceBB.PushBack(BoundingBox3T<double>(DrawFaces[FaceNr].Vertices).GetEpsilonBox(MapT::RoundEpsilon*0.5));
    }

    for (unsigned long Face1Nr=0; Face1Nr<DrawFaces.Size(); Face1Nr++)
        for (unsigned long Face2Nr=0; Face2Nr<DrawFaces.Size(); Face2Nr++)
            if (FaceBB[Face1Nr].Intersects(FaceBB[Face2Nr]) && Face1Nr!=Face2Nr)
                DrawFaces[Face1Nr].FillTJunctions(DrawFaces[Face2Nr], MapT::RoundEpsilon);

    Console->Print("T-Junction removal  :       done\n");


    // Nun erstelle daraus die 'GlobalDrawVertices' und 'FacesDrawIndices' Arrays.
    GlobalDrawVertices.Clear();

    for (unsigned long FaceNr=0; FaceNr<DrawFaces.Size(); FaceNr++)
    {
        FaceChildren[FaceNr]->GetDrawIndices().Clear();

        for (unsigned long VertexNr=0; VertexNr<DrawFaces[FaceNr].Vertices.Size(); VertexNr++)
            FaceChildren[FaceNr]->GetDrawIndices().PushBack(FindVertexIndex(GlobalDrawVertices, DrawFaces[FaceNr].Vertices[VertexNr]));
    }

    Console->Print("Draw structures     :       done\n");


    // Compute neighbourhood information for each edge of each face.
    // That is, determine and store the other face that shares each edge.
    // Note that MORE THAN TWO faces may share a common edge!
    // This is trivially clear when translucent or "masked" faces are present,
    // but can also happen on pure opaque geometry, for example when two
    // square brushes only touch each other on a corner (no overlap).
    // Currently, the ONLY purpose of the resulting 'FacesEdgeNeigbours' is to
    // provide auxiliary information for shadow volume silhouette edge extraction.
    // IF this step computes fast enough, we MIGHT move it into the 'DrawableMapT'
    // constructor, that is, into engine load time.
 /* FacesEdgeNeighbours.PushBackEmpty(FacesDrawIndices.Size());
    for (FaceNr=0; FaceNr<FacesEdgeNeighbours.Size(); FaceNr++)
        FacesEdgeNeighbours[FaceNr].PushBackEmpty(FacesDrawIndices[FaceNr].Size());

    for (FaceNr=0; FaceNr+1<FacesDrawIndices.Size(); FaceNr++)
    {
        const ArrayT<unsigned long>& F1=FacesDrawIndices[FaceNr];

        for (unsigned long Face2Nr=FaceNr+1; Face2Nr<FacesDrawIndices.Size(); Face2Nr++)
        {
            const ArrayT<unsigned long>& F2=FacesDrawIndices[Face2Nr];

            if (BoundingBoxTest(FaceBB[FaceNr], FaceBB[Face2Nr]))
                for (unsigned long V1Nr=0; V1Nr<F1.Size(); V1Nr++)
                    for (unsigned long V2Nr=0; V2Nr<F2.Size(); V2Nr++)
                        if (F1[ V1Nr               ]==F2[(V2Nr+1) % F2.Size()] &&
                            F1[(V1Nr+1) % F1.Size()]==F2[ V2Nr               ] )
                        {
                            // 'F1' and 'F2' share a common edge.
                            FacesEdgeNeighbours[FaceNr ][V1Nr].PushBack(Face2Nr);
                            FacesEdgeNeighbours[Face2Nr][V2Nr].PushBack(FaceNr );
                        }
        }
    }

    // Sanity check: Make sure that each edge is shared by at least two faces.
    for (FaceNr=0; FaceNr<FacesEdgeNeighbours.Size(); FaceNr++)
        for (unsigned long VertexNr=0; VertexNr<FacesEdgeNeighbours[FaceNr].Size(); VertexNr++)
            if (FacesEdgeNeighbours[FaceNr][VertexNr/ *EdgeNr* /].Size()==0)
                Error("Face %lu has no face neighbour at edge %lu.", FaceNr, VertexNr);

    Console->Print("Neighbour graph     :       done\n"); */
}
