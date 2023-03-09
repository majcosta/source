Building Ja2 1.13 for Windows XP
================================

Requires the deprecated v141_xp to be installed.

From the source directory:

```
mkdir buildXp
cd buildXp
cmake .. -G"Visual Studio 17 2022" -A Win32 -T v141_xp
cmake --build . --config Release
```
