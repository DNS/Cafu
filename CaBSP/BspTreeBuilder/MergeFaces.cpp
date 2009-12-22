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

/*******************/
/*** Merge Faces ***/
/*******************/


void BspTreeBuilderT::MergeFaces()
{
    Console->Print(cf::va("\n%-50s %s\n", "*** Merge Faces ***", GetTimeSinceProgramStart()));

    unsigned long NrOfFaces;

    for (unsigned long FirstFace=0; FirstFace<FaceChildren.Size(); FirstFace+=NrOfFaces)
    {
        unsigned long Face1Nr;

        Console->Print(cf::va("%5.1f%%\r", (double)FirstFace/FaceChildren.Size()*100.0));
        // fflush(stdout);      // The stdout console auto-flushes the output.

        NrOfFaces=1;

        for (Face1Nr=FirstFace+NrOfFaces; Face1Nr<FaceChildren.Size(); Face1Nr++)
            if (FaceChildren[Face1Nr]->Polygon.WhatSide(FaceChildren[FirstFace]->Polygon.Plane, MapT::RoundEpsilon)==Polygon3T<double>::InIdentical)
            {
                std::swap(FaceChildren[FirstFace+NrOfFaces], FaceChildren[Face1Nr]);
                NrOfFaces++;
            }

        for (Face1Nr=FirstFace; Face1Nr<FirstFace+NrOfFaces; Face1Nr++)
            for (unsigned long Face2Nr=FirstFace; Face2Nr<FirstFace+NrOfFaces; Face2Nr++)
            {
                const cf::SceneGraph::FaceNodeT* F1=FaceChildren[Face1Nr];
                const cf::SceneGraph::FaceNodeT* F2=FaceChildren[Face2Nr];

                if (F1->Material!=F2->Material) continue;

                if (F1->TI.U      !=F2->TI.U      ) continue;
                if (F1->TI.V      !=F2->TI.V      ) continue;
                if (F1->TI.OffsetU!=F2->TI.OffsetU) continue;
                if (F1->TI.OffsetV!=F2->TI.OffsetV) continue;

                try
                {
                    Polygon3T<double> NewPoly=Polygon3T<double>::Merge(F1->Polygon, F2->Polygon, MapT::RoundEpsilon);

                    // Trotz aller Vorsicht k�nnen im neuen Poly doch Vertices weiter als +/- GeometryEpsilon von der Plane
                    // entfernt liegen oder Vertices zusammenfallen (bei 'schwierigen' Eingabepolys (g�ltig, aber im Grenzbereich)).
                    if (!NewPoly.IsValid(MapT::RoundEpsilon, MapT::MinVertexDist)) continue;

                    FaceChildren[Face1Nr]->Polygon=NewPoly;
                }
                catch (const InvalidOperationE&) { continue; }

                delete FaceChildren[Face2Nr];
                FaceChildren[Face2Nr]              =FaceChildren[FirstFace+NrOfFaces-1];
                FaceChildren[FirstFace+NrOfFaces-1]=FaceChildren[FaceChildren.Size()-1];
                NrOfFaces--;
                FaceChildren.DeleteBack();
                Face1Nr=FirstFace-1;
                break;
            }
    }

    Console->Print(cf::va("Faces               : %10lu\n", FaceChildren.Size()));
}
