// TODO: "State.Vel.x+=frametime"  GLOBALLY? (state-independent, that is)?
// TODO: Entirely remove None-Wrapped Mode (done), use of State.Vel.y, state StateOfExistance_FrozenSpectator (done) ?

#include <string.h>

#include "HumanPlayer.hpp"
#include "EntityCreateParams.hpp"
#include "InfoPlayerStart.hpp"
#include "InfoNodeSpacing.hpp"
#include "Info2DMoveDir.hpp"
#include "Constants_EntityTypeIDs.hpp"
#include "Libs/LookupTables.hpp"
#include "../../GameWorld.hpp"
#include "ConsoleCommands/Console.hpp"
#include "Fonts/Font.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/State.hpp"
#include "TextParser/TextParser.hpp"

#ifndef _WIN32
#define _stricmp strcasecmp
#endif


// These are the places for the corresponding files.
// Keep them in sync!
// That is, if 'CfgFileNames[0]' is found, 'TrlFileNames[0]' and 'EvlFileNames[0]' will be used (Bens preference).
// Otherwise, 'CfgFileNames[1]', 'TrlFileNames[1]' and 'EvlFileNames[1]' are used (Carstens preference).
const char* CfgFileNames[2] = { "VSWM.cfg" , "Games/VSWM/Code/VSWM.cfg" };
const char* TrlFileNames[2] = { "Trial.dat", "Games/VSWM/Trial.dat"     };
const char* EvlFileNames[2] = { "Eval.dat" , "Games/VSWM/Eval.dat"      };

const char StateOfExistance_W4EndOfIdleTime_W4Input    =0;
const char StateOfExistance_W4EndOfIdleTime_NoInput    =1;
const char StateOfExistance_InFlightToNextNode_W4Input =2;
const char StateOfExistance_InFlightToNextNode_NoInput =3;
const char StateOfExistance_TurnTowardsNextNode_W4Input=4;
const char StateOfExistance_TurnTowardsNextNode_NoInput=5;
const char StateOfExistance_ReverseMove_FlyBack        =6;
const char StateOfExistance_ReverseMove_TurnBack       =7;


void EntHumanPlayerT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    Stream << float(State.Velocity.x);
    Stream << float(State.Velocity.y);
    Stream << float(State.Velocity.z);
    Stream << State.StateOfExistance;
    Stream << State.Flags;
    Stream << State.PlayerName;       // TODO: In the old code, the PlayerName apparently is read/written in *baseline* messages only.
    Stream << State.ModelIndex;
    Stream << State.ModelSequNr;
    Stream << State.ModelFrameNr;
    Stream << State.Health;
    Stream << State.Armor;
    Stream << uint32_t(State.HaveItems);
    Stream << uint32_t(State.HaveWeapons);
    Stream << State.ActiveWeaponSlot;
    Stream << State.ActiveWeaponSequNr;
    Stream << State.ActiveWeaponFrameNr;

    for (unsigned int Nr=0; Nr<16; Nr++) Stream << State.HaveAmmo[Nr];
    for (unsigned int Nr=0; Nr<32; Nr++) Stream << uint32_t(State.HaveAmmoInWeapons[Nr]);
}


void EntHumanPlayerT::DoDeserialize(cf::Network::InStreamT& Stream)
{
    float    f =0.0f;
    uint32_t ui=0;

    Stream >> f; State.Velocity.x=f;
    Stream >> f; State.Velocity.y=f;
    Stream >> f; State.Velocity.z=f;
    Stream >> State.StateOfExistance;
    Stream >> State.Flags;
    Stream >> State.PlayerName;     // TODO: In the old code, the PlayerName apparently is read/written in *baseline* messages only.
    Stream >> State.ModelIndex;
    Stream >> State.ModelSequNr;
    Stream >> State.ModelFrameNr;
    Stream >> State.Health;
    Stream >> State.Armor;
    Stream >> ui; State.HaveItems=ui;
    Stream >> ui; State.HaveWeapons=ui;
    Stream >> State.ActiveWeaponSlot;
    Stream >> State.ActiveWeaponSequNr;
    Stream >> State.ActiveWeaponFrameNr;

    for (unsigned int Nr=0; Nr<16; Nr++) Stream >> State.HaveAmmo[Nr];
    for (unsigned int Nr=0; Nr<32; Nr++) { Stream >> ui; State.HaveAmmoInWeapons[Nr]=ui; }
}


// Writes a log file entry.
// Answer must be in {'Y', 'N', 'M'}.
void EntHumanPlayerT::WriteLogFileEntry(char NodeNr, double ReactionTime, char Answer)
{
    if (LogFile==NULL) return;

    fprintf(LogFile, "%02u", NodeNr);
    fprintf(LogFile, " %c", Answer);
    fprintf(LogFile, " %10.1f", ReactionTime*1000.0);
 // fprintf(LogFile, " %c\n", CorrectAnswer ? '+' : '-');
    fprintf(LogFile, "\n");
}


// This structure consists of a triple of angles.
// It could be made local to the EntHumanPlayerT::ComputeAngles() member function,
// but the MS VC++ 6.0 compiler doesn't like that...
// (TripleT has then no linkage, and thus(?) cannot be used as a template argument.)
struct TripleT
{
    unsigned long Heading;
    unsigned long Pitch;
    unsigned long Bank;

    TripleT(unsigned long Heading_=0, unsigned long Pitch_=0, unsigned long Bank_=0) : Heading(Heading_), Pitch(Pitch_), Bank(Bank_) {}
};


void EntHumanPlayerT::ComputeAngles(const IntMatrixT& OldMatrix, const IntMatrixT& NewMatrix,
    unsigned long& PrevHeading, unsigned long& PrevPitch, unsigned long& PrevBank, unsigned long& ThisHeading, unsigned long& ThisPitch, unsigned long& ThisBank)
{
    ArrayT<TripleT> OldAngleTriples;    // List of all angle triples that describe the same orientation as the "OldMarix".
    ArrayT<TripleT> NewAngleTriples;    // List of all angle triples that describe the same orientation as the "NewMatrix".

    IntMatrixT OldRotMatrix=OldMatrix; OldRotMatrix.m[0][3]=0; OldRotMatrix.m[1][3]=0; OldRotMatrix.m[2][3]=0;
    IntMatrixT NewRotMatrix=NewMatrix; NewRotMatrix.m[0][3]=0; NewRotMatrix.m[1][3]=0; NewRotMatrix.m[2][3]=0;

    // Find all angle triples that result in a matrix equivalent to OldRotMatrix
    unsigned long BankQu;

    for (BankQu=0; BankQu<=4; BankQu++)
        for (unsigned long PitchQu=0; PitchQu<=4; PitchQu++)
            for (unsigned long HeadingQu=0; HeadingQu<=4; HeadingQu++)
            {
                IntMatrixT TestMatrix;

                TestMatrix=TestMatrix*IntMatrixT::GetRotationZMatrix(HeadingQu*90);
                TestMatrix=TestMatrix*IntMatrixT::GetRotationXMatrix(PitchQu  *90);
                TestMatrix=TestMatrix*IntMatrixT::GetRotationYMatrix(BankQu   *90);

                if (TestMatrix==OldRotMatrix) OldAngleTriples.PushBack(TripleT(65536-HeadingQu*16384, 65536-PitchQu*16384, BankQu*16384));
            }

    // Find all angle triples that result in a matrix equivalent to NewRotMatrix
    for (BankQu=0; BankQu<=4; BankQu++)
        for (unsigned long PitchQu=0; PitchQu<=4; PitchQu++)
            for (unsigned long HeadingQu=0; HeadingQu<=4; HeadingQu++)
            {
                IntMatrixT TestMatrix;

                TestMatrix=TestMatrix*IntMatrixT::GetRotationZMatrix(HeadingQu*90);
                TestMatrix=TestMatrix*IntMatrixT::GetRotationXMatrix(PitchQu  *90);
                TestMatrix=TestMatrix*IntMatrixT::GetRotationYMatrix(BankQu   *90);

                if (TestMatrix==NewRotMatrix) NewAngleTriples.PushBack(TripleT(65536-HeadingQu*16384, 65536-PitchQu*16384, BankQu*16384));
            }

    // Find a pair of matching angle triples.
    // In general, exactly one of six relative movement directions is possible at each node (up, down, left, right, forward, back).
    // For forward moves (at the start node or for explicit forward moves) it is clear that a pair of angle triples exists that are (component-wise) equal.
    // For regular moves (up, down, left, right) we can find a pair of angle triples that differ about 90 degrees in one component.
    // For back moves, there must be a pair that differs about 180 degrees in one component.
    // The good thing is that these cases are mutually exclusive (at least I think so).
    for (unsigned long OATNr=0; OATNr<OldAngleTriples.Size(); OATNr++)
        for (unsigned long NATNr=0; NATNr<NewAngleTriples.Size(); NATNr++)
        {
            char          DiffCount =0;
            unsigned long Difference=0;

            PrevHeading=OldAngleTriples[OATNr].Heading;
            PrevPitch  =OldAngleTriples[OATNr].Pitch;
            PrevBank   =OldAngleTriples[OATNr].Bank;
            ThisHeading=NewAngleTriples[NATNr].Heading;
            ThisPitch  =NewAngleTriples[NATNr].Pitch;
            ThisBank   =NewAngleTriples[NATNr].Bank;

            if (PrevHeading!=ThisHeading) { DiffCount++; Difference=(PrevHeading>ThisHeading) ? PrevHeading-ThisHeading : ThisHeading-PrevHeading; };
            if (PrevPitch  !=ThisPitch  ) { DiffCount++; Difference=(PrevPitch  >ThisPitch  ) ? PrevPitch  -ThisPitch   : ThisPitch  -PrevPitch;   };
            if (PrevBank   !=ThisBank   ) { DiffCount++; Difference=(PrevBank   >ThisBank   ) ? PrevBank   -ThisBank    : ThisBank   -PrevBank;    };

            // Relative forward move.
            // We get a DiffCount of 0 for the start node and explicit forward moves.
            if (DiffCount==0) return;

            // Regular relative move. The difference MUST be a 90 degree difference!
            // 1st reason: Prefer 90 degree turns over 270 degree turns, which are treated equivalent otherwise.
            // 2nd reason: (?? Does this force a correct solution for certain cases where bank angles are involved ??)
            if (DiffCount==1 && Difference==16384) return;

            // Relative back move (180 degrees).
            if (DiffCount==1 && Difference==32768) return;
        }

    Console->DevWarning(cf::va("Should never get here (ComputeAngles)! %u %u\n", OldAngleTriples.Size(), NewAngleTriples.Size()));
}


