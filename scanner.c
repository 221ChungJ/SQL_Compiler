/*scanner.c*/

//
// Scanner for SimpleSQL programming language. The scanner 
// reads the input stream and turns the characters into 
// language Tokens, such as identifiers, keywords, and 
// punctuation.
//
// Jeremy Chung
// Northwestern University
// CS 211, Winter 2023
//
// Contributing author: Prof. Joe Hummel
//

#include <stdio.h>
#include <stdbool.h>  // true, false
#include <ctype.h>    // isspace, isdigit, isalpha
#include <string.h>   // stricmp

#include "util.h"
#include "scanner.h"


//
// SimpleSQL keywords, in alphabetical order. Note that "static"  
// means the array / variable is not accessible outside this file, 
// which is intentional and good design.
//
static char* keywords[] = { "asc", "avg", "by", "count", "delete", 
  "desc", "from", "inner", "insert", "intersect", "into", "join", 
  "like", "limit", "max", "min", "on", "order", "select", "set", 
  "sum", "union", "update", "values", "where" };

static int numKeywords = sizeof(keywords) / sizeof(keywords[0]);


//
// scanner_init
//
// Initializes line number, column number, and value before
// the start of the next input sequence. 
//
void scanner_init(int* lineNumber, int* colNumber, char* value)
{
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_init)");

  *lineNumber = 1;
  *colNumber  = 0;
  value[0]    = '\0';  // empty string ""
}

static int isKeyword(char* value){
  for(int i = 0; i < numKeywords; i++){
    if(strcasecmp(keywords[i], value) == 0){
      return SQL_KEYW_ASC + i; 
    }
  } 
  return SQL_IDENTIFIER; 
}


//
// scanner_nextToken
//
// Returns the next token in the given input stream, advancing the line
// number and column number as appropriate. The token's string-based 
// value is returned via the "value" parameter. For example, if the 
// token returned is an integer literal, then the value returned is
// the actual literal in string form, e.g. "123". For an identifer,
// the value is the identifer itself, e.g. "ID" or "title". For a 
// string literal, the value is the contents of the string literal
// without the quotes.
//
struct Token scanner_nextToken(FILE* input, int* lineNumber, int* colNumber, char* value)
{
  if (input == NULL)
    panic("input stream is NULL (scanner_nextToken)");
  if (lineNumber == NULL || colNumber == NULL || value == NULL)
    panic("one or more parameters are NULL (scanner_nextToken)");

  struct Token T;

  



