/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

/******************************/
/*** Particle Engine (Code) ***/
/******************************/

#include <math.h>

#include "ParticleEngineMS.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Templates/Array.hpp"


// General notes about this particle engine:
// Particles are generally assumed to be rendered in "additive blend mode", such that no depth sorting of any kind is required.
// Also the code *design* can probably not handle that: Compare e.g. to the article at www.gametutorials.com / Gamasutra,
// that mentions intermediate classes of type 'ParticleSystemT', that e.g. might also include BBs or other sorting aids.
// TODO: Add support to play animated sprites and GIF files!?

const unsigned long MAX_NR_OF_PARTICLES=512;

struct ParticleGroupT
{
    MatSys::RenderMaterialT* RenderMat;                             // The common RenderMaterial of all particles in this group.
    ArrayT<ParticleMST*>     ParticlePtrs;                          // Contains pointers into 'AllParticles' of all alive particles in this group.
};


static ParticleMST            AllParticles[MAX_NR_OF_PARTICLES];    // The global, "static" array of all particles.
static ArrayT<ParticleMST*>   DeadParticlePtrs;                     // Contains pointers into 'AllParticles' of all dead/unused particles.
static ArrayT<ParticleGroupT> ParticleGroups;                       // Each distinct texture gets it own particle group.
static float                  CosTab[256];                          // Look-up table for cosinus values (256 is a full circle).
static float                  SinTab[256];                          // Look-up table for sinus   values (256 is a full circle).


static unsigned long FindGroupIndexByRenderMat(MatSys::RenderMaterialT* RM)
{
    for (unsigned long PGNr=0; PGNr<ParticleGroups.Size(); PGNr++)
        if (ParticleGroups[PGNr].RenderMat==RM) return PGNr;

    return ParticleGroups.Size();
}


class ParticleEngineInitializerT
{
    public:

    ParticleEngineInitializerT()
    {
        for (unsigned long ParticleNr=0; ParticleNr<MAX_NR_OF_PARTICLES; ParticleNr++)
            DeadParticlePtrs.PushBack(&AllParticles[ParticleNr]);

        for (unsigned long Angle=0; Angle<256; Angle++)
        {
            CosTab[Angle]=cos(Angle/128.0f*3.14159265358979323846f);
            SinTab[Angle]=sin(Angle/128.0f*3.14159265358979323846f);
        }
    }


    ~ParticleEngineInitializerT()
    {
        ParticleGroups.Clear();
    }
};

static ParticleEngineInitializerT PEI;


void ParticleEngineMS::RegisterNewParticle(const ParticleMST& Particle)
{
    // If there currently no dead/unused particle exists (they are all alive),
    // we cannot do much - the limits are hit and this particle is ignored.
    if (DeadParticlePtrs.Size()==0) return;

    // Put the pointer from the dead/unused list into the alive/active list of the corresponding group,
    // and overwrite the particle that it pointed to (in the 'AllParticles' array) with 'Particle'.
    unsigned long PGNr=FindGroupIndexByRenderMat(Particle.RenderMat);

    if (PGNr>=ParticleGroups.Size())
    {
        ParticleGroups.PushBackEmpty();

        ParticleGroups[PGNr].RenderMat=Particle.RenderMat;
    }

    ParticleGroupT& PG=ParticleGroups[PGNr];

    PG.ParticlePtrs.PushBack(DeadParticlePtrs[DeadParticlePtrs.Size()-1]);
    DeadParticlePtrs.DeleteBack();
    *PG.ParticlePtrs[PG.ParticlePtrs.Size()-1]=Particle;
}


