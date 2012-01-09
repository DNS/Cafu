/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "DialogInsp-EntityProps.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "GameConfig.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapEntity.hpp"
#include "MapPrimitive.hpp"

#include "wx/notebook.h"
#include "wx/propgrid/propgrid.h"
#include "wx/propgrid/manager.h"
#include "wx/propgrid/advprops.h"

#include "MapCommands/DeleteProp.hpp"
#include "MapCommands/SetProp.hpp"
#include "MapCommands/ChangeClass.hpp"

/* Introduction to the Implementation:
 * ***********************************
 *
 * Initially, consider the situation with just a single entity: The entity keeps a list of key/value pairs (its properties),
 * as well as a reference to the definition of its entity class, which in turn contains definitions of the variables of that entity class.
 * Normally, each entity property is the instantiation of its related variable definition.
 * When we now traverse the properties and variable defintions list for presenting the result to the user, three cases can occur:
 *
 *     1a) We have a key (variable name) for which there is a variable definition as well as an entry in the property list
 *         that specifies the concrete value. This is the normal case.
 *
 *     1b) We find the key among the variable definitions, but not in the property list.
 *         The fact that the key is omitted from the property list just means that the default value from the variable def. is to be used.
 *
 *     2)  There is a property kept in the entity, but no variable definition with the same name is found in the entity class
 *         (maybe the entity doesn't even *have* an entity class).
 *         This is somewhat unusual but frequently happens with entities imported from other game configs or games.
 *
 * In summary, we are confronted with two essential cases: 1) keys that also have a well-known variable definition (with default or
 * custom value) and 2) keys that have no variable definition, but a concrete value.
 * Dealing with and presenting both cases is straightforward.
 *
 * Now consider the situation with multiple (more than one) entities:
 * [...] [overlay...]
 *
 * Nomenclature: property, key/value pair, variable, ...
 */


// Colors used in wxPropertyGrid.
static const wxColour COLOR_CUSTOM      ("#FFFFFF"); // This is the standard color for every property. Its value is custom and the same for all selected entities.
static const wxColour COLOR_DEFAULT     ("#CCDDFF"); // This color is used to indicate that all selected entities have the same value and that this value is the default value.
static const wxColour COLOR_INCONSISTENT("#FFDDCC"); // Indicates that the selected entities have different values in this property.
static const wxColour COLOR_WARNING     ("#FF9797"); // Used to warn the user about possibly faulty properties.


// FIXME: Rename these constants into UNDEF_IN_CLASS, DEFINED, MISMATCH/INCOMPATIBLE ?
enum PropKeyStateE
{
    UNDEFINED,  ///< Key is not defined by this entities class.
    NORMAL,     ///< Key is defined by this entities class.
    MIXED       ///< Key is defined by different classes (Note: UNDEFINED+NORMAL=MIXED).
};


/// "Overlay information" about each property in the grid.
struct PropInfoT
{
    PropKeyStateE       KeyState;
    bool                ValueIsConsistent;  ///< True if the value is consistent (equal) across all selected entities.
    bool                ValueIsDefault;     ///< True if the value matches the default value. This is relevant only if KeyState==NORMAL, ValueIsConsistent==true and all defaults of multiple entities are actually identical.
    const EntClassVarT* ClassVar;

    PropInfoT()
        : KeyState(UNDEFINED),
          ValueIsConsistent(true),
          ValueIsDefault(false),
          ClassVar(NULL)
    {
    }

    PropInfoT(PropKeyStateE KeyState_, bool ValueIsConsistent_, bool ValueIsDefault_, const EntClassVarT* ClassVar_)
        : KeyState(KeyState_),
          ValueIsConsistent(ValueIsConsistent_),
          ValueIsDefault(ValueIsDefault_),
          ClassVar(ClassVar_)
    {
    }
};


/// Helper function to translate the two color string representations:
/// "255 128 64" (cmap file) and "(255,128,64)" (wxPropertyGrid)
/// If wxColourObj is true, the function will create a string that can be used to create a wxColour object from it.
/// Beware: Strings that are not in one of the formats above are not caught and might result in strange return values.
static wxString TranslateColorString(wxString colorstring, bool wxColourObj=false)
{
    if (colorstring.StartsWith("("))
    {
        // wxPropertyGrid representation.
        if (wxColourObj)
        {
            colorstring="RGB"+colorstring;
        }
        else
        {
            colorstring.Replace("(", "");
            colorstring.Replace(")", "");
            colorstring.Replace(",", " ");
        }
    }
    else
    {
        // cmap file representation.
        colorstring.Replace(" ", ",");
        colorstring="("+colorstring+")";
        if (wxColourObj) colorstring="RGB"+colorstring;
    }

    return colorstring;
}


// The purpose of this class is to select model files in the property grid.
// This property is derived from wxLongStringProperty because deriving from
// wxFileProperty and overloading the OnButtonClick method doesn't work.
class GameFilePropertyT : public wxLongStringProperty
{
    public:

    GameFilePropertyT(const wxString& name=wxPG_LABEL,
                      const wxString& label=wxPG_LABEL,
                      const wxString& value=wxEmptyString,
                      MapDocumentT* MapDoc_=NULL,
                      wxString Filter_="All files (*.*)|*.*",
                      wxString SubDir_=wxEmptyString)
        : wxLongStringProperty(name, label, value),
          MapDoc(MapDoc_),
          Filter(Filter_),
          SubDir(SubDir_)
    {
    }