  //
  // repeatedly input characters one by one until a token is found:
  //
  while (true)
  {
    int c = fgetc(input);

    //col number manegment

    
    if (c == EOF)  // no more input, return EOS:
    {
      *colNumber += 1;
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '$';
      value[1] = '\0';

      return T;
    }
    else if(c == 10){
      *lineNumber += 1; 
      *colNumber = 0;
    }
    else if (c == '\''){
      *colNumber += 1;
      T.line = *lineNumber;
      T.col = *colNumber;
      
      c = fgetc(input);
      
      int counter = 0;
      while(true){
        if(c == '\''){
          *colNumber += 1;
          break; 
        }
        value[counter] = c; 
        counter++; 
        *colNumber += 1;
  
        if(c == '\n'){
          printf("**WARNING: string literal @ (%d, %d) not terminated properly.\n", *lineNumber, *colNumber);
          value[counter] = '\0';
          T.id = SQL_STR_LITERAL; 
          return T; 
        }
        
        c = fgetc(input);
      }
      value[counter] = '\0';
      T.id = SQL_STR_LITERAL; 
      return T; 
    }
     else if (c == '\"'){
      *colNumber += 1;
      T.line = *lineNumber;
      T.col = *colNumber;
       
      c = fgetc(input);
      
      int counter = 0;
      while(true){
        if(c == '\"'){
          *colNumber += 1;
          break; 
        }
        value[counter] = c; 
        counter++; 
        *colNumber += 1;
  
        if(c == '\n'){
          printf("**WARNING: string literal @ (%d, %d) not terminated properly.\n", *lineNumber, *colNumber);
          value[counter] = '\0';
          T.id = SQL_STR_LITERAL; 
          return T; 
        }
        
        c = fgetc(input);
      }
      value[counter] = '\0';
      T.id = SQL_STR_LITERAL; 
      return T; 
    }
    else if (isdigit(c) || c == '+' || c == '-'){
      *colNumber += 1;
      T.line = *lineNumber; 
      T.col = *colNumber;

      bool con = true; 
      
      int counter = 0; 
      value[counter] = c; 
      counter++; 
      
      if(c == '-'){
        c = fgetc(input); 
        if(isspace(c)){
            if(c == '\n'){
              T.id = SQL_UNKNOWN;
              T.line = *lineNumber;
              T.col = *colNumber;
      
              value[0] = '-';
              value[1] = '\0';
              
              *lineNumber += 1;
              *colNumber = 0;
                
              return T;
            }
            T.id = SQL_UNKNOWN;
            *colNumber += 1;
          
            value[0] = '-';
            value[1] = '\0';
    
            return T;
        }
        if(c == '-'){
          while(c != '\n'){
            c = fgetc(input);
            *colNumber = -1;
          }
          *lineNumber += 1;
          *colNumber = 0; 
          value[0] = '\0'; 
          con = false; 
        }
      }
      else if(c == '+'){
        c = fgetc(input); 
        if(!isdigit(c)){
          if(c == '\n'){
            T.id = SQL_UNKNOWN;
            T.line = *lineNumber;
            T.col = *colNumber;
    
            value[0] = '+';
            value[1] = '\0';
            
            *lineNumber += 1;
            *colNumber = 0;
              
            return T;
          }
          T.id = SQL_UNKNOWN;
          T.line = *lineNumber;
          T.col = *colNumber;
  
          value[0] = '+';
          value[1] = '\0';
  
          return T;
        }
      }
      else{
        c = fgetc(input);
      }
      if(con){
        bool hasDecimal = false; 
      
        while(true){
          if(!isdigit(c)){
            if(c == '.'){
              if(hasDecimal){
                ungetc(c, input);
                break;
              }
              else{
                hasDecimal = true;
              }
            }
            else{
              ungetc(c, input);
              break; 
            }
          }
          value[counter] = c; 
          counter++; 
          c = fgetc(input);
          *colNumber += 1;
          
        }
  
        value[counter] = '\0';
        if(hasDecimal){
          T.id = SQL_REAL_LITERAL;
        }
        else{
          T.id = SQL_INT_LITERAL; 
        }
        return T;
      }
      
    }
    else if (isalpha(c)){
      *colNumber += 1;
      T.line = *lineNumber; 
      T.col = *colNumber; 

      char tempWord[256] = ""; 
      int length = 0; 

      
      while(true){
        if(isalnum(c) || c == '_'){
          char ch = c; 
          strncat(tempWord, &ch, 1);
          length++; 
          *colNumber += 1;
          c = fgetc(input);  
        }
        else{
          ungetc(c, input);
          break; 
        }
      }
      
      T.id = isKeyword(tempWord);
      for (int i = 0; i < length; i++){
        value[i] = tempWord[i]; 
      }
      *colNumber -= 1;
      value[length] = '\0'; 
      return T; 
    }
    else if (c == '(')  
    {
      *colNumber += 1;
      T.id = SQL_LEFT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ')')  
    {
      *colNumber += 1;
      T.id = SQL_RIGHT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '*')  
    {
      *colNumber += 1;
      T.id = SQL_ASTERISK;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '.')  
    {
      *colNumber += 1;
      T.id = SQL_DOT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '#')  
    {
      *colNumber += 1;
      T.id = SQL_HASH;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ',')  
    {
      *colNumber += 1;
      T.id = SQL_COMMA;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '=')  
    {
      *colNumber += 1;
      T.id = SQL_EQUAL;
      T.line = *lineNumber;
      T.col = *colNumber;
      

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '<')  
    {
      //
      // peek ahead to the next char:
      //
      *colNumber += 1;
      c = fgetc(input);

      if (c == '=')
      {
        T.id = SQL_LTE;
        T.line = *lineNumber;
        T.col = *colNumber;
        *colNumber += 1;

      
        value[0] = '<';
        value[1] = '=';
        value[2] = '\0';

        return T;
      }
      else if (c == '>'){
        T.id = SQL_NOT_EQUAL;
        T.line = *lineNumber;
        T.col = *colNumber;
        *colNumber += 1;

        
        value[0] = '<';
        value[1] = '>';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not form a token, so 
      // we need to put char back to be processed on next call:
      //
      ungetc(c, input);

      T.id = SQL_LT;
      T.line = *lineNumber;
      T.col = *colNumber;
      


      value[0] = '<';
      value[1] = '\0';

      return T;
    }



      
    else if (c == '$')  // this is also EOF / EOS
    {
      *colNumber += 1;
      T.id = SQL_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ';')
    {
      *colNumber += 1;
      T.id = SQL_SEMI_COLON;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '>')  // could be > or >=
    {
      //
      // peek ahead to the next char:
      //
      *colNumber += 1;
      c = fgetc(input);

      if (c == '=')
      {
        T.id = SQL_GTE;
        T.line = *lineNumber;
        T.col = *colNumber;
        *colNumber += 1;

        
        value[0] = '>';
        value[1] = '=';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not form a token, so 
      // we need to put char back to be processed on next call:
      //
      ungetc(c, input);
      T.id = SQL_GT;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '>';
      value[1] = '\0';

      return T;
    }
    else if (isspace(c) && c != 10){
      *colNumber += 1;
    }
    else
    {
      //
      // if we get here, then char denotes an UNKNOWN token:
      //
      *colNumber += 1;
      T.id = SQL_UNKNOWN;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }

  }//while

  //
  // execution should never get here, return occurs
  // from within loop
  //
}
