call Games\DeathMatch\compileMap Test1
call Games\DeathMatch\compileMap TestPatches
call Games\DeathMatch\compileMap AEonsCanyonTower
call Games\DeathMatch\compileMap AEonsCube
call Games\DeathMatch\compileMap BPRockB
call Games\DeathMatch\compileMap BPWxBeta
call Games\DeathMatch\compileMap Gotham
call Games\DeathMatch\compileMap JrBaseHQ
call Games\DeathMatch\compileMap Kidney
call Games\DeathMatch\compileMap ReNoEcho
call Games\DeathMatch\compileMap ReNoElixir
call Games\DeathMatch\compileMap TechDemo
@
@REM Now shutdown the computer.
@REM -c   Allow the shutdown to be aborted by the interactive user.
@REM -f   Forces running applications to close.
@REM -k   Poweroff the computer (reboot if poweroff is not supported).
@REM -t   Specifies countdown in seconds until shutdown (default is 20) or the time of shutdown (in 24 hour notation).
d:\Programme\PsTools\psshutdown.exe -c -f -k -t 300
