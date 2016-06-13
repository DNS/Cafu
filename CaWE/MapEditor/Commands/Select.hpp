/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_COMMAND_SELECT_HPP_INCLUDED
#define CAFU_COMMAND_SELECT_HPP_INCLUDED

#include "../../CommandPattern.hpp"


class MapDocumentT;
class MapElementT;
class MapPrimitiveT;


class CommandSelectT : public CommandT
{
    public:

    // Named constructors for easier command creation.
    static CommandSelectT* Clear (MapDocumentT* MapDocument);
    static CommandSelectT* Add   (MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Add   (MapDocumentT* MapDocument, MapElementT* MapElement);
    static CommandSelectT* Remove(MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Remove(MapDocumentT* MapDocument, MapElementT* MapElement);
    static CommandSelectT* Set   (MapDocumentT* MapDocument, const ArrayT<MapElementT*>& MapElements);
    static CommandSelectT* Set   (MapDocumentT* MapDocument, MapElementT* MapElement);
    static CommandSelectT* Set   (MapDocumentT* MapDocument, const ArrayT<MapPrimitiveT*>& Primitives);

    ~CommandSelectT();

    // CommandT implementation.
    bool Do();
    void Undo();
    wxString GetName() const;


    private:

    // Only named constructors may create this command.
    CommandSelectT(MapDocumentT* MapDocument, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);

    MapDocumentT* m_MapDocument;

    const ArrayT<MapElementT*> m_OldSelection;
    const ArrayT<MapElementT*> m_NewSelection;
};

#endif
