/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompText.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "GuiResources.hpp"
#include "Window.hpp"

#include "Fonts/FontTT.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
//    #undef FindWindow
#else
//    #include <cstring>
    #include <dirent.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif

using namespace cf::GuiSys;


/************************************/
/*** ComponentTextT::VarFontNameT ***/
/************************************/

ComponentTextT::VarFontNameT::VarFontNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentTextT& CompText)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_CompText(CompText)
{
}


// The compiler-written copy constructor would copy m_CompText from Var.m_CompText,
// but we must obviously use the reference to the proper parent instance instead.
ComponentTextT::VarFontNameT::VarFontNameT(const VarFontNameT& Var, ComponentTextT& CompText)
    : TypeSys::VarT<std::string>(Var),
      m_CompText(CompText)
{
}


void ComponentTextT::VarFontNameT::Set(const std::string& v)
{
    // Make sure that m_CompText actually refers to the ComponentTextT instance that contains us!
    assert(this == &m_CompText.m_FontName);

    TypeSys::VarT<std::string>::Set(v);

    m_CompText.m_FontInst = NULL;

    if (m_CompText.GetWindow() && v != "")
    {
        m_CompText.m_FontInst = m_CompText.GetWindow()->GetGui().GetGuiResources().GetFont(v);
    }
}


void ComponentTextT::VarFontNameT::GetChoices(ArrayT<std::string>& Strings, ArrayT<std::string>& Values) const
{
    const std::string DirName   = "Fonts";
    const char*       DirFilter = "d";

#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE          FindHandle=FindFirstFile((DirName+"\\*").c_str(), &FindFileData);
    int             EntryCount=1;   // Lua array numbering starts per convention at 1.

    if (FindHandle==INVALID_HANDLE_VALUE) return;

    do
    {
        if (strcmp(FindFileData.cFileName, "." )==0) continue;
        if (strcmp(FindFileData.cFileName, "..")==0) continue;

        if (DirFilter!=NULL)
        {
            const bool IsDir=(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;

            if (strcmp(DirFilter, "f")==0 &&  IsDir) continue;
            if (strcmp(DirFilter, "d")==0 && !IsDir) continue;
        }

        Strings.PushBack(DirName + "/" + FindFileData.cFileName);
    }
    while (FindNextFile(FindHandle, &FindFileData)!=0);

    // if (GetLastError()!=ERROR_NO_MORE_FILES)
    //     Console->Warning("Error in GetDir() while enumerating directory entries.\n");

    FindClose(FindHandle);
#else
    DIR* Dir=opendir(DirName.c_str());

    if (!Dir) return;

    for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
    {
        if (strcmp(DirEnt->d_name, "." )==0) continue;
        if (strcmp(DirEnt->d_name, "..")==0) continue;

        if (DirFilter!=NULL)
        {
            DIR* TempDir=opendir((DirName+"/"+DirEnt->d_name).c_str());
            bool IsDir=(TempDir!=NULL);

            if (TempDir!=NULL) closedir(TempDir);

            if (strcmp(DirFilter, "f")==0 &&  IsDir) continue;
            if (strcmp(DirFilter, "d")==0 && !IsDir) continue;
        }

        // For portability, only the 'd_name' member of a 'dirent' may be accessed.
        Strings.PushBack(DirName + "/" + DirEnt->d_name);
    }

    closedir(Dir);
#endif

    struct StringComparatorT
    {
        bool operator () (const std::string& s1, const std::string& s2)
        {
            return s1 < s2;
        }
    };

    Strings.QuickSort(StringComparatorT());

    for (unsigned int i = 0; i < Strings.Size(); i++)
        Values.PushBack(Strings[i]);
}


/****************************************/
/*** ComponentTextT::VarTextAlignHorT ***/
/****************************************/

ComponentTextT::VarTextAlignHorT::VarTextAlignHorT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentTextT::VarTextAlignHorT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("left");   Values.PushBack(LEFT);
    Strings.PushBack("center"); Values.PushBack(CENTER);
    Strings.PushBack("right");  Values.PushBack(RIGHT);
}


