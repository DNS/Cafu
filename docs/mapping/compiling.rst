.. _compiling_maps_at_the_command-line:

Compiling Maps at the Command-Line
==================================

When you have finished editing a map in CaWE, you'll want to run it with
Cafu. The engine however requires that the map is in a precomputed form,
and this section describes how you turn your map into a fully
precomputed world file.

In order to use CaBSP, CaPVS and CaLight “by hand” (rather than
:ref:`from within CaWE <compiling_maps_for_cafu>`), you should be
familiar with the command-line interface of Windows or Linux, because
these programs just cannot be started by a simple double-click on them.

Under Windows, you can open the command-line window by opening “Programs
→ Accessories → Command Prompt” (in German: “Programme → Zubehör →
Eingabeaufforderung”) from the Start menu. Initially, the window shows
the directory you are currently in. You can browse into the parent
directory by typing ``cd ..``, as for example in

::

     C:\Windows> cd ..
     C:\>

and you can enter a directory by typing

::

     C:\> cd directoryname
     C:\directoryname>

(Replace “directoryname” with the name of the directory that you want to
open.) Other useful commands are ``dir`` that shows all files that are
in the directory you are currently in and ``e:`` (for example) to change
the current hard drive.

In order to run the programs with the commands presented below, you'll
first have to browse into the directory they are in (which is the
``Cafu-9.06`` directory).

Compiling step 1: CaBSP
-----------------------

This program takes a cmap map file and creates a cw (Cafu world) file
from it. Example:

::

       C:\Cafu-9.06> CaBSP Games/DeathMatch/Maps/MyMap.cmap Games/DeathMatch/Worlds/MyMap.cw

**(!)** **WARNING:** The specified destination file
(``Games/DeathMatch/Worlds/MyMap.cw`` in the above example) gets
overwritten without prior notice! This is also true for mis-spelt file
names. For example, if you type ``Games/DeathMatch/Maps/MyMap.cmap``
instead of ``Games/DeathMatch/Worlds/MyMap.cw`` – catastrophe! As with
most other computer software, in order to prevent a disaster, please
*backup your files*\ **before**\ *you begin*!

For quick tests during world development, you might want to skip the
next two compiling steps (CaPVS and CaLight). You can run the file
obtained from the CaBSP tool directly in the Cafu engine. It will be
unlit and without PVS information (which results in a lower frame rate),
but is usually sufficient for early tests.

Compiling step 2: CaPVS
-----------------------

This program creates the *Potentially Visibility Set* for your worlds.
Example:

::

       C:\Cafu-9.06> CaPVS Games/DeathMatch/Worlds/MyMap.cw

The CaPVS compile switches
~~~~~~~~~~~~~~~~~~~~~~~~~~

Even though the latest versions of CaPVS have become fast enough to deal
with even the most complex worlds within reasonable time bounds,
sometimes it may still be preferable to obtain faster results (at the
cost of accuracy).

Before you read on, though, my advice is to simply skip the rest of this
section, and continue with the next. Why? Well, the compile switches
introduced below were made at a time when CaPVS was inherently slower.
Since then, it has undergone many algorithmic improvements, and the
switches were only left for safety and as a last resort. I hardly ever
use them myself (although their implementation is interesting), and
nowadays never use them for release compiles (the worst world I have
takes 27 hours on a 866 MHz Pentium III this way). During world
development, you should skip CaPVS entirely rather than trying to tune
it for faster compilation. Also keep in mind that the proper use of
``func_detail_object`` entities can have a big impact on performance in
general, and on CaPVS compile time in particular.

