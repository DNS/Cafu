/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CLIPSYS_CLIPWORLD_PRIVATE_HPP_INCLUDED
#define CAFU_CLIPSYS_CLIPWORLD_PRIVATE_HPP_INCLUDED

/// @file
/// This header contains declarations of classes that are privately used by the ClipWorldT and ClipModelT classes.
/// (The classes declared here are also shared by the ClipWorldT and ClipModelT classes, because they are friends of each other.)


namespace cf
{
    namespace ClipSys
    {
        class ClipLinkT
        {
            public:

            ClipModelT*  ClipModel;             ///< This clip model is...
            ClipSectorT* ClipSector;            ///<    ...in this clip sector (the model can be in more sectors and the sector can have more models).
            ClipLinkT*   PrevModelInSector;     ///< The previous model that is in the same sector.
            ClipLinkT*   NextModelInSector;     ///< The next     model that is in the same sector.
            ClipLinkT*   NextSectorOfModel;     ///< The next sector of the same model.
        };


        class ClipSectorT
        {
            public:

            /// The constructor.
            ClipSectorT() : WorldContents(0), ModelContents(0), ListOfModels(NULL) { }

            unsigned long WorldContents;    ///< The combined (or'ed) contents of the world model for this sector.
            unsigned long ModelContents;    ///< The combined (or'ed) contents of the models in ListOfModels.
            ClipLinkT*    ListOfModels;     ///< The list of all clip models (except for the world model) that are in this sector. The world model is not in the list, because it implicitly and always is in every sector anyway.
        };
    }
}

#endif
