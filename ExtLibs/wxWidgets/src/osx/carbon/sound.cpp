/////////////////////////////////////////////////////////////////////////////
// Name:        src/osx/carbon/sound.cpp
// Purpose:     wxSound class implementation: optional
// Author:      Ryan Norton
// Modified by: Stefan Csomor
// Created:     1998-01-01
// RCS-ID:      $Id: sound.cpp 58781 2009-02-08 23:05:19Z KO $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_SOUND

#include "wx/sound.h"

#ifndef WX_PRECOMP
    #include "wx/object.h"
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/timer.h"
#endif

#include "wx/file.h"

// Carbon QT Implementation Details -
//
// Memory:
// 1) OpenDefaultComponent(MovieImportType, kQTFileTypeWave);
// 2) NewMovie(0);
// 3) MovieImportDataRef() //Pass Memory Location to this
// 4) PlayMovie();
// 5) IsMovieDone(), MoviesTask() //2nd param is minimum wait time to allocate to quicktime
//
// File:
// 1) Path as CFString
// 2) Call QTNewDataReferenceFromFullPathCFString
// 3) Call NewMovieFromDataRef
// 4) Call CloseMovieFile
// 4) PlayMovie();
// 5) IsMovieDone(), MoviesTask() //2nd param is minimum wait time to allocate to quicktime
//

#ifdef __WXMAC__
#include "wx/osx/uma.h"
#ifndef __DARWIN__
#include <Movies.h>
#include <Gestalt.h>
#endif
#endif

#if __WXOSX_COCOA__
#include "wx/osx/private.h"
#endif

#include <Carbon/Carbon.h>

//quicktime media layer only required for mac emulation on pc
#ifndef __WXMAC__
#include <qtml.h>
#endif

#ifndef __DARWIN__
#include <QuickTimeComponents.h>
#else
#include <QuickTime/QuickTimeComponents.h>
#endif

//Time between timer calls
#define MOVIE_DELAY 100

static wxTimer* lastSoundTimer=NULL;
static bool lastSoundIsPlaying=false;

#if !defined(__LP64__)
#define USE_QUICKTIME 1
#else
#define USE_QUICKTIME 0
#endif

#if USE_QUICKTIME
// ------------------------------------------------------------------
//          wxQTTimer - Handle Asyncronous Playing
// ------------------------------------------------------------------
class wxQTTimer : public wxTimer
{
public:
    wxQTTimer(Movie movie, bool bLoop, bool* playing) :
        m_movie(movie), m_bLoop(bLoop), m_pbPlaying(playing)
    {
    }

    virtual ~wxQTTimer()
    {
        if(m_pbPlaying)
            *m_pbPlaying = false;

        StopMovie(m_movie);
        DisposeMovie(m_movie);
        Stop();

        //Note that ExitMovies() is not necessary, but
        //the docs are fuzzy on whether or not TerminateQTML is
        ExitMovies();

#ifndef __WXMAC__
        TerminateQTML();
#endif
    }

    void Shutdown()
    {
        delete this;
    }

    void Notify()
    {
        if (m_pbPlaying && !*m_pbPlaying)
        {
            Shutdown();
        }

        if(IsMovieDone(m_movie))
        {
            if (!m_bLoop)
                Shutdown();
            else
            {
                StopMovie(m_movie);
                GoToBeginningOfMovie(m_movie);
                StartMovie(m_movie);
            }
        }
        else
            MoviesTask(m_movie, MOVIE_DELAY); //Give QT time to play movie
    }


    Movie& GetMovie() {return m_movie;}

protected:
    Movie m_movie;
    bool m_bLoop;

public:
    bool* m_pbPlaying;

};


class wxSMTimer : public wxTimer
{
public:
    wxSMTimer(void* hSnd, void* pSndChannel, bool bLoop, bool* playing)
        : m_hSnd(hSnd), m_pSndChannel(pSndChannel), m_bLoop(bLoop), m_pbPlaying(playing)
    {
    }

    virtual ~wxSMTimer()
    {
        if(m_pbPlaying)
            *m_pbPlaying = false;
        SndDisposeChannel((SndChannelPtr)m_pSndChannel, TRUE);
        Stop();
    }