void EntHumanPlayerT::Init2DNodeSequence(const char* TrialFileName, const VectorT& OriginIPS, const double NodeSpacing, const ArrayT< IntrusivePtrT<EntInfo2DMoveDirT> >& AllInfo2DMoveDirEntities)
{
    // In 2D trial files, the '#' is a delimiter and "//" initiates a comment.
    TextParserT TrialTP(TrialFileName, "#");

    ArrayT< IntrusivePtrT<EntInfo2DMoveDirT> > Sequence;

    // This code constructs a sequence from a list of absolute directions. The sequence is stored as a list of info_2d_move_direction entities.
    // Note that each info_2d_move_direction entity represents a direction (namely the unique vector from the info_player_start entity origin
    // to its own origin) and also stores a name for that direction.
    // The absolute *position* of a node is thus obtained by summing all of its preceeding directions.
    while (!TrialTP.IsAtEOF())
    {
        TrialTP.GetNextToken();     // skip the x
        TrialTP.GetNextToken();     // skip the y
        TrialTP.GetNextToken();     // skip the 'z' character
        TrialTP.GetNextToken();     // skip the #
        TrialTP.GetNextToken();     // skip the 0 (have we been here before?)
        std::string AbsoluteDirName=TrialTP.GetNextToken();
        printf("Sequence dir found in %s: %s\n", TrialFileName, AbsoluteDirName.c_str());
        TrialTP.GetNextToken();     // skip the relative direction

        // Find the info_2d_move_direction entity whose name is AbsoluteDirName and add it to the sequence.
        // IMPORTANT NOTE: Direction names in the trial file (AbsoluteDirName) that cannot be found among
        // the info_2d_move_direction entities are silently ignored. This is important in the case of errors,
        // and even more important for the very first line in the trial file, which, given the current file
        // specs, normally contains something like "Start" as the first (dummy) direction.
        for (unsigned long EntityNr=0; EntityNr<AllInfo2DMoveDirEntities.Size(); EntityNr++)
            if (std::string(AllInfo2DMoveDirEntities[EntityNr]->MoveDirName)==AbsoluteDirName)
            {
                Sequence.PushBack(AllInfo2DMoveDirEntities[EntityNr]);
                printf("      Matching entity found.\n");
                break;
            }
    }


    VectorT       PrevNodeCenter =OriginIPS+VectorT(0.0, -NodeSpacing, 0.0);
    unsigned long PrevNodeHeading=0;

    VectorT       ThisNodeCenter =OriginIPS;
    unsigned long ThisNodeHeading=0;

    // Construct the node sequence. Note that n moves yield n+1 nodes!
    for (unsigned long NodeNr=0; NodeNr<=Sequence.Size(); NodeNr++)
    {
        // Store the parameters of the current node in the sequence array.
        const double  StopFactor  =0.85;
        const VectorT ThisNodeStop=scale(ThisNodeCenter, StopFactor)+scale(PrevNodeCenter, 1.0-StopFactor);

        // Search the previous nodes in the sequence to learn if we have already been here.
        unsigned long NNr;
        for (NNr=0; NNr<NodeNr; NNr++)
            if (length(ThisNodeCenter-NodeSequence[NNr].Coordinate)<0.2*NodeSpacing)
                break;

        // Make sure that we always interpolate across the shortest arc,
        // i.e. not from 350° to 10° (which is 340° to the left), but rather from 350° to 370° (which is 20° to the right).
        // The idea is simple: Compare the length of the given arc to the length of the arc when the smaller value is
        // augmented by 360° to the "equivalent orientation". Note that we only ever have to consider the smaller angle
        // for augmenting, because augmenting the larger one just makes the difference always larger.
        if (PrevNodeHeading>ThisNodeHeading)
        {
            if (PrevNodeHeading-ThisNodeHeading > (unsigned long)abs(int(PrevNodeHeading)-int(ThisNodeHeading+65536))) ThisNodeHeading+=65536;
        }
        else
        {
            if (ThisNodeHeading-PrevNodeHeading > (unsigned long)abs(int(ThisNodeHeading)-int(PrevNodeHeading+65536))) PrevNodeHeading+=65536;
        }

        if (ConstantHeading)
        {
            // In constant heading mode, we just travel from true center to true center of each node,
            // and (mis-)use the "Labyrinth Size Trick" for stopping early (keeping a spatial offset) for better perspective view.
            // (A nice side effect is that this strategy also works with 2D/60° labyrinths.)
            NodeSequence.PushBack(LabyrinthNodeT(ThisNodeCenter, ThisNodeCenter, NNr<NodeNr /*visited this node before?*/, 0, 0, 0, 0, 0, 0));
        }
        else
        {
            // "Normal" (non-constant-heading) mode.
            NodeSequence.PushBack(LabyrinthNodeT(ThisNodeCenter, ThisNodeStop, NNr<NodeNr /*visited this node before?*/, PrevNodeHeading, 0, 0, ThisNodeHeading, 0, 0));
        }


        if (NodeNr<Sequence.Size())     // Don't do this in the last iteration, where NodeNr==Sequence.Size().
        {
            PrevNodeCenter =ThisNodeCenter;
            PrevNodeHeading=ThisNodeHeading & 0xFFFF;   // % 65536, modulo 360°

            ThisNodeCenter =ThisNodeCenter+(Sequence[NodeNr]->GetOrigin()-OriginIPS);
            ThisNodeHeading=Sequence[NodeNr]->GetHeading();
        }
    }
}