    // Shows the file selection dialog and makes the choosen file path relative.
    virtual bool OnButtonClick(wxPropertyGrid* propGrid, wxString& value)
    {
        wxString InitialDir =MapDoc->GetGameConfig()->ModDir+SubDir;
        wxString FileNameStr=wxFileSelector("Please select a file", InitialDir, "", "", Filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (FileNameStr=="") return false;

        wxFileName FileName(FileNameStr);
        FileName.MakeRelativeTo(MapDoc->GetGameConfig()->ModDir);
        value=FileName.GetFullPath(wxPATH_UNIX);
        return true;
    }


    private:

    MapDocumentT* MapDoc;
    wxString      Filter;
    wxString      SubDir;
};


BEGIN_EVENT_TABLE(InspDlgEntityPropsT, wxPanel)
    EVT_PG_CHANGED    (ID_PROPERTY_GRID_MAN,   InspDlgEntityPropsT::OnPropertyGridChanged)
    EVT_PG_RIGHT_CLICK(ID_PROPERTY_GRID_MAN,   InspDlgEntityPropsT::OnPropertyGridItemRightClick)
    EVT_MENU          (ID_MENU_CONTEXT_ADD,    InspDlgEntityPropsT::OnContextMenuItemAdd)
    EVT_MENU          (ID_MENU_CONTEXT_DELETE, InspDlgEntityPropsT::OnContextMenuItemDelete)
END_EVENT_TABLE()


wxSizer* InspDlgEntityPropsT::InspectorEntityPropsInit(wxWindow* parent, bool call_fit, bool set_sizer)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    SelectionText=new wxStaticText(parent, -1, wxT("No entity is selected."), wxDefaultPosition, wxDefaultSize, 0);
    item0->Add(SelectionText, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    // BEGIN CUSTOM code, not generated by wxDesigner.

    PropMan=new wxPropertyGridManager(this, ID_PROPERTY_GRID_MAN, wxDefaultPosition, wxDefaultSize, wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLBAR | wxPG_DESCRIPTION);
    PropMan->SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS | wxPG_EX_MODE_BUTTONS);

    PropMan->AddPage("Properties");

    // END CUSTOM code, not generated by wxDesigner.

    item0->Add( PropMan, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2 );

    if (set_sizer)
    {
        parent->SetSizer( item0 );
        if (call_fit)
            item0->SetSizeHints( parent );
    }

    return item0;
}


InspDlgEntityPropsT::InspDlgEntityPropsT(wxWindow* Parent_, MapDocumentT* MapDoc_)
    : wxPanel(Parent_, -1),
      MapDoc(MapDoc_),
      PropMan(NULL),
      ClassKeys(NULL),
      MixedKeys(NULL),
      UnDefKeys(NULL),
      PopUpMenu(NULL),
      SelectionText(NULL),
      LastRightClickedProperty(NULL),
      IsRecursiveSelfNotify(false)
{
    InspectorEntityPropsInit(this);

    // Create popup menu.
    PopUpMenu=new wxMenu("Property");
    PopUpMenu->Append(ID_MENU_CONTEXT_ADD, "Add");
    PopUpMenu->Append(ID_MENU_CONTEXT_DELETE, "Delete");

    MapDoc->RegisterObserver(this);
}


InspDlgEntityPropsT::~InspDlgEntityPropsT()
{
    PropMan->Clear();
    if (MapDoc) MapDoc->UnregisterObserver(this);
}


void InspDlgEntityPropsT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    // Part 1: Determine the list of selected entities.
    SelectedEntities.Clear();

    // Loop over the map selection in order to determine the list of selected entities.
    for (unsigned long i=0; i<NewSelection.Size(); i++)
    {
        MapEntityBaseT* Ent=dynamic_cast<MapEntityBaseT*>(NewSelection[i]);

        if (Ent)
        {
            if (SelectedEntities.Find(Ent)==-1) SelectedEntities.PushBack(Ent);
            continue;
        }

        MapPrimitiveT* Prim=dynamic_cast<MapPrimitiveT*>(NewSelection[i]);

        if (Prim)
        {
            MapEntityBaseT* Ent=Prim->GetParent();
            if (SelectedEntities.Find(Ent)==-1) SelectedEntities.PushBack(Ent);
            continue;
        }
    }

    UpdatePropertyGrid();
}


void InspDlgEntityPropsT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements)
{
    // No need to react on deleted objects.
    // If the deletion of an object changes the current selection the inspector is notified in the NotifySubjectChanged_Selection method.
}


void InspDlgEntityPropsT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const wxString& Key)
{
    if (IsRecursiveSelfNotify) return;

    // If one of the changed entities is part of the current selection, update the grid.
    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        if (MapElements[i]->IsSelected())
        {
            UpdatePropertyGrid();
            break;
        }
    }
}


void InspDlgEntityPropsT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (Detail!=MEMD_ENTITY_CLASS_CHANGED && Detail!=MEMD_GENERIC) return;

    if (IsRecursiveSelfNotify) return;

    // If one of the changed entities is part of the current selection, update the grid.
    for (unsigned long i=0; i<MapElements.Size(); i++)
    {
        if (MapElements[i]->IsSelected())
        {
            UpdatePropertyGrid();
            break;
        }
    }
}


void InspDlgEntityPropsT::NotifySubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==MapDoc);
    MapDoc=NULL;
}


