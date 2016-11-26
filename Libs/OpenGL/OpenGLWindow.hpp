/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_OPENGL_WINDOW_HPP_INCLUDED
#define CAFU_OPENGL_WINDOW_HPP_INCLUDED


/// This struct describes a keyboard event.
struct CaKeyboardEventT
{
    /// This enum describes the type of the key event.
    enum TypeT
    {
        CKE_KEYDOWN,
        CKE_CHAR,
        CKE_KEYUP
    };

    /// This enum describes a key in a key event.
    enum KeyT
    {
        CK_ESCAPE      =0x01,
        CK_1           =0x02,
        CK_2           =0x03,
        CK_3           =0x04,
        CK_4           =0x05,
        CK_5           =0x06,
        CK_6           =0x07,
        CK_7           =0x08,
        CK_8           =0x09,
        CK_9           =0x0A,
        CK_0           =0x0B,
        CK_MINUS       =0x0C,   ///< \- on main keyboard
        CK_EQUALS      =0x0D,
        CK_BACKSPACE   =0x0E,   ///< Backspace
        CK_TAB         =0x0F,
        CK_Q           =0x10,
        CK_W           =0x11,
        CK_E           =0x12,
        CK_R           =0x13,
        CK_T           =0x14,
        CK_Y           =0x15,
        CK_U           =0x16,
        CK_I           =0x17,
        CK_O           =0x18,
        CK_P           =0x19,
        CK_LBRACKET    =0x1A,
        CK_RBRACKET    =0x1B,
        CK_RETURN      =0x1C,   ///< Enter on main keyboard
        CK_LCONTROL    =0x1D,
        CK_A           =0x1E,
        CK_S           =0x1F,
        CK_D           =0x20,
        CK_F           =0x21,
        CK_G           =0x22,
        CK_H           =0x23,
        CK_J           =0x24,
        CK_K           =0x25,
        CK_L           =0x26,
        CK_SEMICOLON   =0x27,
        CK_APOSTROPHE  =0x28,
        CK_GRAVE       =0x29,   ///< Accent Grave
        CK_LSHIFT      =0x2A,
        CK_BACKSLASH   =0x2B,
        CK_Z           =0x2C,
        CK_X           =0x2D,
        CK_C           =0x2E,
        CK_V           =0x2F,
        CK_B           =0x30,
        CK_N           =0x31,
        CK_M           =0x32,
        CK_COMMA       =0x33,
        CK_PERIOD      =0x34,   ///< . on main keyboard
        CK_SLASH       =0x35,   ///< / on main keyboard
        CK_RSHIFT      =0x36,
        CK_MULTIPLY    =0x37,   ///< * on numeric keypad
        CK_LMENU       =0x38,   ///< left Alt
        CK_SPACE       =0x39,
        CK_CAPITAL     =0x3A,
        CK_F1          =0x3B,
        CK_F2          =0x3C,
        CK_F3          =0x3D,
        CK_F4          =0x3E,
        CK_F5          =0x3F,
        CK_F6          =0x40,
        CK_F7          =0x41,
        CK_F8          =0x42,
        CK_F9          =0x43,
        CK_F10         =0x44,
        CK_NUMLOCK     =0x45,
        CK_SCROLL      =0x46,   ///< Scroll Lock
        CK_NUMPAD7     =0x47,
        CK_NUMPAD8     =0x48,
        CK_NUMPAD9     =0x49,
        CK_SUBTRACT    =0x4A,   ///< \- on numeric keypad
        CK_NUMPAD4     =0x4B,
        CK_NUMPAD5     =0x4C,
        CK_NUMPAD6     =0x4D,
        CK_ADD         =0x4E,   ///< + on numeric keypad
        CK_NUMPAD1     =0x4F,
        CK_NUMPAD2     =0x50,
        CK_NUMPAD3     =0x51,
        CK_NUMPAD0     =0x52,
        CK_DECIMAL     =0x53,   ///< . on numeric keypad
        CK_OEM_102     =0x56,   ///< < > | on UK/Germany keyboards
        CK_F11         =0x57,
        CK_F12         =0x58,
        CK_F13         =0x64,   ///< (NEC PC98)
        CK_F14         =0x65,   ///< (NEC PC98)
        CK_F15         =0x66,   ///< (NEC PC98)
        CK_KANA        =0x70,   ///< (Japanese keyboard)
        CK_ABNT_C1     =0x73,   ///< / ? on Portugese (Brazilian) keyboards
        CK_CONVERT     =0x79,   ///< (Japanese keyboard)
        CK_NOCONVERT   =0x7B,   ///< (Japanese keyboard)
        CK_YEN         =0x7D,   ///< (Japanese keyboard)
        CK_ABNT_C2     =0x7E,   ///< Numpad . on Portugese (Brazilian) keyboards
        CK_NUMPADEQUALS=0x8D,   ///< = on numeric keypad (NEC PC98)
        CK_PREVTRACK   =0x90,   ///< Previous Track (DIK_CIRCUMFLEX on Japanese keyboard)
        CK_AT          =0x91,   ///< (NEC PC98)
        CK_COLON       =0x92,   ///< (NEC PC98)
        CK_UNDERLINE   =0x93,   ///< (NEC PC98)
        CK_KANJI       =0x94,   ///< (Japanese keyboard)
        CK_STOP        =0x95,   ///< (NEC PC98)
        CK_AX          =0x96,   ///< (Japan AX)
        CK_UNLABELED   =0x97,   ///< (J3100)
        CK_NEXTTRACK   =0x99,   ///< Next Track
        CK_NUMPADENTER =0x9C,   ///< Enter on numeric keypad
        CK_RCONTROL    =0x9D,
        CK_MUTE        =0xA0,   ///< Mute
        CK_CALCULATOR  =0xA1,   ///< Calculator
        CK_PLAYPAUSE   =0xA2,   ///< Play / Pause
        CK_MEDIASTOP   =0xA4,   ///< Media Stop
        CK_VOLUMEDOWN  =0xAE,   ///< Volume -
        CK_VOLUMEUP    =0xB0,   ///< Volume +
        CK_WEBHOME     =0xB2,   ///< Web home
        CK_NUMPADCOMMA =0xB3,   ///< , on numeric keypad (NEC PC98)
        CK_DIVIDE      =0xB5,   ///< / on numeric keypad
        CK_SYSRQ       =0xB7,
        CK_RMENU       =0xB8,   ///< right Alt
        CK_PAUSE       =0xC5,   ///< Pause
        CK_HOME        =0xC7,   ///< Home on arrow keypad
        CK_UP          =0xC8,   ///< UpArrow on arrow keypad
        CK_PGUP        =0xC9,   ///< PgUp on arrow keypad
        CK_LEFT        =0xCB,   ///< LeftArrow on arrow keypad
        CK_RIGHT       =0xCD,   ///< RightArrow on arrow keypad
        CK_END         =0xCF,   ///< End on arrow keypad
        CK_DOWN        =0xD0,   ///< DownArrow on arrow keypad
        CK_PGDN        =0xD1,   ///< PgDn on arrow keypad
        CK_INSERT      =0xD2,   ///< Insert on arrow keypad
        CK_DELETE      =0xD3,   ///< Delete on arrow keypad
        CK_LWIN        =0xDB,   ///< Left Windows key
        CK_RWIN        =0xDC,   ///< Right Windows key
        CK_APPS        =0xDD,   ///< AppMenu key
        CK_POWER       =0xDE,   ///< System Power
        CK_SLEEP       =0xDF,   ///< System Sleep
        CK_WAKE        =0xE3,   ///< System Wake
        CK_WEBSEARCH   =0xE5,   ///< Web Search
        CK_WEBFAVORITES=0xE6,   ///< Web Favorites
        CK_WEBREFRESH  =0xE7,   ///< Web Refresh
        CK_WEBSTOP     =0xE8,   ///< Web Stop
        CK_WEBFORWARD  =0xE9,   ///< Web Forward
        CK_WEBBACK     =0xEA,   ///< Web Back
        CK_MYCOMPUTER  =0xEB,   ///< My Computer
        CK_MAIL        =0xEC,   ///< Mail
        CK_MEDIASELECT =0xED,   ///< Media Select
    };

    TypeT Type;     ///< The type of this keyboard event.
    int   Key;      ///< If Type is one of CKE_KEYDOWN or CKE_KEYUP, this is one of the CK_* keys from the KeyT enum. If Type is CKE_CHAR instead, this is the (ASCII) character code of the key.
};


/// This struct describes a mouse event.
struct CaMouseEventT
{
    /// This enum describes the type of the mouse event.
    enum TypeT
    {
        CM_BUTTON0=  0,
        CM_BUTTON1=  1,
        CM_BUTTON2=  2,
        CM_BUTTON3=  3,
        CM_MOVE_X =200,
        CM_MOVE_Y =201,
        CM_MOVE_Z =202,    ///< The mouse wheel.
    };

    TypeT Type;     ///< The type of the mouse event.
    int   Amount;   ///< The amount.
};

#endif