void ParticleEngineMS::MoveParticles(float Time)
{
    static ArrayT<ParticleMST*> ChangeGroupParticlePtrs;

    ChangeGroupParticlePtrs.Overwrite();

    for (unsigned long PGNr=0; PGNr<ParticleGroups.Size(); PGNr++)
    {
        ParticleGroupT& PG=ParticleGroups[PGNr];

        for (unsigned long ParticleNr=0; ParticleNr<PG.ParticlePtrs.Size(); ParticleNr++)
        {
            ParticleMST*             Particle    =PG.ParticlePtrs[ParticleNr];
            MatSys::RenderMaterialT* OldRenderMat=Particle->RenderMat;
            const bool               IsStillAlive=Particle->MoveFunction(Particle, Time);

            if (!IsStillAlive)
            {
                // This particle died.
                // Thus, put its pointer back from the alive/active list of this group (index 'PGNr') to the dead/unused list.
                // First, put the pointer into the dead/unused list:
                DeadParticlePtrs.PushBack(Particle);

                // Second, delete it from the alive/active list (and also take the effect on the loop into account):
                PG.ParticlePtrs.RemoveAt(ParticleNr);
                ParticleNr--;
                continue;
            }

            if (OldRenderMat!=Particle->RenderMat)
            {
                // This particle is animated: It has changed its 'RenderMat', and thus the particle group it is in.
                // We want to put it from the current group into the new group,
                // but we cannot do that directly, in order to prevent it from being processed multiple times.
                // First, put the pointer into the "changed group" list:
                ChangeGroupParticlePtrs.PushBack(Particle);

                // Second, delete it from the current (but old) group (and also take the effect on the loop into account):
                PG.ParticlePtrs.RemoveAt(ParticleNr);
                ParticleNr--;
                continue;
            }
        }
    }

    // Put the particles that changed group into their new group.
    for (unsigned long ParticleNr=0; ParticleNr<ChangeGroupParticlePtrs.Size(); ParticleNr++)
    {
        unsigned long NewGroupNr=FindGroupIndexByRenderMat(ChangeGroupParticlePtrs[ParticleNr]->RenderMat);

        if (NewGroupNr>=ParticleGroups.Size())
        {
            // The particle might have changed into a group that does not yet exist,
            // simply because it might have been assigned a RenderMat that we have never seen in RegisterNewParticle() before.
            ParticleGroups.PushBackEmpty();

            ParticleGroups[NewGroupNr].RenderMat=ChangeGroupParticlePtrs[ParticleNr]->RenderMat;
        }

        ParticleGroups[NewGroupNr].ParticlePtrs.PushBack(ChangeGroupParticlePtrs[ParticleNr]);
    }
}


