This is the OpenAL SDK for Windows, downloaded from <http://connect.creativelabs.com/openal>.
Under Linux, we use the openal-soft library instead.

The "OpenAL32.dll" and "wrap_oal.dll" are the two DLLs that are normally installed by the
"redist/oalinst.exe" OpenAL redistributable installer (that, among other directories,
has been omitted from the repository issue for disk space and bandwidth considerations),
but as we eventually want to have them directly in the Cafu directory, we also keep some
copies here.