void InspDlgEntityPropsT::UpdatePropertyGrid()
{
    LastRightClickedProperty=NULL;

    // Part 2: Determine the property infos ("PropInfos") of all selected entities combined.
    // This is done by first determining the PropInfos for each individual entity, then updating the combined state in turn.
    CombinedPropInfos.clear();      // This is necessary in case SelectedEntities.Size()==0.

    for (unsigned long EntNr=0; EntNr<SelectedEntities.Size(); EntNr++)
    {
        const MapEntityBaseT*         Entity=SelectedEntities[EntNr];   // The currently considered entity.
        const EntityClassT*           EntClass=Entity->GetClass();      // The class of the current entity as defined in the game config. Can be NULL if undefined in this game config.
        std::map<wxString, PropInfoT> EntPropInfos;                     // The property infos for the current entity.

        // Insert the special-case property "classname".
        EntPropInfos["classname"]=PropInfoT(NORMAL, true, false, NULL);

        // Loop over the variables as defined by the class of this entity.
        if (EntClass!=NULL)
        {
            for (unsigned long VarNr=0; VarNr<EntClass->GetVariables().Size(); VarNr++)
            {
                const EntClassVarT* Var=EntClass->GetVariables()[VarNr];

                EntPropInfos[Var->GetName()]=PropInfoT(NORMAL, true, true, Var);
            }
        }

        // Loop over the concrete properties in the entity.
        for (unsigned long PropNr=0; PropNr<Entity->GetProperties().Size(); PropNr++)
        {
            const wxString&     Key  =Entity->GetProperties()[PropNr].Key;
            const wxString&     Value=Entity->GetProperties()[PropNr].Value;
            const EntClassVarT* Var  =(EntClass!=NULL) ? EntClass->FindVar(Key) : NULL;

            if (Var!=NULL)
            {
                wxASSERT(EntPropInfos[Key].ClassVar==Var);

                // The concrete property has a corresponding variable definition in the entity class.
                // Now just see if the concrete value is equal to the variables default value.
                EntPropInfos[Key].ValueIsDefault=(Value==Var->GetDefault());
            }
            else
            {
                // The concrete property has *no* corresponding variable definition in the entity class,
                // because the variable (or even the entire entity class!) is undefined in this game config.
                EntPropInfos[Key]=PropInfoT(UNDEFINED, true, false, NULL);
            }
        }


        // Done with determining the current entities EntPropInfos.
        // Now combine them with the overall CombinedPropInfos.
        if (EntNr==0)
        {
            // The first (and possibly only) entity initializes the combined property infos.
            CombinedPropInfos=EntPropInfos;
        }
        else
        {
            // Properly combine ("merge", "overlay") the local entities EntPropInfos with the global CombinedPropInfos.

            // We want to have the user edit only those keys for which *each* selected entity provides a corresponding variable definition.
            // Moreover, all those overlaid variable definitions for a key must be of the *same* type, e.g. all "color", or all "model file name", etc.
            // As a consequence, all kinds of mismatches must be identified and flagged.

            // First identify keys that occur in the CombinedPropInfos, but not in the EntPropInfos of the current entity.
            // Such cases cause the key in CombinedPropInfos to be turned into a mismatch.
            for (std::map<wxString, PropInfoT>::iterator It=CombinedPropInfos.begin(); It!=CombinedPropInfos.end(); ++It)
            {
                if (EntPropInfos.find(It->first)==EntPropInfos.end())
                {
                    It->second.KeyState=MIXED;
                    It->second.ClassVar=NULL;   // No need to keep referring to a variable definition with a mismatch.
                }
            }

            // Now handle all the keys in the EntPropInfos list.
            for (std::map<wxString, PropInfoT>::const_iterator It=EntPropInfos.begin(); It!=EntPropInfos.end(); ++It)
            {
                const wxString&  Key        =It->first;
                const PropInfoT& EntPropInfo=It->second;

                // Handle special-case "classname".
                if (Key=="classname")
                {
                    // Check if classes are the same and otherwise note the inconsistent value.
                    if (SelectedEntities[0]->GetClass()!=Entity->GetClass())
                        CombinedPropInfos[Key].ValueIsConsistent=false;

                    continue;
                }

                // If key not yet in combined property grid, insert it appropriately.
                if (CombinedPropInfos.find(Key)==CombinedPropInfos.end())
                {
                    // All these searches in the CombinedPropInfos map for "Key" are not particularly efficient,
                    // but alas, at least is the code way more readable that way.
                    PropInfoT& InsertedPropInfo=CombinedPropInfos[Key];

                    InsertedPropInfo=EntPropInfo;       // (Re-)Initialize the newly inserted PropInfo.
                    InsertedPropInfo.KeyState=MIXED;    // Keys that are not part of ALL properties are not editable.
                    InsertedPropInfo.ClassVar=NULL;     // No need to keep referring to a variable definition with a mismatch.
                    continue;
                }

                PropInfoT& CombPropInfo=CombinedPropInfos[Key];

                switch (CombPropInfo.KeyState)
                {
                    case UNDEFINED:
                        if (EntPropInfo.KeyState==NORMAL)
                        {
                            CombPropInfo.KeyState=MIXED;
                            CombPropInfo.ClassVar=NULL;     // No need to keep referring to a variable definition with a mismatch.
                        }
                        else
                        {
                            // Keys that have no corresponing variable definition at all in the classes
                            // of all selected entities are implied to be of type "string".
                            // Therefore, such keys of UNDEFINED type can still be edited by the user.
                            wxASSERT(EntPropInfo.KeyState==UNDEFINED);

                            // Undefined keys should always be set.
                            wxASSERT(Entity->FindProperty(Key)!=NULL && SelectedEntities[0]->FindProperty(Key)!=NULL);

                            // Keep a record if all the values are equal.
                            if (Entity->FindProperty(Key)->Value!=SelectedEntities[0]->FindProperty(Key)->Value)
                                CombPropInfo.ValueIsConsistent=false;
                        }
                        break;

                    case NORMAL:
                        if (EntPropInfo.KeyState==NORMAL)
                        {
                            wxASSERT(CombPropInfo.ClassVar!=NULL);
                            wxASSERT(EntPropInfo .ClassVar!=NULL);

                            if (CombPropInfo.ClassVar->GetType()==EntPropInfo.ClassVar->GetType())
                            {
                                // Ok, these two variables are compatible. Note though that
                                // (CombPropInfo.ClassVar==EntPropInfo.ClassVar) is not necessarily true.

                                // Compare property values.
                                const EntPropertyT* EntP1=Entity->FindProperty(Key);
                                const EntPropertyT* EntP2=SelectedEntities[0]->FindProperty(Key);
                                wxString EntV1=EntP1 ? EntP1->Value : EntPropInfo.ClassVar->GetDefault();
                                wxString EntV2=EntP2 ? EntP2->Value : CombPropInfo.ClassVar->GetDefault();

                                if (EntV1!=EntV2)
                                    CombPropInfo.ValueIsConsistent=false;

                                CombPropInfo.ValueIsDefault=(CombPropInfo.ValueIsDefault && EntPropInfo.ValueIsDefault && CombPropInfo.ValueIsConsistent);
                            }
                            else
                            {
                                CombPropInfo.KeyState=MIXED;
                                CombPropInfo.ClassVar=NULL;     // No need to keep referring to a variable definition with a mismatch.
                            }
                        }
                        else
                        {
                            wxASSERT(EntPropInfo.KeyState==UNDEFINED);

                            CombPropInfo.KeyState=MIXED;
                            CombPropInfo.ClassVar=NULL;     // No need to keep referring to a variable definition with a mismatch.
                        }
                        break;

                    case MIXED:
                        // No matter what the EntPropInfo has to contribute, the property is a mismatch already and thus the case is hopeless.
                        // Just make sure that earlier code that determined the mismatch also cleared the ClassVar.
                        wxASSERT(CombPropInfo.ClassVar==NULL);
                        break;
                }
            }
        }
    }


    // Part 3a: Update the selection description text.
    if (SelectedEntities.Size()==0)
    {
        SelectionText->SetLabel("No entity is selected.");
    }
    else if (SelectedEntities.Size()==1)
    {
        const EntPropertyT* EntName=SelectedEntities[0]->FindProperty("name");

        if (EntName==NULL) SelectionText->SetLabel("One entity of class \""+SelectedEntities[0]->GetClass()->GetName()+"\" is selected.");
                      else SelectionText->SetLabel("Entity \""+EntName->Value+"\" of class \""+SelectedEntities[0]->GetClass()->GetName()+"\" is selected.");
    }
    else
    {
        // Two or more entities are selected.
        unsigned long EntNr;

        for (EntNr=1; EntNr<SelectedEntities.Size(); EntNr++)
            if (SelectedEntities[EntNr]->GetClass()!=SelectedEntities[0]->GetClass())
                break;

        SelectionText->SetLabel(wxString::Format("%lu entities (", SelectedEntities.Size())+
            (EntNr<SelectedEntities.Size() ? "of different classes" : "all of class \""+SelectedEntities[0]->GetClass()->GetName()+"\"")+
            ") are selected.");
    }


    // Part 3b: Use the CombinedPropInfos to fill-in our wxPropertyGrid sheet.
    PropMan->ClearSelection();              // Clear the property grid.
    PropMan->GetCurrentPage()->Clear();     // At the moment we only got one page, so we only need to delete this page.

    // Set categories to NULL. They will be recreated if a property is part of a category.
    ClassKeys=NULL;
    MixedKeys=NULL;
    UnDefKeys=NULL;

    for (std::map<wxString, PropInfoT>::const_iterator It=CombinedPropInfos.begin(); It!=CombinedPropInfos.end(); ++It)
    {
        const wxString&  Key     =It->first;
        const PropInfoT& PropInfo=It->second;

        // Pointer to last inserted element. Used to access inserted properties faster (and change their color etc.).
        wxPGProperty* Insert=NULL;

        // Handle special case "classname".
        if (Key=="classname")
        {
            // Create class keys category if not available.
            if (ClassKeys==NULL)
            {
                ClassKeys=new wxPropertyCategory("Properties", wxPG_LABEL);
                PropMan->Append(ClassKeys);
            }

            // Creating choice array and inserting the enum property is only necessary if the entities classnames are the same.
            if (PropInfo.ValueIsConsistent)
            {
                wxArrayString EntClassesList;   // Array of entity class choices.
                int           Selection=0;      // Choice to select after array is created.

                // Add class name of entity to array, if entity class is unknown/undefined in the game config.
                if (!SelectedEntities[0]->GetClass()->IsInGameConfig())
                {
                    EntClassesList.Add(SelectedEntities[0]->GetClass()->GetName());
                }

                // Get entity (to check if it has primitive children) and exclude other MapEntityBaseT derivatives such as the world.
                MapEntityT* EntityTmp=dynamic_cast<MapEntityT*>(SelectedEntities[0]);
                if (EntityTmp!=NULL)
                {
                    const ArrayT<const EntityClassT*>& Classes=MapDoc->GetGameConfig()->GetEntityClasses();

                    for (unsigned long ClassNr=0; ClassNr<Classes.Size(); ClassNr++)
                    {
                        // Entities that have an origin but no primitive children can only be assigned other entities classes that have an origin as well.
                        // Every other combination (entity has no origin (then it has children) or entity has an origin *and* primitive children) can be
                        // assigned *any* entity class.
                        if (EntityTmp->GetClass()->IsSolidClass() || EntityTmp->GetPrimitives().Size()>0 || !Classes[ClassNr]->IsSolidClass())
                            EntClassesList.Add(Classes[ClassNr]->GetName());
                    }

                    class SortNoCaseT
                    {
                        public:

                        static int Compare(const wxString& s1, const wxString& s2)
                        {
                            return wxStricmp(s1, s2);
                        }
                    };

                    EntClassesList.Sort(SortNoCaseT::Compare);  // Can unfortunately not use wxStricmp function directly.

                    Selection=EntClassesList.Index(EntityTmp->GetClass()->GetName());
                    if (Selection<0 || Selection==wxNOT_FOUND) Selection=0;
                }
                else
                {
                    // The entity class is no MapEntityT class (but e.g. the world): Add class name of entity to array.
                    EntClassesList.Add(SelectedEntities[0]->GetClass()->GetName());
                }

                // "Convert" the EntClassList into EntClassChoices.
                // This is important because we have to assign a value to each string item anyway,
                // as the last parameter of wxEnumProperty() is actually a value, not an index
                // (see example code at http://docs.wxwidgets.org/trunk/overview_propgrid.html for details).
                wxPGChoices EntClassesChoices;
                for (unsigned long ClassNr=0; ClassNr<EntClassesList.GetCount(); ClassNr++)
                    EntClassesChoices.Add(EntClassesList[ClassNr], ClassNr);

                Insert=PropMan->Insert(ClassKeys, -1, new wxEnumProperty(Key, wxPG_LABEL, EntClassesChoices, Selection));

                Insert->SetHelpString(SelectedEntities[0]->GetClass()->GetDescription());
                PropMan->SetPropertyBackgroundColour(Insert, COLOR_CUSTOM);
            }
            else
            {
                // Just create an empty string property, since values can't be edited anyway.
                Insert=PropMan->Insert(ClassKeys, -1, new wxStringProperty(Key, wxPG_LABEL, ""));
                Insert->SetHelpString("The selected entities are instances of different classes.");
                PropMan->SetPropertyBackgroundColour(Insert, COLOR_INCONSISTENT);
                PropMan->DisableProperty(Insert);
            }

            Insert->SetClientData((void*)&PropInfo);
            continue;
        }

        // Select category for this key/value pair, create category if not yet available and insert property.
        switch (PropInfo.KeyState)
        {
            case NORMAL:
            {
                if (ClassKeys==NULL)
                {
                    ClassKeys=new wxPropertyCategory("Properties", wxPG_LABEL);
                    PropMan->Append(ClassKeys);
                }

                wxString EntType="";

                // Use empty string per default (is used if value is not consistent).
                wxString KeyValue="";
                // Only get concrete or default value if values are consistent.
                if (PropInfo.ValueIsConsistent)
                {
                    // Set to default value from entity class in case value isn't set in the entity at all.
                    if (!PropInfo.ValueIsDefault) { wxASSERT(SelectedEntities[0]->FindProperty(Key)!=NULL); } // If value is not default, property has to be defined in the entity.
                    KeyValue=PropInfo.ValueIsDefault ? PropInfo.ClassVar->GetDefault() : SelectedEntities[0]->FindProperty(Key)->Value;
                }

                // First insert the key without a value according to the property type.
                switch (PropInfo.ClassVar->GetType())
                {
                    case EntClassVarT::TYPE_INVALID:
                        Insert=PropMan->Insert(ClassKeys, -1, new wxStringProperty(Key, wxPG_LABEL, KeyValue));
                        EntType="Invalid";
                        break;

                    case EntClassVarT::TYPE_CHOICE:
                    {
                        wxPGChoices Choices;        // Array of choices.
                        int         Selection=0;    // Choice to select after array is created.
                        const ArrayT<wxString>& VarChoices=SelectedEntities[0]->GetClass()->FindVar(Key)->GetAuxInfo();

                        for (unsigned long i=0; i<VarChoices.Size(); i++)
                        {
                            Choices.Add(VarChoices[i]);
                            if (VarChoices[i]==KeyValue) Selection=i;
                        }

                        // Create dummy choice, if no choices are defined for this variable.
                        if (VarChoices.Size()==0)
                            Choices.Add("");

                        Insert=PropMan->Insert(ClassKeys, -1, new wxEnumProperty(Key, wxPG_LABEL, Choices, Selection));
                        EntType="Choice";
                        break;
                    }

                    case EntClassVarT::TYPE_COLOR:
                        Insert=PropMan->Insert(ClassKeys, -1, new wxColourProperty(Key, wxPG_LABEL, wxColour(TranslateColorString(KeyValue, true))));
                        EntType="Color";
                        break;

                    case EntClassVarT::TYPE_FILE:
                        Insert=PropMan->Insert(ClassKeys, -1, new GameFilePropertyT(Key, wxPG_LABEL, KeyValue, MapDoc, "All Files (*.*)|*.*", ""));
                        EntType="File";
                        break;

                    case EntClassVarT::TYPE_FILE_MODEL:
                        Insert=PropMan->Insert(ClassKeys, -1, new GameFilePropertyT(Key, wxPG_LABEL, KeyValue, MapDoc, "All Files (*.*)|*.*|Model files (*.mdl)|*.mdl|Model Files (*.ase)|*.ase|Model Files (*.dlod)|*.dlod", "/Models/"));
                        EntType="Model";
                        break;

                    case EntClassVarT::TYPE_FLAGS:
                    {
                        wxPGChoices Flags; // Array of choices.
                        const ArrayT<wxString>& VarFlags=SelectedEntities[0]->GetClass()->FindVar(Key)->GetAuxInfo();

                        for (unsigned long i=0; i<VarFlags.Size(); i++)
                            Flags.Add(VarFlags[i], 1 << i);

                        // Create dummy flag, if no flags are defined for this variable.
                        if (VarFlags.Size()==0)
                            Flags.Add("", 1);

                        long int FlagBits=0;

                        if (!KeyValue.ToLong(&FlagBits))
                            FlagBits=0;

                        if (VarFlags.Size()>1)
                            Insert=PropMan->Insert(ClassKeys, -1, new wxFlagsProperty(Key, wxPG_LABEL, Flags, FlagBits));
                        else
                            Insert=PropMan->Insert(ClassKeys, -1, new wxBoolProperty(Key, wxPG_LABEL, FlagBits!=0));

                        PropMan->SetPropertyAttribute(Insert, wxPG_BOOL_USE_CHECKBOX, (long)1, wxPG_RECURSE); // Use checkboxes instead of choice.

                        EntType="Flags";
                        break;
                    }

                    case EntClassVarT::TYPE_FLOAT:
                    {
                        double value_d=0.0f;
                        if (!KeyValue.ToDouble(&value_d))
                            value_d=0.0f;
                        Insert=PropMan->Insert(ClassKeys, -1, new wxFloatProperty(Key, wxPG_LABEL, value_d));
                        EntType="Float";
                        break;
                    }

                    case EntClassVarT::TYPE_INTEGER:
                    {
                        long value_l=0;
                        if (!KeyValue.ToLong(&value_l))
                            value_l=0;
                        Insert=PropMan->Insert(ClassKeys, -1, new wxIntProperty(Key, wxPG_LABEL, value_l));
                        EntType="Integer";
                        break;
                    }

                    case EntClassVarT::TYPE_STRING:
                        Insert=PropMan->Insert(ClassKeys, -1, new wxStringProperty(Key, wxPG_LABEL, KeyValue));
                        EntType="String";
                        break;

                    default:
                        Insert=PropMan->Insert(ClassKeys, -1, new wxStringProperty(Key, wxPG_LABEL, KeyValue));
                        EntType="Default";
                        break;
                }

                // Insert helptext.
                Insert->SetHelpString(PropInfo.ClassVar->GetDescription()+"\n\nVariable Type: "+EntType);

                if (!PropInfo.ValueIsConsistent)
                {
                    Insert->SetValueToUnspecified();
                    PropMan->SetPropertyBackgroundColour(Insert, COLOR_INCONSISTENT);
                }
                else
                {
                    if (PropInfo.ValueIsDefault)
                    {
                        PropMan->SetPropertyBackgroundColour(Insert, COLOR_DEFAULT);
                    }
                    else
                    {
                        PropMan->SetPropertyBackgroundColour(Insert, COLOR_CUSTOM);
                    }
                }

                // Check if key is unique and make note in helptext.
                if (SelectedEntities[0]->GetClass()->FindVar(Key)->IsUnique())
                {
                    Insert->SetHelpString(Insert->GetHelpString()+"\nUnique value");

                    // Disable key if more than one entity is selected, since unique keys can only be edited for one entity.
                    if (SelectedEntities.Size()>1) PropMan->DisableProperty(Insert);
                }

                break;
            }

            case MIXED:
                if (MixedKeys==NULL)
                {
                    MixedKeys=new wxPropertyCategory("Mixed keys", wxPG_LABEL);     // "Keys with varying meanings"
                    PropMan->Append(MixedKeys);
                }

                Insert=PropMan->Insert(MixedKeys, -1, new wxStringProperty(Key, wxPG_LABEL, ""));
                PropMan->DisableProperty(Insert);
                break;

            case UNDEFINED:
                if (UnDefKeys==NULL)
                {
                    UnDefKeys=new wxPropertyCategory("Undefined keys", wxPG_LABEL);
                    PropMan->Append(UnDefKeys);
                }

                if (!PropInfo.ValueIsConsistent)
                {
                    Insert=PropMan->Insert(UnDefKeys, -1, new wxStringProperty(Key, wxPG_LABEL, ""));
                    PropMan->SetPropertyBackgroundColour(Insert, COLOR_INCONSISTENT);
                }
                else
                {
                    wxASSERT(SelectedEntities[0]->FindProperty(Key)!=NULL);
                    Insert=PropMan->Insert(UnDefKeys, -1, new wxStringProperty(Key, wxPG_LABEL, SelectedEntities[0]->FindProperty(Key)->Value));
                    PropMan->SetPropertyBackgroundColour(Insert, COLOR_CUSTOM);
                }

                break;
        }

        Insert->SetClientData((void*)&PropInfo);
    }

    PropMan->RefreshGrid();
}


