.. _coding_conventions:

Coding Conventions
==================

This chapter outlines the basic styles and patterns that are used
throughout the Cafu source code.

These guidelines intend to achieve uniform code that is easy to read and
maintain by many persons, and should be applied to all newly written
code. Old code that does not comply to these rules can still be found in
some places, and should be updated to conform to these guidelines on
convenient occasions.

Of course, the guidelines described herein are not meant to be
exhaustive, and breaking one of these rules in well justified cases
(usually to increase readability) is an option.

Control Structures
------------------

With respect to brackets positioning, indentation and whitespace,
control structures are laid out like this:

.. code:: cpp

   if (...)
   {
       ...;
   }
   else if (...)
   {
       ...;
   }
   else
   {
       ...;
   }
    
   for (...; ...; ...)
   {
       ...;
   }
    
   while (...)
   {
       ...;
   }
    
   do
   {
       ...;
   }
   while (...);
    
   switch (...)
   {
       case 1: ...; break;
       case 2: ...; break;
       case 3: ...; break;
    
       case 4:
           ...;
           break;
    
       case 5:
       {
           // Extra parentheses are needed for declaring local variables here.
           ...;
           break;
       }
    
       default:
           ...;
           break;
   }
    
   // "if" variants that are also OK (similar applies to "for" and "while").
   if (...)            // Omit unnecessary brackets when there is only a single statement.
       ...;
    
   if (...) ...;       // Optionally omit the line break.
    
   if (...) ...;       // Special vertical alignment.
       else ...;
    
        if (...) ...;  // Another special vertical alignment (rare).
   else if (...) ...;
   else          ...;

-  Indentation is **always 4 spaces** (never a TAB **(!)**).
-  Braces are always vertically aligned and each is on a line of its
   own.
