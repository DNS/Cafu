.. _making_perfect_detail-maps:

Making "perfect" Detail-Maps
============================

The Problem
-----------

|image0| Detail-maps, in order to work well, must meet two requirements:

#. Their average value must be 128 (50%) so that they don't change the
   total brightness of the texture they'll be applied to, and
#. they must not show tile patterns when they are minified (e.g. when
   they're viewed from a great distance).

Getting 1. right is the task of the artist, and is easily done in any
paint program that supports contrast and brightness adjustments and
histogram views.

| Getting 2. right is more difficult if not impossible for the artist.
  For example, I started with the original detail map shown at the right
  (click to enlarge). Observe that this detail-map has a big dark spot
  roughly in the mid of the right half.

|perfectdetailmap-originalscreen.jpg| This screenshot shows the effect
of applying the detail-map to a large terrain. Note the well-visible
pattern both on the near ground as well as on the far rocks. This strong
pattern is caused by the non-balancedness of the detail texture.

The Idea
--------

| If we had a low-frequency graymap E that just represents the “error”
  in the above detail-map, we could subtract that error-map from the
  detail map D to just remove the error, and then add 128 (50%) to move
  the remaining high-frequency parts around 0 back to the average of
  128. That is, the perfectly fixed detail map is found by computing D -
  E + 128.

The Solution
------------

First, observe that E is easily found by blurring a copy of D! Any paint
program can easily do that! I used The Gimp to apply a Gaussian blur
filter to the detail map, creating test images with filter widths of 5,
20, 40 and 60 pixels respectively. The left image below shows my so
obtained “E”, which is the original detail-map blurred with a 60 pixels
width Gauss filter!

The next problem is that computing the above D - E + 128 has a tendency
to produce intermediate values that are less than 0 or greater than 255.
Therefore, this operation can **not** be done with layer techniques in
PhotoShop, PaintShop, Gimp, etc.!

In order to get correct results easily, I just wrote a small C++ program
that reads E and D from two images files on disk, computes D - E + 128,
and writes the result back into a third file. Please send me an email if
you're interested in the exe file for Windows!

And yes, that's all! I applied my program to the above detail-map D and
it's error-map E (left image below). The center image below shows the
result. *Notice how beautifully the dark spot is gone!* In the right
image you see a screenshot of the same scene as above, now with the new
detail-map being applied. Note that the pattern is (almost) entirely
gone!

Also please note that now you see a new “pattern” at the mountains –
this is the result of a not-so-good base texture that is showing up now
that the broken pattern of the detail-map has been fixed and does not
any longer distract from it. This matter is not related to detail-map
issues, and is just fixed by using a better base texture.

|image2| |image3| |perfectdetailmap-result60screen.jpg|

Discussion and Results
----------------------

One open question is how much D should be blurred in order to obtain E.
This is easily answered: As much as possible! In the above example, I
used a blur filter of 60 pixels width, and the pattern as (almost)
completely gone. Using smaller filter sizes reduces the rests even
further. For example, I also made tests with blur filters of 5, 20 and
40 pixels width. The smaller the filter kernel width was, the more
homogeneous became the end result. (Blurring not at all obviously yields
the 128 planar detail map (because then E=D, and D-D+128 = 128) and has
no visual impact at all!)

Therefore, the recommended strategy is to start with a really big amount
of blurring, and only reduce it if that amount was not sufficient to
remove the most significant non-balancedness of the original detail-map.

High-Pass Filters in PhotoShop
------------------------------

After I had completed this article, Kai sent me a link to this Gamasutra
site: http://www.gamasutra.com/features/20010523/hajba_01.htm It deals
with the same problem and is a lot more detailed than this text –
definitively a recommended read! As the solution, High-Pass Filters are
presented which achieve exactly the same results and which are –
contrary to my above statements – well built into PhotoShop and possibly
other paint programs, too. So if you're a dedicated user of your
favourite paint program, you'll probably want to give the built-in
high-pass filter a try before trying my home-grown solution. 😉

.. |image0| image:: /images/textures/perfectdetailmap-original.png
   :class: mediaright
   :width: 200px
   :height: 200px
.. |perfectdetailmap-originalscreen.jpg| image:: /images/textures/perfectdetailmap-originalscreen.jpg
   :class: mediaright
   :width: 200px
   :height: 200px
.. |image2| image:: /images/textures/perfectdetailmap-blurred60.png
   :class: media
   :width: 200px
   :height: 200px
.. |image3| image:: /images/textures/perfectdetailmap-result60.png
   :class: media
   :width: 200px
   :height: 200px
.. |perfectdetailmap-result60screen.jpg| image:: /images/textures/perfectdetailmap-result60screen.jpg
   :class: media
   :width: 200px
   :height: 200px
