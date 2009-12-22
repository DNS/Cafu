@REM Note that this script *must* be run from the Ca3D-Engine/ main directory,
@REM not from Ca3D-Engine/Games/DeathMatch/, or else the tools cannot find e.g. the Textures/*.zip files.
@
build\win32\vc8\release\CaBSP\CaBSP.exe Games/DeathMatch/Maps/%1.cmap Games/DeathMatch/Worlds/%1.cw
build\win32\vc8\release\CaPVS\CaPVS.exe Games/DeathMatch/Worlds/%1.cw
build\win32\vc8\release\CaLight\CaLight.exe Games/DeathMatch/Worlds/%1.cw -gd=Games/DeathMatch
@
@REM Anschlieﬂend den Compi herunterfahren.
@REM -c   Allow the shutdown to be aborted by the interactive user.
@REM -f   Forces running applications to close.
@REM -k   Poweroff the computer (reboot if poweroff is not supported).
@REM -t   Specifies countdown in seconds until shutdown (default is 20) or the time of shutdown (in 24 hour notation).
@REM d:\Programme\PsTools\psshutdown.exe -c -f -k -t 300