void InspDlgEntityPropsT::OnPropertyGridChanged(wxPropertyGridEvent& event)
{
    // Load property info from property to get all entities that belong to this property.
    PropInfoT* PropInfo=(PropInfoT*)event.GetProperty()->GetClientData();

    wxASSERT(PropInfo!=NULL);               // Because the property grid items were created from a list of PropInfoTs.
    wxASSERT(PropInfo->KeyState!=MIXED);    // Because keys with such state have been disabled from editing.

    // Only write value change to entities if the property got its property info and the key isn't a MIXED key
    // (as they are not editable and thus have been disabled for editing above).
    if (PropInfo==NULL || PropInfo->KeyState==MIXED) return;


    wxString NewValue=event.GetProperty()->GetValueAsString();

    // We need to translate the value here according to property type.
    if (wxString(event.GetProperty()->GetClassInfo()->GetClassName())=="wxFlagsProperty" ||
        wxString(event.GetProperty()->GetClassInfo()->GetClassName())=="wxBoolProperty")
    {
        NewValue=wxString::Format("%li", event.GetProperty()->GetValue().GetLong());
    }
    else if (event.GetProperty()->GetValueType()=="wxColour")
    {
        wxColour NewColor; NewColor << event.GetProperty()->GetValue();

        NewValue=wxString::Format("%i %i %i", NewColor.Red(), NewColor.Green(), NewColor.Blue());
    }


    // Have the selected entities available as a set of MapElementTs as well.
    ArrayT<MapElementT*> MapElements;

    for (unsigned long i=0; i<SelectedEntities.Size(); i++)
        MapElements.PushBack(SelectedEntities[i]);

    CommandT* MacroCommand=NULL;

    // Changing the class of an entity.
    if (event.GetProperty()->GetName()=="classname")
    {
        // Only allow a class change if all entities share the same class.
        for (unsigned long i=1; i<SelectedEntities.Size(); i++)
        {
            if (SelectedEntities[i]->GetClass()!=SelectedEntities[0]->GetClass())
            {
                wxMessageBox("Entities have different entity classes.", "Error: Couldn't change entity class.", wxOK | wxICON_ERROR);

                // Intentionally update also this dialog to restore previous property value. TODO: Can the event be vetoed instead?
                NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "");
                return;
            }
        }

        const EntityClassT* NewEntityClass=MapDoc->GetGameConfig()->FindClass(NewValue);

        if (NewEntityClass==NULL)
        {
            wxMessageBox("Could not find entity class \""+NewValue+"\" in the game config.", "Error: Couldn't change entity class.", wxOK | wxICON_ERROR);

            // Intentionally update also this dialog to restore previous property value. TODO: Can the event be vetoed instead?
            NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "");
            return;
        }

        ArrayT<CommandT*> Commands;

        for (unsigned long i=0; i<SelectedEntities.Size(); i++)
        {
            MapEntityT* Ent=dynamic_cast<MapEntityT*>(SelectedEntities[i]);

            if (Ent)
                Commands.PushBack(new CommandChangeClassT(*MapDoc, Ent, NewEntityClass));
        }

        MacroCommand=new CommandMacroT(Commands, "Change entity class");
    }
    else
    {
        wxASSERT(SelectedEntities[0]!=NULL);

        const wxString Key=event.GetProperty()->GetName();

        // Check if key is found in all entities, in the same category (defined or undefined) and has the same type for all objects.
        // First entity as reference.
        bool Undefined=(SelectedEntities[0]->GetClass() && SelectedEntities[0]->GetClass()->FindVar(Key)==NULL);

        for (unsigned long i=1; i<SelectedEntities.Size(); i++)
        {
            if (Undefined) // Just check if key is also undefined (type is always string).
            {
                if (  (SelectedEntities[i]->GetClass() && SelectedEntities[i]->GetClass()->FindVar(Key)!=NULL)
                    || SelectedEntities[i]->FindProperty(Key)==NULL)
                {
                    wxMessageBox("Value is not defined by all entities.", "Error: Couldn't change property value.", wxOK | wxICON_ERROR);
                    NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, ""); // Intentionally update also this dialog to restore previous property value.
                    return;
                }
            }
            else // Check if key is defined and has the same type.
            {
                if (   SelectedEntities[i]->GetClass()==NULL
                    || SelectedEntities[i]->GetClass()->FindVar(Key)==NULL
                    || SelectedEntities[i]->GetClass()->FindVar(Key)->GetType()!=SelectedEntities[0]->GetClass()->FindVar(Key)->GetType())
                {
                    wxMessageBox("Value is not defined by all entities or hasn't the same type.", "Error: Couldn't change property value.", wxOK | wxICON_ERROR);
                    NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, ""); // Intentionally update also this dialog to restore previous property value.
                    return;
                }
            }

        }

        // Check Lua compatibility for entity names.
        if (Key=="name" && !IsLuaIdentifier(NewValue))
        {
            wxMessageBox("An entity name must be a string of letters, digits, and underscores that is\n"
                "not beginning with a digit and is not a reserved Lua keyword or global variable.",
                "Entity name is not a valid script identifier.", wxOK | wxICON_ERROR);

            NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, ""); // Intentionally update also this dialog to restore previous property value.
            return;
        }

        // Handle unique keys here.
        if (SelectedEntities[0]->GetClass()->FindVar(Key) && SelectedEntities[0]->GetClass()->FindVar(Key)->IsUnique())
        {
            // Only change unique when only one entity is selected.
            wxASSERT(SelectedEntities.Size()==1);
            // Since unique properties are disabled, when more than one entity is selected, we should never get here.
            // This is just a security precaution for release code. We forgo writing back the original value here, since this
            // case will probably never happen.
            if (SelectedEntities.Size()>1)
            {
                wxMessageBox("Unique keys can only be changed if one entity is selected.", "Error: Couldn't change unique key.", wxOK | wxICON_ERROR);
                NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, ""); // Intentionally update also this dialog to restore previous property value.
                return;
            }

            // Load all map entities.
            // We cannot call PropObjects[0]->CheckUniqueValues() here, since this method checks all values of the entity
            // for uniqueness so it would fail if another unique marked value is not unique.
            for (unsigned long EntNr=1/*skip world*/; EntNr<MapDoc->GetEntities().Size(); EntNr++)
            {
                MapEntityBaseT* Entity=MapDoc->GetEntities()[EntNr];

                      EntPropertyT*     Prop=Entity->FindProperty(Key);
                const EntClassVarT* ClassVar=Entity->GetClass() ? Entity->GetClass()->FindVar(Key) : NULL;

                // Check if new value is already used by an entity.
                if ((Prop!=NULL && Prop->Value==NewValue) || (ClassVar!=NULL && ClassVar->GetDefault()==NewValue))
                {
                    // Set value in property grid to old value and return.
                    wxMessageBox("This unique value is already used by another entity.", "Error: Couldn't change unique key.", wxOK | wxICON_ERROR);
                    NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, ""); // Intentionally update also this dialog to restore previous property value.
                    return;
                }
            }
        }

        // Set the property to its new value.
        ArrayT<CommandT*> Commands;

        for (unsigned long i=0; i<SelectedEntities.Size(); i++)
            Commands.PushBack(new CommandSetPropertyT(*MapDoc, SelectedEntities[i], Key, NewValue));

        MacroCommand=new CommandMacroT(Commands, "Change property '"+Key+"'");
    }

    // Execution of command triggers recursive update of this dialog. To prevent this we set IsRecursiveSelfNotify.
    IsRecursiveSelfNotify=true;

    // Run command (in SubmitCommand()) and react if its execution fails by updating the whole dialog recursively.
    // Also update whole dialog in case classname has changed.
    if (!MapDoc->GetHistory().SubmitCommand(MacroCommand) || event.GetProperty()->GetName()=="classname")
    {
        IsRecursiveSelfNotify=false;
        NotifySubjectChanged_Modified(MapDoc, MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "");
        return;
    }

    IsRecursiveSelfNotify=false;

    PropInfo->ValueIsConsistent=true;
    PropMan ->SetPropertyBackgroundColour(event.GetProperty(), COLOR_CUSTOM);

    // Special case: Model file of the entity has changed.
    if (PropInfo->ClassVar && PropInfo->ClassVar->GetType()==EntClassVarT::TYPE_FILE_MODEL && event.GetProperty()->GetName()=="model")
    {
        wxPGProperty* Insert=PropMan->GetPropertyByName("collisionModel");

        // If a collisionModel key is found color it to notify the user that it should be changed too.
        if (Insert!=NULL) PropMan->SetPropertyBackgroundColour(Insert, COLOR_WARNING);
    }

    // Check if newly set value is a DEFAULT value and update background color.
    if ((PropInfo->KeyState & NORMAL) && NewValue==PropInfo->ClassVar->GetDefault())
    {
        PropMan->SetPropertyBackgroundColour(event.GetProperty(), COLOR_DEFAULT);
        PropInfo->ValueIsDefault=true;
    }
}


