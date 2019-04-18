.. _how_to_submit_patches:

How to Submit Patches
=====================

Cafu is to a community project and we need and very much appreciate your
help. Your contributions and especially patches are very important for
Cafu. Patches help us to add new features, improve code quality and fix
bugs, so we are happy and grateful if you contribute them. 😀

Changing Cafu
-------------

Please read the :ref:`Coding Conventions <coding_conventions>` and try
to conform to them. In particular, please respect the indentation rules
(4 spaces, no TABs) – patches are really difficult to read otherwise.

Provide documentation
^^^^^^^^^^^^^^^^^^^^^

Bug fixes and elegant program solutions often involve complex code that
can be difficult to understand without documentation, and an
undocumented new feature is hardly useful to anyone but its author.

Therefore, please provide any required documentation such as source code
comments (preferably in familiar and simple
`Doxygen <http://www.doxygen.org>`__ format), high-level overviews,
diagrams, etc. Without documentation, another developer would have to
write it, and the patch cannot be applied until he has time to do it.

Make atomic patches
^^^^^^^^^^^^^^^^^^^

A patch should be self-contained – *one patch for one thing*.

Do not combine multiple new features in a single patch: A patch that
adds bitmaps to menu items and fixes a bug in the network code is a bad
patch. It should be splitted into two patches.

On the other hand, do not split a single code change into multiple
patches: Two patches, one of them being “implementation of new
member-functions”, the other “changes in class documentation to
accommodate new members” are two bad patches. They are related to one,
logically indivisible thing, so they should be in one common patch.

Final Considerations
--------------------

#. On Dive-In, we need “How to contribute” / “Fork me on Bitbucket”.
#. Why Your Project Doesn't Need a Contributor Licensing Agreement,
   http://ebb.org/bkuhn/blog/2014/06/09/do-not-need-cla.html
#. https://kernel.org/doc/html/latest/process/submitting-patches.html
