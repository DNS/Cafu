/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Tree.hpp"
#include "PlantDescription.hpp"

#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "../../ExtLibs/MersenneTwister.h"


static MTRand MersenneTwister;


// Returns a random float in range -1.0 to +1.0.
float RandomFloat()
{
    return 2.0f*float(MersenneTwister.rand())-1.0f;
}


// Berechnet aus den Parametern einen Branch (voll rekursiv, d.h. inkl. aller Children (Sub-Branches und Leaves)).
// 'RelPosOnParent' ist der relative Anbringungsort am Parent (0.0 ist die Wurzel des Parent, 1.0 die Spitze).
// 'BranchLevel'    ist der Level dieses Branches.
// 'BLDs'           sind die Branch-Level-Descriptions des Trees.
// 'BranchMatrix'   ist die Translationsmatrix des Parents an unserem Anbringungsort.
TreeT::BranchT TreeT::ComputeBranch(float RelPosOnParent, char BranchLevel, const PlantDescriptionT* TD, const MatrixT& BranchMatrix, unsigned long& NumOfAllBranchTriangles)
{
    const PlantDescriptionT::BranchLevelDescriptionT& OurBLD=TD->BranchLevelDescriptions[BranchLevel]; // Eine Abk. fuer "our branch level description"
    TreeT::BranchT Branch;                                                                           // Der Branch, den wir berechnen und zurueckgeben wollen


    // Allgemeine Branch-Daten berechnen
    // *********************************

    Branch.Length    =(OurBLD.LengthProfile    .Compute(RelPosOnParent, OurBLD.Length0    , OurBLD.Length1    )+OurBLD.LengthV    *RandomFloat())*Size;
    Branch.StartAngle= OurBLD.StartAngleProfile.Compute(RelPosOnParent, OurBLD.StartAngle0, OurBLD.StartAngle1)+OurBLD.StartAngleV*RandomFloat();
    Branch.Radius    =(OurBLD.RadiusProfile    .Compute(RelPosOnParent, OurBLD.Radius0    , OurBLD.Radius1    )+OurBLD.RadiusV    *RandomFloat())*Size;


    // Die individuellen Segmente berechnen
    // ************************************

    MatrixT       SegmentMatrix=BranchMatrix;                              // Die Transformationsmatrix der Spitze des "vorangegangenen" Segments
    const float   SegmentLength=Branch.Length/float(OurBLD.NrOfSegments);  // Die Laenge jedes einzelnen Segments in Metern
    float         NextChildHeight;                                         // Relative Position unseres naechsten Kindes
    float         ChildHeightDistance;                                     // Der relative Abstand zwischen unseren Kindern
    float         CrossSectionRadiusPreviousSegmentTip=0.0;                // Initialisierung siehe unten! (Hier nur, um Compilerwarnung zu unterdruecken.)
    ArrayT<float> CrossSectionPointsPreviousSegmentTip;                    // Initialisierung siehe unten!

    if (OurBLD.NrOfChildren>0)
    {
        // Der Normalfall - der Ast hat Kinder (Sub-Branches oder Blaetter)
        NextChildHeight    =0.0f;
        ChildHeightDistance=1.0f/((Branch.Length/Size)*float(OurBLD.NrOfChildren));
    }
    else
    {
        // Keine Kinder (kann vorkommen waehrend Debugging oder bei einem Baum im Winter (der Baum hat keine Blaetter))
        NextChildHeight    =2.0f*Branch.Length;
        ChildHeightDistance=0.0f;
    }

    for (unsigned long SegmentNr=0; SegmentNr<OurBLD.NrOfSegments; SegmentNr++)
    {
        const float     RelSegmentRoot=float(SegmentNr  )/float(OurBLD.NrOfSegments);  // Die relative Position der Wurzel dieses Segments
        const float     RelSegmentTip =float(SegmentNr+1)/float(OurBLD.NrOfSegments);  // Die relative Position der Spitze dieses Segments
        const Vector3fT SegmentVector  (0.0f, -SegmentLength, 0.0f);

        Branch.Segments.PushBackEmpty();


        // 1. Bestimme den Drehwinkel um die X-Achse der Transformationsmatrix der Spitze des "vorangegangenen" Segments,
        //    um die Transformationsmatrix der Wurzel dieses Segments zu erhalten.
        float DeltaSegmentAngle=0.0f;

        if (SegmentNr==0) DeltaSegmentAngle+=Branch.StartAngle;
        DeltaSegmentAngle+=OurBLD.AngleProfile_Segments.Compute(float(SegmentNr)/float(OurBLD.NrOfSegments), 180.0f, -180.0f);

        SegmentMatrix=SegmentMatrix*MatrixT::GetRotateXMatrix(-DeltaSegmentAngle);
        Branch.Segments[SegmentNr].Matrix=SegmentMatrix;


        // 2. Initialisiere die CrossSectionPointsPreviousSegmentTip mit den Eckpunkten des Querschnitts der *Wurzel* der ersten Segments!
        //    Init-Code ist erst hier, weil an dieser Stelle die 'SegmentMatrix' stimmt!
        if (SegmentNr==0)
        {
            CrossSectionRadiusPreviousSegmentTip=OurBLD.RadiusProfile_Segments.Compute(0, Branch.Radius, 0);

            for (unsigned long PNr=0; PNr<OurBLD.CrossSectionsResolution; PNr++)
            {
                const float     CoordX=CrossSectionRadiusPreviousSegmentTip*float(cos(double(PNr)/double(OurBLD.CrossSectionsResolution)*2.0*3.141592654));
                const float     CoordZ=CrossSectionRadiusPreviousSegmentTip*float(sin(double(PNr)/double(OurBLD.CrossSectionsResolution)*2.0*3.141592654));
                const Vector3fT Coord  (CoordX, 0.0f, CoordZ);

                const Vector3fT AbsCoord=SegmentMatrix.Mul1(Coord);

                CrossSectionPointsPreviousSegmentTip.PushBack(AbsCoord.x);
                CrossSectionPointsPreviousSegmentTip.PushBack(AbsCoord.y);
                CrossSectionPointsPreviousSegmentTip.PushBack(AbsCoord.z);
            }
        }


        // 3. Erzeuge ggf. Children
        while (NextChildHeight<RelSegmentTip)
        {
            if (NextChildHeight>=OurBLD.FirstChild && NextChildHeight<=OurBLD.LastChild)
            {
                // Beobachtung: "SpeedTree" verhaelt sich mit Rotationen um die Z-Achse der Segmente genauso wie wir:
                // Fuer *alle* BranchLevel (nicht nur fuer den Stamm!) wird zufaellig um plus/minus 180 Grad rotiert!
                Vector3fT ChildOffset       (0.0f, -(NextChildHeight-RelSegmentRoot)*Branch.Length, 0.0f);
                MatrixT   ChildBranchMatrix=SegmentMatrix*MatrixT::GetRotateYMatrix(180.0f*RandomFloat())*MatrixT::GetTranslateMatrix(ChildOffset);

                if ((unsigned long)(BranchLevel+1)<TD->BranchLevelDescriptions.Size())
                    Branch.ChildBranches.PushBack(ComputeBranch(NextChildHeight, BranchLevel+1, TD, ChildBranchMatrix, NumOfAllBranchTriangles));
                else
                {
                    // Wir sind beim letzten BranchLevel angekommen - erzeuge ein Leaf (statt eines Sub-Branches)
                    const float     Distance =(TD->DistanceProfile.Compute(NextChildHeight, TD->Distance0, TD->Distance1)+TD->DistanceV*RandomFloat())*Size;
                    const Vector3fT LeafOffset(0.0f, 0.0f, Distance);  // Drehe (die Y-Achse) nicht mehr um 90 Grad, sondern nimm einfach direkt die Z-Achse.

                    TreeT::BranchT::LeafT Leaf;  // Das Leaf, das wir berechnen

                    // Den Urspung des Leafs eintragen
                    Leaf.Origin=ChildBranchMatrix.Mul1(LeafOffset);

                    // Bestimme die Texture, die wir fuer dieses Leaf verwenden wollen
                    if (LeafRenderMats.Size()==0) continue; // We don't create a leaves if there are no materials for them.

                    unsigned long MatIndex=MersenneTwister.randInt(LeafRenderMats.Size()-1);

                    Leaf.RenderMat=LeafRenderMats[MatIndex];

                    // Die Groesse des Leaf-Polygons in World-Coords ("world size")
                    const unsigned int TextureSizeX=TD->LeafMats[MatIndex]!=NULL ? TD->LeafMats[MatIndex]->GetPixelSizeX() : 1;
                    const unsigned int TextureSizeY=TD->LeafMats[MatIndex]!=NULL ? TD->LeafMats[MatIndex]->GetPixelSizeY() : 1;

                    const float wsX=TD->LeafSize*TD->Size;
                    const float wsY=TD->LeafSize*TD->Size*float(TextureSizeY)/float(TextureSizeX);

                    // Wo sich der Leaf-Origin bzgl. der Leaf-Texture (linke untere Ecke!) befindet (vgl. SpeedTree "Image File Selector")
                    const float RotPointX=0.5f;
                    const float RotPointY=0.55f;

                    // Die konkreten Koordinatenpaare der Eckpunkte des Leaf-Billboard-Polygons bestimmen, relativ zum Origin des Leafs
                    Leaf.Coords[0]=     -RotPointX *wsX;  // links  oben  X
                    Leaf.Coords[1]=(1.0f-RotPointY)*wsY;  // links  oben  Y
                    Leaf.Coords[2]=(1.0f-RotPointX)*wsX;  // rechts oben  X
                    Leaf.Coords[3]=(1.0f-RotPointY)*wsY;  // rechts oben  Y
                    Leaf.Coords[4]=(1.0f-RotPointX)*wsX;  // rechts unten X
                    Leaf.Coords[5]=     -RotPointY *wsY;  // rechts unten Y
                    Leaf.Coords[6]=     -RotPointX *wsX;  // links  unten X
                    Leaf.Coords[7]=     -RotPointY *wsY;  // links  unten Y

                    // Drehe das Polygon noch um einen zufaelligen Wert (max. plus/minus 30 Grad)
                    const float RotAngle   =30.0f/180.0f*3.1415926f*RandomFloat();
                    const float SinRotAngle=sin(RotAngle);
                    const float CosRotAngle=cos(RotAngle);

                    for (char c=0; c<4; c++)
                    {
                        const float RotX=Leaf.Coords[2*c]*CosRotAngle-Leaf.Coords[2*c+1]*SinRotAngle;
                        const float RotY=Leaf.Coords[2*c]*SinRotAngle+Leaf.Coords[2*c+1]*CosRotAngle;

                        Leaf.Coords[2*c  ]=RotX;
                        Leaf.Coords[2*c+1]=RotY;
                    }

                    // Die Farbe dieses Leafs festlegen (Farbe+Dimming+Variance)
                    const float MaxDimming=0.7f;
                    const float Dimming   =(1.0f-NextChildHeight)*MaxDimming;

                    Leaf.Color[0]=1.0f-Dimming;
                    Leaf.Color[1]=1.0f-Dimming;  // 0.5+(RandomFloat()+1.0)*0.25;
                    Leaf.Color[2]=1.0f-Dimming;

                    if (Leaf.Color[0]<0.0f) Leaf.Color[0]=0.0f;
                    if (Leaf.Color[0]>1.0f) Leaf.Color[0]=1.0f;
                    if (Leaf.Color[1]<0.0f) Leaf.Color[1]=0.0f;
                    if (Leaf.Color[1]>1.0f) Leaf.Color[1]=1.0f;
                    if (Leaf.Color[2]<0.0f) Leaf.Color[2]=0.0f;
                    if (Leaf.Color[2]>1.0f) Leaf.Color[2]=1.0f;

                    // Fuege das Leaf in das Array der Leaves dieses Branches ein.
                    AllLeaves.PushBack(Leaf);
                    Branch.LeavesIndices.PushBack(AllLeaves.Size()-1);
                }
            }

            NextChildHeight+=ChildHeightDistance;
        }


        // 4. Ueberfuehre die T-Matrix der Wurzel dieses Segments in die T-Matrix der Spitze dieses Segments,
        //    damit die naechste Iteration korrekte Ausgangsbedingungen vorfindet.
        SegmentMatrix=SegmentMatrix*MatrixT::GetTranslateMatrix(SegmentVector);


        // 5. Berechne die Eckpunkte des Querschnitts an der Spitze dieses Segments
        float         CrossSectionRadiusOurSegmentTip=OurBLD.RadiusProfile_Segments.Compute(RelSegmentTip, Branch.Radius, 0);
        ArrayT<float> CrossSectionPointsOurSegmentTip;
        unsigned long PNr;    // g++ und wpp386 kompatibel

        for (PNr=0; PNr<OurBLD.CrossSectionsResolution; PNr++)
        {
            const float     CoordX=CrossSectionRadiusOurSegmentTip*float(cos(double(PNr)/double(OurBLD.CrossSectionsResolution)*2.0*3.141592654));
            const float     CoordZ=CrossSectionRadiusOurSegmentTip*float(sin(double(PNr)/double(OurBLD.CrossSectionsResolution)*2.0*3.141592654));
            const Vector3fT Coord  (CoordX, 0.0f, CoordZ);

            const Vector3fT AbsCoord=SegmentMatrix.Mul1(Coord);

            CrossSectionPointsOurSegmentTip.PushBack(AbsCoord.x);
            CrossSectionPointsOurSegmentTip.PushBack(AbsCoord.y);
            CrossSectionPointsOurSegmentTip.PushBack(AbsCoord.z);
        }


        // 6. Trage die Eckpunkte der Querschnitte in die globale Tabelle ein,
        //    sodass damit leicht ein GL_TRIANGLE_STRIP gezeichnet werden kann.
        if (GlobalAbsCoordTable.Size() % 3) printf("WARNING: GlobalAbsCoordTable.Size() mod 3 is not 0!\n");

        Branch.Segments[SegmentNr].RadiusAtRoot   =CrossSectionRadiusPreviousSegmentTip;
        Branch.Segments[SegmentNr].FirstCoordIndex=GlobalAbsCoordTable.Size()/3;
        Branch.Segments[SegmentNr].NrOfCoords     =(OurBLD.CrossSectionsResolution+1)*2;

        for (PNr=0; PNr<OurBLD.CrossSectionsResolution; PNr++)
        {
            GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[PNr*3+0]);
            GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[PNr*3+1]);
            GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[PNr*3+2]);

            GlobalTexCoordTable.PushBack(float(PNr)/float(OurBLD.CrossSectionsResolution));
            GlobalTexCoordTable.PushBack(RelSegmentRoot);


            GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[PNr*3+0]);
            GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[PNr*3+1]);
            GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[PNr*3+2]);

            GlobalTexCoordTable.PushBack(float(PNr)/float(OurBLD.CrossSectionsResolution));
            GlobalTexCoordTable.PushBack(RelSegmentTip);
        }

        GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[0]);
        GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[1]);
        GlobalAbsCoordTable.PushBack(CrossSectionPointsPreviousSegmentTip[2]);

        GlobalTexCoordTable.PushBack(1.0f);
        GlobalTexCoordTable.PushBack(RelSegmentRoot);


        GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[0]);
        GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[1]);
        GlobalAbsCoordTable.PushBack(CrossSectionPointsOurSegmentTip[2]);

        GlobalTexCoordTable.PushBack(1.0f);
        GlobalTexCoordTable.PushBack(RelSegmentTip);


        CrossSectionRadiusPreviousSegmentTip=CrossSectionRadiusOurSegmentTip;
        CrossSectionPointsPreviousSegmentTip=CrossSectionPointsOurSegmentTip;

        NumOfAllBranchTriangles+=OurBLD.CrossSectionsResolution*2;
    }

    return Branch;
}


