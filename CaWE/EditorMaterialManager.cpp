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

#include "EditorMaterialManager.hpp"
#include "EditorMaterialDummy.hpp"
#include "EditorMaterialEngine.hpp"
#include "GameConfig.hpp"
#include "MaterialSystem/MaterialManager.hpp"

#include "wx/confbase.h"


// Compare function for sorting materials.
static bool CompareMaterials(EditorMaterialI* const& elem1, EditorMaterialI* const& elem2)
{
    return wxStricmp(elem1->GetName(), elem2->GetName())<0;
}


EditorMatManT::EditorMatManT(const GameConfigT& GameConfig)
    : m_MaterialMan(),
      m_Materials(),
      m_DefaultMaterial(NULL),
      m_LazyMatUpdateCount(0)
{
    // Register all material scripts of this game and obtain all found materials.
    ArrayT<MaterialT*> Materials=m_MaterialMan.RegisterMaterialScriptsInDir(std::string(GameConfig.ModDir)+"/Materials", std::string(GameConfig.ModDir)+"/");

    // Create an editor material for each loaded engine material.
    for (unsigned long MaterialNr=0; MaterialNr<Materials.Size(); MaterialNr++)
        m_Materials.PushBack(new EngineMaterialT(Materials[MaterialNr]));

    if (m_Materials.Size()==0)
    {
        wxMessageBox("WARNING: No materials found in directory\n"+GameConfig.ModDir+"/Materials\n"
                     "We can continue, but the materials will NOT work.\n"
                     "Something is wrong that really should be fixed!", "Missing Materials", wxOK | wxICON_ERROR);

        m_Materials.PushBack(new DummyMaterialT("Error: Materials not loaded!"));
    }

    m_Materials.QuickSort(CompareMaterials);
}


EditorMatManT::~EditorMatManT()
{
    // Delete all the materials from the list.
    for (unsigned long MatNr=0; MatNr<m_Materials.Size(); MatNr++) delete m_Materials[MatNr];
}


EditorMaterialI* EditorMatManT::GetDefaultMaterial()
{
    if (m_DefaultMaterial) return m_DefaultMaterial;

    // m_DefaultMaterial is NULL (so this is the very first call to this function).
    const wxString DefaultName=wxConfigBase::Get()->Read("General/InitialDefaultMaterial", "TechDemo/walls/wall-13b").Lower();

    for (unsigned long MatNr=0; MatNr<m_Materials.Size(); MatNr++)
    {
        // Perform a case-insensitive find.
        if (m_Materials[MatNr]->GetName().Lower().Find(DefaultName)!=wxNOT_FOUND)
        {
            m_DefaultMaterial=m_Materials[MatNr];
            return m_DefaultMaterial;
        }
    }

    for (unsigned long MatNr=0; MatNr<m_Materials.Size(); MatNr++)
    {
        if (m_Materials[MatNr]->GetName().Lower().Find("wall")!=wxNOT_FOUND && m_Materials[MatNr]->ShowInMaterialBrowser())
        {
            m_DefaultMaterial=m_Materials[MatNr];
            return m_DefaultMaterial;
        }
    }

    // Try really hard to avoid dummy materials - take first material that is visible in the editor.
    for (unsigned long MatNr=0; MatNr<m_Materials.Size(); MatNr++)
    {
        if (m_Materials[MatNr]->ShowInMaterialBrowser())
        {
            m_DefaultMaterial=m_Materials[MatNr];
            return m_DefaultMaterial;
        }
    }

    m_DefaultMaterial=FindMaterial(DefaultName, true);
    return m_DefaultMaterial;
}


void EditorMatManT::SetDefaultMaterial(EditorMaterialI* DefaultMat)
{
    m_DefaultMaterial=DefaultMat;
}


EditorMaterialI* EditorMatManT::FindMaterial(const wxString& MatName, bool CreateDummyIfNotFound)
{
    static EditorMaterialI* LastMat=NULL;

    // wxASSERT(MatName!="" || !CreateDummyIfNotFound);

    // Before doing a full search, check if MatName is the same as in the last call.
    if (LastMat && !wxStricmp(MatName, LastMat->GetName()))
        return LastMat;

    for (unsigned long MatNr=0; MatNr<m_Materials.Size(); MatNr++)
        if (!wxStricmp(MatName, m_Materials[MatNr]->GetName()))
        {
            LastMat=m_Materials[MatNr];
            return LastMat;
        }


    // Not found. If we shall not create a dummy in this case, just return NULL.
    if (!CreateDummyIfNotFound) return NULL;


    // Not found - create a dummy as a placeholder for the missing material.
    EditorMaterialI* Dummy=new DummyMaterialT(MatName.c_str());

    m_Materials.PushBack(Dummy);
    LastMat=Dummy;
    return Dummy;
}


void EditorMatManT::LazilyUpdateProxies()
{
    if (m_LazyMatUpdateCount==0) { wxLogDebug("Beginning to cache in materials."); }
    if ((m_LazyMatUpdateCount % 100)==0 && m_LazyMatUpdateCount<m_Materials.Size()) { wxLogDebug("Caching in material number %lu of %lu materials.", m_LazyMatUpdateCount, m_Materials.Size()); }
    if (m_LazyMatUpdateCount+1==m_Materials.Size()) { wxLogDebug("Done caching in %lu materials.", m_Materials.Size()); }

    // Do nothing once we're done caching in.
    if (m_LazyMatUpdateCount>=m_Materials.Size()) return;

    // Also do nothing while one of the mouse buttons is down, the user is probably interacting with the editor then.
    const wxMouseState MS=wxGetMouseState();
    if (MS.LeftIsDown() || MS.MiddleIsDown() || MS.RightIsDown()) return;

    m_Materials[m_LazyMatUpdateCount]->GetImage();
    m_LazyMatUpdateCount++;
}
