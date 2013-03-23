==========================================================================
Cafu Game and Graphics Engine                           http://www.cafu.de
Demo Readme                                                   info@cafu.de
==========================================================================

  Welcome to Cafu, the open-source Game and Graphics Engine for
  multiplayer, cross-platform, real-time 3D Action.

  This readme gets you started with the Cafu binary demo packages. The
  demo packages are archives that contain precompiled program executables
  and everything else that is required to run the Cafu engine demo.
  If you are interested in getting and compiling the source code instead,
  refer to the Cafu documentation at http://www.cafu.de/wiki/

  Contents of this file:

  1. Quick-start for Windows
  2. Quick-start for Linux
  3. Documentation
  4. Release Notes
  5. License
  6. Contact



==========================================================================
1. Quick-start for Windows
==========================================================================

  You can run the Cafu demo by clicking on the Cafu.exe file that is in
  the same directory as this README.txt file.

  Your Windows or Personal Firewall may report Cafu opening a network
  connection. This is because Cafu starts a client and a server, then
  connects to itself even for pure single-player games.
  Thus, please set your firewall to not block Cafu from network access.

  The Cafu World Editor "CaWE" is started analogously by clicking on
  the CaWE.exe file in the same directory.

  More information about the Cafu engine and the editor CaWE can be
  found in the documentation at http://www.cafu.de/wiki/



==========================================================================
2. Quick-start for Linux
==========================================================================

  Extracting the Cafu .tar.gz archive makes automatically sure
  that the executable bit is properly set for all relevant files, and
  thus you can (double-)click the "Cafu-run" file that is in the same
  directory as this README.txt file in order to start the Cafu engine.
  "Cafu-run" is a shell script that sets the proper working directory
  before starting the Cafu executable.

  Alternatively, you can run Cafu from a command line shell by cd'ing
  into the same directory as this file (e.g.   cd ~carsten/Cafu/ ), and
  then just type   ./Cafu

  The Cafu World Editor "CaWE" is started analogously by clicking on
  the "CaWE-run" file or by running ./CaWE-bin respectively.

  More information about the Cafu engine and the editor CaWE can be
  found in the documentation at http://www.cafu.de/wiki/



==========================================================================
3. Documentation
==========================================================================

  If you need help, please refer to the user manual at
  http://www.cafu.de/wiki/. It has a "Quick Start" section that
  briefly summarizes everything to get you started, and also has more
  detailed information about installing, de-installing, requirements etc.

  Additional documentation and information about all advanced aspects of
  Cafu (e.g. map editing and development) is found at

  a) the Cafu wiki  at http://www.cafu.de/wiki/
  b) the Cafu forum at http://www.cafu.de/forum/



==========================================================================
4. Release Notes
==========================================================================

  Known problems specific to all supported platforms:
  None.

  Known problems specific to the Windows platform:
  a) Occasionally distorted background music playback in the TechDemo
     map near the base of the ladder to the emergency exit.

  Known problems specific to the Linux platform:
  a) Screen resolution cannot be changed at runtime (this is a
     limitation of the X server, not Cafu).
  b) Blank/black screen after starting. On some systems, it takes a long
     time between starting the engine and the initial appearance of the
     "Loading..." screen. The screen is then fully black, and it takes up
     to one or two minutes until the splash screen appears. This problem
     has only been observed on systems that do not meet the minimum system
     requirements, and is due to inadequate hardware and/or driver support.



==========================================================================
5. License
==========================================================================

  Please refer to file LICENSE.txt in the same directory as this readme
  for license information.



==========================================================================
6. Contact
==========================================================================

  If you experience problems or if you have any questions or suggestions,
  visit the official Cafu website at

  http://www.cafu.de

  You will find background information, discussion forums, IRC chat
  details, and many other resources to help you out. You're heartily
  invited to join the forums and the IRC chat to place your questions and
  suggestions.

  You can also contact the Cafu author Carsten Fuchs directly by
  sending an email to

  info@cafu.de
