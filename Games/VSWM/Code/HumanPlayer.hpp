#ifndef CAFU_HUMANPLAYER_HPP_INCLUDED
#define CAFU_HUMANPLAYER_HPP_INCLUDED

#include <stdio.h>

#include "BaseEntity.hpp"
#include "../../PlayerCommand.hpp"
#include "Libs/IntMatrix.hpp"


class EntInfo2DMoveDirT;


class EntHumanPlayerT : public BaseEntityT
{
    private:

    struct LabyrinthNodeT
    {
        // The coordinate of the center of the node.
        VectorT Coordinate;

        // The coordinate to stop the movement at when arriving from the previous node (needed for a better perspective view).
        VectorT StopCoordinate;

        // Have we been at this node earlier in the sequence?
        bool AlreadyVisited;

        // Camera orientation at the previous node, before leaving (and rotating) there towards this node.
        // These angles are not necessarily the same as the This... angles from the previous node,
        // but they describe the same viewer orientation, such that it becomes trivial to interpolate
        // them to *our* This... angles, which are our actually desired angles for this node.
        // The type of the angle variables is "unsigned long", because we also need to store the 360 degree equivalent (2^16, instead of 0) sometimes.
        unsigned long PrevHeading;
        unsigned long PrevPitch;
        unsigned long PrevBank;

        // Camera orientation when arriving at this node, and while waiting for the players Y/N descision.
        // We interpolate to these angles after the player has made his Y/N descision at the *previous* node.
        unsigned long ThisHeading;
        unsigned long ThisPitch;
        unsigned long ThisBank;


        // Default ctor. Only needed because LabyrinthNodeTs are stored in an ArrayT.
        LabyrinthNodeT() { }

        // Constructor that initializes all our components.
        LabyrinthNodeT(const VectorT& c, const VectorT& sc, bool avis, unsigned long ph, unsigned long pp, unsigned long pb, unsigned long th, unsigned long tp, unsigned long tb)
            : Coordinate(c), StopCoordinate(sc), AlreadyVisited(avis), PrevHeading(ph), PrevPitch(pp), PrevBank(pb), ThisHeading(th), ThisPitch(tp), ThisBank(tb) { }
    };

    enum MoveDirectionT { MoveDir_Up, MoveDir_Down, MoveDir_Left, MoveDir_Right, MoveDir_Forward, MoveDir_Backward, MoveDir_None };


    EntityStateT State;         // The current state of this entity.
    char  FileLocationIndex;    // Which set of locations for the files "VSWM.cfg", "Trial.dat" and "Eval.dat" should be used (see 'CfgFileNames', 'TrlFileNames' and 'EvlFileNames' in HumanPlayer.cpp).
    char  IgnorePCs;            // Ignore the first "IgnorePCs" PlayerCommands in ProcessConfigString
    float TotalIdleTime;        // Time that is spent at each node before the rotation towards the next node starts
    float Mode3WaitTime;        // How much time of the TotalIdleTime do we spend in QueryMode 3 waiting for input? (Implicit new state!)
    float TotalTurnTime;        // How long the rotation towards the next node takes
    float TotalPathTime;        // How long it takes to travel the path to the halt of the next node
    char  QueryMode;            // See separate info in HumanPlayer.cpp and Run_USAF_VSWM.bat
    bool  DebugModeOn;          // Should we emit debug info?
    bool  NoCountDownButW4Key;  // Does not display the "3.. 2.. 1.." count-down, but rather a "Ready!" message and waits for a keypress.
    bool  NoReverseMove;        // When true, reverse moves in self-paced mode are not permitted (the reverse move key is then disabled).
    bool  ConstantHeading;      // When true, the heading (and pitch and bank) are fixed at 0 values throughout the sequence.
    bool  QM3_TurnOffTimeOut;   // Turns off the time-out in QM3 when true.
    FILE* LogFile;              // File handle for the log file

    ArrayT<LabyrinthNodeT> NodeSequence;    // The sequence of labyrinth nodes to visit
    ArrayT<PlayerCommandT> PlayerCommands;  // The sequence of human player commands

    // Override the base class methods.
    void DoSerialize(cf::Network::OutStreamT& Stream) const;
    void DoDeserialize(cf::Network::InStreamT& Stream);

    void WriteLogFileEntry(char NodeNr, double ReactionTime, char Answer);

    void ComputeAngles(const IntMatrixT& OldMatrix, const IntMatrixT& NewMatrix,
                       unsigned long& OldHeading, unsigned long& OldPitch, unsigned long& OldBank,
                       unsigned long& NewHeading, unsigned long& NewPitch, unsigned long& NewBank);
    void Init2DNodeSequence(const char* TrialFileName, const VectorT& OriginIPS, const double NodeSpacing, const ArrayT< IntrusivePtrT<EntInfo2DMoveDirT> >& AllInfo2DMoveDirEntities);
    void Init3DNodeSequence(const char* TrialFileName, const VectorT& OriginIPS, const double NodeSpacing);


    public:

    EntHumanPlayerT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);
   ~EntHumanPlayerT();

    void ProcessConfigString(const void* ConfigData, const char* ConfigString);
    void Think(float FrameTime, unsigned long ServerFrameNr);
    void PostDraw(float FrameTime, bool FirstPersonView);
};

#endif