void EntHumanPlayerT::Init3DNodeSequence(const char* TrialFileName, const VectorT& OriginIPS, const double NodeSpacing)
{
    // In 3D trial files, there are no delimiters except white-space and '#' initiates a comment.
    TextParserT TrialTP(TrialFileName, "", true, '#');

    ArrayT<char> SequenceFromDisk;

 /* // This code constructs a sequence from a list of absolute directions.
    // Please note: The sequence ALWAYS starts in the center node at (0,0,0).
    while (!TP.IsAtEOF())
    {
        std::string Token=TP.GetNextToken();

             if (Token=="Right"  ) SequenceFromDisk.PushBack(1);
        else if (Token=="Left"   ) SequenceFromDisk.PushBack(2);
        else if (Token=="Forward") SequenceFromDisk.PushBack(3);
        else if (Token=="Back"   ) SequenceFromDisk.PushBack(4);
        else if (Token=="Up"     ) SequenceFromDisk.PushBack(5);
        else if (Token=="Down"   ) SequenceFromDisk.PushBack(6);
    } */

    // This code constructs a sequence from a list of absolute coordinates.
    // It does so by first constructing a list of absolute directions.
    // As above, the sequence ALWAYS starts in the center node at (0,0,0).
    // NOTE: Cafu uses a right handed coordinate system, whereas the coords in Trial.dat
    //       are given in a left handed coordinate system. Thus the read order X, Z, Y!
    int OldX=atoi(TrialTP.GetNextToken().c_str());
    int OldZ=atoi(TrialTP.GetNextToken().c_str());
    int OldY=atoi(TrialTP.GetNextToken().c_str());

    while (!TrialTP.IsAtEOF())
    {
        int NewX=atoi(TrialTP.GetNextToken().c_str());
        int NewZ=atoi(TrialTP.GetNextToken().c_str());
        int NewY=atoi(TrialTP.GetNextToken().c_str());

        int DiffX=NewX-OldX;
        int DiffY=NewY-OldY;
        int DiffZ=NewZ-OldZ;

        while (DiffX>0) { SequenceFromDisk.PushBack(1); DiffX--; }  // Right
        while (DiffX<0) { SequenceFromDisk.PushBack(2); DiffX++; }  // Left
        while (DiffY>0) { SequenceFromDisk.PushBack(3); DiffY--; }  // Forward
        while (DiffY<0) { SequenceFromDisk.PushBack(4); DiffY++; }  // Backward
        while (DiffZ>0) { SequenceFromDisk.PushBack(5); DiffZ--; }  // Up
        while (DiffZ<0) { SequenceFromDisk.PushBack(6); DiffZ++; }  // Down

        OldX=NewX;
        OldY=NewY;
        OldZ=NewZ;
    }


    if (SequenceFromDisk.Size()>0)
    {
        // Append another arbitrary move to the sequence, because otherwise, the last move from the disk is omitted.
        // (Below we generate a "node" sequence, not a "move" sequence, where n moves generate n nodes, including the start node.
        //  However, we want n moves to generate n+1 nodes (including the start node), thus add a last move that is never executed.)
        SequenceFromDisk.PushBack(1);
    }


    // Key idea: If not read from disk, we pre-compute the ("random") sequence of nodes that are
    // visited during the test. The sequence is then stored as an array of LabyrinthNodeTs.
    // During the test, we use the State.Flags variable as an index into the sequence array.
    // In order to move to the next node, all we need to do is incrementing the 'State.Flags' variable.
    // + Really elegant solution of the problems induced by the client prediction algorithm.
    // + When the sequence is read from disk, very flexible.
    // + Reduces test-time computations ("Think()ing") to a minimum.
    // - When more than 256 nodes are visited, the sequence repeats, with a possibly ugly wrap around.

    // For our movements, we consider a 3x3x3 grid. The corners or intersections of the grid correspond to the nodes.
    // Thus, each of the 27 possible nodes has coordinates (x, y, z), with x, y and z in { -1, 0, 1}.
    // We usually calculate with the real numbers, but where array indices are involved, we usually
    // convert the {-1, 0, 1} to {0, 1, 2}.


    // Our initial position and orientation, described by the "NewNodeMatrix" is at node (0, 0, 0),
    // with all angles at zero. Because we need a stop coordinate even for this very first node,
    // we also need a matrix for the "previous" node (which does not exist).
    // The matrix of the previous node, "OldNodeMatrix", gets initialized as if we arrived from the node at (0, -1, 0).
    IntMatrixT OldNodeMatrix;
    IntMatrixT NewNodeMatrix;

    OldNodeMatrix.m[1][3]=-1;

    // Construct the node sequence.
    for (unsigned long NodeNr=0; NodeNr<SequenceFromDisk.Size(); NodeNr++)
    {
        // Store the parameters of the current node in the sequence array.
        if (ConstantHeading)
        {
            const VectorT NewNodeCenter=OriginIPS+VectorT(NewNodeMatrix.m[0][3], NewNodeMatrix.m[1][3], NewNodeMatrix.m[2][3])*NodeSpacing;

            // Search the previous nodes in the sequence to learn if we have already been here.
            unsigned long NNr;
            for (NNr=0; NNr<NodeNr; NNr++)
                if (length(NewNodeCenter-NodeSequence[NNr].Coordinate)<0.2*NodeSpacing)
                    break;

            // In constant heading mode, we just travel from true center to true center of each node,
            // and (mis-)use the "Labyrinth Size Trick" for stopping early (keeping a spatial offset) for better perspective view.
            // (A nice side effect is that this strategy also works with 2D/60° labyrinths.)
            NodeSequence.PushBack(LabyrinthNodeT(NewNodeCenter, NewNodeCenter, NNr<NodeNr /*visited this node before?*/, 0, 0, 0, 0, 0, 0));
        }
        else
        {
            unsigned long PrevHeading=0;    // Camera orientation at the previous node, before leaving (and rotating) there towards this node.
            unsigned long PrevPitch  =0;
            unsigned long PrevBank   =0;
            unsigned long ThisHeading=0;    // Camera orientation when arriving at this node, and while waiting for the players Y/N descision.
            unsigned long ThisPitch  =0;
            unsigned long ThisBank   =0;

            ComputeAngles(OldNodeMatrix, NewNodeMatrix, PrevHeading, PrevPitch, PrevBank, ThisHeading, ThisPitch, ThisBank);


            const VectorT OldNodeCenter=OriginIPS+VectorT(double(OldNodeMatrix.m[0][3])*NodeSpacing, double(OldNodeMatrix.m[1][3])*NodeSpacing, double(OldNodeMatrix.m[2][3])*NodeSpacing);
            const VectorT NewNodeCenter=OriginIPS+VectorT(double(NewNodeMatrix.m[0][3])*NodeSpacing, double(NewNodeMatrix.m[1][3])*NodeSpacing, double(NewNodeMatrix.m[2][3])*NodeSpacing);
            const double  StopFactor   =0.85;
            const VectorT NewNodeStop  =scale(NewNodeCenter, StopFactor)+scale(OldNodeCenter, 1.0-StopFactor);

            // Search the previous nodes in the sequence to learn if we have already been here.
            unsigned long NNr;
            for (NNr=0; NNr<NodeNr; NNr++)
                if (length(NewNodeCenter-NodeSequence[NNr].Coordinate)<0.2*NodeSpacing)
                    break;

            NodeSequence.PushBack(LabyrinthNodeT(NewNodeCenter, NewNodeStop, NNr<NodeNr /*visited this node before?*/, PrevHeading, PrevPitch, PrevBank, ThisHeading, ThisPitch, ThisBank));
        }


        // Move to next (legal) node.
        IntMatrixT TestNodeMatrix;

        // Create the sequence as read from the Trial.dat file (already stored in the SequenceFromDisk array).
        for (char TryCount=0; TryCount<6; TryCount++)
        {
            TestNodeMatrix=NewNodeMatrix;

            // Choose new move direction (in the LOCAL viewer coordinate system)
            switch (TryCount)
            {
                case 0: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationZMatrix( 90); break;
                case 1: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationZMatrix(-90); break;
                case 2: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationXMatrix( 90); break;
                case 3: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationXMatrix(-90); break;
                case 4: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationXMatrix(  0); break;
                case 5: TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetRotationXMatrix(180); break;
            }

            // Move forward one step (in the LOCAL viewer coordinate system)
            TestNodeMatrix=TestNodeMatrix*IntMatrixT::GetTranslationMatrix(0, 1, 0);

            // Test if that movement is actually what was wanted
            const int AbsMoveDirX=TestNodeMatrix.m[0][3]-NewNodeMatrix.m[0][3];
            const int AbsMoveDirY=TestNodeMatrix.m[1][3]-NewNodeMatrix.m[1][3];
            const int AbsMoveDirZ=TestNodeMatrix.m[2][3]-NewNodeMatrix.m[2][3];
            char      AbsMoveDir =0;

                 if (AbsMoveDirX== 1) AbsMoveDir=1;     // Right
            else if (AbsMoveDirX==-1) AbsMoveDir=2;     // Left
            else if (AbsMoveDirY== 1) AbsMoveDir=3;     // Forward
            else if (AbsMoveDirY==-1) AbsMoveDir=4;     // Backward
            else if (AbsMoveDirZ== 1) AbsMoveDir=5;     // Up
            else if (AbsMoveDirZ==-1) AbsMoveDir=6;     // Down

            if (AbsMoveDir==SequenceFromDisk[NodeNr]) break;
        }

        OldNodeMatrix= NewNodeMatrix;
        NewNodeMatrix=TestNodeMatrix;
    }
}