void ParticleEngineMS::DrawParticles()
{
    // Actually draw the particles now.
    static MatSys::MeshT ParticleGroupMesh(MatSys::MeshT::TriangleFan);
    ParticleGroupMesh.Vertices.Overwrite();
    ParticleGroupMesh.Vertices.PushBackEmpty(4);

    const MatrixT& ModelView=MatSys::Renderer->GetMatrixModelView();


    for (unsigned long PGNr=0; PGNr<ParticleGroups.Size(); PGNr++)
    {
        MatSys::Renderer->SetCurrentMaterial(ParticleGroups[PGNr].RenderMat);

        for (unsigned long ParticleNr=0; ParticleNr<ParticleGroups[PGNr].ParticlePtrs.Size(); ParticleNr++)
        {
            const ParticleMST* Particle=ParticleGroups[PGNr].ParticlePtrs[ParticleNr];

         // const float LocalXAxis[3]={ Particle->Radius,              0.0, 0.0 };
         // const float LocalYAxis[3]={              0.0, Particle->Radius, 0.0 };

         // const float BillBoardVecX[3]={ ModelView.m[0]*Particle->Radius, ModelView.m[4]*Particle->Radius, ModelView.m[8]*Particle->Radius };
         // const float BillBoardVecY[3]={ ModelView.m[1]*Particle->Radius, ModelView.m[5]*Particle->Radius, ModelView.m[9]*Particle->Radius };

            const float LocalXAxis[3]={ CosTab[Particle->Rotation]*Particle->Radius                   , -SinTab[Particle->Rotation]*Particle->Radius                   , 0 };
            const float LocalYAxis[3]={ SinTab[Particle->Rotation]*Particle->Radius*Particle->StretchY,  CosTab[Particle->Rotation]*Particle->Radius*Particle->StretchY, 0 };

            const float BillBoardVecX[3]={ ModelView.m[0][0]*LocalXAxis[0]+ModelView.m[1][0]*LocalXAxis[1], ModelView.m[0][1]*LocalXAxis[0]+ModelView.m[1][1]*LocalXAxis[1], ModelView.m[0][2]*LocalXAxis[0]+ModelView.m[1][2]*LocalXAxis[1] };
            const float BillBoardVecY[3]={ ModelView.m[0][0]*LocalYAxis[0]+ModelView.m[1][0]*LocalYAxis[1], ModelView.m[0][1]*LocalYAxis[0]+ModelView.m[1][1]*LocalYAxis[1], ModelView.m[0][2]*LocalYAxis[0]+ModelView.m[1][2]*LocalYAxis[1] };

            MatSys::Renderer->SetGenPurposeRenderingParam(0, Particle->Color[0]/255.0f);
            MatSys::Renderer->SetGenPurposeRenderingParam(1, Particle->Color[1]/255.0f);
            MatSys::Renderer->SetGenPurposeRenderingParam(2, Particle->Color[2]/255.0f);
            MatSys::Renderer->SetGenPurposeRenderingParam(3, Particle->Color[3]/255.0f);

            // It seems that the Y-axis points DOWN...
            ParticleGroupMesh.Vertices[0].SetTextureCoord(0.0f, 0.0f);
            ParticleGroupMesh.Vertices[0].SetOrigin(Particle->Origin[0]-BillBoardVecX[0]+BillBoardVecY[0],
                                                    Particle->Origin[1]-BillBoardVecX[1]+BillBoardVecY[1],
                                                    Particle->Origin[2]-BillBoardVecX[2]+BillBoardVecY[2]);

            ParticleGroupMesh.Vertices[1].SetTextureCoord(1.0f, 0.0f);
            ParticleGroupMesh.Vertices[1].SetOrigin(Particle->Origin[0]+BillBoardVecX[0]+BillBoardVecY[0],
                                                    Particle->Origin[1]+BillBoardVecX[1]+BillBoardVecY[1],
                                                    Particle->Origin[2]+BillBoardVecX[2]+BillBoardVecY[2]);

            ParticleGroupMesh.Vertices[2].SetTextureCoord(1.0f, 1.0f);
            ParticleGroupMesh.Vertices[2].SetOrigin(Particle->Origin[0]+BillBoardVecX[0]-BillBoardVecY[0],
                                                    Particle->Origin[1]+BillBoardVecX[1]-BillBoardVecY[1],
                                                    Particle->Origin[2]+BillBoardVecX[2]-BillBoardVecY[2]);

            ParticleGroupMesh.Vertices[3].SetTextureCoord(0.0f, 1.0f);
            ParticleGroupMesh.Vertices[3].SetOrigin(Particle->Origin[0]-BillBoardVecX[0]-BillBoardVecY[0],
                                                    Particle->Origin[1]-BillBoardVecX[1]-BillBoardVecY[1],
                                                    Particle->Origin[2]-BillBoardVecX[2]-BillBoardVecY[2]);

            // Warning: This testing code requires "twoSided" materials!
            // ParticleGroupMesh.Vertices[4*ParticleNr+0].SetOrigin(Particle->Origin[0]-300.0, Particle->Origin[1], Particle->Origin[2]+300.0);
            // ParticleGroupMesh.Vertices[4*ParticleNr+1].SetOrigin(Particle->Origin[0]+300.0, Particle->Origin[1], Particle->Origin[2]+300.0);
            // ParticleGroupMesh.Vertices[4*ParticleNr+2].SetOrigin(Particle->Origin[0]+300.0, Particle->Origin[1], Particle->Origin[2]-300.0);
            // ParticleGroupMesh.Vertices[4*ParticleNr+3].SetOrigin(Particle->Origin[0]-300.0, Particle->Origin[1], Particle->Origin[2]-300.0);

            // It's stupid to have each particle drawn as an individual mesh, as we could have drawn all particles of a common group
            // in a single "Quads" mesh, but then the only way to get particles individually colored is using per-vertex mesh colors
            // ("useMeshColors" as material property). This however is not yet supported by most shaders, so that I rather revert to
            // this solution for now.
            MatSys::Renderer->RenderMesh(ParticleGroupMesh);
        }
    }
}
