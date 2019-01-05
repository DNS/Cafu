.. _becoming_an_editor:

Becoming an Editor
==================

In order to be able to create and edit pages at the Cafu Wiki, you have
to login via the ``Login`` button at the bottom of the Cafu Wiki pages.

In order to get a username and passwort for the login, follow these
steps:

-  Register yourself at the Cafu forum at http://forum.cafu.de
   That means that your login data for the forum and the Wiki are in
   fact *shared*, the same login data will work with both the forum and
   the Wiki. Therefore, if you are already a registered forum user,
   there is no need to register a second time.
-  Login to the forum to make sure that your username and password work
   correctly.
-  Your login details will immediately work also at the Wiki login.

If you're a Wiki beginner - notes for new editors
-------------------------------------------------

Editing the Cafu Wiki pages is very easy, but there are a few concepts
to get used to:

-  Please read (or at least glance at) the `DokuWiki
   Manual <http://wiki.splitbrain.org/wiki:manual>`__. The Cafu Wiki is
   implemented with the DokuWiki software, and this is the related
   manual.
-  If you don't want to read the entire manual, you should at least read
   http://wiki.splitbrain.org/wiki:syntax
-  New pages are created by first creating new links to them. Clicking
   on such a link whose target page does not yet exist will
   automatically offer you the choice to create the contents for that
   page.
-  **NAMESPACES** are very important! They're used to properly structure
   all contents within this Wiki. Please *make sure* that you read the
   section about namespaces at
   http://wiki.splitbrain.org/wiki:namespaces
-  Create new links (that is, pages) within their proper namespace. If
   for example you want to add a new tutorial for CaWE, add a link to
   the :ref:`The Cafu Documentation <the_cafu_documentation>` page with
   link code like
   ::

        [[Mapping:CaWE:Tutorials:MyCoolTut]]  

-  How do you know which namespace should be used? Look at the link
   names of the other pages at the
   :ref:`The Cafu Documentation <the_cafu_documentation>` page. Their
   names including their namespace are shown when you hover with the
   mouse over the links to other pages. Also the **Index** page (see the
   button at the bottom right of every page) is very helpful!

Why is all that talk about namespaces so important? Well, links (and
thus, pages) cannot easily be renamed later. While it is not impossible,
renaming has the tencency to break references from other pages to the
renamed document, which in fact can be cumbersome. So some foresight can
save a lot of trouble later.
