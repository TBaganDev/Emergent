#include "ast.hpp"
#include <set>
#include <map>

using namespace llvm;
using namespace llvm::sys;

int indent_level = 0; //Records the current level of indentation (depth)
int low_indent = 0;
std::set<int> pipes;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
// static std::map<std::string, AllocaInst*> NamedValues; // local var table(s)
// static std::map<std::string, Value *> GlobalNamedValues; //global var table


// /// CREDITTED: LLVM Tutorial 7
// /// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
// /// the function.  This is used for mutable variables etc.
// static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
//                                           const std::string &VarName) {
//   IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
//                  TheFunction->getEntryBlock().begin());
//   return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), 0,
//                            VarName.c_str());
// }

void ast::loadModule() {
  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);
}

bool ast::printIR() {
  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::F_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return true;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************
  return false;
}

void SemanticError(const char *string) {
    fprintf(stderr, "Semantic Error: %s\n", string);
}

//Registers a pipe at the current depth,
//so any added text in tree will show the pipe symbol
int startIndent() {
  pipes.insert(indent_level);
  return indent_level;
}

//Removes a pipe at a given depth
void endIndent(int level) {
  pipes.erase(level);
}

std::string currentIndent() {
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
}

ast::Node::~Node() {};
std::string ast::Node::ast() const {
  return "";
};

std::string ast::Series::ast() const {
  std::string text = "<series>";
  if(items.empty()) {
    return text + " Ø";
  }
  text += ":";
  int indent = startIndent();
  indent_level++;
  int size = items.size() - 1;
  // All items except last have the |- pipe.
  for(int i = 0; i < size; i++) {
    auto item = items[i];
    text += currentIndent();
    text += "|-  " + item->ast();
  }
  endIndent(indent);
  text += currentIndent();
  text += "\\-  " + items[size]->ast();
  indent_level--;
  return text;
};

Value * ast::Series::codegen() {
  return 0;
};

std::string ast::Program::ast() const {
  std::string text = "<program>:";
  int indent = startIndent();
  indent_level++;
  if(models && neighbourhoods) {
    text += currentIndent();
    text += "|-  " + neighbourhoods->ast();
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + models->ast();
  } else if(neighbourhoods) {
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + neighbourhoods->ast();
  } else {
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + models->ast();
  }
  indent_level--;
  return text + "\n";
};

Value * ast::Program::codegen() {
  return 0;
};

std::string ast::Model::ast() const {
  std::string text = "<model> " + model_id + " ~ " + neighbourhood_id + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + states->ast();
  indent_level--;
  return text;
};

Value * ast::Model::codegen() {
  return 0;
};


std::string ast::State::ast() const {
  std::string text = "<state> " + id;
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

Value * ast::State::codegen() {
  return 0;
};


std::string ast::Neighbourhood::ast() const {
  std::string text = "<neighbourhood> " + id + " ~ " + std::to_string(dimensions) + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + neighbours->ast();
  indent_level--;
  return text;
};

Value * ast::Neighbourhood::codegen() {
  return 0;
};


std::string ast::Neighbour::ast() const {
  std::string text = "<neighbour> " + id + ":";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + coordinate->ast();
  indent_level--;
  return text;
};

Value * ast::Neighbour::codegen() {
  return 0;
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

std::string ast::Expression::ast() const {
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

Value * ast::Expression::codegen() {
  return 0;
};


std::string ast::Coordinate::ast() const {
  std::string text = "<coordinate>:";
  indent_level++;
  text += currentIndent();
  text += "\\-  " + vector->ast();
  indent_level--;
  return text;
};

Value * ast::Coordinate::codegen() {
  return 0;
};


std::string ast::Integer::ast() const {
  return "<integer> " + std::to_string(value);
};

Value * ast::Integer::codegen() {
  return 0;
};


std::string ast::Decimal::ast() const {
  return "<decimal> " + std::to_string(value);
};

Value * ast::Decimal::codegen() {
  return 0;
};


std::string ast::Identifier::ast() const {
  return "<identifier> " + id;
};

Value * ast::Identifier::codegen() {
  return 0;
};


std::string ast::Negation::ast() const {
  std::string text = "<negation>:";
  indent_level++;
  text += currentIndent();
  text += "\\- " + value->ast();
  indent_level--;
  return text;
};

Value * ast::Negation::codegen() {
  return 0;
};


std::string ast::Negative::ast() const {
  std::string text = "<negative>:";
  indent_level++;
  text += "\\- " + value->ast();
  indent_level--;
  return text;
};

Value * ast::Negative::codegen() {
  return 0;
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

Value * ast::Cardinality::codegen() {
  return 0;
};

// Value * ast::NatNode::codegen() {
//   //IntegerType Type::getInt16Ty(TheContext)
//   return ConstantInt::get(Type::getInt16Ty(TheContext), value);
// };

// Value * ast::FloatNode::codegen() {
//   return ConstantFP::get(TheContext, APFloat(value));
// };

// Value * ast::BoolNode::codegen() {
//   //Type: Type::getInt1Ty(TheContext)
//   if(value) {
//     return ConstantInt::getTrue(TheContext);
//   }
//   return ConstantInt::getFalse(TheContext);
// };

// Value * ast::IdentifierNode::codegen() {
//   Value *variable = NamedValues[id];
//   if(!variable) {
//     SemanticError("Unrecognised variable identifier.");
//     return nullptr;
//   }
//   return Builder.CreateLoad(variable, id.c_str());
// };

// Value * ast::ExprNode::codegen() {
//   Value *left = lhs->codegen();
//   Value *right = rhs->codegen();
//   if(!left || !right) {
//     return nullptr;
//   }
//   //Need to check types using Type

//   //Performs correct operation
//   // switch(op) {
//     // case MOD:
//     //   //Can't find modulus for this
//     //   return Builder.CreateFRem(left, right, "mod");
//     // case PLUS:
//     //   return Builder.CreateFAdd(left, right, "add");
//     // case MINUS:
//     //   return Builder.CreateFSub(left, right, "sub");
//     // case ASTERIX:
//     //   return Builder.CreateFMul(left, right, "mul");
//     // case DIV:
//     //   return Builder.CreateFDiv(left, right, "div");
//     // case EQ:
//     // case AND:
//     // case OR:
//     // case NE:
//     // case LT:
//     //   left = Builder.CreateFCmpULT(left, right, "lth");
//     //   //Converts
//     // case LE:
//     // case GE:
//     // case GT:
//     //   return nullptr;
//   // }

//   SemanticError("Operation not recognised.");
//   return nullptr;
// };