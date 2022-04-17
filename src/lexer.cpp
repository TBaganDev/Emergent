#include "lexer.hpp"
#include <algorithm>
#include <string.h>
#include <string>
#include <map>

static std::string identifier, string;    
static float decimal;    
static int integer, line, column;

lexer::TOKEN lexer::getToken(FILE *file) {
  int prev = ' ';
  int next = ' ';

  // Moves pointer forward, storing it in prev.
  auto nextToken = [&] () {
    column++;
    prev = getc(file);
  };

  // Stores the character ahead of the pointer in next.
  auto lookahead = [&] () {
    column++;
    next = getc(file);
  };

  // Iterates through all digits, returning the strings.
  auto iter_digits = [&] () {
    std::string number;
    do {
      number += prev;
      nextToken();
    } while (isdigit(prev));
    return number;
  };



  // Moves pointer over all whitespace.
  while(isspace(prev)) {
    if(prev == '\r' || prev == '\n') {
      line++;
      column = 1;
    }
    nextToken();
  }

  // Indicates a keyword or id is next.
  if(isalpha(prev) || (prev == '_')) {
    identifier = prev;
    nextToken();

    while(isalnum(prev) || prev == '_') {
      identifier += prev;
      nextToken();
    }

    // Maps the read string to the Token keyword.
    std::map<std::string, lexer::TOKEN_TYPE> keywords;
    keywords["neighbourhood"] = NEIGHBOURHOOD;
    keywords["model"] = MODEL;
    keywords["state"] = STATE;
    keywords["any"] = ANY;
    keywords["some"] = SOME;
    keywords["of"] = OF;
    keywords["all"] = ALL;
    keywords["default"] = DEFAULT;
    keywords["and"] = AND;
    keywords["or"] = OR;
    keywords["xor"] = XOR;
    keywords["not"] = NOT;
    keywords["implies"] = IMPLY;

    std::map<std::string, lexer::TOKEN_TYPE>::iterator it = keywords.find(identifier);
    if(it != keywords.end()) {
      return returnToken(identifier.c_str(), it->second);
    }
    // If no keyword is matched, must be an identifier.
    return returnToken(identifier.c_str(), ID);
  }

  if(prev == ':') {
    nextToken();
    return returnToken(":", COLON);
  }
  if(prev == '{') {
    nextToken();
    return returnToken("{", LBRACE);
  }
  if(prev == '}') {
    nextToken();
    return returnToken("}", RBRACE);
  }
  if(prev == '(') {
    nextToken();
    return returnToken("(", LPAREN);
  }
  if(prev == ')') {
    nextToken();
    return returnToken(")", RPAREN);
  }
  if(prev == ',') {
    nextToken();
    return returnToken(",", COMMA);
  }
  if(prev == '[') {
    nextToken();
    return returnToken("[", LSQUAR);
  }
  if(prev == ']') {
    nextToken();
    return returnToken("]", RSQUAR);
  }
  if(prev == '|') {
    nextToken();
    return returnToken("|", PIPE);
  }

  if(isdigit(prev)) {
    // Returns the digits before the point.
    std::string wholes = iter_digits();
    integer = strtod(wholes.c_str(), nullptr);

    if(prev == '.') {
        // Returns digits after the point.
        std::string fractions = iter_digits();
        decimal = strtof(fractions.c_str(), nullptr) + integer;
        return returnToken(wholes + '.' + fractions, DEC_LIT);
    } else {
        // Must be whole number, as there is no point.
        return returnToken(wholes, INT_LIT);
    }
  }

  if(prev == '.') {
    // Assumed to be fraction <1.
    std::string fractions = iter_digits();
    decimal = strtof(fractions.c_str(), nullptr);
    return returnToken(fractions, DEC_LIT);
  }

  if(prev == '=') {
    lookahead();
    if (next == '=') {
      nextToken();
      return returnToken("==", EQ);
    } else {
      prev = next;
    }
  }

  if(prev == '!') {
    lookahead();
    if (next == '=') {
      nextToken();
      return returnToken("!=", NE);
    } else {
      prev = next;
    }
  }

  if(prev == '<') {
    lookahead();
    if (next == '=') {
      nextToken();
      return returnToken("<=", LE);
    } else {
      prev = next;
      return returnToken("<", LT);
    }
  }

  if (prev == '>') {
    lookahead();
    if (next == '=') {
      nextToken();
      return returnToken(">=", GE);
    } else {
      prev = next;
      return returnToken(">", GT);
    }
  }

  if (prev == '/') {
    nextToken();
    if (prev == '/') {
      // Treats the rest of the line like whitespace.
      do {
        nextToken();
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
  std::string char_string(1, character);
  nextToken();
  return returnToken(char_string, (lexer::TOKEN_TYPE) character);
}

void lexer::resetLexer() {
  line = 1;
  column = 1;
}

static lexer::TOKEN lexer::returnToken(std::string lexeme, lexer::TOKEN_TYPE type) {
  lexer::TOKEN token = {
    type,
    lexeme,
    line,
    column - (int) (lexeme.length()) - 1
  };
  return token;
}