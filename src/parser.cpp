#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <iostream>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace lexer;
using namespace ast;

FILE *source;
static std::deque<TOKEN> token_buffer;
static TOKEN token;

bool parser::openFile(char * filename) {
  resetLexer();
  source = fopen(filename, "r");
  return source == NULL;
}

void parser::closeFile() {
  fclose(source);
}

void nextToken() {
  if(token_buffer.empty()) {
    token_buffer.push_back(getTok(source));
  }

  TOKEN temp = token_buffer.front();
  token_buffer.pop_front();
  token = temp;
  return;
}

void prevToken(TOKEN tok) {
  token_buffer.push_front(token);
  token = tok;
}

void ParsingError(const char *string) {
  fprintf(stderr, "Parsing Error:\n")
  fprintf(stderr, ">>> Expected %s.\n", string);
  fprintf(stderr, "Instead Got: \'%s\'.\n", token.lexeme.c_str());
  fprintf(stderr, "On Line %d, ", token.lineNo);
  fprintf(stderr, "On Column %d\n", token.columnNo);
}

std::shared_ptr<Node> parser::BinaryParsing(
  std::shared_ptr<Node> *(parse_function)(),
  std::vector<TOKEN_TYPE> first_set,
  std::vector<TOKEN_TYPE> follow_set,
  std::string error_message
) {
  auto left = parse_function();
  if(!left) {
    return nullptr;
  }
  TOKEN temp = token;
  nextToken(); //Lookahead required to determine if next token in follow set or first set
  if(std::find(first_set.begin(), first_set.end(), token.type) != first_set.end()) {
    lexer::TOKEN_TYPE op = token.type;
    nextToken();
    auto right = parse_function();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<Expression>(left, op, right);
  } else if(std::find(follow_set.begin(), follow_set.end(), token.type) != follow_set.end()) {
    prevToken(temp);
    return left;
  }

  ParsingError(error_message);
  return nullptr;
}

std::shared_ptr<Node> parser::SeriesParsing(
  std::shared_ptr<Node> *(parse_function)(),
  std::vector<TOKEN_TYPE> first_set,
  std::vector<TOKEN_TYPE> follow_set,
  std::string error_message,
  TOKEN_TYPE seperator
) {
  std::vector<std::shared_ptr<Node>> series;
  TOKEN temp;
  do {
    auto item = parse_function();
    if(!item) {
      return nullptr;
    }
    series.push_back(std::move(item));
    //Lookahead required to determine if next token in follow set or first set
    nextToken(); 
    if(seperator != ERROR) {
      if(token.type != seperator) {
        break;
      }
      nextToken();
    }
    temp = token;
  } while(std::find(first_set.begin(), first_set.end(), token.type) != first_set.end());
  
  if(std::find(follow_set.begin(), follow_set.end(), token.type) != follow_set.end()) {
    prevToken(temp);
    return std::make_shared<Series>(std::move(series));
  }

  ParsingError(error_message);
  return nullptr;
};

std::shared_ptr<Node> parser::ParseProgram() {
  nextToken();
  std::vector<std::shared_ptr<Node>> models;
  std::vector<std::shared_ptr<Node>> neighbourhoods;
  do {
    if(token.type == MODEL) {
      auto model = ParseModel();
      if(!model) {
        return nullptr;
      }
      models.push_back(std::move(model));
    } else if (token.type == NEIGHBOURHOOD) {
      auto neighbourhood = ParseNeighbourhood();
      if(!neighbourhood) {
        return nullptr;
      }
      neighbourhoods.push_back(std::move(neighbourhood));
    }
    nextToken();
  } while(token.type == MODEL || token.type == NEIGHBOURHOOD);
  if(models.empty() && neighbourhoods.empty()) {
    ParsingError("\'model\' or \'neighbourhood\'");
    return nullptr;
  }
  auto models_series = std::make_shared<Series>(models);
  auto neighbourhood_series = std::make_shared<Series>(neighbourhoods);
  return std::make_shared<Program>(models_series, neighbourhood_series);
}

