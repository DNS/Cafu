
minizip/    by Gilles Vollant <info@winimage.com>
        Mini zip and unzip based on zlib
        See http://www.winimage.com/zLibDll/unzip.html

The minizip library is originally distibuted with zlib in zlib/contrib/minizip.
We moved the minizip library from the zlib/contrib directory up to the same
level as the other vendor libraries in order to be able to use it independently
from the zlib: Specifically under Linux, we'll want to use our local copy of
minizip with the *systems* copy of zlib in the future, so that separating them
(e.g. for building them independently) was required.
See r214 to r217 for details.
