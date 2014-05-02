When the CaWE application is started with the `--update-doxygen` switch,
it automatically generates "template" header files in this -initially empty-
directory.

These header files contain as much documentation for our scripting classes
as can automatically be generated.

They are intended to help documentation writers to keep the actual header
files in `src/` up-to-date (e.g. by using a tool like BeyondCompare for
synchronization).

The actual scripting reference documentation is created by Doxygen from the
contents of `src/`.
