#pragma once

#include <string.h>
#include <string>

namespace lexer {
  // Type of Token noted within the Grammar.
  enum TOKEN_TYPE {
    // Special Tokens
    END_OF_FILE = 0, // End of file
    ERROR = -100,    // Erroneous Token 
    ID = -1,      // Identifier [a-zA-Z_][a-zA-Z_0-9]*

    // Keywords
    NEIGHBOURHOOD = -2, // Neighbourhood "neighbourhood"
    MODEL = -3,         // Model "model"
    STATE = -4,         // State "state"
    SET = -5,           // Collection/Set "set"
    CELL = -6,          // Cell iterated over "cell"
    ALL = -7,           // All Neighbours "all"
    DEFAULT = -8,       // Default Tag "default"
    THIS = -9,          // Cell pointed to "this"

    // Literals
    INT_LIT = -10, // Integer [0-9]+
    DEC_LIT = -11, // Rational [0-9]+.[0-9]+

    // Logical operators
    AND = -12,   // And 'and'
    OR = -13,    // Or 'or'
    XOR = -14,   // Exclusive Or 'xor'
    NOT = -15,   // Negation 'not'
    IMPLY = -16, // Implication 'implies'

    // Comparison Operators
    EQ = -17,      // Equal "=="
    NE = -18,      // Not Equal "!="
    LE = -19,      // Less than or Equal to "<="
    LT = int('<'), // Less than "<"
    GE = -20,      // Greater than or Equal to ">="
    GT = int('>'), // Greater than ">"

    // Numeric Operations
    ADD = int('+'),  // Addition "+"
    SUB = int('-'),  // Substraction "-"
    MULT = int('*'), // Multiplication "*"
    DIV = int('/'),  // Division "/"
    MOD = int('%'),  // Modular (getting the remainer) "%"

    // Delimeters (Scope Control)
    COLON = int(':'),  // Colon ":"
    LBRACE = int('{'), // Left Brace "{"
    RBRACE = int('}'), // Right brace "}"
    LPAREN = int('('), // Left Parenthesis "("
    RPAREN = int(')'), // Right parenthesis ")"
    COMMA = int(','),  // Comma ","
    LSQUAR = int('['), // Left Square Bracket "["
    RSQUAR = int(']'), // Right Square Bracket "]"
    PIPE = int('|'),   // Pipe (Cardinality) "|"
  };

  // Stores data related to each Token.
  struct TOKEN {
    int type = ERROR;
    std::string lexeme;
    int line;
    int column;
  };

  // Returns the next lexer token from the source input.
  TOKEN getToken(FILE *file);
  // Resets the lexer's pointer.
  void resetLexer();
  // Generates and returns a Token given the current lexer's state.
  static TOKEN returnToken(std::string lexeme, TOKEN_TYPE type);
}
