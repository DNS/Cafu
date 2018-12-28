import re, os

start_txt = """
====== The Cafu Documentation ======

Welcome to the Cafu Documentation! \\
The purpose of this Wiki is to provide documentation about using and editing the [[http://www.cafu.de|Cafu Engine]].
For developers, all aspects of Cafu related editing like mapping, modding, modelling, texturing, etc. are covered.

The Cafu Engine "root" website is at http://www.cafu.de

As with all Wikis, this site lives from its readers contributions. If you want to help, please read [[BecomingAnEditor]] for an explanation on how you can edit and create new pages --- it's easy!



===== Cafu User Manual =====

The Cafu User Manual describes the //Cafu Engine Demo// packages that are released periodically
to demonstrate the features of the Cafu Engine.

  - [[UserManual::QuickStart]]
  - [[UserManual::Installation]]
    * [[UserManual:Installation#minimum_system_requirements|Minimum System Requirements]]
    * [[UserManual:Installation#installing_cafu|Installing the Cafu Demo]]
    * [[UserManual:Installation#upgrading|Upgrading]]
    * [[UserManual:Installation#de-installation|De-Installation]]
  - [[UserManual::Running]]
    * [[UserManual:Running#The_Main_Menu|The Main Menu]]
    * [[UserManual:Running#Keyboard_Layout|Keyboard Layout]]
    * [[UserManual:Running#The_Command_Console|The Command Console]]
  - [[UserManual:FurtherInformation]]
    * [[UserManual:FurtherInformation#Windows_and_Linux_specifics|Windows and Linux specifics]]
    * [[UserManual:FurtherInformation#Known_Issues|Known Issues]]
    * [[UserManual:FurtherInformation#Contact_and_Support|Contact and Support]]
  - [[General::FAQs_Users]]



===== Game and Application Developer Manual =====

This manual describes how the Cafu engine can be modified to implement own ideas and concepts, how new content is created for use within its worlds, and how entirely new games and applications can be realized with the Cafu engine.


==== General ====
  - [[General::Manifest]]
  - [[general::developer_faq]]

==== The Map Editor ====
  - **The CaWE User's Guide**
    - [[Mapping:CaWE:Install]]
    - Getting started
      - [[Mapping:CaWE:Intro]]
      - [[Mapping:CaWE:MainWindow]]
      - [[Mapping:CaWE:Views|2D and 3D Views]] ([[mapping:cawe:views#video|Video]])
      - [[Mapping:CaWE:MaterialBrowser]]
      - [[Mapping:CaWE:YourFirstMap]] ([[http://www.cafu.de/flash/Your_First_Map.htm|Flash Tutorial]])
    - Map Editing Tools {{ cawe_toolbar.png?80}}
      - [[Mapping::CaWE::EditingTools::Selection]]
      - [[Mapping::CaWE::EditingTools::Camera]]
      - [[Mapping::CaWE::EditingTools::NewBrush]]
      - [[Mapping::CaWE::EditingTools::NewEntity]] ([[http://www.cafu.de/flash/Placing_a_Model.htm|Flash Tutorial]])
      - [[Mapping::CaWE::EditingTools::NewBezierPatch]]
      - [[Mapping::CaWE::EditingTools::NewTerrain]]
      - [[Mapping::CaWE::EditingTools::NewLight]]
      - [[Mapping::CaWE::EditingTools::NewDecal]]
      - [[Mapping::CaWE::EditingTools::EditFaceProps]]
      - [[Mapping::CaWE::EditingTools::EditTerrain]]
      - [[Mapping::CaWE::EditingTools::Clip]]
      - [[Mapping::CaWE::EditingTools::Morph]]
    - Selected Topics
      - [[Mapping::CaWE::Groups]]
      - [[Mapping::CaWE::Leaks]] ([[http://www.cafu.de/flash/Dealing_with_Leaks.htm|Flash Tutorial]])
      - [[Mapping::CaWE::Teleporters]]
      - [[Mapping::CaWE::Lighting]]
      - [[Mapping::CaWE::Porting]]
    - Menu Reference
      - [[Mapping::CaWE::MenuReference::File]]
      - [[Mapping::CaWE::MenuReference::Edit]]
      - [[Mapping::CaWE::MenuReference::Map]]
      - [[Mapping::CaWE::MenuReference::View]]
      - [[Mapping::CaWE::MenuReference::Tools]]
      - [[Mapping::CaWE::MenuReference::Compile]]
      - [[Mapping::CaWE::MenuReference::Window]]
      - [[Mapping::CaWE::MenuReference::Help]]
    - Dialog Reference
      - [[Mapping::CaWE::Dialogs::Options]]
      - [[Mapping::CaWE::Dialogs::ReplaceMaterials]]
      - [[Mapping::CaWE::Dialogs::PasteSpecial]]
      - [[Mapping::CaWE::Dialogs::FindEntities]]
      - [[Mapping::CaWE::Dialogs::ObjectProps]]
      - [[Mapping::CaWE::Dialogs::GotoBrushEntity]]
      - [[Mapping::CaWE::Dialogs::EntityReport]]
      - [[Mapping::CaWE::Dialogs::MapError]]
      - [[Mapping::CaWE::Dialogs::Transform]] 
      - [[Mapping::CaWE::Dialogs::FaceProperties]] 
      - [[Mapping::CaWE::Dialogs::EntityInspector]]
    - Reference
      - [[Mapping::CaWE::Reference::EntityGuide]]
      - [[http://api.cafu.de/lua/|Lua Scripting Reference Documentation]]
    - Tutorials Synopsis
      * [[http://www.cafu.de/flash/Your_First_Map.htm|Flash: Your First Map]] ([[Mapping::CaWE::YourFirstMap|Related Article]])
      * [[http://www.cafu.de/flash/Placing_a_Model.htm|Flash: Placing a Model]] ([[Mapping::CaWE::EditingTools::NewEntity|Related Article]])
      * [[http://www.cafu.de/flash/Dealing_with_Leaks.htm|Flash: Dealing with Leaks]] ([[Mapping::CaWE::Leaks|Related Article]])
      * [[Mapping::CaWE::Teleporters]]
      * [[Mapping:CaWE:Tutorials:sky|Adding a sky]]
  - [[Mapping::compiling_new]]
  - [[Mapping::Compiling]]
  - [[Mapping::CreatingTerrainHeightMaps]]

/* This used to be "below" the Model Editor, but I think the time is ripe
   for finally considering the GUI Editor to be good and relevant enough to
   deserve the place immediately following the Map Editor and "above" the
   Model Editor. */
==== The GUI Editor ====
  - [[GuiSys::GuiEditor|The GUI Editor]]
  - [[GuiSys::making_new_fonts]]
  - [[GuiSys::guifiles]]
  - [[http://api.cafu.de/lua/|Lua Scripting Reference Documentation]]

==== The Model Editor ====
  - [[ModelEditor::Introduction]]
  - [[ModelEditor::MainWindow]]
  - [[ModelEditor::HowTos]] ([[modeleditor:howtos#get_my_model_into_cafu|Video]])
  - Model Elements
    - [[ModelEditor::Skeleton]]
    - [[ModelEditor::Meshes]]
    - [[ModelEditor::Skins]]
    - [[ModelEditor::GUI Fixtures]]
    - [[ModelEditor::Animations]]
    - [[ModelEditor::Channels]]
  - Program Dialogs
    - [[ModelEditor::SceneSetup]]
    - [[ModelEditor::Submodels]]
    - [[ModelEditor::Level-of-Detail Models]]
    - [[ModelEditor::Transform]]
  - [[ModelEditor::MenuReference]]
  - [[ModelEditor::ModelFiles]]
  - [[Modelling::DependenciesAmongModels]]

==== Textures ====
  - [[Textures::fileformats|Supported File Formats]]
  - [[Textures::filetypes|Texture types]]
  - [[Textures::SkyDomes]]
  - Tutorials
    * [[Textures::myfirst|Using own textures]]
    * [[Textures::Perfect_Detail_Maps|Making "perfect" detail-maps]]

/* eventually turn this into "The Material Editor", and integrate section "Textures": */
==== The Material System ====
  - [[MatSys::Introduction]]
  - [[MatSys::MatViewer]]
  - [[MatSys::cmat_Manual]]
    - [[MatSys::cmat_Manual#Overview]]
    - [[MatSys::cmat_Manual::TextureMapSpecifications]]
    - [[MatSys::cmat_Manual::ShaderSpecifications]]
    - [[MatSys::cmat_Manual::ExpressionsAndTables]]
    - [[MatSys::cmat_Manual::KeywordReference]]

/*
    - [[MatSys::For_ShaderWriters|The Shader Writers Perspective]]
    - [[MatSys::For_APIUsers|The API Users Perspective]]
    - [[MatSys::For_Developers|The Developers Perspective]]
*/

==== At the Core: The Cafu Source Code ====
  - [[cppDev::GettingStarted]]
  - [[cppDev::IDEs]] ([[cppdev:ides#video|Video]])
  - [[cppDev::SubmitPatches]]
  - [[cppDev::CodingConventions]]
  - [[http://api.cafu.de/c++/|C++ Reference Documentation]]
  - Selected Topics
    - [[cppDev::GameCodeOverview]]
    - [[cppDev::StartNewGame]]
    - [[cppDev::LoadingGameWorlds]]
    - [[cppDev::FbxSDK]]
"""

already_seen = {}

for l in start_txt.splitlines():
    m = re.match(r".*\[\[(.*?)[#|\]]", l)
    #m = re.match(r".*\[(.*)\]", l)
    if m:
        erg = m.group(1).lower().replace("::", "/").replace(":", "/").replace(" ", "_")
        if erg[0:5] != "http/":
            if erg in already_seen:
                continue
            print("    {}{}".format("" if os.path.exists(erg + ".html") else "### ", erg))
            already_seen[erg] = 1

print("")
for root, dirs, files in os.walk("."):
    # print(root, dirs, files)
    for fn in files:
        if fn.endswith(".html"):
            p = os.path.join(root, fn)
            p = p[2:-5].replace("\\", "/")
            if p in already_seen:
                continue
            print("    {}".format(p))
            already_seen[p] = 1

    for excl in [".git", "_build", "_static", "_templates"]:
        if excl in dirs:
            dirs.remove(excl)

print(len(already_seen))
