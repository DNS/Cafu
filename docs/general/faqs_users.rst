.. _user_faq:

User FAQ
========

Why does my Personal Firewall report Cafu.exe?
----------------------------------------------

When you start Cafu, it internally starts a client and a server, then
connects to itself when a new map is loaded. This is done even for pure
single-player games, and thus your Windows or Personal Firewall reports
that ``Cafu.exe`` is establishing a network connection.

Therefore, please set your firewall to not block Cafu from network
access. If you do not plan to host a Cafu game server or join a Cafu
internet game, there is of course no harm in blocking access from and to
the internet for ``Cafu.exe``, but for single-player games, basic access
to the local network is required.

Cafu reports an "Initialization Error", what can I do?
------------------------------------------------------

|Cafu Initialization Error| If you see an initialization error similar
to the one in the image to the right, the most frequent cause is that
the drivers for your sound or graphics hardware are outdated or
incomplete (e.g. missing an OpenGL implementation). Please install or
update the drivers for your hardware first, then try again.

If that doesn't help, run ``Cafu.exe`` from the `command
prompt <https://en.wikipedia.org/wiki/Command_Prompt>`__ with the
``-log console.txt`` option. For example:

.. code:: bash

   > Cafu.exe -log console.txt

The contents of the newly generated file ``console.txt`` might give you
a clue about why the initialization error occurred. If it doesn't,
please post both the error message and the contents of the
``console.txt`` file at our `forum <http://forum.cafu.de>`__ or
`mailing-lists <http://www.cafu.de/support/mailing-lists>`__.

Note that you can run

.. code:: bash

   > Cafu.exe --help

to see a list of all available command line options. They might be
useful to work around the problem, and if you post at the forums, we
might ask you to try some of them to further narrow the problem down.

Why do you use OpenGL? I think that DirectX is better.
------------------------------------------------------

Here are some features of OpenGL that made us choose it as the main
rendering API:

-  very well documented,
-  portable and available on many platforms,
-  simple to use,
-  independent industry standard,
-  powerful extension mechanism that makes it possible to support the
   latest features of new 3D graphics boards immediately.

You may also find `this blog
message <http://blog.wolfire.com/2010/01/Why-you-should-use-OpenGL-and-not-DirectX>`__
interesting. 😉

Why is the image quality low? How can I improve it?
---------------------------------------------------

In normal operation, the Cafu engine provides state-of-the-art image
quality that is achieved by employing the latest methods in rendering
technology. However, if the image quality is lower than you expected or
not as good as shown in the `reference screenshots at the Cafu
website <http://www.cafu.de/gallery>`__, these are the common causes for
reduced image quality:

-  If your graphics board is of an older make and model, it might not
   have a programmable GPU. Programmable GPUs however are required for
   rendering the sophisticated image effects of the Cafu engine. As a
   consequence, if Cafu has detected that no programmable GPU is present
   on your system, it automatically falls back into the best compatible
   mode that still works on your system. Normally you should also see an
   info message about this situation, but it is easy to miss the
   message.
   Unfortunately, the fall-back mode cannot provide the image quality
   that programmable GPUs can provide, as then the lighting and shadows
   have to be computed per-vertex rather than per-pixel, or their
   effects even have to be disabled entirely. In this case, you will see
   no dynamic lighting and no shadows at all.
   If this situation applies to your system, nothing can be done about
   it but getting a more recent graphics card.
-  The texture detail in the initial options dialog is only set to
   “medium” quality by default. We have chosen that setting in order to
   not disappoint the FPS freaks among you, because higher quality
   normally also reduces the FPS. You can set the texture detail to the
   “high” setting in order to get higher texture resolution and thus
   higher image quality. Please note that the “high” quality setting can
   sometimes considerably lower the frame-rate, depending on the details
   of your graphics board. You may also want to read the other FAQ about
   low FPS in this regard.
-  The default display resolution of Cafu is 1024*768. That setting has
   been proven a good compromise between image quality and frame-rate,
   but you might increase it within the console in order to increase the
   image quality. 2048*1536 looks really cool, but then the FPS is also
   lower. 😉
