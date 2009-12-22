List of files that are related to the TechDemo island:

a1) Island-Sketch.xcf
The "source" file of the island, based on a sketch by Kai, from which (almost) all other files were derived.
Especially the original, unblurred beach contour mask (background layer of Island-MaskBeach.xcf) was created from this.

a2) Island-Sketch.png
Just a duplicate of Island-Sketch.xcf, exported to png, for simplified handling / reference.



b1) Island-MaskBeach.xcf
This contains the original beach mask derived from Island-Sketch.xcf, and layers three copies of it at different blur levels.
The blurred copies in turn have varying translucency, so that the end result defines the beach contour with varying slope.

b2) Island-MaskBeach.bmp
Just the exported end result of b1), because World-Machine can only load bmp files for masks.

b3) Island-MaskVolkano.bmp
b4) Island-MaskRocks.bmp
More mask files that describe the logical structure of the island, for use in the Island-WorldMachineFile.tmd file.
These two are simple derivatives and exports from Island-MaskBeach.xcf.



c) Island-WorldMachineFile.tmd
The World-Machine file that describes how the island is logically built.
It uses the Island-Mask*.bmp files for mask input (each describes a logical component of the island).
The Island.ter file (a Terragen heightmap) is created from this file the *.bmp files by World-Machine.

d) Island.ter
The heightmap file in Terragen file format of the island, created by World-Machine.

e) Island-TerragenSettings.tgw
The Terragen World File, where all settings except the heightmap itself are stored.
Especially the Surface Maps that eventually define the texture of the terrain are stored here.

f) Island-BaseTex.bmp
Terragen loads Island.ter and Island-TerragenSettings.tgw for input, and produces this file as the end result.
It is used as the basis texture for the island terrain.



x) Island-Readme.txt
This file.