Anyway, the latest version of this tool has two new command line
switches that were made to lower its compile times by effectively
trading quality for speed. Both switches control the creation of
*SuperLeaves*. To understand what SuperLeaves are, you must first have a
rough idea what *leaves* are: Leaves are spatial convex cells into which
CaBSP subdivides your world and in which the action of the game takes
place later on. Simplified spoken, you can imagine that each room of
your world corresponds to a leaf. CaPVS then determines the PVS by
calculating if any unobstructed lines of sight exist between any pair of
two leaves. It is not only the sheer number of leaves that makes the
computation so slow, but also their spatial relations among each other.
In some cases, there are unnecessarily too many leaves for the purposes
of CaPVS, and it is preferable to merge some of them. Thus, a
*SuperLeaf* is a set or a group of simple leaves. One could say that
SuperLeaves subdivide the world in cells as leaves do, but the
subdivision is “coarser” than with the simple leaves. When you run CaPVS
on a world, it tells you how many SuperLeaves it constructed from the
simple leaves. By default (without command line switches), the
SuperLeaves are identical to the simple leaves, as is their number.

Now about the actual switches: Running CaPVS without any parameters (not
even a world name) will print out a self-explaining usage information.
I'll not repeat it here, but rather give you some tips on the usage of
the two mentioned switches. Note that you always trade quality for
speed, but the compromise is usually a very good one (quality remains
high even for quite fast compiles).

In general, try to reduce the compile time with care. If the situation
was actually hopeless before, it does not make sense to use the switches
to bring the compile time down to a few minutes or seconds. (That would
be more like not running CaPVS at all.) Rather, a good idea would be to
send me an email with a brief description of the problem, along with the
world you are trying to compile (I'll only believe that you created a
world that's too hard for CaPVS after I've seen it myself 😉 ).

Then, consider two strategies: You can either choose to work “top-down”
by choosing the parameters such that the number of SuperLeaves is
gradually reduced in small steps. The quality remains high, but the
downside is that you possibly have to wait another long time until you
know if the reduction was enough.

Thus, I'd recommend a “bottom-up” approach: Initially reduce the number
of SuperLeaves a lot, start with a quite small number. If CaPVS then
takes only a few minutes, but you want it rather to spend some hours,
adjust the parameter values carefully so that the number of SuperLeaves
increases slowly and gradually. You'll probably have to re-try and
experiment a few times until you get a feel of it.

Finally, you should prefer the ``-minAreaSL`` switch over the
``-maxRecDepthSL`` switch, because the former is more sensitive to the
actual world geometry. Do not worry about high numbers on
``-minAreaSL``. When I last used it, I usually had values ranging from
1,000,000 to 5,000,000.

Compiling step 3: CaLight
-------------------------

This program calculates the lighting for a map. This program can take
very long too, but you have better control over it by specifying command
line switches. Examples:

::

       C:\Cafu-9.06> CaLight
       C:\Cafu-9.06> CaLight Games/DeathMatch/Worlds/MyMap.cw
       C:\Cafu-9.06> CaLight Games/DeathMatch/Worlds/MyMap.cw -BlockSize 8 -StopUE 2
       C:\Cafu-9.06> CaLight Games/DeathMatch/Worlds/MyMap.cw -fast

The first line prints out detailed usage information for the CaLight
tool, the second line runs default lighting, which is recommended for
most cases. The third and fourth lines are for very fast lighting, which
is useful for quick tests during map development.

Please note that it is almost never reasonable to set BlockSize below 3
and StopUE below 0.1 for highest quality lighting, even though that is
possible (some of the worlds of the Cafu demo releases were compiled
with a StopUE of 0.05).

During the second phase of the computations (“bounce lighting”), note
that you can interrupt the program by pressing the ``SPACE`` button.
That's sometimes useful for quick tests during map development.

Finally, a word about memory consumption: CaLight requires *plenty* of
it! I would recommend to run CaLight only on computers with at least 1
GB of RAM, it may or may not run with less. If *permanent swapping*
(extensive disk activity) occurs during the bounce lighting phase of
CaLight, better abort it! Under such circumstances, it will proceed
*very* slow anyway, and the lengthy swap activity is certainly not
healthy for your hard-disk. **(!)**

Done: Running the world with Cafu
---------------------------------

If not already there, copy your world into the
``Games/DeathMatch/Worlds`` directory if you made the world for the
DeathMatch MOD, or into the ``Games/OtherMOD/Worlds`` directory of the
MOD you made the world for. Then simply run ``Cafu.exe`` as stated in
the user manual of the current demo release. Your world will be listed
among the other worlds in the dialogs world list (“server options”).
