We use small parts of code from the LightWave 8 SDK for lwo model file loading.
Neither code nor SDK nor website mention an explicit license.

The code in this directory was taken from the LightWave 8 SDK, which is available at
    http://www.newtek.com/products/lightwave/developer/LW80/index.html
from subdirectory sample\Utility\lwobject.

In lwo2.c and lwob.c I inserted
#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning 4018: '>' : signed/unsigned mismatch
    #pragma warning(disable:4018)
#endif
for warning-free compilation though.
