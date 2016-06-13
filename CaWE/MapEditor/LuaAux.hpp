/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_LUA_AUX_HPP_INCLUDED
#define CAFU_LUA_AUX_HPP_INCLUDED


// MOVEMENT INFO
// CF: Achtung: Diese Konstanten zu entfernen und durch AngleT's pitch(), roll() und yaw() zu ersetzen *kann*
// problematisch sein, weil im Code ab und zu auch statt [PITCH] einfach [0] usw. geschrieben wird.
// Am besten einfach vorübergehend den [] operator mal privat machen, um alle Stellen zuverlässig zu finden...
// Es bleibt zu hoffen, dass nirgendwo direkt .x .y oder .z geschrieben wird...
enum
{
    PITCH=0,    // Nose up/down.
    YAW,        // Heading.
    ROLL        // Bank angle.
};

#endif
