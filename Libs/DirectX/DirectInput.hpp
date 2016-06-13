/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*****************************/
/*** Direct Input (Header) ***/
/*****************************/

#ifndef CAFU_DIRECTINPUT_HPP_INCLUDED
#define CAFU_DIRECTINPUT_HPP_INCLUDED

#define WIN32_LEAN_AND_MEAN

#include <windows.h>    // Header file for Win32.
#include <dinput.h>     // Header file for DirectInput.


class DirectInputT
{
    private:

    LPDIRECTINPUT7       lpDirectInput7;            // DirectInput 7.0 Interface.
    LPDIRECTINPUTDEVICE7 lpDIMouseDevice7;          // DirectInput 7.0 Mouse Device Interface.


    public:

    /// Create DirectInput object.
    DirectInputT();

    /// Initialize DirectInput:
    /// - Create DirectInput 7.0 object
    /// - Create Mouse Device
    ///       Set DataFormat
    ///       Set CooperativeLevel (Foreground and Non-Exclusive)
    ///       Set BufferSize to 64 setzen for buffered mouse events.
    ///       Aquire mouse.
    /// This is separated from the constructor to return errors without the use of exceptions.
    /// If Initialize() fails the deconstructor will clean up everything.
    HRESULT Initialize(HINSTANCE hInstance, HWND hWindow);

    /// Release DirectInput object. This must be possible outside of the destructor if for example
    /// the hWindow object from Initialize becomes invalid because the associated windows has been
    /// closed and reopened.
    void Release();

    /// Destrcutor.
   ~DirectInputT();

    /// Reads the next mouse event from the mouse buffer and returns it in MouseEvent.
    /// ReadNrOfEvents is 1 if the event was read and 0 if the buffer was empty.
    /// If something different from DI_OK or DI_BUFFEROVERFLOW is returned,
    /// MouseEvent and ReadNrOfEvents are undefined.
    HRESULT GetNextMouseEvent(DIDEVICEOBJECTDATA* MouseEvent, DWORD* ReadNrOfEvents);

    /// Returns the current state of the mouse buttons.
    /// If the n-th bit of the return value is set, the n-th mouse button is pressed.
    char GetMouseButtonsImmediate();
};

#endif