-  Some graphics boards with early generations of programmable GPUs are
   inherently limited in the image quality that they can provide (at a
   reasonable performance). For example, GPUs that are based on the NV2X
   series of NVidia GPUs do show specular highlights that look “blocky”.
   Nothing can be done about that (except getting a more recent 3D board
   with at least NV3X or ATI Radeon 9600 or higher support).

Why is the frame-rate (FPS) low? How can I improve it?
------------------------------------------------------

During the past few years, many people have become used to use 3D
engines that employ relatively old rendering technologies on relatively
new graphics accelerator boards. This combination, observed in games
like Counter-Strike, Quake3 and similar, achieved impressive frame-rates
(often a hundred FPS and more) and fluid display updates.

The difference with Cafu is that Cafu employs dynamic, per-pixel
lighting and shadows. The underlying rendering techniques are much more
sophisticated than previous rendering methods, and relatively new.
Despite a thoroughly optimized implementation, these techniques
inherently cause lower frame-rates than older the techniques. In fact,
if you compare Cafu to comparable 3D engines that are in the same
category (only very few are, like Doom3), Cafu beats surprisingly well.
;)

There are many factors that have an effect on rendering performance. You
can tweak some of them in order to manipulate the frame-rate, but the
actual change in frame-rate much depends on your computer system and its
components. Some of the suggestions below may even have no observable
effect on your system at all. This is because every system has a
different bottle-neck, and only finding and removing the bottle-neck
yields the greatest effect.

Here is a list of options and suggestions that might help with the
performance. You may simply try them out, and see what works best for
your system. However, please note that most of these options *will buy
you faster rendering at the cost of decreased image quality*.

Vertical synchronization.
   People frequently report that their frame-rate seems to be capped to
   60 or 30 (or another multiple of 15) FPS. This is usually because
   *vertical synchronization* is enabled for your graphics board. Please
   see the Wikipedia article at
   `Vertical_synchronization <https://en.wikipedia.org/wiki/Vertical_synchronization>`__
   for technical details on the matter.
   Find the related setting in the control panel of your graphics driver
   to disable the vertical synchronization. Your graphics driver and
   board will then render the frames without the delay inherent to
   vertical synchronization, which in turn increases the FPS.
Video memory and texture size.
   If your graphics board has too little video memory to store all the
   textures simultaneously that Cafu needs for rendering a single frame,
   the FPS usually suffers significantly. In this case you can set a
   lower texture quality in the initial Options dialog.
   This effectively scales all textures down to not exceed a maximum
   size, and therefore improves the chances that they all fit into video
   RAM simultaneously. Another option is to add physical memory to your
   graphics board. 128 MB normally work well, 256 MB are even better and
   can store the textures even in maximum resolution.
Display resolution.
   The default display resolution of Cafu is 1024*768. That setting has
   been proven a good compromise between image quality and frame-rate,
   but you might decrease it within the console in order to increase the
   frame-rate. Reducing to 800*600 or even to 640*480 helps a lot.
Pixel fill-rate.
   A common reason for low FPS is that dynamic lighting and shadows
   consume a *very high pixel fill-rate*. Besides reducing the display
   resolution as mentioned above, an effective way to improve FPS in
   this case is to turn dynamic lighting and shadow effects entirely
   off.
   One way to achieve this is to set the ``cl_maxLights`` console
   variable to ``0``. This can be done in the game console or by putting
   the statement
   .. code:: lua

      cl_maxLights=0;

   | into your ``config.lua`` file.
   | When Cafu is run from the command-line, it is also possible to
     specify a renderer DLL that makes use of less graphics features.
     For example, you might try the OpenGL 1.2 renderer in order to
     increase the FPS:

   ::

      C:\Cafu-9.06> Cafu.exe -clRenderer ..\Libs\build\win32\vc8\release\MaterialSystem\RendererOpenGL12.dll

.. |Cafu Initialization Error| image:: /images/cafu_init_error.png
   :class: mediaright
   :width: 240px
