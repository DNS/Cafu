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
/*** Model Proxy ***/
/*******************/

#include "Loader_lwo.hpp"
#include "Loader_md5.hpp"
#include "Model_ase.hpp"
#include "Model_cmdl.hpp"
#include "Model_dlod.hpp"
#include "Model_dummy.hpp"
#include "Model_mdl.hpp"
#include "Model_proxy.hpp"
#include "String.hpp"


/// This is the big global model pool (or rather, its manager), implemented as a Singleton, as only one instance is ever needed.
/// If there ever was an intention to share the model pool across exe/dll modules, we'll have to hide the data behind yet another pure ABC...
/// The main purpose of the ModelPoolManagerT (versus simply making its data global variables) is
/// a) to foresee for its possibly future use as pure ABC as indicated above, and
/// b) to have it clean up all remaining (orphaned) pool contents in its destructor (which is called on program exit).
/// Although there is is not really a requirement for this kind of cleaning-up at program exit, I consider b) to be more important than a) for now.
class ModelPoolManagerT
{
    public:

    ArrayT<ModelT*>       ModelPool;       ///< Pool of available models.
    ArrayT<unsigned long> ModelPoolCounts; ///< Number of model proxys using the ModelT at index.

    /// The destructor.
    /// I don't think that it is possible to have some ModelPool[i] still allocated here whose ModelPoolCounts[i] is > 0,
    /// because then there would be a preceeding missing call to a ModelProxyT destructor somehow, which should never occur.
    /// However, "properly orphaned" pool entries (ModelPool[i] is still allocated with ModelPoolCounts[i]==0) can well occur
    /// when the ModelProxyT, that effectively manages the pool, too, has decided to NOT remove the model from the pool even
    /// though its reference count has dropped to 0.
    ~ModelPoolManagerT()
    {
        for (unsigned long PoolNr=0; PoolNr<ModelPool.Size(); PoolNr++)
        {
            // Assert(ModelPoolCounts[PoolNr]==0);
            delete ModelPool[PoolNr];
        }
    }

    /// This method implements the Singleton pattern.
    static ModelPoolManagerT& Get()
    {
        // NOTE: This leads to the static DEinitialization order problem as soon as somebody creates
        // a static ModelProxyT object (and this MPM is destructed before that ModelProxyT object).
        // FIXME: Update documentation above and remove the (not any longer needed) dtor!
        // static ModelPoolManagerT MPM;
        // return MPM;

        static ModelPoolManagerT* MPM=new ModelPoolManagerT();

        return *MPM;
    }


    private:

    /// The usual constructor is only for private use (Singleton pattern).
    ModelPoolManagerT()
    {
        ModelPool      .PushBack(new ModelDummyT("dummy"));
        ModelPoolCounts.PushBack(1);
    }

    ModelPoolManagerT(const ModelPoolManagerT&);    // Use of the Copy    Constructor is not allowed.
    void operator = (const ModelPoolManagerT&);     // Use of the Assignment Operator is not allowed.
};


// Some abbreviations, for convenience.
static ArrayT<ModelT*>&       ModelPool      =ModelPoolManagerT::Get().ModelPool;
static ArrayT<unsigned long>& ModelPoolCounts=ModelPoolManagerT::Get().ModelPoolCounts;


// The constructor.
ModelProxyT::ModelProxyT()
{
    // Assign index 0 to a model proxy that is contructed by the default contructor.
    // The model pool manager stores a dummy model at index 0 that always exists.
    PoolIndex=0;
    ModelPoolCounts[PoolIndex]++;
}


