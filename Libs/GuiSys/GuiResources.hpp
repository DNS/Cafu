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

#ifndef CAFU_GUISYS_GUI_RESOURCES_HPP_INCLUDED
#define CAFU_GUISYS_GUI_RESOURCES_HPP_INCLUDED

#include "Templates/Array.hpp"

#include <string>


namespace cf { class TrueTypeFontT; }
class CafuModelT;
class ModelManagerT;


namespace cf
{
    namespace GuiSys
    {
        /// This class manages and provides resources (fonts and models) for GuiImplT instances.
        /// One GuiResourcesT can be commonly used for several GuiImplT instances at once.
        class GuiResourcesT
        {
            public:

            /// The constructor.
            GuiResourcesT(ModelManagerT& ModelMan);

            /// The destructor.
            ~GuiResourcesT();

            /// Returns (a pointer to) a font instance for the given filename.
            /// The returned font instance is possibly shared with other users, and must not be deleted.
            /// @param FontName   The name of the font to return.
            /// @returns A pointer to the specified font, or NULL if there was an error (e.g. the font could not be loaded).
            TrueTypeFontT* GetFont(const std::string& FontName);

            /// Returns (a pointer to) a model instance for the given filename.
            /// @see ModelManagerT::GetModel() for more details.
            const CafuModelT* GetModel(const std::string& FileName, std::string& ErrorMsg);


            private:

            GuiResourcesT(const GuiResourcesT&);        ///< Use of the Copy Constructor    is not allowed.
            void operator = (const GuiResourcesT&);     ///< Use of the Assignment Operator is not allowed.

            ArrayT<TrueTypeFontT*> m_Fonts;     ///< The fonts that are used within the GUIs.
            ModelManagerT&         m_ModelMan;  ///< The model manager from which any models that occur in the GUIs are aquired.
        };
    }
}

#endif
