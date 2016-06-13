/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************************************/
/*** Material Manager Implementation ***/
/***************************************/

#include <math.h>
#include <stdio.h>

#ifdef _WIN32
    #include <direct.h>
    #if defined(_MSC_VER)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
    #endif
#else
    #include <cstring>
    #include <dirent.h>
#endif

#include "MaterialManagerImpl.hpp"
#include "Expression.hpp"
#include "Material.hpp"
#include "ConsoleCommands/Console.hpp"
#include "String.hpp"
#include "TextParser/TextParser.hpp"


static TableT* ParseTable(TextParserT& TP)
{
    // The "table" keyword has already been parsed by the caller.
    TableT Table;

    Table.Name=TP.GetNextToken();

    Table.ColumnSnap =false;    // Per default, interpolate across columns.
    Table.ColumnClamp=false;    // Per default, wrap        across columns.

    // Start table parsing.
    while (true)
    {
        std::string Token=TP.GetNextToken();

             if (Token=="{"    ) break;                     // Start of table definition.
        else if (Token=="snap" ) Table.ColumnSnap =true;    // Snap to column values (don't interpolate).
        else if (Token=="clamp") Table.ColumnClamp=true;    // Clamp columns (don't wrap).
        else                     return NULL;               // Invalid token.
    }

    // Parse the table definition.
    bool NextRowSnap =false;
    bool NextRowClamp=false;

    while (true)
    {
        std::string Token=TP.GetNextToken();

             if (Token=="}"    ) break;                 // End of table definition.
        else if (Token=="snap" ) NextRowSnap =true;     // Snap new row to row values (don't interpolate).
        else if (Token=="clamp") NextRowClamp=true;     // Clamp next row (don't wrap).
        else if (Token=="{")                            // Start of row definition.
        {
            // Parse row.
            Table.Rows.PushBackEmpty();
            Table.Rows[Table.Rows.Size()-1].RowSnap =NextRowSnap;
            Table.Rows[Table.Rows.Size()-1].RowClamp=NextRowClamp;

            Token=TP.GetNextToken();

            if (Token!="}")     // If Token=="}", the row is empty.
            {
                while (true)
                {
                    // The token should be a number, followed either by a comma or the closing bracket.
                    Table.Rows[Table.Rows.Size()-1].Data.PushBack(float(atof(Token.c_str())));

                    Token=TP.GetNextToken();

                    if (Token=="}") break;      // End of row definition.
                    if (Token==",") { Token=TP.GetNextToken(); continue; }  // Continue with next number.
                    return NULL;                // Not a '}' and not a ',' after a number - invalid token.
                }
            }

            // Reset for next row (if any).
            NextRowSnap =false;
            NextRowClamp=false;
        }
        else return NULL;   // Invalid token.
    }

    return new TableT(Table);
}


MaterialManagerImplT::MaterialManagerImplT() : MaterialManagerI()
{
    const double Pi=3.14159265358979323846;

    // Add sinTable as a global default table.
    Tables.PushBack(new TableT);
    Tables[0]->Name="sinTable";
    Tables[0]->Rows.PushBackEmpty();
    Tables[0]->Rows[0].RowSnap =false;
    Tables[0]->Rows[0].RowClamp=false;
    for (unsigned long i=0; i<256; i++) Tables[0]->Rows[0].Data.PushBack(float(sin(double(i)/256.0*2.0*Pi)));

    // Add cosTable as a global default table.
    Tables.PushBack(new TableT);
    Tables[1]->Name="cosTable";
    Tables[1]->Rows.PushBackEmpty();
    Tables[1]->Rows[0].RowSnap =false;
    Tables[1]->Rows[0].RowClamp=false;
    for (unsigned long i=0; i<256; i++) Tables[1]->Rows[0].Data.PushBack(float(cos(double(i)/256.0*2.0*Pi)));

    // Add sinTable01 as a global default table. sinTable01[i] == sinTable[i]/2.0+0.5
    Tables.PushBack(new TableT);
    Tables[2]->Name="sinTable01";
    Tables[2]->Rows.PushBackEmpty();
    Tables[2]->Rows[0].RowSnap =false;
    Tables[2]->Rows[0].RowClamp=false;
    for (unsigned long i=0; i<256; i++) Tables[2]->Rows[0].Data.PushBack(Tables[0]->Rows[0].Data[i]/2.0f+0.5f);

    // Add cosTable01 as a global default table. cosTable[i] == cosTable[i]/2.0+0.5
    Tables.PushBack(new TableT);
    Tables[3]->Name="cosTable01";
    Tables[3]->Rows.PushBackEmpty();
    Tables[3]->Rows[0].RowSnap =false;
    Tables[3]->Rows[0].RowClamp=false;
    for (unsigned long i=0; i<256; i++) Tables[3]->Rows[0].Data.PushBack(Tables[1]->Rows[0].Data[i]/2.0f+0.5f);

    // WARNING: Adding more tables here also requires changes in MaterialManagerImplT::ClearAllMaterials().
}


