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

/***************************/
/*** Direct Input (Code) ***/
/***************************/

#include "DirectInput.hpp"


DirectInputT::DirectInputT()
{
    lpDirectInput7     =NULL;
    lpDIMouseDevice7   =NULL;
}


HRESULT DirectInputT::Initialize(HINSTANCE hInstance, HWND hWindow)
{
    // DirectInput7 Objekt erzeugen
    HRESULT hResult=DirectInputCreateEx(hInstance,                          // Programm-Instanz
                                        DIRECTINPUT_VERSION,                // Aktuelle Version (7.0)
                                        IID_IDirectInput7,                  // Interface Identifier
                                        (LPVOID*)&lpDirectInput7,           // Zeiger auf DI-Object
                                        NULL);
    if (FAILED(hResult)) return hResult;


    // BufferSize für gepufferten Keyboard-Input setzen
    DIPROPDWORD DIPropDW;

    DIPropDW.diph.dwSize      =sizeof(DIPROPDWORD);         // Größe der gesamten Struktur in Bytes
    DIPropDW.diph.dwHeaderSize=sizeof(DIPROPHEADER);        // Größe der Property-Header-Struktur in Bytes
    DIPropDW.diph.dwObj       =0;                           // Keine Object-Identifier, ganzes (Keyboard-)Device
    DIPropDW.diph.dwHow       =DIPH_DEVICE;                 // Ganzes (Keyboard-)Device
    DIPropDW.dwData           =64;                          // BufferSize


    // Mouse Device erzeugen
    hResult=lpDirectInput7->CreateDeviceEx(GUID_SysMouse,                   // System Mouse
                                           IID_IDirectInputDevice7,         // Device Interface Identifier
                                           (LPVOID*)&lpDIMouseDevice7,      // Zeiger auf DI Keyboard Device Object
                                           NULL);
    if (FAILED(hResult)) return hResult;

    // DataFormat (vordefiniert) setzen
    hResult=lpDIMouseDevice7->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hResult)) return hResult;

    // CooperativeLevel setzen.
    // I used to use the DISCL_NONEXCLUSIVE flag here instead of DISCL_EXCLUSIVE, but this seems to have a bad
    // effect on using the normal Windows event queue for keyboard input at the same time.
    // For example, when I pressed and held a key, then moved the mouse for a few seconds, then released the key,
    // it often took several seconds(!!) until the WM_KEYUP message arrived (and the player continued to walk if
    // the key happend to be one of the arrow keys).
    // I don't know why, but using DISCL_EXCLUSIVE solved the problem, which I found out on accident by experimentation.
    // Q3 uses DISCL_EXCLUSIVE for its mouse input, too, but I'd be interested to learn more about the backgrounds of
    // this behaviour. Found nothing on the internet though, and the days of DirectInput seem to be counted anyway...
    hResult=lpDIMouseDevice7->SetCooperativeLevel(hWindow, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hResult)) return hResult;

    // BufferSize für gepufferten Mouse-Input setzen
    // Benutze einfach DIPropDW vom Keyboard nochmal!
    hResult=lpDIMouseDevice7->SetProperty(DIPROP_BUFFERSIZE, &DIPropDW.diph);
    if (FAILED(hResult)) return hResult;

    // Acquire Mouse
    hResult=lpDIMouseDevice7->Acquire();
    if (FAILED(hResult)) return hResult;

    return DI_OK;
}


void DirectInputT::Release()
{
    if (lpDIMouseDevice7)
    {
        lpDIMouseDevice7->Unacquire();
        lpDIMouseDevice7->Release();
        lpDIMouseDevice7=NULL;
    }

    if (lpDirectInput7)
    {
        lpDirectInput7->Release();
        lpDirectInput7=NULL;
    }
}


DirectInputT::~DirectInputT()
{
    Release();
}


HRESULT DirectInputT::GetNextMouseEvent(DIDEVICEOBJECTDATA* MouseEvent, DWORD* ReadNrOfEvents)
{
    // Diese Schleife bricht normalerweise von selbst spätestens im zweiten Durchlauf ab, und könnte daher auch
    // durch eine Endlosschleife ersetzt werden. Der count dient nur zur Sicherheit, falls etwas wirklich schiefgeht.
    for (char count=3; count; count--)
    {
        *ReadNrOfEvents=1;
        HRESULT hResult=lpDIMouseDevice7->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), MouseEvent, ReadNrOfEvents, 0);

        if (SUCCEEDED(hResult)) return hResult;

        // Normalerweise ist spätestens hier beim zweiten Schleifendurchlauf alles zu Ende!
        // Andernfalls stimmt etwas ganz und gar nicht (Acquire OK, aber GetDeviceData liefert InputLost/NotAcqu).
        if (hResult!=DIERR_INPUTLOST && hResult!=DIERR_NOTACQUIRED) return hResult;

        // We either just lost the keyboard input, or keyboard was not aquired, so try to reacquire it
        // (DIERR_INPUTLOST only occurs once, thereafter the device is treated as not acquired.)
        hResult=lpDIMouseDevice7->Acquire();
        if (FAILED(hResult)) return hResult;        // Report serious failure, including DIERR_OTHERAPPHASPRIO
    }

    return DIERR_GENERIC;
}


char DirectInputT::GetMouseButtonsImmediate()
{
    for (char count=3; count; count--)
    {
        DIMOUSESTATE MouseState;
        HRESULT      hResult=lpDIMouseDevice7->GetDeviceState(sizeof(MouseState), &MouseState);

        if (hResult!=DIERR_INPUTLOST && hResult!=DIERR_NOTACQUIRED)
        {
            char Buttons=0;

            for (char ButtonNr=0; ButtonNr<4; ButtonNr++)
                if (MouseState.rgbButtons[ButtonNr] & 0x80)
                    Buttons|=1 << ButtonNr;

            return Buttons;
        }

        // Reacquire Mouse
        hResult=lpDIMouseDevice7->Acquire();
        if (FAILED(hResult)) return 0;
    }

    return 0;
}