/****************************************/
/*** ComponentTextT::VarTextAlignVerT ***/
/****************************************/

ComponentTextT::VarTextAlignVerT::VarTextAlignVerT(const char* Name, const int& Value, const char* Flags[])
    : TypeSys::VarT<int>(Name, Value, Flags)
{
}


void ComponentTextT::VarTextAlignVerT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    Strings.PushBack("top");    Values.PushBack(TOP);
    Strings.PushBack("middle"); Values.PushBack(MIDDLE);
    Strings.PushBack("bottom"); Values.PushBack(BOTTOM);
}


/**********************/
/*** ComponentTextT ***/
/**********************/

namespace
{
    const char* FlagsIsLongString[] = { "IsLongString", NULL };
    const char* FlagsPaddingLabels[] = { "Labels", "hor.", "vert.", NULL };
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


const char* ComponentTextT::DocClass =
    "This components adds text to its window.";


const cf::TypeSys::VarsDocT ComponentTextT::DocVars[] =
{
    { "Text",     "The text to show in this window." },
    { "Font",     "The name of the font." },
    { "Scale",    "The scale that is applied for rendering the text." },
    { "Padding",  "Padding between text and window rectangle." },
    { "Color",    "The text color." },
    { "Alpha",    "The alpha component of the color." },
    { "horAlign", "How the text is aligned horizontally (left, center, right)." },
    { "verAlign", "How the text is aligned vertically (top, middle, bottom)." },
    { NULL, NULL }
};


ComponentTextT::ComponentTextT()
    : ComponentBaseT(),
      m_Text("Text", "", FlagsIsLongString),
      m_FontName("Font", "Fonts/Arial", NULL, *this),
      m_FontInst(NULL),
      m_Scale("Scale", 1.0f),
      m_Padding("Padding", Vector2fT(0.0f, 0.0f), FlagsPaddingLabels),
      m_Color("Color", Vector3fT(0.5f, 0.5f, 1.0f), FlagsIsColor),
      m_Alpha("Alpha", 1.0f),
      m_AlignHor("horAlign", VarTextAlignHorT::LEFT),
      m_AlignVer("verAlign", VarTextAlignVerT::TOP)
{
    FillMemberVars();
}


ComponentTextT::ComponentTextT(const ComponentTextT& Comp)
    : ComponentBaseT(Comp),
      m_Text(Comp.m_Text),
      m_FontName(Comp.m_FontName, *this),
      m_FontInst(NULL),
      m_Scale(Comp.m_Scale),
      m_Padding(Comp.m_Padding),
      m_Color(Comp.m_Color),
      m_Alpha(Comp.m_Alpha),
      m_AlignHor(Comp.m_AlignHor),
      m_AlignVer(Comp.m_AlignVer)
{
    FillMemberVars();
}


void ComponentTextT::FillMemberVars()
{
    GetMemberVars().Add(&m_Text);
    GetMemberVars().Add(&m_FontName);
    GetMemberVars().Add(&m_Scale);
    GetMemberVars().Add(&m_Padding);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Alpha);
    GetMemberVars().Add(&m_AlignHor);
    GetMemberVars().Add(&m_AlignVer);

    GetMemberVars().AddAlias("hor. Align", &m_AlignHor);
    GetMemberVars().AddAlias("ver. Align", &m_AlignVer);
}


ComponentTextT* ComponentTextT::Clone() const
{
    return new ComponentTextT(*this);
}