MaterialManagerImplT::~MaterialManagerImplT()
{
    for (unsigned long TableNr=0; TableNr<Tables.Size(); TableNr++)
        delete Tables[TableNr];
    Tables.Clear();

    for (std::map<std::string, MaterialT*>::const_iterator Mat=Materials.begin(); Mat!=Materials.end(); Mat++)
        delete Mat->second;
    Materials.clear();
}


MaterialT* MaterialManagerImplT::RegisterMaterial(const MaterialT& Mat)
{
    MaterialT*& MatPtr=Materials[Mat.Name];

    // Creating a new copy of Mat here makes it clear to the user who has ownership of Mat (the user)
    // and who has ownership of the registered material (we, the material manager).
    // It also ensures that there is never a danger to register the same material instance accidently twice,
    // as would be possible if we registered a user-created material instance whose name got possibly mangled
    // to resolve name collisions.
    if (!MatPtr) MatPtr=new MaterialT(Mat);

    return MatPtr;
}


ArrayT<MaterialT*> MaterialManagerImplT::RegisterMaterialScript(const std::string& FileName, const std::string& BaseDir)
{
    ArrayT<MaterialT*> NewMaterials;

    for (unsigned long Nr=0; Nr<MaterialScriptFileNames.Size(); Nr++)
        if (MaterialScriptFileNames[Nr]==FileName) return NewMaterials;

    MaterialScriptFileNames.PushBack(FileName);

    // Materials in a script do only see the locally defined tables.
    ArrayT<TableT*> ThisScriptsTables;
    ThisScriptsTables.PushBack(Tables[0]);  // The sinTable   is global.
    ThisScriptsTables.PushBack(Tables[1]);  // The cosTable   is global.
    ThisScriptsTables.PushBack(Tables[2]);  // The sinTable01 is global.
    ThisScriptsTables.PushBack(Tables[3]);  // The cosTable01 is global.

    // Get the materials from the script.
    TextParserT TextParser(FileName.c_str(), "({[]}),");

    try
    {
        while (!TextParser.IsAtEOF())
        {
            const std::string Token=TextParser.GetNextToken();

            if (Token=="table")
            {
                TableT* T=ParseTable(TextParser);

                // If the table could not be parsed (e.g. due to a syntax error or unknown token),
                // abort the parsing of the entire file - the file might be something else than a material script.
                // Even if it was, we cannot easily continue anyway.
                if (!T)
                {
                    Console->Print("Error parsing "+FileName+" near "+Token+cf::va(" at input byte %lu.\n", TextParser.GetReadPosByte()));
                    break;
                }

                // We do not check for duplicate tables.
                Tables.PushBack(T);
                ThisScriptsTables.PushBack(T);
            }
            else if (Token=="dofile")
            {
                TextParser.AssertAndSkipToken("(");
                std::string Include=TextParser.GetNextToken();
                TextParser.AssertAndSkipToken(")");

                NewMaterials.PushBack(RegisterMaterialScript(BaseDir+Include, BaseDir+cf::String::GetPath(Include)+"/"));
            }
            else
            {
                // If the material cannot be parsed (e.g. due to a syntax error or unknown token),
                // the parsing of the entire file is aborted - the file might be something else than a material script.
                // Even if it was, we cannot easily continue anyway.

                MaterialT*  NewMat=new MaterialT(Token, BaseDir, TextParser, ThisScriptsTables);
                MaterialT*& Mat   =Materials[NewMat->Name];

                if (Mat==NULL)
                {
                    Mat=NewMat;
                    NewMaterials.PushBack(NewMat);
                }
                else
                {
                    Console->Print("File " + FileName + ", material \"" + NewMat->Name + "\": duplicate definition (ignored).\n");
                    delete NewMat;
                }
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        Console->Print("Error parsing "+FileName+cf::va(" at input byte %lu.\n", TextParser.GetReadPosByte()));
    }

    return NewMaterials;
}


ArrayT<MaterialT*> MaterialManagerImplT::RegisterMaterialScriptsInDir(const std::string& DirName, const std::string& BaseDir, const bool Recurse)
{
    ArrayT<MaterialT*> NewMaterials;

    if (DirName=="") return NewMaterials;

#ifdef _MSC_VER
    WIN32_FIND_DATA FindFileData;

    HANDLE hFind=FindFirstFile((DirName+"\\*").c_str(), &FindFileData);

    // MessageBox(NULL, hFind==INVALID_HANDLE_VALUE ? "INV_VALUE!!" : FindFileData.cFileName, "Starting parsing.", MB_ICONINFORMATION);
    if (hFind==INVALID_HANDLE_VALUE) return NewMaterials;

    ArrayT<std::string> DirEntNames;

    do
    {
        // MessageBox(NULL, FindFileData.cFileName, "Material Script Found", MB_ICONINFORMATION);
        if (!_stricmp(FindFileData.cFileName, "."  )) continue;
        if (!_stricmp(FindFileData.cFileName, ".." )) continue;
        if (!_stricmp(FindFileData.cFileName, "cvs")) continue;

        DirEntNames.PushBack(DirName+"/"+FindFileData.cFileName);
    } while (FindNextFile(hFind, &FindFileData)!=0);

    if (GetLastError()==ERROR_NO_MORE_FILES) FindClose(hFind);
#else
    DIR* Dir=opendir(DirName.c_str());

    if (!Dir) return NewMaterials;

    ArrayT<std::string> DirEntNames;

    for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
    {
        if (!strcasecmp(DirEnt->d_name, "."  )) continue;
        if (!strcasecmp(DirEnt->d_name, ".." )) continue;
        if (!strcasecmp(DirEnt->d_name, "cvs")) continue;

        // For portability, only the 'd_name' member of a 'dirent' may be accessed.
        DirEntNames.PushBack(DirName+"/"+DirEnt->d_name);
    }

    closedir(Dir);
#endif


    for (unsigned long DENr=0; DENr<DirEntNames.Size(); DENr++)
    {
#ifdef _WIN32
        bool IsDirectory=true;

        FILE* TempFile=fopen(DirEntNames[DENr].c_str(), "r");
        if (TempFile!=NULL)
        {
            // This was probably a file (instead of a directory).
            fclose(TempFile);
            IsDirectory=false;
        }
#else
        bool IsDirectory=false;

        // Sigh. And this doesn't work under Win32... (opendir does not return NULL for files!?!)
        DIR* TempDir=opendir(DirEntNames[DENr].c_str());
        if (TempDir!=NULL)
        {
            // This was a directory.
            closedir(TempDir);
            IsDirectory=true;
        }
#endif

        if (IsDirectory)
        {
            if (Recurse)
                NewMaterials.PushBack(RegisterMaterialScriptsInDir(DirEntNames[DENr], BaseDir));
        }
        else
        {
            if (DirEntNames[DENr].length()>5)
                if (std::string(DirEntNames[DENr].c_str()+DirEntNames[DENr].length()-5)==".cmat")
                    NewMaterials.PushBack(RegisterMaterialScript(DirEntNames[DENr], BaseDir));
        }
    }

    return NewMaterials;
}


bool MaterialManagerImplT::HasMaterial(const std::string& MaterialName) const
{
    std::map<std::string, MaterialT*>::const_iterator It=Materials.find(MaterialName);

    return It!=Materials.end();
}


MaterialT* MaterialManagerImplT::GetMaterial(const std::string& MaterialName) const
{
    // Note that we are *not* just writing   return Materials[MaterialName]   here, because that
    // would implicitly create a NULL entry for every MaterialName that does not actually exist.
    std::map<std::string, MaterialT*>::const_iterator It=Materials.find(MaterialName);

    if (It!=Materials.end()) return It->second;

    Console->Print(cf::va("%s (%u): ", __FILE__, __LINE__)+"GetMaterial(\""+MaterialName+"\") returns NULL.\n");
    return NULL;
}


// void MaterialManagerImplT::ClearAllMaterials()
// {
//     MaterialScriptFileNames.Clear();
//
//     // Can well delete Tables (except the first four stock tables!), all expressions that refer to one have made their own copies.
//     for (unsigned long TableNr=4; TableNr<Tables.Size(); TableNr++)
//         delete Tables[TableNr];
//     while (Tables.Size()>4) Tables.DeleteBack();
//
//     for (unsigned long MaterialNr=0; MaterialNr<Materials.Size(); MaterialNr++)
//         delete Materials[MaterialNr];
//     Materials.Clear();
// }
