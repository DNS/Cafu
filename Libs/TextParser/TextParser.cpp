/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**************************/
/*** Text Parser (Code) ***/
/**************************/

#include <stdio.h>
#include <sstream>
#include "TextParser.hpp"


TextParserT::TextParserT(const char* Input, std::string Delims, bool IsFileName, char CommentChar)
    : Delimiters(Delims),
      CommentInitChar(CommentChar),
      BeginOfToken((unsigned long)-1),
      EndOfToken((unsigned long)-1),
      LastTokenWasQuoted(false)
{
    if (Input)
    {
        if (IsFileName)
        {
            FILE* ParseTextFile=fopen(Input, "rb");

            if (ParseTextFile)
            {
                const int InPos       =ftell(ParseTextFile); fseek(ParseTextFile,     0, SEEK_END);
                const int InFileLength=ftell(ParseTextFile); fseek(ParseTextFile, InPos, SEEK_SET);

                if (InFileLength>0)
                {
                    // If InFileLength==0, what can happen with 0-byte large files,
                    // the access to &TextBuffer[0] throws an index-out-of-bounds exception.
                    TextBuffer.PushBackEmpty(InFileLength);
                    if (fread(&TextBuffer[0], 1, InFileLength, ParseTextFile)==0) { }   // Must check the return value of fread() with GCC 4.3...
                }

                fclose(ParseTextFile);
            }
        }
        else
        {
            for (unsigned long CNr=0; Input[CNr]; CNr++)
                TextBuffer.PushBack(Input[CNr]);
        }
    }

    // Make sure that TextBuffer is zero-terminated.
    TextBuffer.PushBack(0);
}


bool TextParserT::IsCharInDelimiters(const char c) const
{
    const char* Set=Delimiters.c_str();

    while (*Set)
    {
        if (c==*Set) return true;
        Set++;
    }

    return false;
}


// Philosophy: This method does NEVER alter the TextBuffer.
// BeginOfToken is the first character of the current token, EndOfToken is the last one. Period.
std::string TextParserT::GetNextToken() /*throw (ParseError)*/
{
    if (PutBackTokens.Size()>0)
    {
        std::string Token=PutBackTokens[PutBackTokens.Size()-1];

        PutBackTokens.DeleteBack();
        return Token;
    }

    LastTokenWasQuoted=false;


    // Skip space
    BeginOfToken=EndOfToken+1;

    while (true)
    {
        while (BeginOfToken<TextBuffer.Size())
        {
            if (TextBuffer[BeginOfToken]>32) break;
            BeginOfToken++;
        }
        if (BeginOfToken>=TextBuffer.Size()) throw ParseError();

        const char c=TextBuffer[BeginOfToken];

        if (CommentInitChar!=0)
        {
            // A custom character that indicates the begin of a comment was specified.
            if (c!=CommentInitChar) break;                  // This token is not the beginning of a comment.
        }
        else
        {
            if (c!='/') break;                              // This token is not the beginning of a comment.
            if (BeginOfToken+1>=TextBuffer.Size()) break;   // A '/' as the last character in the file is no comment either.
            if (TextBuffer[BeginOfToken+1]!='/') break;     // A '/' followed by a character other than '/' is no comment either.
        }

        // Skip rest of line
        while (BeginOfToken<TextBuffer.Size())
        {
            if (TextBuffer[BeginOfToken]=='\n') break;
            BeginOfToken++;
        }
        if (BeginOfToken>=TextBuffer.Size()) throw ParseError();
    }


    // Check for tokens that are not delimited by the usual whitespace characters, but by '"'
    if (TextBuffer[BeginOfToken]=='"')
    {
        EndOfToken=BeginOfToken+1;

        while (EndOfToken<TextBuffer.Size())
        {
            if (TextBuffer[EndOfToken]=='"') break;
            if (TextBuffer[EndOfToken]=='\n') throw ParseError();   // Quoted token crosses end of line!
            EndOfToken++;
        }
        if (EndOfToken>=TextBuffer.Size()) throw ParseError();      // Quoted token until EOF!

        LastTokenWasQuoted=true;
        return std::string(&TextBuffer[BeginOfToken+1], EndOfToken-BeginOfToken-1);
    }


    // Check for tokens that are delimiters.
    if (IsCharInDelimiters(TextBuffer[BeginOfToken]))
    {
        EndOfToken=BeginOfToken;
        return std::string(&TextBuffer[BeginOfToken], 1);
    }


    // Find end of regular token.
    EndOfToken=BeginOfToken+1;

    while (EndOfToken<TextBuffer.Size())
    {
        if (TextBuffer[EndOfToken]<=32) break;
        if (IsCharInDelimiters(TextBuffer[EndOfToken])) break;
        EndOfToken++;
    }
    // EndOfToken past EOF? Should never happen, because we added a trailing 0 at the end of TextBuffer in the constructor.
    if (EndOfToken>=TextBuffer.Size()) throw ParseError();

    EndOfToken--;
    return std::string(&TextBuffer[BeginOfToken], EndOfToken-BeginOfToken+1);
}