void ComponentTextT::UpdateDependencies(WindowT* Window)
{
    const bool WindowChanged = Window != GetWindow();

    ComponentBaseT::UpdateDependencies(Window);


    // Update resources that are provided directly or indirectly by the window.
    if (WindowChanged)
    {
        m_FontInst = NULL;

        if (GetWindow() && m_FontName.Get() != "")
        {
            m_FontInst = GetWindow()->GetGui().GetGuiResources().GetFont(m_FontName.Get());
        }
    }


/*
    // The window may or may not have changed, and/or the components of the window may have changed.
    m_Transform = NULL;

    if (GetWindow())
    {
        // It would be possible to break this loop as soon as we have assigned a non-NULL pointer to m_Transform.
        // However, this is only because the Transform component is, at this time, the only sibling component that
        // we're interested in, whereas the loop below is suitable for resolving additional dependencies, too.
        for (unsigned int CompNr = 0; CompNr < GetWindow()->GetComponents().Size(); CompNr++)
        {
            IntrusivePtrT<ComponentBaseT> Comp = GetWindow()->GetComponents()[CompNr];

            if (m_Transform == NULL)
                m_Transform = dynamic_pointer_cast<ComponentTransformT>(Comp);
        }
    }
*/
}


void ComponentTextT::Render() const
{
    if (!m_FontInst) return;

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->GetTransform()->GetSize().x;
    const float y2 = GetWindow()->GetTransform()->GetSize().y;

    int LineCount = 1;
    const size_t TextLength = m_Text.Get().length();

    for (size_t i = 0; i+1 < TextLength; i++)
        if (m_Text.Get()[i] == '\n')
            LineCount++;

    unsigned int TextCol = 0;
    TextCol |= (unsigned int)(m_Alpha.Get()    * 255.0f) << 24;
    TextCol |= (unsigned int)(m_Color.Get()[0] * 255.0f) << 16;
    TextCol |= (unsigned int)(m_Color.Get()[1] * 255.0f) << 8;
    TextCol |= (unsigned int)(m_Color.Get()[2] * 255.0f) << 0;

    const float MaxTop      = m_FontInst->GetAscender(m_Scale.Get());
    const float LineSpacing = m_FontInst->GetLineSpacing(m_Scale.Get());
    float       LineOffsetY = 0.0f;

    size_t LineStart = 0;

    while (true)
    {
        const size_t LineEnd = m_Text.Get().find('\n', LineStart);
        std::string  Line    = (LineEnd==std::string::npos) ? m_Text.Get().substr(LineStart) : m_Text.Get().substr(LineStart, LineEnd-LineStart);

        float AlignX = 0.0f;
        float AlignY = 0.0f;

        switch (m_AlignHor.Get())
        {
            case VarTextAlignHorT::LEFT:  AlignX = m_Padding.Get().x; break;
            case VarTextAlignHorT::RIGHT: AlignX = x2 - x1 - m_FontInst->GetWidth(Line, m_Scale.Get()) - m_Padding.Get().x; break;
            default:                      AlignX = (x2 - x1 - m_FontInst->GetWidth(Line, m_Scale.Get()))/2.0f; break;
        }

        switch (m_AlignVer.Get())
        {
            case VarTextAlignVerT::TOP:    AlignY = m_Padding.Get().y + MaxTop; break;   // Without the +MaxTop, the text baseline ("___") is at the top border of the window.
            case VarTextAlignVerT::BOTTOM: AlignY = y2 - y1 - m_Padding.Get().y - (LineCount-1)*LineSpacing; break;
            default:                       AlignY = (y2 - y1 - LineCount*LineSpacing)/2.0f + MaxTop; break;
        }

        m_FontInst->Print(x1 + AlignX, y1 + AlignY + LineOffsetY, m_Scale.Get(), TextCol, "%s", Line.c_str());

        if (LineEnd == std::string::npos) break;
        LineStart = LineEnd+1;
        LineOffsetY += LineSpacing;
    }
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentTextT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "text component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentTextT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentTextT();
}

const luaL_Reg ComponentTextT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentTextT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentTextT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentTextT", "GuiSys::ComponentBaseT", ComponentTextT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