    void Notify()
    {
        if (m_pbPlaying && !*m_pbPlaying)
        {
            Shutdown();
        }

        SCStatus stat;

        if (SndChannelStatus((SndChannelPtr)m_pSndChannel, sizeof(SCStatus), &stat) != 0)
            Shutdown();

        //if the sound isn't playing anymore, see if it's looped,
        //and if so play it again, otherwise close things up
        if (stat.scChannelBusy == FALSE)
        {
            if (m_bLoop)
            {
                if(SndPlay((SndChannelPtr)m_pSndChannel, (SndListHandle) m_hSnd, true) != noErr)
                    Shutdown();
            }
            else
                Shutdown();
        }
    }

    void Shutdown()
    {
        delete this;
    }

    void* GetChannel() {return m_pSndChannel;}

protected:
    void* m_hSnd;
    void* m_pSndChannel;
    bool m_bLoop;

public:
    bool* m_pbPlaying;
};

// ------------------------------------------------------------------
//          wxSound
// ------------------------------------------------------------------

//Determines whether version 4 of QT is installed
Boolean wxIsQuickTime4Installed (void)
{
#ifdef __WXMAC__
    short error;
    long result;

    error = Gestalt (gestaltQuickTime, &result);
    return (error == noErr) && (((result >> 16) & 0xffff) >= 0x0400);
#else
    return true;
#endif
}

inline bool wxInitQT ()
{
    if (wxIsQuickTime4Installed())
    {
        #ifndef __WXMAC__
        int nError;
        //-2093 no dll
            if ((nError = InitializeQTML(0)) != noErr)
                wxLogSysError(wxString::Format(wxT("Couldn't Initialize Quicktime-%i"), nError));
        #endif
        EnterMovies();
        return true;
    }
    else
    {
        wxLogSysError(wxT("Quicktime is not installed, or Your Version of Quicktime is <= 4."));
        return false;
    }
}

#endif

wxSound::wxSound()
: m_hSnd(NULL), m_waveLength(0), m_pTimer(NULL), m_type(wxSound_NONE)
{
}

wxSound::wxSound(const wxString& sFileName, bool isResource)
: m_hSnd(NULL), m_waveLength(0), m_pTimer(NULL), m_type(wxSound_NONE)
{
    Create(sFileName, isResource);
}

wxSound::wxSound(int size, const wxByte* data)
: m_hSnd((char*)data), m_waveLength(size), m_pTimer(NULL), m_type(wxSound_MEMORY)
{
}

wxSound::~wxSound()
{
}

bool wxSound::Create(const wxString& fileName, bool isResource)
{
    Stop();

    if (isResource)
    {
#ifdef __WXMAC__
        m_type = wxSound_RESOURCE;

        Str255 lpSnd ;

        wxMacStringToPascal( fileName , lpSnd ) ;

        m_sndname = fileName;
        m_hSnd = (char*) GetNamedResource('snd ', (const unsigned char *) lpSnd);
#else
        return false;
#endif
    }
    else
    {
        m_type = wxSound_FILE;
        m_sndname = fileName;
    }

    return true;
}

