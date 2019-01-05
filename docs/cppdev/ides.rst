.. _ides_and_text_editors:

IDEs and Text Editors
=====================

In section
:ref:`Getting Started with the Cafu Source Code <getting_started_with_the_cafu_source_code>`
we have introduced you to working with the Cafu source code, but daily
work is a lot more convenient if a text editor or IDE can automate the
most frequent tasks.

Besides normal text editor functionality, what we most importantly need
is:

-  Starting the build (SCons) with a single press of a key or button.
-  A click on a compiler error message must open the relevant source
   file at the reported position.
-  Searching in all project files.

In this section we briefly present several text editors and IDEs that
achieve these goals (and more).

UltraEdit
---------

`UltraEdit <http://www.ultraedit.com/>`__ is a commercial, very powerful
text editor. It is available for Windows, and in slightly different
editions also for Linux and MacOS. Personally, I do most of my
development with UltraEdit, especially under Windows.

UltraEdit has excellent implementations for all of the above
requirements: Setting up a “tool” (running the command ``scons`` in the
proper working directory, e.g. ``D:\Cafu``) is very easy, clicking in
the output window to open files on error messages is trivial, and the
implementation of find and replace is awesome.

See the video below for a short example session with UltraEdit that
demonstrates the basic features outlined above.

Geany
-----

`Geany <http://www.geany.org>`__ is a lightweight, open-source text
editor that, at least regarding the above features, is similar to
UltraEdit. Geany is available for Windows, Linux, MacOs, and more.

I've only recently started to use it under Linux (using Geany 1.22), and
my first impression is very good. Find and replace is not as convenient
as in UltraEdit, but tool setup and many other basic features are quite
the same.

The video at the bottom of this section was made for UltraEdit, but
because Geany is very much alike, you can use it for an overview as
well.

Visual Studio
-------------

If you are a Visual Studio veteran and wonder why we don't simply
provide you with Visual Studio project and solution files, please see
:ref:`Where are the Visual Studio project files? <where_are_the_visual_studio_project_files>`
for our reasoning behind the Cafu build process.

In short, we don't have Visual Studio specific project files available
at this time, but until then, we can fortunately integrate SCons into
Visual Studio in a very similar manner than it is used with other IDEs:
Please refer to the related SCons documentation at
http://www.scons.org/wiki/IDEIntegration for details.

Eclipse, Xcode, ...
-------------------

Unfortunately, I have little experience with these and other IDEs, but
generally, they are expected to work very much like the alternatives
mentioned above.

In fact, the SCons documentation at
http://www.scons.org/wiki/IDEIntegration explains how to integrate SCons
with Exclipse and Xcode, but please understand that we've not yet tried
this ourselves (and therefore can provide little if any help about
them).

.. _cppdev_ides_video:

Video
-----

`This video <https://youtu.be/xLe1xmdA8YY>`__ shows a minimal example
session in UltraEdit, demonstrating the core set of features that we use
when working with the Cafu source code.

The editor Geany is very similar, and we suspect that other IDEs work
similarly as well.

Additional notes:

-  Set the playback quality to “720p HD” if possible.
-  There is no audio track in this video.
-  All mouse actions shown in the video can also be done via the
   keyboard, such as ``Ctrl+Shift+0`` to start SCons, ``Alt+S, I`` to
   open the “Find in Files” dialog, ``ESC`` to close the output pane,
   etc. Using these keys does not produce a comprehensible video, but
   normally, we use our editors like this. 🙂
