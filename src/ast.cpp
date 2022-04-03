#include "ast.hpp"
#include <set>
#include <map>

using namespace llvm;
using namespace llvm::sys;

int indent_level = 0; //Records the current level of indentation (depth)
std::set<int> pipes;

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::map<std::string, AllocaInst*> NamedValues; // local var table(s)
static std::map<std::string, Value *> GlobalNamedValues; //global var table


/// CREDITTED: LLVM Tutorial 7
/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getDoubleTy(TheContext), 0,
                           VarName.c_str());
}

void ast::load_module() {
  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);
}

bool ast::print_ir() {
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
int start_indent() {
  pipes.insert(indent_level);
  return indent_level;
}

//Removes a pipe at a given depth
void end_indent(int level) {
  pipes.erase(level);
}

std::string current_indent() {
  if(pipes.empty()) {
    std::string text = std::string((indent_level - 1) * 4, ' ');
    return "\n" + text;
  }
  std::string text = "\n";
  for(int level = 0; level < indent_level - 1; level++) {
    if(pipes.count(level) > 0) {
      text = text + "|   ";
    } else {
      text = text + "    ";
    }
  }
  return text;
}

std::string operator_string(int value) {
  switch(value) {
    case EQ: return "EQUALS";
    case AND: return "AND";
    case OR: return "OR";
    case NE: return "NOT EQUALS";
    case LT: return "LESS THAN";
    case LE: return "LESS THAN OR EQUAL";
    case GE: return "GREATER THAN";
    case GT: return "GREATER THAN OR EQUAL";
    case DIV: return "DIVIDE";
    case MOD: return "MODULUS";
    case ASTERIX: return "MULTIPLY";
    case MINUS: return "SUBTRACT";
    case PLUS: return "ADDITION";
  }
  return "UNKNOWN";
}

std::string type_string(int value) {
  switch(value) {
    case INT_TOK: return "INT";
    case FLOAT_TOK: return "FLOAT";
    case BOOL_TOK: return "BOOL";
    case VOID_TOK: return "VOID";
  }
  return "UNKNOWN";
}

ast::Node::~Node() {};
std::string ast::Node::to_string() const {
  return "";
};

std::string ast::SequenceNode::to_string() const {
  if(nodes.empty()) {
    return "NONE";
  }
  //Prints out all nodes in the sequences with appropriate formatting
  std::string text = "<sequence>:";
  int indent = start_indent();
  indent_level++;
  for(int i = 0; i < nodes.size() - 1; i++) {
    auto node = nodes.at(i);
    text = text + current_indent();
    text = text + "|-  " + node->to_string();
  }
  end_indent(indent);
  text = text + current_indent();
  text = text + "\\-  " + nodes.at(nodes.size() - 1)->to_string();
  indent_level--;
  return text;
};

Value * ast::SequenceNode::codegen() {
  return 0;
};

std::string ast::ProgramNode::to_string() const {
  std::string text = "<program>";
  int indent = start_indent();

  indent_level++;
  if(externs) {
    text = text + current_indent();
    text = text + "|-  " + externs->to_string();
    end_indent(indent);
  } else {
    end_indent(indent);
  }
  text = text + current_indent();
  text = text + "\\-  " + local_decls->to_string();
  return text + "\n";
};
Value * ast::ProgramNode::codegen() {
  return 0;
};

std::string ast::NatNode::to_string() const {
  return "<natural>: " + std::to_string(value);
};
Value * ast::NatNode::codegen() {
  //IntegerType Type::getInt16Ty(TheContext)
  return ConstantInt::get(Type::getInt16Ty(TheContext), value);
};

std::string ast::FloatNode::to_string() const {
  return "<float>: " + std::to_string(value);
};
Value * ast::FloatNode::codegen() {
  return ConstantFP::get(TheContext, APFloat(value));
};

std::string ast::BoolNode::to_string() const {
  if(value) {
    return "<boolean>: true";
  }
  return "<boolean>: false";
};
Value * ast::BoolNode::codegen() {
  //Type: Type::getInt1Ty(TheContext)
  if(value) {
    return ConstantInt::getTrue(TheContext);
  }
  return ConstantInt::getFalse(TheContext);
};

std::string ast::IdentifierNode::to_string() const {
  return "<identifier>: " + id;
};
Value * ast::IdentifierNode::codegen() {
  Value *variable = NamedValues[id];
  if(!variable) {
    SemanticError("Unrecognised variable identifier.");
    return nullptr;
  }
  return Builder.CreateLoad(variable, id.c_str());
};

std::string ast::NotNode::to_string() const {
  std::string text = "<not>:";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + value->to_string();
  indent_level--;
  return text;
};
Value * ast::NotNode::codegen() {
  return 0;
};

std::string ast::NegNode::to_string() const {
  std::string text = "<negative>:";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + value->to_string();
  indent_level--;
  return text;
};
Value * ast::NegNode::codegen() {
  return 0;
};

std::string ast::CallNode::to_string() const {
  std::string text = "<function_call> " + id + ":";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + args->to_string();
  indent_level--;
  return text;
};
Value * ast::CallNode::codegen() {
  return 0;
};

std::string ast::ExprNode::to_string() const {
  std::string text = "<operation> " + operator_string(op) + ":";
  int indent = start_indent();
  indent_level++;
  text = text + current_indent();
  text = text + "|-  " + lhs->to_string();
  end_indent(indent);
  text = text + current_indent();
  text = text + "\\-  " + rhs->to_string();
  indent_level--;
  return text;
};
Value * ast::ExprNode::codegen() {
  Value *left = lhs->codegen();
  Value *right = rhs->codegen();
  if(!left || !right) {
    return nullptr;
  }
  //Need to check types using Type

  //Performs correct operation
  switch(op) {
    case MOD:
      //Can't find modulus for this
      return Builder.CreateFRem(left, right, "mod");
    case PLUS:
      return Builder.CreateFAdd(left, right, "add");
    case MINUS:
      return Builder.CreateFSub(left, right, "sub");
    case ASTERIX:
      return Builder.CreateFMul(left, right, "mul");
    case DIV:
      return Builder.CreateFDiv(left, right, "div");
    case EQ:
    case AND:
    case OR:
    case NE:
    case LT:
      left = Builder.CreateFCmpULT(left, right, "lth");
      //Converts
    case LE:
    case GE:
    case GT:
      return nullptr;
  }

  SemanticError("Operation not recognised.");
  return nullptr;
};

std::string ast::AssignNode::to_string() const {
  std::string text = "<assign> " + id +":";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + expr->to_string();
  indent_level--;
  return text;
};
Value * ast::AssignNode::codegen() {
  return 0;
};

std::string ast::ReturnNode::to_string() const {
  std::string text = "<return>:";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + expr->to_string();
  indent_level--;
  return text;
};
Value * ast::ReturnNode::codegen() {
  return 0;
};

std::string ast::ConditionalNode::to_string() const {
  std::string text = "<conditional>:";
  int indent = start_indent();
  indent_level++;
  text = text + current_indent();
  text = text + "|-  " + condition->to_string();
  text = text + current_indent();
  text = text + "|-  " + if_block->to_string();
  end_indent(indent);
  text = text + current_indent();
  text = text + "\\-  " + else_block->to_string();
  indent_level--;
  return text;
};
Value * ast::ConditionalNode::codegen() {
  return 0;
};

std::string ast::WhileNode::to_string() const {
  std::string text = "<while>:";
  int indent = start_indent();
  indent_level++;
  text = text + current_indent();
  text = text + "|-  " + condition->to_string();
  end_indent(indent);
  text = text + current_indent();
  text = text + "\\-  " + stmt->to_string();
  indent_level--;
  return text;
};
Value * ast::WhileNode::codegen() {
  return 0;
};

std::string ast::EmptyNode::to_string() const {
  return "<empty>";
};
Value * ast::EmptyNode::codegen() {
  return 0;
};

std::string ast::VarDeclNode::to_string() const {
  std::string type_name = type_string(type);
  return "<variable>: " + type_name + " " + id;
};
Value * ast::VarDeclNode::codegen() {
  return 0;
};

std::string ast::ParamNode::to_string() const {
  std::string type_name = type_string(type);
  return "<parameter>: " + type_name + " " + id;
};
Value * ast::ParamNode::codegen() {
  return 0;
};

std::string ast::BlockNode::to_string() const {
  std::string text = "<block>";
  int indent = start_indent();
  indent_level++;
  if(!decls) {
    end_indent(indent);
    text = text + current_indent();
    text = text + "\\-  " + stmts->to_string();
    indent_level--;
  } else {
    text = text + current_indent();
    text = text + "|-  " + decls->to_string();
    end_indent(indent);
    text = text + current_indent();
    text = text + "\\-  " + stmts->to_string();
    indent_level--;
  }
  return text;
};
Value * ast::BlockNode::codegen() {
  return 0;
};

std::string ast::FunDeclNode::to_string() const {
  std::string text = "<function_declaration> " + type_string(type); + " " + id + ":";
  int indent = start_indent();
  indent_level++;
  text = text + current_indent();
  text = text + "|-  " + params->to_string();
  end_indent(indent);
  text = text + current_indent();
  text = text + "\\-  " + block->to_string();
  indent_level--;
  return text;
};
Value * ast::FunDeclNode::codegen() {
  return 0;
};

std::string ast::ExternNode::to_string() const {
  std::string text = "<extern> " + type_string(type); + " " + id + ":";
  indent_level++;
  text = text + current_indent();
  text = text + "\\-  " + params->to_string();
  indent_level--;
  return text;
};
Value * ast::ExternNode::codegen() {
  return 0;
};