TreeT::TreeT(const PlantDescriptionT* TD, unsigned long RandomSeed)
    : Size(0.0f),
      BarkRenderMat(NULL),
      TreeBounds(),
      DrawLeaves(true),
      UseRealBlendingForLeaves(true)
{
    assert(TD!=NULL);

    // 1. Get render materials from tree description.
    BarkRenderMat=TD->BarkRenderMat;

    for (unsigned long i=0; i<TD->LeafRenderMats.Size(); i++)
        LeafRenderMats.PushBack(TD->LeafRenderMats[i]);

    // 2. Precompute tree.
    unsigned long NumOfAllBranchTriangles=0;

    MersenneTwister.seed(RandomSeed);

    Size=TD->Size+TD->SizeV*RandomFloat();

    Trunk=ComputeBranch(0.0f, 0, TD, MatrixT(), NumOfAllBranchTriangles);

    // 4. TODO: Create billboards from tree for LoD purposes!

    // Initialize tree bounding box.
    TreeBounds=BoundingBox3fT(Vector3fT(GlobalAbsCoordTable[0], GlobalAbsCoordTable[1], GlobalAbsCoordTable[2]));

    // Calculate tree mesh bounding box.
    for (unsigned long i=1; i<GlobalAbsCoordTable.Size()/3; i++)
        TreeBounds.Insert(Vector3fT(GlobalAbsCoordTable[i*3], GlobalAbsCoordTable[i*3+1], GlobalAbsCoordTable[i*3+2]));
}


