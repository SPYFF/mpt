/**
 * @file
 * @brief Public domain implementations of in-place string trim functions
 * @author Michael Burr michael.burr@nth-element.com
 * @date 2010
*/


#include <ctype.h>
#include <string.h>

/**
 * Strip whitespace from the beginning of a string
 */
char* ltrim(char* s)
{
    char* newstart = s;

    while (isspace( *newstart)) {
        ++newstart;
    }

    // newstart points to first non-whitespace char (which might be '\0')
    memmove( s, newstart, strlen( newstart) + 1); // don't forget to move the end '\0'

    return s;
}

/**
 * Strip whitespace from the end of a string
 */
char* rtrim( char* s)
{
    char* end = s + strlen( s);

    // find the last non-whitespace character
    while ((end != s) && isspace( *(end-1))) {
            --end;
    }

    // at this point either (end == s) and s is either empty or all whitespace
    //      so it needs to be made empty, or
    //      end points just past the last non-whitespace character (it might point
    //      at the '\0' terminator, in which case there's no problem writing
    //      another there).
    *end = '\0';

    return s;
}

/**
 * Strip whitespace from the beginning and end of a string
 */
char*  trim( char* s)
{
    return rtrim( ltrim( s));
}


/**
 * Strip down path and get filename
 * @author Kelemen Tamas
 */
char * basename(char * string)
{
    char * p = string;
    char * ret;
    while(*(p++)) {
        if(*p == '/') ret = p+1;
    }
    return ret;
}