EntHumanPlayerT::EntHumanPlayerT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin)
    : BaseEntityT(EntityCreateParamsT(ID, std::map<std::string, std::string>(), NULL, NULL, MapFileID, GameWorld, Origin),
                  BoundingBox3dT(Vector3dT( 300.0,  300.0,   100.0),
                                 Vector3dT(-300.0, -300.0, -1728.8)),   // 68*25.4 == 1727.2
                  0),
      State(VectorT(),
            StateOfExistance_W4EndOfIdleTime_NoInput,
            0,       // Flags
            0,       // ModelIndex
            0,       // ModelSequNr
            0.0,     // ModelFrameNr
            100,     // Health
            0,       // Armor
            0,       // HaveItems
            0,       // HaveWeapons
            0,       // ActiveWeaponSlot
            0,       // ActiveWeaponSequNr
            0.0)     // ActiveWeaponFrameNr
{
    // Read the initialisation data from the configuration file.
    // Therefore search all the CfgFileNames.
    for (FileLocationIndex=0; FileLocationIndex<2; FileLocationIndex++)
    {
        FILE* ConfigFile=fopen(CfgFileNames[FileLocationIndex], "r");
        if (ConfigFile) { fclose(ConfigFile); break; }
    }

    if (FileLocationIndex>=2) FileLocationIndex=0;


    IgnorePCs          =3;
    TotalIdleTime      =3.0;    // 0.5
    Mode3WaitTime      =0.5;    // 0.1
    TotalTurnTime      =1.0;    // 0.6
    TotalPathTime      =2.5;    // 3.0
    QueryMode          =0;      // 2
    DebugModeOn        =false;
    NoCountDownButW4Key=false;
    NoReverseMove      =false;
    ConstantHeading    =false;
    QM3_TurnOffTimeOut =false;
    LogFile            =NULL;


    try
    {
        TextParserT TP(CfgFileNames[FileLocationIndex], "", true, '#');

    	while (!TP.IsAtEOF())
        {
            std::string Token=TP.GetNextToken();

            if (Token=="TotalIdleTime")
            {
                TotalIdleTime=float(atof(TP.GetNextToken().c_str()));

                if (TotalIdleTime< 0.0) TotalIdleTime= 0.0;
                if (TotalIdleTime>60.0) TotalIdleTime=60.0;
            }
            else if (Token=="Mode3WaitTime")
            {
                Mode3WaitTime=float(atof(TP.GetNextToken().c_str()));

                // It is not sensible to clamp to TotalIdleTime, because the order of the statements in the file config is not fixed!
                if (Mode3WaitTime< 0.0) Mode3WaitTime= 0.0;
                if (Mode3WaitTime>60.0) Mode3WaitTime=60.0;
            }
            else if (Token=="TotalTurnTime")
            {
                TotalTurnTime=float(atof(TP.GetNextToken().c_str()));

                if (TotalTurnTime< 0.0) TotalTurnTime= 0.0;
                if (TotalTurnTime>60.0) TotalTurnTime=60.0;
            }
            else if (Token=="TotalPathTime")
            {
                TotalPathTime=float(atof(TP.GetNextToken().c_str()));

                if (TotalPathTime< 0.0) TotalPathTime= 0.0;
                if (TotalPathTime>60.0) TotalPathTime=60.0;
            }
            else if (Token=="QueryMode")
            {
                QueryMode=(char)atol(TP.GetNextToken().c_str());

                // Input values are in {1, 2, 3, 4}, convert to {0, 1, 2, 3}.
                QueryMode--;

                if (QueryMode>3) QueryMode=0;
            }
            else if (Token=="DebugModeON")
            {
                DebugModeOn=true;
            }
            else if (Token=="NoCountDownButW4Key")
            {
                NoCountDownButW4Key=true;
            }
            else if (Token=="NoReverseMove")
            {
                NoReverseMove=true;
            }
            else if (Token=="ConstantHeading")
            {
                ConstantHeading=true;
                TotalTurnTime=0.0;
            }
            else if (Token=="QM3_TurnOffTimeOut")
            {
                QM3_TurnOffTimeOut=true;
            }
        }
    }
    catch (const TextParserT::ParseError&)
    {
        printf("WARNING: TextParserT::ParseError exception caught while parsing \"%s\".\n", CfgFileNames[FileLocationIndex]);
    }




    // We don't want to tie this code to a specific CW world file.
    // Especially, the scaling of the labyrinth may differ across world files.
    // Thus, we first find the InfoPlayerStart and the InfoNodeSpacing entities.
    // The spatial distance between them yields the spacing between the individual nodes.
    const ArrayT<unsigned long>& AllEntityIDs=GameWorld->GetAllEntityIDs();
    VectorT                      Origin_IPS;
    VectorT                      Origin_INS;
    ArrayT< IntrusivePtrT<EntInfo2DMoveDirT> >   AllInfo2DMoveDirEntities;

    for (unsigned long EntityIDNr=0; EntityIDNr<AllEntityIDs.Size(); EntityIDNr++)
    {
        IntrusivePtrT<BaseEntityT> BaseEntity=GameWorld->GetBaseEntityByID(AllEntityIDs[EntityIDNr]);
        if (BaseEntity==NULL) continue;

        if (BaseEntity->GetType()==&EntInfoPlayerStartT::TypeInfo) { Origin_IPS=BaseEntity->GetOrigin(); continue; }
        if (BaseEntity->GetType()==&EntInfoNodeSpacingT::TypeInfo) { Origin_INS=BaseEntity->GetOrigin(); continue; }
        if (BaseEntity->GetType()==&EntInfo2DMoveDirT  ::TypeInfo) { AllInfo2DMoveDirEntities.PushBack(static_pointer_cast<EntInfo2DMoveDirT>(BaseEntity)); continue; }

        Console->DevWarning("Should never get here (ctor HP)!");
        printf("%s (%u): WARNING: Should never get here!\n", __FILE__, __LINE__);
    }

    const double NodeSpacing=length(Origin_IPS-Origin_INS);
    // GameWorld->PrintDebug("NodeSpacing: %10.3f   ( this message should appear twice (cl & sv)! )", NodeSpacing);

    try
    {
        TextParserT TrialTP(TrlFileNames[FileLocationIndex], "", true, '#');

        TrialTP.GetNextToken();
        TrialTP.GetNextToken();
        std::string ThirdToken=TrialTP.GetNextToken();

        // Figure out whether it is the specification of a 2D or 3D node path.
        // Unfortunately, we cannot pass TrialTP to the Init?DNodeSequence() methods directly,
        // because the comment handling is different, depending on the 2D or 3D property
        // (and we had to put back the first three tokens anyway).
        if (ThirdToken=="z") Init2DNodeSequence(TrlFileNames[FileLocationIndex], Origin_IPS, NodeSpacing, AllInfo2DMoveDirEntities);
                        else Init3DNodeSequence(TrlFileNames[FileLocationIndex], Origin_IPS, NodeSpacing);
    }
    catch (const TextParserT::ParseError&)
    {
        printf("WARNING: TextParserT::ParseError exception caught while parsing \"%s\".\n", TrlFileNames[FileLocationIndex]);
    }



    // We finally have gathered enough information to set-up the state of this entity.
    State.Flags         =0;
    m_Origin            =NodeSequence[State.Flags].StopCoordinate;
    State.Velocity      =VectorT();
    m_Heading           =(unsigned short)NodeSequence[State.Flags].ThisHeading;
    m_Pitch             =(unsigned short)NodeSequence[State.Flags].ThisPitch;
    m_Bank              =(unsigned short)NodeSequence[State.Flags].ThisBank;
 // State_CountCorrect  =0;
 // State_CountIncorrect=0;
 // State_CountMissed   =0;

    State.StateOfExistance=StateOfExistance_W4EndOfIdleTime_NoInput;

    // Note that there is either a count-down, or we wait for a key press.
    // In the case of the key-press, in order to make 'Draw()' properly print the "Ready!" message,
    // set the timer 'State.Velocity.x' to 4.0 in order to prepend that the count-down is long over.
    if (NoCountDownButW4Key) State.Velocity.x=4.0;

    // LABYRINTH-SIZE TRICK, PART #0:
    // (See PART #1 and #2 for a description.)
    m_Origin=m_Origin-NodeSequence[State.Flags].Coordinate;
    if (ConstantHeading) m_Origin=m_Origin-VectorT(0.0, 0.15*4876.8, 0.0);
}


