/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MixerTrackMan.hpp"
#include "MixerTrack.hpp"

#include <iostream>
#include <stdexcept>


MixerTrackManT* MixerTrackManT::GetInstance()
{
    static MixerTrackManT MixerTrackManInstance;

    return &MixerTrackManInstance;
}


MixerTrackManT::MixerTrackManT()
{
}


MixerTrackManT::~MixerTrackManT()
{
    ReleaseAll();
}


MixerTrackT* MixerTrackManT::GetMixerTrack(unsigned int Priority)
{
    unsigned long i=0;

    // Look for a free mixer track and return it.
    for (i=0; i<MixerTracks.Size(); i++)
    {
        if (!MixerTracks[i]->IsUsed())
        {
            // Remove previous buffer from mixer track and return the mixer track.
            MixerTracks[i]->StopAndDetach();

            return MixerTracks[i];
        }
    }

    // No free mixer track found -> create new one.
    try
    {
        MixerTracks.PushBack(new MixerTrackT());

        // Return the newly created mixer track.
        return MixerTracks[i];
    }
    catch (const std::runtime_error& RE)
    {
        std::cout << "OpenAL: " << RE.what() << "\n";

        // Okay no free track found and we couldn't create a new one (probably because of resource restrictions).
        // Kick a sound from a mixer track that has a lower priority than the passed priority for this request.
        for (i=0; i<MixerTracks.Size(); i++)
        {
            if (MixerTracks[i]->GetPriority()<Priority)
            {
                // Remove buffer with lower priority from mixer track and return the mixer track.
                MixerTracks[i]->StopAndDetach();

                return MixerTracks[i];
            }
        }

        // The worst case: no free mixer track found, a new mixer track could not be created and all playing sounds have
        // a higher priority than our request.
        return NULL;
    }
}


void MixerTrackManT::CleanUp()
{
    for (unsigned long i=0; i<MixerTracks.Size();)
    {
        if (!MixerTracks[i]->IsUsed())
        {
            delete MixerTracks[i];
            MixerTracks.RemoveAtAndKeepOrder(i);
            continue;
        }
        // Only increase i if no mixer track to clean up was found, since RemoveAndKeepOrder()
        // moves the next array entry to the current position.
        i++;
    }
}


void MixerTrackManT::UpdateAll()
{
    for (unsigned long i=0; i<MixerTracks.Size(); i++)
        MixerTracks[i]->Update();
}


void MixerTrackManT::ReleaseAll()
{
    for (unsigned long i=0; i<MixerTracks.Size(); i++)
        delete MixerTracks[i];

    MixerTracks.Clear();
}