// Global Draw Information for the current frame (saves us redundant parameter passing)
Vector3fT gdi_BillBoardRtVec;
Vector3fT gdi_BillBoardUpVec;
float     gdi_DistanceToTreeRoot;


void TreeT::DrawBranch(const BranchT& Branch) const
{
    // ACHTUNG: Im folgenden setzen wir fuer die LoD-Entscheidungen voraus, dass
    // a) der Radius der Querschnitte der Segmente eines Asts von der Wurzel zur Spitze hin monoton abnimmt und
    // b) der Radius des Querschnitts der Wurzel eines Kind-Asts niemals groesser ist als der Radius des Querschitts seines Parent-Asts an dieser Stelle
    //    (das ist sowieso eine allgemeine Voraussetzung, nicht nur fuer LoD).


    // Zeichne alle Segmente dieses Branches
    unsigned long SegmentNr;

    for (SegmentNr=0; SegmentNr<Branch.Segments.Size(); SegmentNr++)
    {
        /* const float SegmentStart[3] = { 0.0, 0.0, 0.0 };
        const float SegmentEnd  [3] = { 0.0, 0.0, Branch.Length/float(Branch.Segments.Size()) };
        float       Result      [3]; */

        // Zeichne den "Bone" bzw. die "Achse" des Segments
        /* glColor3f(0.8, 0.5, 0.3);
        glBegin(GL_LINE_STRIP);
          Branch.Segments[SegmentNr].Matrix.TransformVector(SegmentStart, Result); glVertex3f(Result[0], Result[1], Result[2]);
          Branch.Segments[SegmentNr].Matrix.TransformVector(SegmentEnd  , Result); glVertex3f(Result[0], Result[1], Result[2]);
        glEnd(); */

        // Zeichne einen roten Punkt am Ende (d.h. der "Spitze") des Segments
        /* glColor3f(1.0, 0.0, 0.0);
        glPointSize(5.0);
        glBegin(GL_POINTS);
          glVertex3f(Result[0], Result[1], Result[2]);
        glEnd();
        glPointSize(1.0); */

        // Zeichne die Segment-Polygone in Wire-Frame Darstellung
        /* glColor3f(0.0, 0.8, 0.0);
        glDrawArrays(GL_LINE_STRIP, Branch.Segments[SegmentNr].FirstCoordIndex, Branch.Segments[SegmentNr].NrOfCoords); */

        // Zeichne die Segment-Polygone texturiert.
        // Wenn der Radius des ersten Segments dieses Asts zu klein wird, brich ab! (Und spare den noch kleineren Rest.)
        if (Branch.Segments[SegmentNr].RadiusAtRoot/gdi_DistanceToTreeRoot<0.001f) break; // glColor3f(1,0,0); else glColor3f(1,1,1);

        #if 0
        glDrawArrays(GL_TRIANGLE_STRIP, Branch.Segments[SegmentNr].FirstCoordIndex, Branch.Segments[SegmentNr].NrOfCoords);
        #endif

        static MatSys::MeshT Mesh(MatSys::MeshT::TriangleStrip);
        Mesh.Vertices.Overwrite();

        static MatSys::MeshT::VertexT Vertex;

        int AbsOffset=Branch.Segments[SegmentNr].FirstCoordIndex*3;
        int TexOffset=Branch.Segments[SegmentNr].FirstCoordIndex*2;

        for (unsigned long i=0; i<Branch.Segments[SegmentNr].NrOfCoords; i++)
        {
            Vertex.SetOrigin      (Vector3fT(GlobalAbsCoordTable[AbsOffset+i*3], GlobalAbsCoordTable[AbsOffset+i*3+1], GlobalAbsCoordTable[AbsOffset+i*3+2]));
            Vertex.SetTextureCoord(GlobalTexCoordTable[TexOffset+i*2], GlobalTexCoordTable[TexOffset+i*2+1]);

            Mesh.Vertices.PushBack(Vertex);
        }

        MatSys::Renderer->RenderMesh(Mesh);
    }

    // Wenn noch nicht mal das erste Segment dieses Astes vom LoD uebrig gelassen wurde, fange mit den Kindern gar nicht erst an!
    if (SegmentNr==0) return;


    // Zeichne rekursiv die Segmente aller Children dieses Branches
    for (unsigned long ChildBranchNr=0; ChildBranchNr<Branch.ChildBranches.Size(); ChildBranchNr++)
        DrawBranch(Branch.ChildBranches[ChildBranchNr]);
}


