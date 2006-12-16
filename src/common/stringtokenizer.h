//==========================================================================
//  STRINGTOKENIZER.H - part of
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//
//  Declaration of the following classes:
//    StringTokenizer  : string tokenizer utility class
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2005 Andras Varga

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#ifndef __STRINGTOKENIZER_H
#define __STRINGTOKENIZER_H

#include <string>
#include <vector>

/**
 * String tokenizer class, based on strtok(). The separator can be specified.
 * Does *not* honor quoted substrings, quotation marks are treated just like 
 * any other character.
 *
 * Example usage:
 *
 * <pre>
 * const char *str = "34 42 13 46 72 41"; // input
 * std::vector<int> numbers;  // array to hold result
 *
 * StringTokenizer tokenizer(str);
 * const char *token;
 * while ((token = tokenizer.nextToken())!=NULL)
 *     numbers.push_back(atoi(token));
 * </pre>
 *
 * @ingroup SimSupport
 */
class StringTokenizer
{
  private:
    char *str; // copy of full string
    char *rest; // rest of string (to be tokenized)
    char *strend; // points to terminating zero of str
    std::string delimiter;

  public:
    /**
     * Constructor. The class will make its own copy of the input string
     * and of the delimiters string.
     */
    StringTokenizer(const char *str, const char *delimiters=" ");

    /**
     * Destructor.
     */
    ~StringTokenizer();

    /**
     * Change delimiters. This allows for switching delimiters during
     * tokenization.
     */
    void setDelimiter(const char *s);

    /**
     * Returns true if there's more token (the next nextToken() call won't return NULL).
     */
    bool hasMoreTokens()  {return rest!=NULL;}

    /**
     * Returns the next token. The returned pointers will stay valid as long
     * as the tokenizer object exists. If there're no more tokens,
     * a NULL pointer will be returned.
     */
    const char *nextToken();

    /**
     * Utility function: tokenizes the full input string at once, and
     * returns the tokens in a string vector.
     */
    std::vector<std::string> asVector();
};

#endif