EntHumanPlayerT::~EntHumanPlayerT()
{
    if (LogFile!=NULL)
    {
        // fprintf(LogFile, "EOF\n");
        fclose(LogFile);
        LogFile=NULL;
    }
}


void EntHumanPlayerT::ProcessConfigString(const void* ConfigData, const char* ConfigString)
{
    // Take player commands (either on server side or from client prediction) and store them for later processing.
    if (strcmp(ConfigString, "PlayerCommand")==0)
    {
        // We ignore the first "IgnorePCs" PlayerCommands.
        // This is necessary in "Wrapped Mode", because in this mode, we do not pseudo-ignore initial
        // PlayerCommands by waiting for the FIRE-key to be pressed. Without this forced ignorance
        // of PlayerCommands, we would immediately start to process them, including their 'FrameTime's.
        // That is really bad, because the first (or second?) of them includes the time that the client took to load the new world!
        // This is usually longer than the first state takes, such that we see something of that state only when it is nearly over,
        // or nothing at all!
        if (IgnorePCs) IgnorePCs--;
                  else PlayerCommands.PushBack(*((PlayerCommandT*)ConfigData));
    }
}


void EntHumanPlayerT::Think(float FrameTime, unsigned long /*ServerFrameNr*/)
{
    // Client-Prediction imposed rules for the Think() function of HumanPlayer entites:
    // 1. The parameter FrameTime must not be used for computations.
    //    Use the frame-time inside the PlayerCommands instead, which are authorative.
    //    (Reason: For prediction, clients have no clue what value to pass in for FrameTime.)
    //    EXCEPTION: For calls to this function for the purpose of client prediction,
    //    a negative value is passed in, which may be used as a flag to controll debug output.
    // 2. The flow of control must always be predictable. Especially, functions for random number
    //    creation like rand(), must NEVER be used.
    //    SOLUTION: Pre-compute random numbers in the constructor from a fixed random seed!

    // Information about the state machine:
    // A diagram accompanies this source code, both in .dia file format, as well as .png.
    // The diagram shows the essential working of the state machine.
    // Note: We have "pairs" of states. A pair of states consists of two states,
    // One state is suffixed by _W4Input ("wait for input"), the other by _NoInput.
    // In _W4Input states, player input is possible, in _NoInput states not.
    // There is nothing special about pairs of states, except that timers for time-out
    // determination are shared (see State.Velocity.x below), and that the source codes
    // are quite similar, which is simultaneously a benefit and a danger!

    // INFORMATION ABOUT MIS-USED VARIABLES:
    // Due to the fact that this MOD differs from a first person shooter MOD
    // (the kind of MOD I had in mind when designing the interfaces to the Cafu engine),
    // some variables of the players state are unused, other are missing.
    // Thus, we mis-use the otherwise un-used variables for our purposes.
    // This can be confusing, and therefore here is an overview about this situation:
    // State.Flags      - the node counter, used as the index into the NodeSequence array.
    // State.Velocity.x - usually a timer. Set to 0 before a new state is entered,
    //                    then counts the time in seconds until it is determined that
    //                    the time-out value for the current pair of states has been reached.
    // State.Velocity.y - during a test, the sum of all the players reaction times.
    unsigned short& State_CountCorrect  =State.HaveAmmo[0];
    unsigned short& State_CountIncorrect=State.HaveAmmo[1];
    unsigned short& State_CountMissed   =State.HaveAmmo[2];

    // Some abbreviations for simplification.
    // Note that the START_KEYS must always be disjoint from the YES_KEYS,
    // because otherwise if the START_KEYS are held down a single frame too long,
    // they are taken as answer for the first move - which obviously yields the wrong answer.
    // Thus, to be safe, always choose all four sets of keys to be pairwise disjoint.
    const unsigned long YES_KEYS         =PCK_TurnRight;       // PCK_Jump         | PCK_TurnRight
    const unsigned long NO_KEYS          =PCK_TurnLeft;        // PCK_MoveBackward | PCK_TurnLeft
    const unsigned long START_KEYS       =PCK_Use;
    const unsigned long NEXT_MOVE_KEYS   =PCK_MoveForward;
    const unsigned long REVERSE_MOVE_KEYS=NoReverseMove ? 0 : PCK_MoveBackward;

    // Open the log file for writing, if in "Wrapper-Mode", not in Prediction-Mode, and not already done so.
    if (LogFile==NULL && FrameTime>=0.0) LogFile=fopen(EvlFileNames[FileLocationIndex], "w");

    // LABYRINTH-SIZE TRICK, PART #1:
    // The following line un-does the second part of the trick, which is at the end of this method.
    m_Origin=m_Origin+NodeSequence[State.Flags].Coordinate;
    if (ConstantHeading) m_Origin=m_Origin+VectorT(0.0, 0.15*4876.8, 0.0);

    for (unsigned long PCNr=0; PCNr<PlayerCommands.Size(); PCNr++)
    {
        switch (State.StateOfExistance)
        {
            case StateOfExistance_W4EndOfIdleTime_W4Input:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;


                const bool YesIsPressed=(PlayerCommands[PCNr].Keys & YES_KEYS)!=0;
                const bool  NoIsPressed=(PlayerCommands[PCNr].Keys & NO_KEYS )!=0;

                if ((YesIsPressed || NoIsPressed) && !(YesIsPressed && NoIsPressed))
                {
                    if (NodeSequence[State.Flags].AlreadyVisited==YesIsPressed)
                    {
                        // Count the CORRECT operator descision
                        State_CountCorrect++;
                    }
                    else
                    {
                        // Count the INCORRECT operator descision
                        State_CountIncorrect++;
                    }

                    // State.Velocity.x counts the seconds since this state was entered.
                    // Sum up the reaction times in State.Velocity.y.
                    double ResponseTime=0.0;

                    switch (QueryMode)
                    {
                        case 0: ResponseTime=                            State.Velocity.x; break;
                        case 1: ResponseTime=              TotalPathTime+State.Velocity.x; break;
                        case 2: ResponseTime=TotalTurnTime+TotalPathTime+State.Velocity.x; break;
                        case 3: ResponseTime=TotalTurnTime+TotalPathTime+State.Velocity.x; break;
                    }

                    State.Velocity.y+=ResponseTime;

                    if (FrameTime>=0.0)
                        WriteLogFileEntry(State.Flags, ResponseTime, YesIsPressed ? 'Y' : 'N');

                    if (DebugModeOn && FrameTime>=0.0 /* Don't issue a message in prediction mode! */)
                        Console->Print(cf::va("Response time: %4u msec", (unsigned long)(ResponseTime*1000.0+0.5)));


                    // Transit to the _NoInput pair state.

                    // Handle the special case of the QueryMode 2 with the QM3_TurnOffTimeOut variant.
                    // This line is necessary to get the frame properly colored (green becomes immediately red here).
                    // If you want to adjust the length of the period during which the frame is colored red, do NOT do
                    // State.Velocity.x=TotalIdleTime-DesiredTimeForRedFrame;
                    // but rather adjust the parameters for the TotalIdleTime and Mode3WaitTime in the VSWM.cfg file
                    // accordingly (such that TotalIdleTime-Mode3WaitTime==DesiredTimeForRedFrame).
                    if (QueryMode==2 && QM3_TurnOffTimeOut) State.Velocity.x=Mode3WaitTime;

                    State.StateOfExistance=StateOfExistance_W4EndOfIdleTime_NoInput;
                    break;
                }


                if (QueryMode<2)
                {
                    // In QueryModes 0 and 1, the individual moves are already clearly separated
                    // by the red colored frames, that appear at least once for each move,
                    // no matter how the operator behaves.
                    if (State.Velocity.x<TotalIdleTime) break;
                }
                else if (QueryMode==2)
                {
                    // In the variant "QM3_TurnOffTimeOut" of this mode, we want to turn off the time-out entirely,
                    // and thus the user can leave this state only by pressing YES or NO, handled above.
                    if (QM3_TurnOffTimeOut) break;

                    // Because in QueryMode 2 the moves are not guaranteed to be clearly separated
                    // (the frames stays always green if the operator does not press any key),
                    // we artificially introduce another *implicit* state.
                    // This state is entered if we got no response in this state for longer than Mode3WaitTime.
                    // It counts as "missed answer" and its frame is colored red.
                    if (State.Velocity.x<Mode3WaitTime) break;
                }
                else // QueryMode 3, self-paced
                {
                    // If this is the beginning of the sequence, don't care if the REVERSE MOVE key is pressed or not.
                    // Actually, if this is the beginning of the sequence, we should never have gotten here in the first place!
                    if (State.Flags==0) break;

                    // In self-paced query mode 3 we consider it as a time-out if, instead of YES or NO,
                    // one of the keys for NEXT or REVERSE move was pressed!
                    // As long as that does not happen, we break out here (meaning we stay in this state, doing nothing).
                    if ((PlayerCommands[PCNr].Keys & (NEXT_MOVE_KEYS | REVERSE_MOVE_KEYS))==0) break;
                }


                // GameWorld->PrintDebug("                 miss  %u", State.Flags);
                State_CountMissed++;    // Count the MISSED answers

                if (FrameTime>=0.0)
                    WriteLogFileEntry(State.Flags, 99999.9999, 'M');


                if (QueryMode==2)
                {
                    // Continue the special case treatment from above.
                    // Note that we never get here if QM3_TurnOffTimeOut==true.
                    State.StateOfExistance=StateOfExistance_W4EndOfIdleTime_NoInput;
                    break;
                }


                if (QueryMode==3)
                {
                    // IMPORTANT: We already recorded a "MISSED" entry (code above), in order to indicate that neither Y nor N was pressed.
                    // Now, we additionally record a "REVERSE" or "SKIP" entry!
                    WriteLogFileEntry(State.Flags,
                                      TotalTurnTime+TotalPathTime+State.Velocity.x,
                                      ((PlayerCommands[PCNr].Keys & REVERSE_MOVE_KEYS)!=0) ? 'R' : 'S');

                    // If any of the "reverse move" keys is pressed, initiate the reverse move sequence.
                    if ((PlayerCommands[PCNr].Keys & REVERSE_MOVE_KEYS)!=0 && State.Flags>0)
                    {
                        State.Flags--;
                        State.Velocity.x=0.0;
                        State.StateOfExistance=StateOfExistance_ReverseMove_FlyBack;
                        break;
                    }
                }


                // TRANSIT TO THE NEXT STATE
                if ((unsigned long)State.Flags+1<NodeSequence.Size())
                {
                    State.Flags++;
                    State.Velocity.x=0.0;

                    State.StateOfExistance=(QueryMode<2) ? StateOfExistance_TurnTowardsNextNode_NoInput
                                                         : StateOfExistance_TurnTowardsNextNode_W4Input;
                }
                else
                {
                    // End of sequence reached - this was the last node.
                    // We simply stay in this StateOfExistance and at this node until the quit messages comes through.

                    // In the ideal case, with the client & server running simultaneously,
                    // and packet delivery via the localhost is performed instanteously,
                    // the client performs no prediction at all.
                    // Thus, simply call 'SingleOpenGLWindow->PostQuitMessage()', without caring if the call
                    // is made from a prediction Think()ing, or a real server-side Think()ing.
                    // In the worst case (with prediction), the quit message is posted multiple times.
                    // if (SingleOpenGLWindow) SingleOpenGLWindow->PostQuitMessage();   // FIXME: Set ConVar "quit" to true instead!

                    if (FrameTime>=0.0 && LogFile!=NULL)
                    {
                     // fprintf(LogFile, "EOF1\n");
                        fclose(LogFile);
                        LogFile=NULL;
                    }
                }

                // GameWorld->PrintDebug("Shipping to node # %3u (after time-out (missing keyboard input))", State.Flags);
                break;
            }

            case StateOfExistance_W4EndOfIdleTime_NoInput:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                if (QueryMode==3)   // Self-paced query mode.
                {
                    // If no key of interest is pressed, do nothing.
                    if (State.Flags==0) { if ((PlayerCommands[PCNr].Keys & (START_KEYS     | REVERSE_MOVE_KEYS))==0) break; }
                                   else { if ((PlayerCommands[PCNr].Keys & (NEXT_MOVE_KEYS | REVERSE_MOVE_KEYS))==0) break; }

                    // Do NOT record anything about the VERY FIRST press of the start key, as it is intended to start the sequence.
                    // For subsequent moves, a Y or N entry has already been recorded before we got here.
                    // Now, we additionally record a "REVERSE" or "SKIP" entry!
                    if (State.Flags>0)
                        WriteLogFileEntry(State.Flags,
                                          TotalTurnTime+TotalPathTime+State.Velocity.x,
                                          ((PlayerCommands[PCNr].Keys & REVERSE_MOVE_KEYS)!=0) ? 'R' : 'S');

                    // If any of the "reverse move" keys is pressed, initiate the reverse move sequence.
                    if ((PlayerCommands[PCNr].Keys & REVERSE_MOVE_KEYS)!=0)
                    {
                        if (State.Flags==0) break;

                        State.Flags--;
                        State.Velocity.x=0.0;
                        State.StateOfExistance=StateOfExistance_ReverseMove_FlyBack;
                        break;
                    }

                    // START or NEXT MOVE key is pressed, we fall out below, transiting to the next state.
                }
                else
                {
                    if (State.Flags>0)
                    {
                        if (State.Velocity.x<TotalIdleTime) break;
                    }
                    else
                    {
                        if (NoCountDownButW4Key)
                        {
                            // If there is no count-down, wait until one of the 'START_KEYS' is pressed.
                            // No other key must be pressed at the same time.
                            if ((PlayerCommands[PCNr].Keys & (~START_KEYS))!=0) break;
                            if ((PlayerCommands[PCNr].Keys &   START_KEYS )==0) break;
                        }
                        else
                        {
                            // The count-down should take three seconds, independent of the value of 'TotalIdleTime'.
                            // ATTENTION: Changing the '3.0' here requires changes in 'Draw()', too!
                            if (State.Velocity.x<3.0) break;
                        }
                    }
                }

                // TRANSIT TO THE NEXT STATE
                if ((unsigned long)State.Flags+1<NodeSequence.Size())
                {
                    State.Flags++;
                    State.Velocity.x=0.0;

                    State.StateOfExistance=(QueryMode<2) ? StateOfExistance_TurnTowardsNextNode_NoInput
                                                         : StateOfExistance_TurnTowardsNextNode_W4Input;
                }
                else
                {
                    // End of sequence reached - this was the last node.
                    // We simply stay in this StateOfExistance and at this node until the quit messages comes through.

                    // In the ideal case, with the client & server running simultaneously,
                    // and packet delivery via the localhost is performed instanteously,
                    // the client performs no prediction at all.
                    // Thus, simply call 'SingleOpenGLWindow->PostQuitMessage()', without caring if the call
                    // is made from a prediction Think()ing, or a real server-side Think()ing.
                    // In the worst case (with prediction), the quit message is posted multiple times.
                    // if (SingleOpenGLWindow) SingleOpenGLWindow->PostQuitMessage();   // FIXME: Set ConVar "quit" to true instead!

                    if (FrameTime>=0.0 && LogFile!=NULL)
                    {
                     // fprintf(LogFile, "EOF2\n");
                        fclose(LogFile);
                        LogFile=NULL;
                    }
                }

                // GameWorld->PrintDebug("Shipping to node # %3u (after keyboard input, after end of idle time)", State.Flags);
                break;
            }

            case StateOfExistance_TurnTowardsNextNode_W4Input:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                if (DebugModeOn && FrameTime>=0.0 /* Don't issue a message in prediction mode! */)
                {
                    Console->Print(cf::va("%4u   %4u", (unsigned long)(PlayerCommands[PCNr].FrameTime*1000.0+0.5), (unsigned long)(State.Velocity.x*1000.0+0.5)));
                }

                const double TurnRatio=State.Velocity.x/TotalTurnTime;
                m_Origin =scale(NodeSequence[State.Flags-1].Coordinate    ,     TurnRatio)+
                          scale(NodeSequence[State.Flags-1].StopCoordinate, 1.0-TurnRatio);
                m_Heading=(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisHeading+(1.0-TurnRatio)*NodeSequence[State.Flags].PrevHeading);
                m_Pitch  =(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisPitch  +(1.0-TurnRatio)*NodeSequence[State.Flags].PrevPitch);
                m_Bank   =(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisBank   +(1.0-TurnRatio)*NodeSequence[State.Flags].PrevBank);


                const bool YesIsPressed=(PlayerCommands[PCNr].Keys & YES_KEYS)!=0;
                const bool  NoIsPressed=(PlayerCommands[PCNr].Keys & NO_KEYS )!=0;

                if ((YesIsPressed || NoIsPressed) && !(YesIsPressed && NoIsPressed))
                {
                    if (NodeSequence[State.Flags].AlreadyVisited==YesIsPressed)
                    {
                        // Count the CORRECT operator descision
                        State_CountCorrect++;
                    }
                    else
                    {
                        // Count the INCORRECT operator descision
                        State_CountIncorrect++;
                    }

                    // State.Velocity.x counts the seconds since this state was entered.
                    // Sum up the reaction times in State.Velocity.y.
                    double ResponseTime=0.0;

                    switch (QueryMode)
                    {
                     // case 0: break;  // This state is never entered in QueryMode 0
                     // case 1: break;  // This state is never entered in QueryMode 1
                        case 2: ResponseTime=State.Velocity.x; break;
                        case 3: ResponseTime=State.Velocity.x; break;
                    }

                    State.Velocity.y+=ResponseTime;

                    if (FrameTime>=0.0)
                        WriteLogFileEntry(State.Flags, ResponseTime, YesIsPressed ? 'Y' : 'N');

                    if (DebugModeOn && FrameTime>=0.0 /* Don't issue a message in prediction mode! */)
                        Console->Print(cf::va("Response time: %4u msec", (unsigned long)(ResponseTime*1000.0+0.5)));

                    // Transit to the _NoInput pair state
                    State.StateOfExistance=StateOfExistance_TurnTowardsNextNode_NoInput;
                    break;
                }


                if (State.Velocity.x<TotalTurnTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=0.0;
                m_Origin =NodeSequence[State.Flags-1].Coordinate;
                m_Heading=(unsigned short)NodeSequence[State.Flags].ThisHeading;
                m_Pitch  =(unsigned short)NodeSequence[State.Flags].ThisPitch;
                m_Bank   =(unsigned short)NodeSequence[State.Flags].ThisBank;
                State.StateOfExistance=StateOfExistance_InFlightToNextNode_W4Input;
                break;
            }

            case StateOfExistance_TurnTowardsNextNode_NoInput:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                const double TurnRatio=State.Velocity.x/TotalTurnTime;
                m_Origin =scale(NodeSequence[State.Flags-1].Coordinate    ,     TurnRatio)+
                          scale(NodeSequence[State.Flags-1].StopCoordinate, 1.0-TurnRatio);
                m_Heading=(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisHeading+(1.0-TurnRatio)*NodeSequence[State.Flags].PrevHeading);
                m_Pitch  =(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisPitch  +(1.0-TurnRatio)*NodeSequence[State.Flags].PrevPitch);
                m_Bank   =(unsigned short)(TurnRatio*NodeSequence[State.Flags].ThisBank   +(1.0-TurnRatio)*NodeSequence[State.Flags].PrevBank);

                if (State.Velocity.x<TotalTurnTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=0.0;
                m_Origin =NodeSequence[State.Flags-1].Coordinate;
                m_Heading=(unsigned short)NodeSequence[State.Flags].ThisHeading;
                m_Pitch  =(unsigned short)NodeSequence[State.Flags].ThisPitch;
                m_Bank   =(unsigned short)NodeSequence[State.Flags].ThisBank;
                State.StateOfExistance=(QueryMode!=1) ? StateOfExistance_InFlightToNextNode_NoInput : StateOfExistance_InFlightToNextNode_W4Input;
                break;
            }

            case StateOfExistance_InFlightToNextNode_W4Input:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                const double FlightPathRatio=State.Velocity.x/TotalPathTime;
                m_Origin=scale(NodeSequence[State.Flags  ].StopCoordinate,     FlightPathRatio)+
                         scale(NodeSequence[State.Flags-1].Coordinate    , 1.0-FlightPathRatio);


                const bool YesIsPressed=(PlayerCommands[PCNr].Keys & YES_KEYS)!=0;
                const bool  NoIsPressed=(PlayerCommands[PCNr].Keys & NO_KEYS )!=0;

                if ((YesIsPressed || NoIsPressed) && !(YesIsPressed && NoIsPressed))
                {
                    if (NodeSequence[State.Flags].AlreadyVisited==YesIsPressed)
                    {
                        // Count the CORRECT operator descision
                        State_CountCorrect++;
                    }
                    else
                    {
                        // Count the INCORRECT operator descision
                        State_CountIncorrect++;
                    }

                    // State.Velocity.x counts the seconds since this state was entered.
                    // Sum up the reaction times in State.Velocity.y.
                    double ResponseTime=0.0;

                    switch (QueryMode)
                    {
                     // case 0: break;  // This state is never entered in QueryMode 0
                        case 1: ResponseTime=              State.Velocity.x; break;
                        case 2: ResponseTime=TotalTurnTime+State.Velocity.x; break;
                        case 3: ResponseTime=TotalTurnTime+State.Velocity.x; break;
                    }

                    State.Velocity.y+=ResponseTime;

                    if (FrameTime>=0.0)
                        WriteLogFileEntry(State.Flags, ResponseTime, YesIsPressed ? 'Y' : 'N');

                    if (DebugModeOn && FrameTime>=0.0 /* Don't issue a message in prediction mode! */)
                        Console->Print(cf::va("Response time: %4u msec", (unsigned long)(ResponseTime*1000.0+0.5)));

                    // Transit to the _NoInput pair state
                    State.StateOfExistance=StateOfExistance_InFlightToNextNode_NoInput;
                    break;
                }


                if (State.Velocity.x<TotalPathTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=0.0;
                m_Origin=NodeSequence[State.Flags].StopCoordinate;
                State.StateOfExistance=StateOfExistance_W4EndOfIdleTime_W4Input;
                break;
            }

            case StateOfExistance_InFlightToNextNode_NoInput:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                const double FlightPathRatio=State.Velocity.x/TotalPathTime;
                m_Origin=scale(NodeSequence[State.Flags  ].StopCoordinate,     FlightPathRatio)+
                         scale(NodeSequence[State.Flags-1].Coordinate    , 1.0-FlightPathRatio);

                if (State.Velocity.x<TotalPathTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=0.0;
                m_Origin=NodeSequence[State.Flags].StopCoordinate;
                State.StateOfExistance=(QueryMode<1) ? StateOfExistance_W4EndOfIdleTime_W4Input : StateOfExistance_W4EndOfIdleTime_NoInput;
                break;
            }

            case StateOfExistance_ReverseMove_FlyBack:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                const double FlightPathRatio=(TotalPathTime-State.Velocity.x)/TotalPathTime;
                m_Origin=scale(NodeSequence[State.Flags+1].StopCoordinate,     FlightPathRatio)+
                         scale(NodeSequence[State.Flags  ].Coordinate    , 1.0-FlightPathRatio);

                if (State.Velocity.x<TotalPathTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=0.0;
                m_Origin=NodeSequence[State.Flags].Coordinate;
                State.StateOfExistance=StateOfExistance_ReverseMove_TurnBack;
                break;
            }

            case StateOfExistance_ReverseMove_TurnBack:
            {
                State.Velocity.x+=PlayerCommands[PCNr].FrameTime;

                const double TurnRatio=(TotalTurnTime-State.Velocity.x)/TotalTurnTime;
                m_Origin =scale(NodeSequence[State.Flags].Coordinate    ,     TurnRatio)+
                          scale(NodeSequence[State.Flags].StopCoordinate, 1.0-TurnRatio);
                m_Heading=(unsigned short)(TurnRatio*NodeSequence[State.Flags+1].ThisHeading+(1.0-TurnRatio)*NodeSequence[State.Flags+1].PrevHeading);
                m_Pitch  =(unsigned short)(TurnRatio*NodeSequence[State.Flags+1].ThisPitch  +(1.0-TurnRatio)*NodeSequence[State.Flags+1].PrevPitch);
                m_Bank   =(unsigned short)(TurnRatio*NodeSequence[State.Flags+1].ThisBank   +(1.0-TurnRatio)*NodeSequence[State.Flags+1].PrevBank);

                if (State.Velocity.x<TotalTurnTime) break;

                // TRANSIT TO THE NEXT STATE
                State.Velocity.x=(State.Flags>0) ? 0.0 : 4.0;
                m_Origin =NodeSequence[State.Flags].StopCoordinate;
                m_Heading=(unsigned short)NodeSequence[State.Flags].ThisHeading;
                m_Pitch  =(unsigned short)NodeSequence[State.Flags].ThisPitch;
                m_Bank   =(unsigned short)NodeSequence[State.Flags].ThisBank;
                State.StateOfExistance=(State.Flags>0) ? StateOfExistance_W4EndOfIdleTime_W4Input : StateOfExistance_W4EndOfIdleTime_NoInput;
                break;
            }
        }
    }

    // LABYRINTH-SIZE TRICK, PART #2:
    // Although m_Origin and all computations take place in world space,
    // compensate for the actual Node center position.
    // That is, reach out always from the absolute center (0,0,0) of the world.
    // All the action will then take place in the inner 3x3x3 labyrinth of the cube,
    // while the computations remain untouched in "real" world coordinates.
    // This way it is possible to have *arbitrarily* large labyrinths!
    m_Origin=m_Origin-NodeSequence[State.Flags].Coordinate;
    if (ConstantHeading) m_Origin=m_Origin-VectorT(0.0, 0.15*4876.8, 0.0);

    PlayerCommands.Clear();
}