void TreeT::GetLeavesAfterLoD(const BranchT& Branch, ArrayT<unsigned long>& LeavesAfterLoD, unsigned long& NrOfLeavesAfterLoD) const
{
    // Wenn wir Leaves weglassen, koennen wir dies nicht global fuer den ganzen Baum tun, sondern zunaechst nur fuer diesen Branch.
    // Entscheide nun also, wieviele Leaves von diesem Branch wir zeichnen wollen.
    // Beginne das Weglassen an der Wurzel des Branches, sodass Leaves an der Branch-Spitze bis zuletzt vorhanden sind.
    unsigned long ActualNrOfLeaves=Branch.LeavesIndices.Size();   // Wenn nichts anderes entschieden wird, zeichne alle Leaves.

    const float NearestDistLoD = 40.0f; //prev. 200
    const float FarthestDistLoD=200.0f; //prev. 1000

    if (gdi_DistanceToTreeRoot>NearestDistLoD && gdi_DistanceToTreeRoot<=FarthestDistLoD)
    {
        // Der Baum ist zwischen 'NearestDistLoD' und 'FarthestDistLoD' Metern entfernt.
        ActualNrOfLeaves=(unsigned long)((1.0f-(gdi_DistanceToTreeRoot-NearestDistLoD)/(FarthestDistLoD-NearestDistLoD))*Branch.LeavesIndices.Size());
        if (ActualNrOfLeaves==0) ActualNrOfLeaves=1;
    }
    else if (gdi_DistanceToTreeRoot>FarthestDistLoD)
    {
        // Der Baum ist weiter als 'FarthestDistLoD' Meter entfernt.
        ActualNrOfLeaves=1;
    }

    if (ActualNrOfLeaves>Branch.LeavesIndices.Size()) ActualNrOfLeaves=Branch.LeavesIndices.Size();


    // Notiere in 'LeavesAfterLoD', welche Leaves von 'Branch' wir zeichnen wollen.
    for (unsigned long LeafNr=Branch.LeavesIndices.Size()-ActualNrOfLeaves; LeafNr<Branch.LeavesIndices.Size(); LeafNr++)
        LeavesAfterLoD[NrOfLeavesAfterLoD++]=Branch.LeavesIndices[LeafNr];


    // Bearbeite rekursiv auch die Leaves aller Children dieses Branches.
    for (unsigned long ChildBranchNr=0; ChildBranchNr<Branch.ChildBranches.Size(); ChildBranchNr++)
        GetLeavesAfterLoD(Branch.ChildBranches[ChildBranchNr], LeavesAfterLoD, NrOfLeavesAfterLoD);
}


