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
using namespace parser;

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

// Pops the next token from the stack.
void nextToken() {
  if(token_buffer.empty()) {
    token_buffer.push_back(getToken(source));
  }

  TOKEN temp = token_buffer.front();
  token_buffer.pop_front();
  token = temp;
  return;
}

// Pushes back the previous token, resetting the token to prev argument.
void prevToken(TOKEN prev) {
  token_buffer.push_front(token);
  token = prev;
}

// Outputs parsing error to terminal.
void ParsingError(std::string caller, std::string error) {
  fprintf(stderr, "Parsing Error: %s\n", caller.c_str());
  fprintf(stderr, ">>> Expected %s.\n", error.c_str());
  fprintf(stderr, "Instead Got: \'%s\'.\n", token.lexeme.c_str());
  fprintf(stderr, "On Line %d, ", token.line);
  fprintf(stderr, "On Column %d\n", token.column);
}

std::shared_ptr<Node> parser::BinaryParsing(
  std::shared_ptr<Node> (parse_function)(),
  std::vector<TOKEN_TYPE> first_set,
  std::vector<TOKEN_TYPE> follow_set,
  std::string error_message
) {
  auto left =  parse_function();
  if(!left) {
    return nullptr;
  }
  TOKEN temp = token;
  nextToken(); //Lookahead required to determine if next token in follow set or first set
  if(std::find(first_set.begin(), first_set.end(), token.type) != first_set.end()) {
    TOKEN_TYPE op = (TOKEN_TYPE) token.type;
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

  ParsingError("Binary", error_message);
  return nullptr;
}

std::shared_ptr<Node> parser::SeriesParsing(
  std::shared_ptr<Node> (parse_function)(),
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
    temp = token;
    nextToken();
    if(seperator != ERROR) {
      if(seperator != (TOKEN_TYPE) token.type) {
        break;
      }
      nextToken();
      temp = token;
    }
  } while(std::find(first_set.begin(), first_set.end(), token.type) != first_set.end());
  
  if(std::find(follow_set.begin(), follow_set.end(), token.type) != follow_set.end()) {
    prevToken(temp);
    return std::make_shared<Series>(std::move(series));
  }
  prevToken(temp);
  ParsingError("Series", error_message);
  return nullptr;
};

std::shared_ptr<Node> parser::ParseProgram() {
  nextToken();
  std::vector<std::shared_ptr<Node>> models;
  std::vector<std::shared_ptr<Node>> neighbourhoods;
  do {
    if(token.type == MODEL) {
      auto model = parser::ParseModel();
      if(!model) {
        return nullptr;
      }
      models.push_back(std::move(model));
    } else if (token.type == NEIGHBOURHOOD) {
      auto neighbourhood = parser::ParseNeighbourhood();
      if(!neighbourhood) {
        return nullptr;
      }
      neighbourhoods.push_back(std::move(neighbourhood));
    }
    nextToken();
  } while(token.type == MODEL || token.type == NEIGHBOURHOOD);
  if(models.empty() && neighbourhoods.empty()) {
    ParsingError("Program", "\'model\' or \'neighbourhood\'");
    return nullptr;
  }
  auto models_series = std::make_shared<Series>(models);
  auto neighbourhood_series = std::make_shared<Series>(neighbourhoods);
  return std::make_shared<Program>(models_series, neighbourhood_series);
}