static void DrawLight(float r, float g, float b, float /*a*/)
{
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );

    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f));


    const int            FrameWidth=8;
    static MatSys::MeshT Mesh(MatSys::MeshT::Quads);

    Mesh.Vertices.Overwrite();
    Mesh.Vertices.PushBackEmpty(16);

    // Left bar.
    Mesh.Vertices[ 0].SetOrigin(         0,   0);
    Mesh.Vertices[ 1].SetOrigin(FrameWidth,   0);
    Mesh.Vertices[ 2].SetOrigin(FrameWidth, 600);
    Mesh.Vertices[ 3].SetOrigin(         0, 600);

    // Right bar.
    Mesh.Vertices[ 4].SetOrigin(800-FrameWidth,   0);
    Mesh.Vertices[ 5].SetOrigin(           800,   0);
    Mesh.Vertices[ 6].SetOrigin(           800, 600);
    Mesh.Vertices[ 7].SetOrigin(800-FrameWidth, 600);

    // Top bar.
    Mesh.Vertices[ 8].SetOrigin(    FrameWidth,          0);
    Mesh.Vertices[ 9].SetOrigin(800-FrameWidth,          0);
    Mesh.Vertices[10].SetOrigin(800-FrameWidth, FrameWidth);
    Mesh.Vertices[11].SetOrigin(    FrameWidth, FrameWidth);

    // Bottom bar.
    Mesh.Vertices[12].SetOrigin(    FrameWidth, 600-FrameWidth);
    Mesh.Vertices[13].SetOrigin(800-FrameWidth, 600-FrameWidth);
    Mesh.Vertices[14].SetOrigin(800-FrameWidth,            600);
    Mesh.Vertices[15].SetOrigin(    FrameWidth,            600);


    static MaterialT*               FrameMaterial =MaterialManager->GetMaterial("StatusFrameMaterial");
    static MatSys::RenderMaterialT* FrameRenderMat=MatSys::Renderer->RegisterMaterial(FrameMaterial);

    MatSys::Renderer->SetCurrentMaterial(FrameRenderMat);
    MatSys::Renderer->SetCurrentAmbientLightColor(r, g, b);
    MatSys::Renderer->RenderMesh(Mesh);


    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
}


