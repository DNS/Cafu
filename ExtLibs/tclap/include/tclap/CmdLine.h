// -*- Mode: c++; c-basic-offset: 4; tab-width: 4; -*-

/******************************************************************************
 *
 *  file:  CmdLine.h
 *
 *  Copyright (c) 2003, Michael E. Smoot .
 *  Copyright (c) 2004, Michael E. Smoot, Daniel Aarno.
 *  All rights reverved.
 *
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#ifndef TCLAP_CMDLINE_H
#define TCLAP_CMDLINE_H

#include <tclap/SwitchArg.h>
#include <tclap/MultiSwitchArg.h>
#include <tclap/UnlabeledValueArg.h>
#include <tclap/UnlabeledMultiArg.h>

#include <tclap/XorHandler.h>
#include <tclap/HelpVisitor.h>
#include <tclap/VersionVisitor.h>
#include <tclap/IgnoreRestVisitor.h>

#include <tclap/CmdLineOutput.h>

#include <tclap/Constraint.h>
#include <tclap/ValuesConstraint.h>

#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace TCLAP
{

template<typename T> void DelPtr(T ptr)
{
    delete ptr;
}

template<typename C> void ClearContainer(C& c)
{
    typedef typename C::value_type value_type;
    std::for_each(c.begin(), c.end(), DelPtr<value_type>);
    c.clear();
}


/**
 * The base class that manages the command line definition and passes
 * along the parsing to the appropriate Arg classes.
 */
class CmdLine : public CmdLineInterface
{
  protected:

    /**
     * The list of arguments that will be tested against the
     * command line.
     */
    std::list<Arg*> _argList;

    /**
     * The name of the program.  Set to argv[0].
     */
    std::string _progName;

    /**
     * A message used to describe the program.  Used in the usage output.
     */
    std::string _message;

    /**
     * The version to be displayed with the --version switch.
     */
    std::string _version;

    /**
     * The number of arguments that are required to be present on
     * the command line. This is set dynamically, based on the
     * Args added to the CmdLine object.
     */
    int _numRequired;

    /**
     * The character that is used to separate the argument flag/name
     * from the value.  Defaults to ' ' (space).
     */
    char _delimiter;

    /**
     * The handler that manages xoring lists of args.
     */
    XorHandler _xorHandler;

    /**
     * A list of Args to be explicitly deleted when the destructor
     * is called.  At the moment, this only includes the three default
     * Args.
     */
    std::list<Arg*> _argDeleteOnExitList;

    /**
     * A list of Visitors to be explicitly deleted when the destructor
     * is called.  At the moment, these are the Vistors created for the
     * default Args.
     */
    std::list<Visitor*> _visitorDeleteOnExitList;

    /**
     * Object that handles all output for the CmdLine.
     */
    CmdLineOutput& _output;

    /**
     * Throws an exception listing the missing args.
     */
    void missingArgsException();

    /**
     * Checks whether a name/flag string matches entirely matches
     * the Arg::blankChar.  Used when multiple switches are combined
     * into a single argument.
     * \param s - The message to be used in the usage.
     */
    bool _emptyCombined(const std::string& s);

    /**
     * Perform a delete ptr; operation on ptr when this object is deleted.
     */
    void deleteOnExit(Arg* ptr);

    /**
     * Perform a delete ptr; operation on ptr when this object is deleted.
     */
    void deleteOnExit(Visitor* ptr);

  private:

    /**
     * Prevent accidental copying.
     */
    CmdLine(const CmdLine& rhs);
    CmdLine& operator=(const CmdLine& rhs);

  public:

    /**
     * Command line constructor. Defines how the arguments will be
     * parsed.
     * \param message - The message to be used in the usage
     * output.
     * \param delimiter - The character that is used to separate
     * the argument flag/name from the value.  Defaults to ' ' (space).
     * \param version - The version number to be used in the
     * --version switch.
     * \param helpAndVersion - Whether or not to create the Help and
     * Version switches. Defaults to true.
     */
    CmdLine(const std::string& message,
            CmdLineOutput& output,
            const char delimiter = ' ',
            const std::string& version = "none");

    /**
     * Deletes any resources allocated by a CmdLine object.
     */
    virtual ~CmdLine();

    /**
     * Adds an argument to the list of arguments to be parsed.
     * \param a - Argument to be added.
     */
    void add( Arg& a );

    /**
     * An alternative add.  Functionally identical.
     * \param a - Argument to be added.
     */
    void add( Arg* a );

    /**
     * Add two Args that will be xor'd.  If this method is used, add does
     * not need to be called.
     * \param a - Argument to be added and xor'd.
     * \param b - Argument to be added and xor'd.
     */
    void xorAdd( Arg& a, Arg& b );

    /**
     * Add a list of Args that will be xor'd.  If this method is used,
     * add does not need to be called.
     * \param xors - List of Args to be added and xor'd.
     */
    void xorAdd( std::vector<Arg*>& xors );

    /**
     * Parses the command line.
     * \param argc - Number of arguments.
     * \param argv - Array of arguments.
     */
    void parse(int argc, const char* const* argv);