// The constructor.
ModelProxyT::ModelProxyT(const std::string& FileName)
{
    // If there is already the appropriate model in the pool, this proxy becomes another reference to it.
    for (unsigned long PoolNr=0; PoolNr<ModelPool.Size(); PoolNr++)
        if (ModelPool[PoolNr] && ModelPool[PoolNr]->GetFileName()==FileName)
        {
            PoolIndex=PoolNr;
            ModelPoolCounts[PoolIndex]++;
            return;
        }

    // No model with FileName was yet found in the pool.
    // So first decide which concrete model class it actually is, then add it to the pool.
    ModelT* NewModel=NULL;

    try
    {
             if (cf::String::EndsWith(FileName, "ase"    )) NewModel=new ModelAseT (FileName);
        else if (cf::String::EndsWith(FileName, "dlod"   )) NewModel=new ModelDlodT(FileName);
        else if (cf::String::EndsWith(FileName, "mdl"    )) NewModel=new ModelMdlT (FileName);
        else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "md5anim")) { LoaderMd5T Loader(FileName); NewModel=new CafuModelT(Loader); }
        else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT Loader(FileName); NewModel=new CafuModelT(Loader); }
        else throw ModelT::LoadError();
    }
    catch (const ModelT::LoadError&)
    {
        // Okay, the model could not be loaded for some reason.
        // Do just substitute a dummy model for now.
        NewModel=new ModelDummyT(FileName);
    }

    // Finally find a good place in the pool:
    // We could simply PushBack(), but maybe there is an unused slot somewhere else (earlier).
    for (PoolIndex=0; PoolIndex<ModelPool.Size(); PoolIndex++)
        if (ModelPool[PoolIndex]==NULL)
            break;

    if (PoolIndex<ModelPool.Size())
    {
        ModelPool      [PoolIndex]=NewModel;
        ModelPoolCounts[PoolIndex]=1;
    }
    else
    {
        ModelPool      .PushBack(NewModel);
        ModelPoolCounts.PushBack(1);
    }
}


// The destructor.
ModelProxyT::~ModelProxyT()
{
    ModelPoolCounts[PoolIndex]--;

    if (ModelPoolCounts[PoolIndex]==0)
    {
        // TODO: Echt löschen z.B. nur wenn ModelResources.Size()>20, und dann am besten erstmal prüfen,
        //       ob "ältere" unbenutzte ModelResources zuerst freigegeben werden können!
        delete ModelPool[PoolIndex];
        ModelPool[PoolIndex]=NULL;
    }
}


// The copy constructor.
ModelProxyT::ModelProxyT(const ModelProxyT& Other)
{
    PoolIndex=Other.PoolIndex;

    ModelPoolCounts[PoolIndex]++;
}


// The assignment operator.
ModelProxyT& ModelProxyT::operator = (const ModelProxyT& Other)
{
    // ATTENTION: The order of these statements is important in order to handle self-assignment implicitly right!
    ModelPoolCounts[      PoolIndex]--;
    ModelPoolCounts[Other.PoolIndex]++;

    if (ModelPoolCounts[PoolIndex]==0)
    {
        // TODO: Echt löschen z.B. nur wenn ModelPool.Size()>20, und dann am besten erstmal prüfen,
        //       ob "ältere" unbenutzte ModelPool Einträge zuerst freigegeben werden können!
        delete ModelPool[PoolIndex];
        ModelPool[PoolIndex]=NULL;
    }

    PoolIndex=Other.PoolIndex;

    return *this;
}


const ModelT* ModelProxyT::GetRealModel() const
{
    return ModelPool[PoolIndex];
}


const std::string& ModelProxyT::GetFileName() const
{
    return ModelPool[PoolIndex]->GetFileName();
}


void ModelProxyT::Draw(int SequenceNr, float FrameNr, float LodDist, const ModelT* SubModel) const
{
    ModelPool[PoolIndex]->Draw(SequenceNr, FrameNr, LodDist, SubModel);
}


bool ModelProxyT::GetGuiPlane(int SequenceNr, float FrameNr, float LodDist, Vector3fT& GuiOrigin, Vector3fT& GuiAxisX, Vector3fT& GuiAxisY) const
{
    return ModelPool[PoolIndex]->GetGuiPlane(SequenceNr, FrameNr, LodDist, GuiOrigin, GuiAxisX, GuiAxisY);
}


void ModelProxyT::Print() const
{
    ModelPool[PoolIndex]->Print();
}


int ModelProxyT::GetNrOfSequences() const
{
    return ModelPool[PoolIndex]->GetNrOfSequences();
}


const float* ModelProxyT::GetSequenceBB(int SequenceNr, float FrameNr) const
{
    return ModelPool[PoolIndex]->GetSequenceBB(SequenceNr, FrameNr);
}


// float ModelProxyT::GetNrOfFrames(int SequenceNr) const
// {
//     return ModelPool[PoolIndex]->GetNrOfFrames();
// }


float ModelProxyT::AdvanceFrameNr(int SequenceNr, float FrameNr, float DeltaTime, bool Loop) const
{
    return ModelPool[PoolIndex]->AdvanceFrameNr(SequenceNr, FrameNr, DeltaTime, Loop);
}