void EntHumanPlayerT::PostDraw(float /*FrameTime*/, bool FirstPersonView)
{
    // We only draw HUD information here for the own player, like test results or the green/yellow/red frame.
    // Thus, there is no point in drawing anything if the draw is not from our own perspective.
    if (!FirstPersonView) return;

    switch (State.StateOfExistance)
    {
        case StateOfExistance_ReverseMove_FlyBack:
        case StateOfExistance_ReverseMove_TurnBack:
            DrawLight(0.5f, 0.5f, 0.5f, 0.5f);
            break;

        case StateOfExistance_W4EndOfIdleTime_W4Input:
        case StateOfExistance_TurnTowardsNextNode_W4Input:
        case StateOfExistance_InFlightToNextNode_W4Input:
            DrawLight(0.0f, 1.0f, 0.0f, 0.5f);
            break;

        case StateOfExistance_W4EndOfIdleTime_NoInput:
            // SPECIAL CASE 1a: The frame should always be yellow for this state,
            // but because this state is also the initial state for a new test sequence,
            // we choose it to be red iff we are at the first node in the sequence.
            if (State.Flags==0)
            {
                // Apply the first special case.
                DrawLight(1.0f, 0.0f, 0.0f, 0.5f);
            }
            else
            {
                // SPECIAL CASE 2: Should still always be yellow, but for the new implicit state
                // that serves for clearly separating moves in QueryMode 2, we need to color it red
                // on entering that implicit state in QueryMode 2.
                if (QueryMode!=2)
                {
                    // The normal case.
                    DrawLight(1.0f, 1.0f, 0.0f, 0.5f);
                }
                else
                {
                    // Apply the second special case.
                    if (State.Velocity.x<Mode3WaitTime) DrawLight(1.0f, 1.0f, 0.0f, 0.5f);
                                                   else DrawLight(1.0f, 0.0f, 0.0f, 0.5f);
                }
            }

            // SPECIAL CASE 1b: Print a count-down message 3... 2... 1... on the screen.
            if (State.Flags==0)
            {
                static FontT Font1("Fonts/Arial");
                const float  TimeRemaining=3.0f-float(State.Velocity.x);
                const int FrameWidth =1024;
                const int FrameHeight= 786;

                     if (TimeRemaining>=2.0) Font1.Print(FrameWidth/2-47, FrameHeight/2, float(FrameWidth), float(FrameHeight), 0x00FF0000, "   3   ");
                else if (TimeRemaining>=1.0) Font1.Print(FrameWidth/2-47, FrameHeight/2, float(FrameWidth), float(FrameHeight), 0x00FF0000, "   2   ");
                else if (TimeRemaining>=0.0) Font1.Print(FrameWidth/2-47, FrameHeight/2, float(FrameWidth), float(FrameHeight), 0x00FF0000, "   1   ");
             // else if (TimeRemaining>=0.0) Font1.Print(FrameWidth/2-47, FrameHeight/2, float(FrameWidth), float(FrameHeight), 0x00FF0000, "  Go!  ");
                else
                {
                    // OLD CODE (yes, it was only one line):
                    // Font1.Print(FrameWidth/2-75, FrameHeight/2, float(FrameWidth), float(FrameHeight), 0x00FF0000, "Ready!");

                    static const std::string        HudImgMatName="Hud/Ready";
                    static unsigned long            HudImgWidth  =MaterialManager->GetMaterial(HudImgMatName)->GetPixelSizeX();
                    static unsigned long            HudImgHeight =MaterialManager->GetMaterial(HudImgMatName)->GetPixelSizeY();
                    static MatSys::RenderMaterialT* HudImgRM     =MatSys::Renderer->RegisterMaterial(MaterialManager->GetMaterial(HudImgMatName));

                    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
                    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );
                    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );

                    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
                    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());
                    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, float(FrameWidth), float(FrameHeight), 0.0f, -1.0f, 1.0f));

                    static MatSys::MeshT TextMesh(MatSys::MeshT::Quads);
                    TextMesh.Vertices.Overwrite();
                    TextMesh.Vertices.PushBackEmpty(4);

                    const unsigned long w=HudImgWidth /2;
                    const unsigned long h=HudImgHeight/2;

                    TextMesh.Vertices[0].SetOrigin(FrameWidth/2-w, FrameHeight/2-h); TextMesh.Vertices[0].SetTextureCoord(0.0f, 0.0f);
                    TextMesh.Vertices[1].SetOrigin(FrameWidth/2+w, FrameHeight/2-h); TextMesh.Vertices[1].SetTextureCoord(1.0f, 0.0f);
                    TextMesh.Vertices[2].SetOrigin(FrameWidth/2+w, FrameHeight/2+h); TextMesh.Vertices[2].SetTextureCoord(1.0f, 1.0f);
                    TextMesh.Vertices[3].SetOrigin(FrameWidth/2-w, FrameHeight/2+h); TextMesh.Vertices[3].SetTextureCoord(0.0f, 1.0f);

                    MatSys::Renderer->SetCurrentMaterial(HudImgRM);
                    MatSys::Renderer->RenderMesh(TextMesh);

                    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
                    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
                    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
                }
            }
            break;

        case StateOfExistance_TurnTowardsNextNode_NoInput:
            if (QueryMode<2) DrawLight(1.0f, 0.0f, 0.0f, 0.5f);
                        else DrawLight(1.0f, 1.0f, 0.0f, 0.5f);
            break;

        case StateOfExistance_InFlightToNextNode_NoInput:
            if (QueryMode<1) DrawLight(1.0f, 0.0f, 0.0f, 0.5f);
                        else DrawLight(1.0f, 1.0f, 0.0f, 0.5f);
            break;
    }
}