void InspDlgEntityPropsT::OnPropertyGridItemRightClick(wxPropertyGridEvent& event)
{
    LastRightClickedProperty=event.GetProperty();

    PopupMenu(PopUpMenu);
}


void InspDlgEntityPropsT::OnContextMenuItemAdd(wxCommandEvent& event)
{
    const wxString NewKey=wxGetTextFromUser("Please enter the name of the new key.", "New Key", "");
    if (NewKey=="") return;

    // Check if a property with this key already exists.
    if (PropMan->GetPropertyByName(NewKey)!=NULL)
    {
        wxMessageBox("A key with the same name already exists.", "Couldn't create property", wxOK | wxICON_ERROR);
        return;
    }

    ArrayT<CommandT*> Commands;

    for (unsigned long i=0; i<SelectedEntities.Size(); i++)
        Commands.PushBack(new CommandSetPropertyT(*MapDoc, SelectedEntities[i], NewKey, ""));

    CommandT* MacroCommand=new CommandMacroT(Commands, "Add Property '"+NewKey+"'");

    // Note that we also receive the update notification recursively (triggered by command execution in SubmitCommand()).
    // This is intentional, or else we had to manually fix the PropMan, CombinedPropInfos, etc. here, which is error-prone.
    // The complete recursive update is much less work and 100% safe, because it guarantees that no new bugs are introduced here.
    MapDoc->GetHistory().SubmitCommand(MacroCommand);
}