std::shared_ptr<Node> parser::ParseModel() {
  if(token.type != MODEL) {
    ParsingError("Model", "\'model\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("Model", "identifier");
    return nullptr;
  }
  std::string model_id = token.lexeme;
  nextToken();
  if(token.type != COLON) {
    ParsingError("Model", "\':\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("Model", "identifier");
    return nullptr;
  }
  std::string neighbourhood_id = token.lexeme;
  nextToken();
  if(token.type != LBRACE) {
    ParsingError("Model", "\'{\'");
    return nullptr;
  }
  nextToken();
  auto states = parser::ParseStates();
  if(!states) {
    return nullptr;
  }
  nextToken();
  if(token.type != RBRACE) {
    ParsingError("Model", "\'}\'");
    return nullptr;
  }
  return std::make_shared<Model>(model_id, neighbourhood_id, states);
}

std::shared_ptr<Node> parser::ParseNeighbourhood() {
  if(token.type != NEIGHBOURHOOD) {
    ParsingError("Neighbourhood", "\'neighbourhood\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("Neighbourhood", "identifier");
    return nullptr;
  }
  std::string id = token.lexeme;
  nextToken();
  if(token.type != COLON) {
    ParsingError("Neighbourhood", "\':\'");
    return nullptr;
  }
  nextToken();
  if(token.type != NAT_LIT) {
    ParsingError("Neighbourhood", "natural literal");
    return nullptr;
  }
  int dimensions = std::stoi(token.lexeme);
  nextToken();
  if(token.type != LBRACE) {
    ParsingError("Neighbourhood", "\'{\'");
    return nullptr;
  }
  nextToken();
  auto neighbours = parser::ParseNeighbours();
  if(!neighbours) {
    return nullptr;
  }
  nextToken();
  if(token.type != RBRACE) {
    ParsingError("Neighbourhood", "\'}\'");
    return nullptr;
  }
  return std::make_shared<Neighbourhood>(id, dimensions, neighbours);
}

std::shared_ptr<Node> parser::ParseNeighbours() {
  std::vector<TOKEN_TYPE> first_set = {ID, LSQUAR};
  std::vector<TOKEN_TYPE> follow_set = {RBRACE};
  return parser::SeriesParsing(&parser::ParseNeighbour,first_set, follow_set, "\'}\'",COMMA);
}

std::shared_ptr<Node> parser::ParseNeighbour() {
  std::string id;
  if(token.type == ID) {
    id = token.lexeme;
    nextToken();
  }
  auto coordinate = parser::ParseCoordinate();
  if(!coordinate) {
    return nullptr;
  }
  return std::make_shared<Neighbour>(id, coordinate);
}

std::shared_ptr<Node> parser::ParseStates() {
  std::vector<TOKEN_TYPE> first_set = {DEFAULT, STATE};
  std::vector<TOKEN_TYPE> follow_set = {RBRACE};
  return parser::SeriesParsing(&parser::ParseState, first_set, follow_set, "\'}\'", ERROR);
}

std::shared_ptr<Node> parser::ParseState() {
  if(token.type == DEFAULT) {
    nextToken();
    if(token.type != STATE) {
      ParsingError("State", "\'state\'");
      return nullptr;
    }
    nextToken();
    if(token.type != ID) {
      ParsingError("State", "identifier");
      return nullptr;
    }
    return std::make_shared<State>(true, token.lexeme);
  } else if(token.type == STATE) {
    nextToken();
    if(token.type != ID) {
      ParsingError("State", "identifier");
      return nullptr;
    }
    std::string id = token.lexeme;
    nextToken();
    if(token.type != LBRACE) {
      ParsingError("State", "\'{\'");
      return nullptr;
    }
    nextToken();
    if(token.type == RBRACE) {
      return std::make_shared<State>(false, id);
    } else {
      auto predicate = ParsePredicate();
      if(!predicate) {
        return nullptr;
      }
      nextToken();
      if(token.type != RBRACE) {
        ParsingError("State", "\'}\'");
        return nullptr;
      }
      return std::make_shared<State>(id, predicate);
    }
  }
  ParsingError("State", "\'default\' or \'state\'");
  return nullptr;
}

std::shared_ptr<Node> parser::ParsePredicate() {
  std::vector<TOKEN_TYPE> first_set = {OR};
  std::vector<TOKEN_TYPE> follow_set = {RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseExDisjunction, first_set, follow_set, "\'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseExDisjunction() {
  std::vector<TOKEN_TYPE> first_set = {XOR};
  std::vector<TOKEN_TYPE> follow_set = {OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseConjucation, first_set, follow_set, 
    "\'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseConjucation() {
  std::vector<TOKEN_TYPE> first_set = {AND};
  std::vector<TOKEN_TYPE> follow_set = {XOR, OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseEquivalence, first_set, follow_set, 
    "\'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseEquivalence() {
  std::vector<TOKEN_TYPE> first_set = {EQ, NE};
  std::vector<TOKEN_TYPE> follow_set = {AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseRelation, first_set, follow_set,
    "\'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseRelation() {
  std::vector<TOKEN_TYPE> first_set = {LE, LT, GE, GT};
  std::vector<TOKEN_TYPE> follow_set = {EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseTranslation, first_set, follow_set, 
    "\'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseTranslation() {
  std::vector<TOKEN_TYPE> first_set = {ADD, SUB};
  std::vector<TOKEN_TYPE> follow_set = {LE, LT, GE, GT, EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseScaling, first_set, follow_set, 
    "\'<=\', \'<\', \'>=\', \'>\', \'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}

std::shared_ptr<Node> parser::ParseScaling() {
  std::vector<TOKEN_TYPE> first_set = {MULT, DIV, MOD};
  std::vector<TOKEN_TYPE> follow_set = {ADD, SUB, LE, LT, GE, GT, EQ, NE, AND, XOR, OR, RBRACE, PIPE, RPAREN};
  return parser::BinaryParsing(&parser::ParseElement, first_set, follow_set, 
    "\'-\', \'+\', \'<=\', \'<\', \'>=\', \'>\', \'==\', \'!=\', \'and\', \'xor\', \'or\', \'}\', \'|\' or \')\'");
}


std::shared_ptr<Node> parser::ParseElement() {
  if(token.type == SUB || token.type == NOT) {
    // Negation or Negative
    TOKEN_TYPE type = (TOKEN_TYPE) token.type;
    nextToken();
    auto element = parser::ParseElement();
    if(!element) {
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
    auto predicate = parser::ParsePredicate();
    if(!predicate) {
      return nullptr;
    }
    if(token.type != RPAREN) {
      ParsingError("Element", "\')\'");
      return nullptr;
    }
    return predicate; 
  }
  if(token.type == PIPE) {
    // Cardinality
    nextToken();
    if(token.type == SET) {
      auto set = parser::ParseSet();
      if(!set) {
        return nullptr;
      }
      nextToken();
      if(token.type != PIPE) {
        ParsingError("Element", "\'|\'");
        return nullptr;
      }
      return set;
    }
  }
  switch(token.type) {
    case LSQUAR: return parser::ParseCoordinate();
    case NAT_LIT: return std::make_shared<Integer>(std::stoi(token.lexeme));
    case DEC_LIT: return std::make_shared<Decimal>(std::stof(token.lexeme));
    case ID:
    case THIS: return std::make_shared<Identifier>(token.lexeme);
  }
  ParsingError("Element", "\'-\', \'not\', \'(\', \'[\', \'|\', \'this\', identifier, natural literal or decimal literal");
  return nullptr;
}

std::shared_ptr<Node> parser::ParseSet() {
  if(token.type != SET) {
    ParsingError("Set", "\'set\'");
    return nullptr;
  }
  nextToken();
  if(token.type != ID) {
    ParsingError("Set", "\'identifier\'");
    return nullptr;
  }
  std::string variable = token.lexeme;
  nextToken();
  if(token.type != IN) {
    ParsingError("Set", "\'in\'");
    return nullptr;
  }
  nextToken();
  std::shared_ptr<Node> coordinates;
  if(token.type == ALL) {
    coordinates = nullptr;
  } else {
    coordinates = parser::ParseCoordinates();
    if(!coordinates) {
      return nullptr;
    }
  }
  nextToken();
  if(token.type != COLON) {
    ParsingError("Set", "\':\'");
    return nullptr;
  }
  nextToken();
  auto predicate = parser::ParsePredicate();
  if(!predicate) {
    return nullptr;
  }
  return std::make_shared<Cardinality>(variable, coordinates, predicate);
}

std::shared_ptr<Node> parser::ParseCoordinate() {
  if(token.type != LSQUAR) {
    ParsingError("Coordinate", "\'[\'");
    return nullptr;
  }
  nextToken();
  auto vector = parser::ParseVector();
  if(!vector) {
    return nullptr;
  }
  nextToken();
  if(token.type != RSQUAR) {
    ParsingError("Coordinate", "\']\'");
    return nullptr;
  }
  return std::make_shared<Coordinate>(vector);
}

std::shared_ptr<Node> parser::ParseInteger() {
  int factor = 1;
  if(token.type == SUB) {
    factor = -1;
    nextToken();
  }
  if(token.type != NAT_LIT) {
    ParsingError("Integer", "\'-\' or natural literal");
    return nullptr;
  }
  return std::make_shared<Integer>(factor * std::stoi(token.lexeme));
}

std::shared_ptr<Node> parser::ParseVector() {
  std::vector<TOKEN_TYPE> first_set = {SUB, NAT_LIT};
  std::vector<TOKEN_TYPE> follow_set = {RSQUAR};
  return parser::SeriesParsing(&parser::ParseInteger, first_set, follow_set, "\']\'", COMMA);
}

std::shared_ptr<Node> parser::ParseCoordinates() {
  std::vector<TOKEN_TYPE> first_set = {LSQUAR};
  std::vector<TOKEN_TYPE> follow_set = {COLON};
  return parser::SeriesParsing(&parser::ParseCoordinate, first_set, follow_set, "\':\'",COMMA);
}