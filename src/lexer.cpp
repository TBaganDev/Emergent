#include "lexer.hpp"
#include <algorithm>
#include <string.h>
#include <string>
#include <map>
#include <charconv>
static std::string identifier, string;    
static float decimal;    
static int integer, line, column;


static int prev = ' ';
static int next = ' ';

// Moves pointer forward, storing it in prev.
void nextToken(FILE *file) {
  column++;
  prev = getc(file);
}

// Stores the character ahead of the pointer in next.
void lookAhead(FILE *file) {
    column++;
    next = getc(file);
}

// Iterates through all digits, returning the string.
std::string iterDigits(FILE *file) {
  std::string number;
  do {
    number += prev;
    nextToken(file);
  } while(isdigit(prev));
  return number;
}

lexer::TOKEN lexer::getToken(FILE *file) {
  // Moves pointer over all whitespace.
  while(isspace(prev)) {
    if(prev == '\r' || prev == '\n') {
      line++;
      column = 0;
    }
    nextToken(file);
  }

  // Indicates a single character
  if(prev == '\'') {
    nextToken(file);
    std::string character(1, prev);
    nextToken(file);
    if(prev != '\'') {
      std::string extra(1, prev);
      character = character + extra;
      // Erroneous Token Identified
      nextToken(file);
      return returnToken(character, ERROR);
    }
    nextToken(file);
    return returnToken(character, CHAR);
  }

  // Indicates a keyword or id is next.
  if(isalpha(prev) || (prev == '_')) {
    identifier = prev;
    nextToken(file);

    while(isalnum(prev) || prev == '_') {
      identifier += prev;
      nextToken(file);
    }

    // Maps the read string to the Token keyword.
    std::map<std::string, lexer::TOKEN_TYPE> keywords;
    keywords["neighbourhood"] = NEIGHBOURHOOD;
    keywords["model"] = MODEL;
    keywords["state"] = STATE;
    keywords["set"] = SET;
    keywords["all"] = ALL;
    keywords["default"] = DEFAULT;
    keywords["this"] = THIS;
    keywords["in"] = IN;
    keywords["and"] = AND;
    keywords["or"] = OR;
    keywords["xor"] = XOR;
    keywords["not"] = NOT;

    std::map<std::string, lexer::TOKEN_TYPE>::iterator it = keywords.find(identifier);
    if(it != keywords.end()) {
      return returnToken(identifier.c_str(), it->second);
    }
    // If no keyword is matched, must be an identifier.
    return returnToken(identifier.c_str(), ID);
  }

  if(isdigit(prev)) {
    // Returns the digits before the point.
    std::string wholes = iterDigits(file);
    
    integer = strtod(wholes.c_str(), nullptr);

    if(prev == '.') {
        // Returns digits after the point.
        std::string fractions = iterDigits(file);
        decimal = strtof(fractions.c_str(), nullptr) + integer;
        return returnToken(wholes + '.' + fractions, DEC_LIT);
    } else {
        // Must be whole number, as there is no point.
        return returnToken(wholes, NAT_LIT);
    }
  }

  if(prev == '.') {
    // Assumed to be fraction <1.
    std::string fractions = iterDigits(file);
    decimal = strtof(fractions.c_str(), nullptr);
    return returnToken(fractions, DEC_LIT);
  }

  if(prev == '=') {
    lookAhead(file);
    if (next == '=') {
      nextToken(file);
      return returnToken("==", EQ);
    } else {
      prev = next;
    }
  }

  if(prev == '!') {
    lookAhead(file);
    if (next == '=') {
      nextToken(file);
      return returnToken("!=", NE);
    } else {
      prev = next;
    }
  }

  if(prev == '<') {
    lookAhead(file);
    if (next == '=') {
      nextToken(file);
      return returnToken("<=", LE);
    } else {
      prev = next;
      return returnToken("<", LT);
    }
  }

  if (prev == '>') {
    lookAhead(file);
    if (next == '=') {
      nextToken(file);
      return returnToken(">=", GE);
    } else {
      prev = next;
      return returnToken(">", GT);
    }
  }
  

  if (prev == '/') {
    nextToken(file);
    if (prev == '/') {
      // Treats the rest of the line like whitespace.
      do {
        nextToken(file);
      } while (!(prev == '\n' || prev == '\r' || prev == EOF));

      if (prev != EOF) {
        return getToken(file);
      } else {
        return returnToken("EOF",END_OF_FILE);
      }
    }
    return returnToken("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (prev == EOF) {
    column++;
    return returnToken("EOF", END_OF_FILE);
  }
  // If no other cases suit, return the character as its ascii value.
  int character = prev;
  nextToken(file);
  std::string char_string(1, character);
  return returnToken(char_string, (lexer::TOKEN_TYPE) character);
}

void lexer::resetLexer() {
  line = 1;
  column = 1;
}

static lexer::TOKEN lexer::returnToken(std::string lexeme, lexer::TOKEN_TYPE type) {
  lexer::TOKEN resulting_token = {
    type,
    lexeme,
    line,
    column - (int) (lexeme.length()) - 1
  };
  return resulting_token;
}