/**
 * @mainpage C++ Source Code Documentation
 *
 * @section overview Overview
 *
 * Welcome to the <em>Cafu Engine C++ Source Code Documentation</em>.
 *
 * This reference documentation describes the classes, functions, namespaces and other elements of the C++ source code in detail.
 *
 * If you're new to Cafu, the documentation in this manual might look overwhelming at first:
 *
 *   - For an overview and introduction to Cafu, see the documentation at http://www.cafu.de/wiki/
 *   - The Developer Resources page is at http://www.cafu.de/developers
 *   - If you're looking for the \emph{scripting} reference documentation instead, see http://docs.cafu.de/lua/
 *
 *
 * @section libsandapps Libraries and Applications
 *
 * Cafu consists of a set of closely related C++ \emph{programming libraries} that together form the C++ API
 * that you can use to create your own applications.
 *
 * In addition, Cafu also has a set of closely related \emph{applications}, especially the Cafu Engine, the map compilation programs
 * and the Cafu World Editor, that you are encouraged to customize and re-use for your own projects.
 * These applications are part of the C++ API and documented in this manual as well.
 *
 *
 * @section multisub Multiple Implementations for Sub-Systems
 *
 * Some of the Cafu sub-systems come with multiple, alternative implementations:
 *
 *   - For example, the Material System comes with an OpenGL 1.2 implementation that is used as a fall-back for old hardware,
 *     and several newer implementations that employ more modern features of OpenGL.
 *   - Another example is the Sound System, that comes with an implementation based on the FMOD library
 *     and another one that is based on OpenAL.
 *
 * In such cases, in order to avoid confusion from bringing in multiple classes with identical names,
 * only \emph{one} of these sub-systems and its documentation is contained here,
 * usually the one that is the most informative (and the best documented).
 */