    /**
     * Parses the command line.
     * \param args - A vector of strings representing the args.
     * args[0] is still the program name.
     */
    void parse(std::vector<std::string>& args);

    const CmdLineOutput& getOutput() const { return _output; }
    const std::string& getVersion() const override { return _version; }
    const std::string& getProgramName() const override { return _progName; }
    const std::list<Arg*>& getArgList() const override { return _argList; }
    const XorHandler& getXorHandler() const override { return _xorHandler; }
    char getDelimiter() const override { return _delimiter; }
    const std::string& getMessage() const override { return _message; }

    /**
     * Allows the CmdLine object to be reused.
     */
    void reset();

};


///////////////////////////////////////////////////////////////////////////////
//Begin CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////

inline CmdLine::CmdLine(const std::string& m,
                        CmdLineOutput& output,
                        char delim,
                        const std::string& v)
    :
    _argList(std::list<Arg * >()),
    _progName("not_set_yet"),
    _message(m),
    _version(v),
    _numRequired(0),
    _delimiter(delim),
    _xorHandler(XorHandler()),
    _argDeleteOnExitList(std::list<Arg * >()),
    _visitorDeleteOnExitList(std::list<Visitor * >()),
    _output(output)
{
    Arg::setDelimiter( _delimiter );
}

inline CmdLine::~CmdLine()
{
    ClearContainer(_argDeleteOnExitList);
    ClearContainer(_visitorDeleteOnExitList);
}

inline void CmdLine::xorAdd( std::vector<Arg*>& ors )
{
    _xorHandler.add( ors );

    for (ArgVectorIterator it = ors.begin(); it != ors.end(); it++)
    {
        (*it)->forceRequired();
        (*it)->setRequireLabel( "OR required" );
        add( *it );
    }
}

inline void CmdLine::xorAdd( Arg& a, Arg& b )
{
    std::vector<Arg*> ors;
    ors.push_back( &a );
    ors.push_back( &b );
    xorAdd( ors );
}

inline void CmdLine::add( Arg& a )
{
    add( &a );
}

inline void CmdLine::add( Arg* a )
{
    for ( ArgListIterator it = _argList.begin(); it != _argList.end(); it++ )
        if ( *a == *(*it) )
            throw( SpecificationException(
                       "Argument with same flag/name already exists!",
                       a->longID() ) );

    _argList.push_back(a);

    if ( a->isRequired() )
        _numRequired++;
}

inline void CmdLine::parse(int argc, const char* const* argv)
{
    // this step is necessary so that we have easy access to
    // mutable strings.
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i]);

    parse(args);
}

inline void CmdLine::parse(std::vector<std::string>& args)
{
    _progName = args.front();
    args.erase(args.begin());

    int requiredCount = 0;

    for (int i = 0; static_cast<unsigned int>(i) < args.size(); i++)
    {
        bool matched = false;

        for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
        {
            if ( (*it)->processArg( &i, args ) )
            {
                requiredCount += _xorHandler.check( *it );
                matched = true;
                break;
            }
        }

        // checks to see if the argument is an empty combined
        // switch and if so, then we've actually matched it
        if ( !matched && _emptyCombined( args[i] ) )
            matched = true;

        if ( !matched && !Arg::ignoreRest() )
            throw(CmdLineParseException("Couldn't find match "
                                        "for argument",
                                        args[i]));
    }

    if ( requiredCount < _numRequired )
        missingArgsException();

    if ( requiredCount > _numRequired )
        throw(CmdLineParseException("Too many arguments!"));
}

inline bool CmdLine::_emptyCombined(const std::string& s)
{
    if ( s.length() > 0 && s[0] != Arg::flagStartChar() )
        return false;

    for ( int i = 1; static_cast<unsigned int>(i) < s.length(); i++ )
        if ( s[i] != Arg::blankChar() )
            return false;

    return true;
}

inline void CmdLine::missingArgsException()
{
    int count = 0;

    std::string missingArgList;
    for (ArgListIterator it = _argList.begin(); it != _argList.end(); it++)
    {
        if ( (*it)->isRequired() && !(*it)->isSet() )
        {
            missingArgList += (*it)->getName();
            missingArgList += ", ";
            count++;
        }
    }
    missingArgList = missingArgList.substr(0, missingArgList.length() - 2);

    std::string msg;
    if ( count > 1 )
        msg = "Required arguments missing: ";
    else
        msg = "Required argument missing: ";

    msg += missingArgList;

    throw (CmdLineParseException(msg));
}

inline void CmdLine::deleteOnExit(Arg* ptr)
{
    _argDeleteOnExitList.push_back(ptr);
}

inline void CmdLine::deleteOnExit(Visitor* ptr)
{
    _visitorDeleteOnExitList.push_back(ptr);
}

inline void CmdLine::reset()
{
    for ( ArgListIterator it = _argList.begin(); it != _argList.end(); it++ )
        (*it)->reset();

    _progName.clear();
}

///////////////////////////////////////////////////////////////////////////////
//End CmdLine.cpp
///////////////////////////////////////////////////////////////////////////////



} //namespace TCLAP
#endif