void InspDlgEntityPropsT::OnContextMenuItemDelete(wxCommandEvent& event)
{
    // Just in case the user pressed e.g. CTRL+Q (clear selection) in the meanwhile...
    if (LastRightClickedProperty==NULL) return;

    // Only handle deletes on properties that are not a category.
    if (LastRightClickedProperty->IsCategory()) return;

    // Key is not undefined (cannot be deleted).
    if (LastRightClickedProperty->GetParent()!=UnDefKeys)
    {
        wxMessageBox("Only undefined keys can be deleted.");
        return;
    }

    ArrayT<CommandT*> Commands;

    for (unsigned long i=0; i<SelectedEntities.Size(); i++)
        Commands.PushBack(new CommandDeletePropertyT(*MapDoc, SelectedEntities[i], LastRightClickedProperty->GetName()));

    CommandT* MacroCommand=new CommandMacroT(Commands, "Delete Property '"+LastRightClickedProperty->GetName()+"'");

    // Note that we also receive the update notification recursively (triggered by command execution in SubmitCommand()).
    // This is intentional, or else we had to manually fix the PropMan, CombinedPropInfos, etc. here, which is error-prone.
    // The complete recursive update is much less work and 100% safe, because it guarantees that no new bugs are introduced here.
    MapDoc->GetHistory().SubmitCommand(MacroCommand);
}