void TreeT::DrawLeaf(const BranchT::LeafT& Leaf) const
{
    static MatSys::MeshT LeafMesh(MatSys::MeshT::TriangleFan);

    if (LeafMesh.Vertices.Size()==0) LeafMesh.Vertices.PushBackEmptyExact(4);

    if (Leaf.RenderMat==NULL) return; // Can't draw a leaf that has no render material.

    MatSys::Renderer->SetCurrentMaterial(Leaf.RenderMat);

    static MatSys::MeshT::VertexT VertexTmp;
    VertexTmp.SetColor(Leaf.Color[0], Leaf.Color[1], Leaf.Color[2]);

    // Siehe http://www.lighthouse3d.com/opengl/billboarding/index.php3 fuer eine gute Erklaerung zu Billboards!
    VertexTmp.SetOrigin(Leaf.Origin.x + gdi_BillBoardRtVec.x*Leaf.Coords[0] + gdi_BillBoardUpVec.x*Leaf.Coords[1],
                        Leaf.Origin.y + gdi_BillBoardRtVec.y*Leaf.Coords[0] + gdi_BillBoardUpVec.y*Leaf.Coords[1],
                        Leaf.Origin.z + gdi_BillBoardRtVec.z*Leaf.Coords[0] + gdi_BillBoardUpVec.z*Leaf.Coords[1]);
    VertexTmp.SetTextureCoord(0.0f, 1.0f);

    LeafMesh.Vertices[0]=VertexTmp;

    VertexTmp.SetOrigin(Leaf.Origin.x + gdi_BillBoardRtVec.x*Leaf.Coords[2] + gdi_BillBoardUpVec.x*Leaf.Coords[3],
                        Leaf.Origin.y + gdi_BillBoardRtVec.y*Leaf.Coords[2] + gdi_BillBoardUpVec.y*Leaf.Coords[3],
                        Leaf.Origin.z + gdi_BillBoardRtVec.z*Leaf.Coords[2] + gdi_BillBoardUpVec.z*Leaf.Coords[3]);
    VertexTmp.SetTextureCoord(1.0f, 1.0f);

    LeafMesh.Vertices[1]=VertexTmp;

    VertexTmp.SetOrigin(Leaf.Origin.x + gdi_BillBoardRtVec.x*Leaf.Coords[4] + gdi_BillBoardUpVec.x*Leaf.Coords[5],
                        Leaf.Origin.y + gdi_BillBoardRtVec.y*Leaf.Coords[4] + gdi_BillBoardUpVec.y*Leaf.Coords[5],
                        Leaf.Origin.z + gdi_BillBoardRtVec.z*Leaf.Coords[4] + gdi_BillBoardUpVec.z*Leaf.Coords[5]);
    VertexTmp.SetTextureCoord(1.0f, 0.0f);

    LeafMesh.Vertices[2]=VertexTmp;

    VertexTmp.SetOrigin(Leaf.Origin.x + gdi_BillBoardRtVec.x*Leaf.Coords[6] + gdi_BillBoardUpVec.x*Leaf.Coords[7],
                        Leaf.Origin.y + gdi_BillBoardRtVec.y*Leaf.Coords[6] + gdi_BillBoardUpVec.y*Leaf.Coords[7],
                        Leaf.Origin.z + gdi_BillBoardRtVec.z*Leaf.Coords[6] + gdi_BillBoardUpVec.z*Leaf.Coords[7]);
    VertexTmp.SetTextureCoord(0.0f, 0.0f);

    LeafMesh.Vertices[3]=VertexTmp;

    MatSys::Renderer->RenderMesh(LeafMesh);
}


