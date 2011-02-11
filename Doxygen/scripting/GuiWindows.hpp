

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