int TextParserT::GetNextTokenAsInt() /*throw (ParseError)*/
{
    int i;
    std::istringstream iss(GetNextToken());

    iss >> i;

    return i;
}


float TextParserT::GetNextTokenAsFloat() /*throw (ParseError)*/
{
#if defined(_MSC_VER) && (_MSC_VER <= 1900)     // 1900 == Visual C++ 14.0 (2015)
    const std::string s = GetNextToken();

    // There is a bug in Microsoft's iostream implementation up to Visual C++ 2015,
    // see http://trac.cafu.de/ticket/150 for details.
    return float(atof(s.c_str()));
#else
    float f;
    std::istringstream iss(GetNextToken());

    iss >> f;

    return f;
#endif
}


void TextParserT::PutBack(const std::string& Token)
{
    PutBackTokens.PushBack(Token);
}


std::string TextParserT::PeekNextToken()
{
    std::string Peek=GetNextToken();
    PutBack(Peek);
    return Peek;
}


void TextParserT::AssertAndSkipToken(const std::string& Token)
{
    if (GetNextToken()!=Token) throw ParseError();
}


std::string TextParserT::SkipLine()
{
    std::string Result;

    // Initialize with all previously put back tokens.
    while (PutBackTokens.Size()>0)
    {
        Result+=PutBackTokens[PutBackTokens.Size()-1];
        Result+=" ";
        PutBackTokens.DeleteBack();
    }

    // If there are no more characters in TextBuffer, bail out early.
    if (EndOfToken+1>=TextBuffer.Size()) return Result;

    BeginOfToken=EndOfToken+1;
    EndOfToken  =BeginOfToken;

    while (true)
    {
        if (TextBuffer[EndOfToken]=='\n') break;
        if (EndOfToken+1>=TextBuffer.Size()) break;

        EndOfToken++;
    }

    return Result + std::string(&TextBuffer[BeginOfToken], EndOfToken-BeginOfToken+1);
}


std::string TextParserT::SkipBlock(const std::string& OpeningToken, const std::string& ClosingToken, bool CallerAlreadyReadOpeningToken)
{
    if (!CallerAlreadyReadOpeningToken) GetNextToken();

    unsigned long StartPos=EndOfToken+1;    // The character after the OpeningToken.
    unsigned long NestedLevel=1;

    while (true)
    {
        const std::string Token=GetNextToken();

        if (Token==OpeningToken)
        {
            NestedLevel++;
        }
        else if (Token==ClosingToken)
        {
            NestedLevel--;
            if (NestedLevel==0) break;
        }
    }

    unsigned long EndPos=BeginOfToken-1;    // The character before the ClosingToken.

    // Skip any initial newline characters (not no other kind of white-space).
    while ((TextBuffer[StartPos]=='\n' || TextBuffer[StartPos]=='\r') && StartPos<=EndPos) StartPos++;

    return std::string(&TextBuffer[StartPos], EndPos-StartPos+1);
}


unsigned long TextParserT::GetReadPosByte() const
{
    return EndOfToken+1;
}


float TextParserT::GetReadPosPercent() const
{
    return float(EndOfToken+1)/float(TextBuffer.Size());
}


bool TextParserT::IsAtEOF() const
{
    if (PutBackTokens.Size()>0) return false;

    // Skip space
    unsigned long TestToken=EndOfToken+1;

    while (true)
    {
        while (TestToken<TextBuffer.Size())
        {
            if (TextBuffer[TestToken]>32) break;
            TestToken++;
        }
        if (TestToken>=TextBuffer.Size()) return true;

        const char c=TextBuffer[TestToken];

        if (CommentInitChar!=0)
        {
            // A custom character that indicates the begin of a comment was specified.
            if (c!=CommentInitChar) break;                  // This token is not the beginning of a comment.
        }
        else
        {
            if (c!='/') break;                          // This token is not the beginning of a comment.
            if (TestToken+1>=TextBuffer.Size()) break;  // A '/' as the last character in the file is no comment either.
            if (TextBuffer[TestToken+1]!='/') break;    // A '/' followed by a character other than '/' is no comment either.
        }

        // Skip rest of line
        while (TestToken<TextBuffer.Size())
        {
            if (TextBuffer[TestToken]=='\n') break;
            TestToken++;
        }
        if (TestToken>=TextBuffer.Size()) return true;
    }

    return false;   // Not yet at EOF.
}
