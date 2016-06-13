/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_CLIP_HPP_INCLUDED
#define CAFU_COMMAND_CLIP_HPP_INCLUDED

#include "../../CommandPattern.hpp"
#include "../MapBrush.hpp"


/// This struct describes and holds the result of clipping a brush.
struct ClipResultT
{
    ClipResultT()
    {
        Workpiece=NULL;
        Front    =NULL;
        Back     =NULL;
    }

    ~ClipResultT()
    {
        delete Front;
        delete Back;
    }

    MapBrushT* Workpiece;
    MapBrushT* Front;
    MapBrushT* Back;
};


class MapDocumentT;
class CommandSelectT;
class CommandDeleteT;


class CommandClipT : public CommandT
{
    public:

    /// The constructor.
    CommandClipT(MapDocumentT& MapDoc_, const ArrayT<ClipResultT*>& ClipResults_);

    /// The destructor.
    ~CommandClipT();

    // Implementation of the CommandT interface.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    MapDocumentT&              MapDoc;
    const ArrayT<ClipResultT*> ClipResults;
    CommandSelectT*            CommandSelect;   ///< Subcommand used by this command to change the selection.
    CommandDeleteT*            CommandDelete;   ///< Subcommand used by this command to delete the original brushes.
};

#endif
