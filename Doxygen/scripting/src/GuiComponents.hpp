namespace GUI
{



/// @cppName{ComponentBaseT}
class ComponentBaseT
{
    public:

    get();
    set();
    GetExtraMessage();
    interpolate();
};


/// @cppName{ComponentBasicsT}
class ComponentBasicsT : public ComponentBaseT
{
    public:



    public:

    string Name;
    boolean Show;
};


/// @cppName{ComponentBorderT}
class ComponentBorderT : public ComponentBaseT
{
    public:



    public:

    number Width;
    tuple Color;
    number Alpha;
};


/// @cppName{ComponentChoiceT}
class ComponentChoiceT : public ComponentBaseT
{
    public:

    set();
    GetSelItem();


    public:

    table Choices;
    number Selection;
};


/// @cppName{ComponentImageT}
class ComponentImageT : public ComponentBaseT
{
    public:



    public:

    string Material;
    tuple Color;
    number Alpha;
};


/// @cppName{ComponentListBoxT}
class ComponentListBoxT : public ComponentBaseT
{
    public:

    GetSelItem();


    public:

    table Items;
    number Selection;
    tuple BgColorOdd;
    number BgAlphaOdd;
    tuple BgColorEven;
    number BgAlphaEven;
    tuple BgColorSel;
    number BgAlphaSel;
    tuple TextColorSel;
    number TextAlphaSel;
};


/// @cppName{ComponentModelT}
class ComponentModelT : public ComponentBaseT
{
    public:

    GetNumAnims();
    SetAnim();
    GetNumSkins();


    public:

    string Name;
    number Animation;
    number Skin;
    tuple Pos;
    number Scale;
    tuple Angles;
    tuple CameraPos;
};


/// @cppName{ComponentSelectionT}
class ComponentSelectionT : public ComponentBaseT
{
    public:

};


/// @cppName{ComponentTextEditT}
class ComponentTextEditT : public ComponentBaseT
{
    public:

    SetText();


    public:

    number CursorPos;
    number CursorType;
    number CursorRate;
    tuple CursorColor;
    number CursorAlpha;
};


/// @cppName{ComponentTextT}
class ComponentTextT : public ComponentBaseT
{
    public:



    public:

    string Text;
    string Font;
    number Scale;
    tuple Padding;
    tuple Color;
    number Alpha;
    number hor. Align;
    number ver. Align;
};


/// @cppName{ComponentTransformT}
class ComponentTransformT : public ComponentBaseT
{
    public:



    public:

    tuple Pos;
    tuple Size;
    number Rotation;
};

}   // namespace GUI