std::shared_ptr<Node> parser::ParseModel() {
  if(token.type != MODEL) {
    ParsingError("\'model\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("identifier");
    return nullptr;
  }
  std::string model_id = token.lexeme;
  nextToken();
  if(token.type != COLON) {
    ParsingError("\':\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("identifier");
    return nullptr;
  }
  std::string neighbourhood_id = token.lexeme;
  nextToken();
  if(token.type != LBRACE) {
    ParsingError("\'{\'");
    return nullptr;
  }
  nextToken();
  auto states = ParseStates();
  if(!states) {
    return nullptr;
  }
  nextToken();
  if(token.type != RBRACE) {
    ParsingError("\'}\'");
    return nullptr;
  }
  return std::make_shared<Model>(model_id, neighbourhood_id, states);
}

std::shared_ptr<Node> parser::ParseNeighbourhood() {
  if(token.type != NEIGHBOURHOOD) {
    ParsingError("\'neighbourhood\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("identifier");
    return nullptr;
  }
  std::string id = token.lexeme;
  nextToken();
  if(token.type != COLON) {
    ParsingError("\':\'");
    return nullptr;
  }
  nextToken();
  if(token.type != INT_LIT) {
    ParsingError("integer literal");
    return nullptr;
  }
  int dimensions = std::stoi(token.lexeme);
  nextToken();
  if(token.type != LBRACE) {
    ParsingError("\'{\'");
    return nullptr;
  }
  nextToken();
  auto neighbours = ParseNeighbours();
  if(!neighbours) {
    return nullptr;
  }
  nextToken();
  if(token.type != RBRACE) {
    ParsingError("\'}\'");
    return nullptr;
  }
  return std::make_shared<Neighbourhood>(id, dimensions, neighbours);
}

std::shared_ptr<Node> parser::ParseNeighbours() {
  vector<TOKEN_TYPE> first_set = {COMMA};
  vector<TOKEN_TYPE> follow_set = {RBRACE};
  return SeriesParsing(parser::ParseNeighbour,first_set, follow_set, "\'}\'",ERROR);
}

std::shared_ptr<Node> parser::ParseNeighbour() {
  std::string id;
  if(token.type == ID) {
    id = token.lexeme;
  }
  nextToken();
  auto coordinate = ParseCoordinate();
  if(!coordinate) {
    return nullptr;
  }
  return std::make_shared<Coordinate>(id, coordinate);
}

std::shared_ptr<Node> parser::ParseStates() {
  vector<TOKEN_TYPE> first_set = {DEFAULT, STATE};
  vector<TOKEN_TYPE> follow_set = {RBRACE};
  return SeriesParsing(parser::ParseState, first_set, follow_set, "\'}\'", ERROR);
}

std::shared_ptr<Node> parser::ParseState() {
  if(token.type == DEFAULT) {
    nextToken();
    if(token.type != STATE) {
      ParsingError("\'state\'");
      return nullptr;
    }
    nextToken();
    if(token.type != ID) {
      ParsingError("identifier");
      return nullptr;
    }
    return std::make_shared<State>(true, token.lexeme);
  } else if(token.type == STATE) {
    nextToken();
    if(token.type != ID) {
      ParsingError("identifier");
      return nullptr;
    }
    std::string id = token.lexeme;
  }
  ParsingError("\'default\' or \'state\'");
  return nullptr;
}

std::shared_ptr<Node> parser::ParsePredicate() {
  vector<TOKEN_TYPE> first_set = {OR};
  vector<TOKEN_TYPE> follow_set = {RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, "\'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseExDisjunction() {
  vector<TOKEN_TYPE> first_set = {XOR};
  vector<TOKEN_TYPE> follow_set = {OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, 
    "\'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseConjucation() {
  vector<TOKEN_TYPE> first_set = {AND};
  vector<TOKEN_TYPE> follow_set = {XOR, OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, 
    "\'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseEquivalence() {
  vector<TOKEN_TYPE> first_set = {EQ, NE};
  vector<TOKEN_TYPE> follow_set = {AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set,
    "\'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseRelation() {
  vector<TOKEN_TYPE> first_set = {LE, LT, GE, GT};
  vector<TOKEN_TYPE> follow_set = {EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, 
    "\'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseTranslation() {
  vector<TOKEN_TYPE> first_set = {ADD, SUB};
  vector<TOKEN_TYPE> follow_set = {LE, LT, GE, GT, EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, 
    "\'<=\', \'<\', \'>=\', \'>\', \'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseScaling() {
  vector<TOKEN_TYPE> first_set = {MULT, DIV, MOD};
  vector<TOKEN_TYPE> follow_set = {ADD, SUB, LE, LT, GE, GT, EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return BinaryParsing(parser::ParseExDisjunction(), first_set, follow_set, 
    "\'-\', \'+\', \'<=\', \'<\', \'>=\', \'>\', \'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}


std::shared_ptr<Node> parser::ParseElement() {
  if(token.type == SUB || token.type == NOT) {
    // Negation or Negative
    TOKEN_TYPE type = token.type;
    nextToken();
    auto element = ParseElement();
    if(!element)) {
      return nullptr;
    }
    if(type == SUB) {
      return std::make_shared<Negative>(element);
    }
    return std::make_shared<Negation>(element);
  }
  if(token.type == LPAREN) {
    // Sub-Expression
    nextToken();
    auto predicate = ParsePredicate();
    if(!predicate) {
      return nullptr;
    }
    if(token.type != RPAREN) {
      ParsingError("\')\'");
      return nullptr;
    }
    return predicate; 
  }
  if(token.type == PIPE) {
    // Cardinality
    nextToken();
    if(token.type != SET) {
      auto set = ParseSet();
      if(!set) {
        return nullptr;
      }
      nextToken();
      if(token.type != PIPE) {
        ParsingError("\'|\'");
        return nullptr;
      }
      return set;
    }
  }
  switch(token.type) {
    case LSQUAR: return ParseCoordinate();
    case INT_LIT: return std::make_shared<Integer>(std::stoi(token.lexeme));
    case DEC_LIT: return std::make_shared<Decimal>(std::stof(token.lexeme));
    case ID:
    case THIS: return std::make_shared<Identifier>(token.lexeme);
  }
  ParsingError("\'-\', \'not\', \'(\', \'[\', \'|\', \'this\', identifier, integer literal or decimal literal");
  return nullptr;
}

std::shared_ptr<Node> parser::ParseSet() {
  if(token.type != SET) {
    ParsingError("\'set\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("\'identifier\'");
    return nullptr;
  }
  std::string variable = token.lexeme;
  nextToken();
  if(token.type != IN) {
    ParsingError("\'in\'");
    return nullptr;
  }
  nextToken();
  if(token.type != COLON) {
    ParsingError("\':\'");
    return nullptr;
  }  
  nextToken();
  std::shared_ptr<Node> coordinates;
  if(token.type == ALL) {
    coordinates = std::make_shared<Series>();
  } else {
    coordinates = ParseCoordinates();
    if(!coordinates) {
      return nullptr;
    }
  }
  nextToken();
  if(token.type != COLON) {
    ParsingError("\':\'");
    return nullptr;
  }
  nextToken();
  auto predicate = ParsePredicate();
  if(!predicate) {
    return nullptr;
  }
  return std::make_shared<Cardinality>(variable, coordinates, predicate);
}

std::shared_ptr<Node> parser::ParseCoordinate() {
  if(token.type != LSQUAR) {
    ParsingError("\'[\'");
    return nullptr;
  }
  nextToken();
  auto vector = ParseVector();
  if(!vector) {
    return nullptr;
  }
  nextToken();
  if(token.type != RSQUAR) {
    ParsingError("\']\'");
    return nullptr;
  }
  return std::make_shared<Coordinate>(vector);
}

// Function performs the parse for embedded integer literals in series.
std::shared_ptr<Node> ParseInteger() {
  if(token.type != INT_LIT) {
    ParsingError("integer literal");
    return nullptr;
  }
  return std::make_shared<Integer>(std::stoi(token.lexeme));
}

std::shared_ptr<Node> parser::ParseVector() {
  std::vector<TOKEN_TYPE> first_set = {INT_LIT};
  std::vector<TOKEN_TYPE> follow_set = {RBRACE};
  return SeriesParsing(ParseInteger, first_set, follow_set, "\'}\'", COMMA);
}

std::shared_ptr<Node> parser::ParseCoordinates() {
  std::vector<TOKEN_TYPE> first_set = {LSQUAR};
  std::vector<TOKEN_TYPE> follow_set = {COLON};
  return SeriesParsing(parser::ParseCoordinate, first_set, follow_set, "\':\'",COMMA);
}