-  Note the space in ``if (``, ``for (``, ``while (``, etc.
-  Note the space after the semicolons in the ``for`` statement.
-  There is one empty line after the closing brace of a multi-line
   control structure.
-  Omit unnecessary braces (but beware of cascaded ``if`` statements!).

Classes
-------

Class declarations follow this pattern:

.. code:: cpp

   class MyClassT : public Base
   {
       public:
    
       MyClassT(float x_=0);
    
       float GetX() const { return x; }
    
    
       private:
    
       float x;
   };

-  Note the ``T`` suffix (short for “Type”) at the end of the class
   name, indicating a custom type.
-  Classes that are used as interfaces (ABCs (abstract base classes),
   pure-virtual classes) are suffixed by a capital ``I`` (short for
   “Interface”) instead.
-  Definition of methods in-line is OK if the method is short (as
   ``GetX()`` above). Most methods should instead be defined as
   described below.

Methods and Functions
---------------------

The definition of methods and functions follows this pattern:

.. code:: cpp

   MyClassT::MyClassT(float x_) : Base(...), x(x_)
   {
       ...;
   }
    
    
   // Alternative constructor definition (preferred!).
   MyClassT::MyClassT(float x_)
       : Base(...),
         x(x_),       // Note that indentation is now really 6 spaces.
         y(0)         // This is because vertical alignment takes precedence in this case.
   {
       ...;
   }
    
    
   float MyClassT::GetX() const
   {
       return x;
   }

-  Note the **two** blank lines between each function definition.

Whitespace
----------

As a general rule, use whitespace as you'd use it in written English
prose.

That is, do *not* use whitespace

-  with unary operators (e.g. ``-``, ``!``, ``~``, ``++``, ``–`` etc.),
-  with binary arithmetic operators (e.g. ``+``, ``*``, etc.),
-  with comparison operators (e.g. ``==``, ``>``, ``>=``, etc.),
-  with other operators like ``=`` (assignment), ``::`` (scope
   resolution), ``->`` (member by pointer), etc.

Not using whitespace around the above mentioned operators (all but
unary) works best when your code editor supports syntax highlighting
that renders the operators in a style or color that is different from
that of the operands. If your code editor doesn't support highlighting
of operators and/or using a space before and after the operator improves
the readability of the code, using such whitespace is fine, too.

*Use* whitespace

-  with logical and bitwise operators (e.g. ``&&``, ``||``, ``&``,
   ``|``, ``^`` etc.),
-  with the ternary conditional operator (``cond ? a : b``),
-  after commas (e.g. in lists) and semicolons (e.g. in for-loops),
-  after keywords (e.g. ``if``, ``for``, ``while`` etc.).

**(!)** Whitespace must only ever consist of SPACE characters, never of
TABs. Setup your text editor to automatically replace one press of the
TAB key with the insertion of four SPACE characters. Avoid whitespace at
the end of lines, setup your text editor to remove it automatically.
Files must end with exactly *one* newline character.

Indentation
-----------

For the indentation of blocks of code, we use the “Allman” or
“university” style, where the opening and closing brackets are
vertically aligned and each bracket is on a separate line as shown in
all examples above and below. The enclosed block of code is always
indented by four spaces (*never* by a TAB character!).

Example:

.. code:: cpp

   bool IsValidTime(unsigned long Hours, unsigned long Minutes, unsigned long Seconds)
   {
       if (Hours<24 && Minutes<60 && Seconds<60)
       {
           // Yes, this is a valid time.
           return true;
       }
       else
       {
           // No, this is invalid.
           return false;
       }
   }

Of course, in real code this trivial example would be expressed as

.. code:: cpp

   bool IsValidTime(unsigned long Hours, unsigned long Minutes, unsigned long Seconds)
   {
       return Hours<24 && Minutes<60 && Seconds<60;
   }

which is both much shorter and more readable.

Vertical Alignment
------------------

Make use of vertical alignment. Obviously,

.. code:: cpp

       std::string Search     []={ "mo",     "tu",      "we" };
       std::string Replacement[]={ "Monday", "Tuesday", "Wednesday" };

is more expressive and suggestive than

.. code:: cpp

       std::string Search[]={ "mo", "tu", "we" };
       std::string Replacement[]={ "Monday", "Tuesday", "Wednesday" };

Parentheses
-----------

Use parentheses whenever necessary to clarify the operator precedence.
Even if a set of parentheses is redundant with respect to the definition
of the C++ language, use them e.g. when whitespace alone is insufficient
or you had to lookup the proper precedence of not-so-frequently used
operators in literature.

Both of these examples are fine:

.. code:: cpp

   bool IsValidTime(unsigned long Hours, unsigned long Minutes, unsigned long Seconds)
   {
       // Okay: Whitespace (and programmer knowledge) alone make operator precedence clear.
       // This is the preferred variant.
       return Hours<24 && Minutes<60 && Seconds<60;
   }
    
   bool IsValidTime2(unsigned long Hours, unsigned long Minutes, unsigned long Seconds)
   {
       // Also okay: (redundant) parentheses make operator precedence explicitly clear.
       return (Hours<24) && (Minutes<60) && (Seconds<60);
   }

Avoid “null” parentheses, though. For example

.. code:: cpp

       return (Hours<24 && Minutes<60 && Seconds<60);

in the above examples should rarely be written.

Comments
--------

All comments are to be written in the English language as error-free as
possible, with the proper or at least best possible spelling, grammar
and punctuation. Prefer C++ comment styles over C comments:

.. code:: cpp

       int yes=1;    // Use // to initiate most comments.
       int no =0;    /* Do NOT write C-sytle one-line comments like this. */

We use `Doxygen <http://www.stack.nl/~dimitri/doxygen/>`__ for writing
code documentation. Doxygen comments are to be written “inline”, that
is, into the header files with the declarations. Within Doxygen
comments, we use the JavaDoc style with the ``JAVADOC_AUTOBRIEF`` option
set to ``YES``. Prefer /// for multi-line comments above the declaration
and ///< for short single-line comments trailing the declaration.

Conclusions
-----------

The above presented coding conventions and guidelines are to achieve a
uniform and readable code style. Exceptions should be kept to a minimum,
but are allowed whenever they increase the readability.

References
----------

These references give general information about coding conventions,
listed here to help improving this section. The coding conventions that
they describe may differ from the conventions established here, and are
not meant to apply to Cafu.

-  http://de.wikipedia.org/wiki/Quelltextformatierung
-  http://de.wikipedia.org/wiki/Programmierstil
-  http://de.wikipedia.org/wiki/Einr%C3%BCckungsstil
-  http://en.wikipedia.org/wiki/Code_convention
