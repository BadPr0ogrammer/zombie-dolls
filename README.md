This is the trial project for standalone RBFX RagDolls sample game.
To build it:
1. Build and install RBFX engine from https://github.com/rbfx/rbfx
2. At the same level of rbfx, clone monster-dolls, so that ../rbfx is the default place for the RBFX engine,
   "../rbfx/build/install" is the install dir.
3. If above is false, set Urho3D_DIR and Urho3D_Generated_DIR cmake options as needed.
4. CMake and build zombie-dolls project as usual
5. Output zombie-dolls.exe file sould be placed next to Urho3D.dll and the RBFX assets("Data" and "CoreData" dirs).
6. Report problems occurs to Bad Progrmmer ;)
