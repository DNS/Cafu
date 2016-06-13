/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************************/
/*** Text Parser (Header) ***/
/****************************/

#ifndef CAFU_TEXT_PARSER_HPP_INCLUDED
#define CAFU_TEXT_PARSER_HPP_INCLUDED

#include "Templates/Array.hpp"

#include <string>

/// This is a class for parsing text. It has the following features:
/// a) C++ style comments ("// ...") are recognized.
/// b) Quoted tokens are recognized (ie. "1 2 3" is returned as ONE token).
class TextParserT
{
    public:

    /// Error when parsing a text/file.
    class ParseError { };

    /// The constructor.
    /// If IsFileName=true,  Input specifies the name of the input file.
    /// If IsFileName=false, Input is interpreted as the input string itself.
    /// If the constructor experiences any problems (e.g. the input file cannot be read), it creates a text parser for the text of length 0 (the empty text).
    /// Delims specifies token delimiters that are returned as separate tokens even if they are not bordered by white-space.
    /// CommentChar specifies the character that starts a comment. The default 0 makes "//" being recognized as comment start.
    /// If CommentChar has another value, that character is considered as initiating a comment.
    /// @param Input An input string or the filename of an input file depending on IsFileName.
    /// @param Delims Specifies token delimiters that are returned as separate tokens even if they are not bordered by white-space.
    /// @param IsFileName Whether the input string is a real string to parse or the name of a file to parse.
    /// @param CommentChar Char that initiates a comment line (default is '//').
    TextParserT(const char* Input, std::string Delims="", bool IsFileName=true, const char CommentChar='\0');

    /// Returns the next token. ParseError is thrown when an error is encountered (e.g. EOF), after which the parsing cannot be continued.
    /// @throw ParseError
    std::string GetNextToken();

    /// Returns the next token as an int. ParseError is thrown when an error is encountered (e.g. EOF), after which the parsing cannot be continued.
    /// @throw ParseError
    int GetNextTokenAsInt();

    /// Returns the next token as a float. ParseError is thrown when an error is encountered (e.g. EOF), after which the parsing cannot be continued.
    /// @throw ParseError
    float GetNextTokenAsFloat();

    /// Puts back the string Token, such that the next call to GetNextToken() returns Token.
    /// This function can be called multiple times in sequence, GetNextToken() will return all put back tokens in a stack-like manner.
    /// No checks are performed if Token was actually returned by a previous call by GetNextToken().
    /// @param Token The string to put back in the string parsed by TextParserT.
    void PutBack(const std::string& Token);

    /// Returns a peek at the next token without reading over it.
    /// This is equivalent to:  "std::string T=GetNextToken(); PutBack(T); return T;".
    /// @throw ParseError
    std::string PeekNextToken();

    /// Makes sure that the next token is equal to Token. Otherwise, a ParseError is thrown.
    /// This is short for:  "if (TP.GetNextToken()!=Token) throw TextParserT::ParseError();".
    /// @param Token The string asserted as the next token.
    /// @throw ParseError
    void AssertAndSkipToken(const std::string& Token);

    /// Returns whether the last read "real" token was a "quoted" token.
    ///
    /// "Real" token means that this method doesn't take into account tokens that have been put back with the PutBack() method.
    /// That is, when more than one token has been put back at a time (i.e. PutBack() has been called two times in a row without
    /// an intermediate call to one of the Get...() methods), the result of this method is not reliable and should not be used.
    ///
    /// @returns whether the last read "real" token was a "quoted" token.
    bool WasLastTokenQuoted() const { return LastTokenWasQuoted; }

    /// Skips tokens until the end of the current line of text. This method always succeeds (it never throws).
    /// @returns the rest (skipped portion) of the line, preceeded by previously put back or peeked tokens.
    ///     (Note that previously put back or peeked tokens are returned as well. The whitespace that is used
    ///      to separate these tokens is however not guaranteed to be identical with the original input text.)
    std::string SkipLine();

    /// Skips a whole "block" of tokens, e.g. a { ... } or ( ... ) block. The block can contain nested blocks.
    /// It can (and must) be stated if the caller has already read the opening token.
    /// @param OpeningToken Opening token of the block.
    /// @param ClosingToken Closing token of the block.
    /// @param CallerAlreadyReadOpeningToken The opening token has already been read by the caller.
    /// @returns the (possibly multi-line) content of the skipped block.
    std::string SkipBlock(const std::string& OpeningToken, const std::string& ClosingToken, bool CallerAlreadyReadOpeningToken);

    /// Returns the current read position in the input file in byte.
    /// Tokens that have been put back are not taken into account.
    unsigned long GetReadPosByte() const;

    /// Returns the current read position in the input file in percent.
    /// Tokens that have been put back are not taken into account.
    float GetReadPosPercent() const;

    /// Returns whether the parser has reached the EOF or not.
    bool IsAtEOF() const;


    private:

    ArrayT<char>        TextBuffer;
    std::string         Delimiters;
    char                CommentInitChar;
    unsigned long       BeginOfToken;
    unsigned long       EndOfToken;
    ArrayT<std::string> PutBackTokens;
    bool                LastTokenWasQuoted;

    bool IsCharInDelimiters(const char c) const;
};

#endif