bool wxSound::DoPlay(unsigned flags) const
{
    Stop();

#if USE_QUICKTIME

    Movie movie;

    switch(m_type)
    {
    case wxSound_MEMORY:
        {
            if (!wxInitQT())
                return false;
            Handle myHandle, dataRef = nil;
            MovieImportComponent miComponent;
            Track targetTrack = nil;
            TimeValue addedDuration = 0;
            long outFlags = 0;
            OSErr err;
            ComponentResult result;

            myHandle = NewHandleClear((Size)m_waveLength);

            BlockMove(m_hSnd, *myHandle, m_waveLength);

            err = PtrToHand(&myHandle, &dataRef, sizeof(Handle));

            if (memcmp(&m_hSnd[8], "WAVE", 4) == 0)
                miComponent = OpenDefaultComponent(MovieImportType, kQTFileTypeWave);
            else if (memcmp(&m_hSnd[8], "AIFF", 4) == 0)
                miComponent = OpenDefaultComponent(MovieImportType, kQTFileTypeAIFF);
            else if (memcmp(&m_hSnd[8], "AIFC", 4) == 0)
                miComponent = OpenDefaultComponent(MovieImportType, kQTFileTypeAIFC);
            else
            {
                wxLogSysError(wxT("wxSound - Location in memory does not contain valid data"));
                return false;
            }

            movie = NewMovie(0);

            result = MovieImportDataRef(miComponent,                dataRef,
                                        HandleDataHandlerSubType,   movie,
                                        nil,                        &targetTrack,
                                        nil,                        &addedDuration,
                                        movieImportCreateTrack,     &outFlags);

            if (result != noErr)
            {
                wxLogSysError(wxString::Format(wxT("Couldn't import movie data\nError:%i"), (int)result));
            }

            SetMovieVolume(movie, kFullVolume);
            GoToBeginningOfMovie(movie);

            DisposeHandle(myHandle);
        }
        break;
    case wxSound_RESOURCE:
        {
            SoundComponentData data;
            unsigned long numframes, offset;

            ParseSndHeader((SndListHandle)m_hSnd, &data, &numframes, &offset);
            //m_waveLength = numFrames * data.numChannels;

            SndChannelPtr pSndChannel;
            SndNewChannel(&pSndChannel, sampledSynth,
                initNoInterp
                + (data.numChannels == 1 ? initMono : initStereo), NULL);

            if(SndPlay(pSndChannel, (SndListHandle) m_hSnd, flags & wxSOUND_ASYNC ? 1 : 0) != noErr)
                return false;

            if (flags & wxSOUND_ASYNC)
            {
                lastSoundTimer = ((wxSMTimer*&)m_pTimer)
                    = new wxSMTimer(pSndChannel, m_hSnd, flags & wxSOUND_LOOP ? 1 : 0,
                                    &lastSoundIsPlaying);
                lastSoundIsPlaying = true;

                ((wxTimer*)m_pTimer)->Start(MOVIE_DELAY, wxTIMER_CONTINUOUS);
            }
            else
                SndDisposeChannel(pSndChannel, TRUE);

            return true;
        }
        break;
    case wxSound_FILE:
        {
            if (!wxInitQT())
                return false;

            OSErr err = noErr ;

            Handle dataRef = NULL;
            OSType dataRefType;

            err = QTNewDataReferenceFromFullPathCFString(wxCFStringRef(m_sndname,wxLocale::GetSystemEncoding()),
                (UInt32)kQTNativeDefaultPathStyle, 0, &dataRef, &dataRefType);

            wxASSERT(err == noErr);

            if (NULL != dataRef || err != noErr)
            {
                err = NewMovieFromDataRef( &movie, newMovieDontAskUnresolvedDataRefs , NULL, dataRef, dataRefType );
                wxASSERT(err == noErr);
                DisposeHandle(dataRef);
            }

            if (err != noErr)
            {
                wxLogSysError(
                    wxString::Format(wxT("wxSound - Could not open file: %s\nError:%i"), m_sndname.c_str(), err )
                    );
                return false;
            }
        }
        break;
    default:
        return false;
    }//end switch(m_type)

    //Start the movie!
    StartMovie(movie);

    if (flags & wxSOUND_ASYNC)
    {
        //Start timer and play movie asyncronously
        lastSoundTimer = ((wxQTTimer*&)m_pTimer) =
            new wxQTTimer(movie, flags & wxSOUND_LOOP ? 1 : 0,
                          &lastSoundIsPlaying);
        lastSoundIsPlaying = true;
        ((wxQTTimer*)m_pTimer)->Start(MOVIE_DELAY, wxTIMER_CONTINUOUS);
    }
    else
    {
        wxASSERT_MSG(!(flags & wxSOUND_LOOP), wxT("Can't loop and play syncronously at the same time"));

        //Play movie until it ends, then exit
        //Note that due to quicktime caching this may not always
        //work 100% correctly
        while (!IsMovieDone(movie))
            MoviesTask(movie, 1);

        DisposeMovie(movie);
    }
#endif

    return true;
}

bool wxSound::IsPlaying()
{
    return lastSoundIsPlaying;
}

void wxSound::Stop()
{
    if (lastSoundIsPlaying)
    {
        delete (wxTimer*&) lastSoundTimer;
        lastSoundIsPlaying = false;
        lastSoundTimer = NULL;
    }
}

void* wxSound::GetHandle()
{
#if USE_QUICKTIME
    if(m_type == wxSound_RESOURCE)
        return (void*)  ((wxSMTimer*)m_pTimer)->GetChannel();

    return (void*) ((wxQTTimer*) m_pTimer)->GetMovie();
#endif
    return NULL;
}

#endif //wxUSE_SOUND
