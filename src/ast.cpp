#include "ast.hpp"
#include <set>

using namespace ast;

//Records the current level of indentation (depth)
static int indent_level = 0; 
static std::set<int> pipes;

void ast::incDepth() {
  indent_level++;
}

void ast::decDepth() {
  indent_level--;
}

int ast::startIndent() {
  pipes.insert(indent_level);
  return indent_level;
};


void ast::endIndent(int level) {
  pipes.erase(level);
};

std::string ast::currentIndent() {
  if(pipes.empty()) {
    std::string text = std::string((indent_level - 1) * 4, ' ');
    return "\n" + text;
  }
  std::string text = "\n";
  for(int level = 0; level < indent_level - 1; level++) {
    if(pipes.count(level) > 0) {
      text += "|   ";
    } else {
      text += "    ";
    }
  }
  return text;
};

std::string ast::Node::ast() const {
  return "";
};

void ast::Node::SemanticError(std::string caller, std::string error) {
  fprintf(stderr, "Semantic Error: %s\n", caller.c_str());
  fprintf(stderr, ">>> %s.\n", error.c_str());
  fprintf(stderr, "For text: \'%s\'\n", current_token.lexeme.c_str());
  fprintf(stderr, "Line: %d, ", current_token.line);
  fprintf(stderr, "Column %d.\n", current_token.column);
};

std::string ast::Program::ast() const {
  std::string text = "<program>:";
  int indent = startIndent();
  indent_level++;
  if(!models.empty() && !neighbourhoods.empty()) {
    text += currentIndent();
    text += "|-  " + ast::seriesAST<Neighbourhood>("<neighbourhoods>", neighbourhoods);
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + ast::seriesAST<Model>("<models>", models);
  } else if(!neighbourhoods.empty()) {
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + ast::seriesAST<Neighbourhood>("<neighbourhoods>", neighbourhoods);
  } else {
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + ast::seriesAST<Model>("<models>", models);
  }
  indent_level--;
  return text + "\n";
};

std::string ast::Model::ast() const {
  std::string text = "<model> " + model_id + " ~ " + neighbourhood_id + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + states->ast();
  indent_level--;
  return text;
};

std::string ast::State::ast() const {
  std::string char_string(1, character);
  std::string text = "<state> " + id + " " + char_string;
  if(is_default) {
    return text + " ~ default"; 
  }
  text += ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + predicate->ast();
  indent_level--;
  return text;
};


std::string ast::Neighbourhood::ast() const {
  std::string text = "<neighbourhood> " + id + " ~ " + std::to_string(dimensions) + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + neighbours->ast();
  indent_level--;
  return text;
};

std::string ast::Neighbour::ast() const {
  std::string text = "<neighbour> " + id + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + coordinate->ast();
  indent_level--;
  return text;
};

std::string operatorString(TOKEN_TYPE type) {
  switch(type) {
    case AND: return "AND";
    case OR: return "OR";
    case XOR: return "XOR";
    case NOT: return "NOT";
    case EQ: return "EQUALS";
    case NE: return "NOT EQUALS";
    case LE: return "LESS THAN OR EQUAL";
    case LT: return "LESS THAN";
    case GE: return "GREATER THAN OR EQUAL";
    case GT: return "GREATER THAN";
    case ADD: return "ADD";
    case SUB: return "SUBTRACT";
    case MULT: return "MULTIPLY";
    case DIV: return "DIVIDE";
    case MOD: return "MODULUS";
  }
  return "ERROR";
}

std::string ast::Binary::ast() const {
  std::string text = "<expression> " + operatorString(operation) + ":";
  int indent = startIndent();
  indent_level++;
  text += currentIndent();
  text += "|-  " + left->ast();
  endIndent(indent);
  text += currentIndent();
  text += "\\-  " + right->ast();
  indent_level--;
  return text;
};

std::string ast::Coordinate::ast() const {
  std::string text = "<coordinate>:";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + vector->ast();
  indent_level--;
  return text;
};


std::string ast::Integer::ast() const {
  return "<integer> " + std::to_string(value);
};

std::string ast::Decimal::ast() const {
  return "<decimal> " + std::to_string(value);
};

std::string ast::Identifier::ast() const {
  return "<identifier> " + id;
};

std::string ast::Negation::ast() const {
  std::string text = "<negation>:";
  indent_level++;
  text += currentIndent();
  text += "\\- " + value->ast();
  indent_level--;
  return text;
};


std::string ast::Negative::ast() const {
  std::string text = "<negative>:";
  indent_level++;
  text += "\\- " + value->ast();
  indent_level--;
  return text;
};

std::string ast::Cardinality::ast() const {
  std::string text = "<cardinality> " + variable;
  indent_level++;
  if(!coords) {
    //Any keyword used
    text += " any:";
  } else {
    int indent = startIndent();
    text += ":";
    text += currentIndent();
    text += "|-  " + coords->ast();
    endIndent(indent);
  }
  text += currentIndent();
  text += "\\-  " + predicate->ast();
  indent_level--;
  return text;
};