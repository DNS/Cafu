/**
 * @defgroup GUI Graphical User Interface
 *
 * This module groups all script classes related to graphical user interfaces (GUIs) in Cafu.
 *
 * Cafu GUIs are like computer desktops: They are made of a hierarchy of windows.
 * Each window has graphical attributes and elements, it can contain text and can be interacted with.
 * GUIs can be used both in 2D, e.g. for the game's Main Menu, as well as in 3D,
 * e.g. for the in-game controls of a teleporter station, to call lifts, etc.
 *
 * GUI scripts are used both to describe the window setup and contents, as well as
 * to react to window events like mouse cursor movement and clicks, keyboard input, etc.
 * In a sense, a GUI is like a mini operating system, and the GUI scripts are the means to program it.
 *
 * For example GUI scripts, see the files in
 *   - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/GUIs
 *
 * @{
 */


/// @cppName{WindowT}
class WindowT
{
    public:

    set();
    get();
    interpolate();
    SetName();
    GetName();
    AddChild();
    RemoveChild();
};


/// @cppName{ModelWindowT}
class ModelWindowT : public WindowT
{
    public:

    SetModel();
    GetModelNrOfSqs();
    SetModelSequNr();
    SetModelPos();
    SetModelScale();
    SetModelAngles();
    SetCameraPos();
};


/// @cppName{ListBoxT}
class ListBoxT : public WindowT
{
    public:

    Clear();
    Append();
    Insert();
    GetNumRows();
    GetRowText();
    SetRowText();
    GetSelection();
    SetSelection();
    GetRowHeight();
    SetRowHeight();
    SetOddRowBgColor();
    SetEvenRowBgColor();
    SetRowTextColor();
    SetSelRowBgColor();
    SetSelRowTextColor();
};


/// @cppName{EditWindowT}
class EditWindowT : public WindowT
{
    public:

    set();
    GetTextCursorPos();
    SetTextCursorPos();
    SetTextCursorType();
    SetTextCursorRate();
    SetTextCursorColor();
};


/// @cppName{ChoiceT}
class ChoiceT : public WindowT
{
    public:

    Clear();
    Append();
    Insert();
    GetNumChoices();
    GetChoice();
    SetChoice();
    GetSelection();
    SetSelection();
};

/** @} */   // End of group GUI.