/// Describes an element in the bucket sorted array of leaves.
struct BucketLLElemT
{
    unsigned long LeafNr;     ///< Index into 'AllLeaves'.
    unsigned long NextElem;   ///< "Pointer" (index) to next element, '0xFFFFFFFF' is "NULL".
};


void TreeT::Draw() const
{
    if (BarkRenderMat==NULL) return; // We can't draw a tree without a render material for our bark.

    // IMPORTANT NOTE:
    // The 'Origin' member and the 'PlayerPos' parameter describe coordinates
    // in  the *OpenGL* coordinate system (y-axis points up, z-axis towards the viewer)!
    // Actually, *all* coords in this class are in the OpenGL coord system!

    #if 0
    glVertexPointer  (3 /*components per coord*/, GL_FLOAT, 0 /*Stride*/, &GlobalAbsCoordTable[0]);
    glTexCoordPointer(2 /*components per coord*/, GL_FLOAT, 0 /*Stride*/, &GlobalTexCoordTable[0]);
    #endif

    MatSys::Renderer->SetCurrentMaterial(BarkRenderMat);

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        // Get view matrix in model space:
        const MatrixT ModelViewMatrixInv=MatSys::Renderer->GetMatrixModelView().GetInverse();

        const Vector3fT ViewerXAxis(ModelViewMatrixInv[0][0], ModelViewMatrixInv[1][0], ModelViewMatrixInv[2][0]); // The x-axis of the viewers coordinate system, expressed in model space.
        const Vector3fT ViewerYAxis(ModelViewMatrixInv[0][1], ModelViewMatrixInv[1][1], ModelViewMatrixInv[2][1]); // The y-axis of the viewers coordinate system, expressed in model space.
        const Vector3fT ViewerZAxis(ModelViewMatrixInv[0][2], ModelViewMatrixInv[1][2], ModelViewMatrixInv[2][2]); // The z-axis of the viewers coordinate system, expressed in model space.
        const Vector3fT ViewerPos  (ModelViewMatrixInv[0][3], ModelViewMatrixInv[1][3], ModelViewMatrixInv[2][3]); // The position of the viewer in model space.

        const Vector3fT ViewerXUnit=normalizeOr0(ViewerXAxis);
        const Vector3fT ViewerYUnit=normalizeOr0(ViewerYAxis);
        const Vector3fT ViewerZUnit=normalizeOr0(ViewerZAxis);

        gdi_BillBoardRtVec.x=ViewerXUnit.x; gdi_BillBoardUpVec.x=ViewerYUnit.x;
        gdi_BillBoardRtVec.y=ViewerXUnit.y; gdi_BillBoardUpVec.y=ViewerYUnit.y;
        gdi_BillBoardRtVec.z=ViewerXUnit.z; gdi_BillBoardUpVec.z=ViewerYUnit.z;

        // Evtl. die Hoehe des Betrachters ausser acht lassen?? (Sodass nur die Entfernung ueber Grund zählt.)
        gdi_DistanceToTreeRoot=length(ViewerPos);

        DrawBranch(Trunk);

        if (DrawLeaves)
        {
            // Apply the LoD technique in order to determine which leaves should be drawn.
            unsigned long NrOfLeavesAfterLoD=0;

            static ArrayT<unsigned long> LeavesAfterLoD;
            if (LeavesAfterLoD.Size()<AllLeaves.Size()) LeavesAfterLoD.PushBackEmpty(AllLeaves.Size()-LeavesAfterLoD.Size());

            #if 1
            GetLeavesAfterLoD(Trunk, LeavesAfterLoD, NrOfLeavesAfterLoD);
            #else
            #warning "LoD is OFF!"
            for (unsigned long LeafNr=0; LeafNr<AllLeaves.Size(); LeafNr++)
                LeavesAfterLoD[NrOfLeavesAfterLoD++]=LeafNr;
            #endif


            if (UseRealBlendingForLeaves)
            {
                // Verwende "echtes" Blending, mit allen Konsequenzen (muss ALLES fuer JEDES Frame korrekt z-sortieren).

                // 1. Compute the distances to all leaves (that were left by LoD).
                static ArrayT<float> AllLeaves_Dists;
                if (AllLeaves_Dists.Size()<AllLeaves.Size()) AllLeaves_Dists.PushBackEmpty(AllLeaves.Size()-AllLeaves_Dists.Size());

                // Old method: Compute (the square of) the (euclidean) distances from the camera to all leaves.
                // The problem is that the normal vector of our billboard polygons does actually *NOT* point towards the camera point,
                // but is rather perpendicular to the camera *plane*!
                // See http://www.lighthouse3d.com/opengl/billboarding/index.php3 for more information.
                    // for (unsigned long LeafNr=0; LeafNr<NrOfLeavesAfterLoD; LeafNr++)
                //   {
                //     const BranchT::LeafT& Leaf=AllLeaves[LeavesAfterLoD[LeafNr]];
                //
                    //     const float DeltaX=gdi_RelativePlayerPos[0]-Leaf.Origin.x;
                //     const float DeltaY=gdi_RelativePlayerPos[1]-Leaf.Origin.y;
                //     const float DeltaZ=gdi_RelativePlayerPos[2]-Leaf.Origin.z;
                    //
                //     AllLeaves_Dists[LeafNr]=DeltaX*DeltaX+DeltaY*DeltaY+DeltaZ*DeltaZ;
                //   }

                // Correct method: Consider the orthogonal projection of each leaf onto the plane spanned by the camera
                // in order to obtain the correct distance. It is easiest to derive the following code by printing out
                // the 'ModelViewMatrix' (and whatever else is required), and then to draw it all on paper.
                for (unsigned long LeafNr=0; LeafNr<NrOfLeavesAfterLoD; LeafNr++)
                {
                    const BranchT::LeafT& Leaf=AllLeaves[LeavesAfterLoD[LeafNr]];

                    const float DeltaX=Leaf.Origin.x-ViewerPos.x;
                    const float DeltaY=Leaf.Origin.y-ViewerPos.y;
                    const float DeltaZ=Leaf.Origin.z-ViewerPos.z;

                    AllLeaves_Dists[LeafNr]=-(DeltaX*ViewerZUnit.x+DeltaY*ViewerZUnit.y+DeltaZ*ViewerZUnit.z);
                }


                // 2. Determine the min and max distances of all leaves.
                float MinDist=AllLeaves_Dists[0];
                float MaxDist=AllLeaves_Dists[0];

                for (unsigned long LeafNr=1; LeafNr<NrOfLeavesAfterLoD; LeafNr++)
                {
                    if (AllLeaves_Dists[LeafNr]<MinDist) MinDist=AllLeaves_Dists[LeafNr];
                    if (AllLeaves_Dists[LeafNr]>MaxDist) MaxDist=AllLeaves_Dists[LeafNr];
                }


                // 3. Use "Bucket Sort".
                static ArrayT<unsigned long> BucketLLHeads;    // Size - how many buckets?  BucketLLHeads.Size() e.g. == 2048, or c*NrOfLeavesAfterLoD.
                static ArrayT<BucketLLElemT> BucketLLElems;    // Size - how many leaves to sort in?  BucketLLElems.Size() >= AllLeaves.Size();

                if (BucketLLHeads.Size()<AllLeaves.Size()) BucketLLHeads.PushBackEmpty(AllLeaves.Size()-BucketLLHeads.Size());
                if (BucketLLElems.Size()<AllLeaves.Size()) BucketLLElems.PushBackEmpty(AllLeaves.Size()-BucketLLElems.Size());

                // How many buckets do we *actually use*? Here, use as many buckets as there are leaves (after LoD).
                // Changing this value is one way to increase (or decrease) the "quality" of the sorting.
                // (But note that the size of 'BucketLLHeads' must then probably be adapted above!)
                const unsigned long BucketLLHeads_USE=NrOfLeavesAfterLoD;

                // Set all head pointers to '0xFFFFFFFF' (our special "NULL").
                for (unsigned long LLHeadNr=0; LLHeadNr<BucketLLHeads_USE; LLHeadNr++)
                    BucketLLHeads[LLHeadNr]=0xFFFFFFFF;

                const float   ScaleFactor =(BucketLLHeads_USE-1)/(MaxDist-MinDist);
                unsigned long NextFreeElem=0;

                for (unsigned long LeafNr=0; LeafNr<NrOfLeavesAfterLoD; LeafNr++)
                {
                    const unsigned long BucketNr=BucketLLHeads_USE-1 - (unsigned long)((AllLeaves_Dists[LeafNr]-MinDist)*ScaleFactor);

                    // Insert leaf with index 'LeafNr' into the linked list at bucket 'BucketNr'.
                    BucketLLElems[NextFreeElem].LeafNr  =LeavesAfterLoD[LeafNr];
                    BucketLLElems[NextFreeElem].NextElem=BucketLLHeads[BucketNr];

                    BucketLLHeads[BucketNr]=NextFreeElem;
                    NextFreeElem++;
                }


                // 4. Render all leaves in sorted order, using alpha blending.
                for (unsigned long BucketNr=0; BucketNr<BucketLLHeads_USE; BucketNr++)
                {
                    // Note: We do not make any attempt to sort the linked list starting at 'BucketLLHeads[BucketNr]',
                    // because for our purposes the sorting error by not doing so is so small that it is hardly ever noticeable at all!
                    // Adding a sort algorithm here is the second way to increase the "quality" of the sorting.
                    for (unsigned long LLElemIdx=BucketLLHeads[BucketNr]; LLElemIdx!=0xFFFFFFFF; LLElemIdx=BucketLLElems[LLElemIdx].NextElem)
                        DrawLeaf(AllLeaves[BucketLLElems[LLElemIdx].LeafNr]);
                }
            }
            else
            {
                for (unsigned long LeafNr=0; LeafNr<NrOfLeavesAfterLoD; LeafNr++)
                    DrawLeaf(AllLeaves[LeavesAfterLoD[LeafNr]]);
            }
        }

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
}
