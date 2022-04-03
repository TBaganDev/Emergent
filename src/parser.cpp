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

static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;
FILE *pFile;

bool parser::registerFile(char * filename) {
  resetLexer();
  pFile = fopen(filename, "r");
  if(pFile == NULL) {
    return true;
  }
  return false;
}

void parser::closeFile() {
  fclose(pFile); // close the file that contains the code that was parsed
}

TOKEN parser::getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(getTok(pFile));

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

void parser::putBackToken(TOKEN tok) {
  tok_buffer.push_front(CurTok);
  CurTok = tok;
}

void ParsingError(const char *string) {
  fprintf(stderr, "Parsing Error: %s\n", string);
  fprintf(stderr, "Instead got: \'%s\'.\n", CurTok.lexeme.c_str());
  fprintf(stderr, "On line %d\n", CurTok.lineNo);
  fprintf(stderr, "On column %d\n", CurTok.columnNo);
}

/*
    NOTE:
      All nullable non-terminals (except for else for practical reasons)
      will check their follow set at the end of their parse. This is determine
      when to end the parse and move on to the next non-terminal. Due to this,
      after each call of a nullable ParseFunction, it will not be necessary to
      call getNextToken().
*/


/*
  program ::= extern_list decl_list
      | decl_list
*/
std::shared_ptr<Node> parser::ParseProgram() {
  if(CurTok.type == EXTERN) {
    //Parses extern_list
    auto extern_list = ParseExternList();
    if(!extern_list) {
      return nullptr;
    }
    //Parses decl_list
    auto decl_list = ParseDeclList();
    if(!decl_list) {
      return nullptr;
    }
    return std::make_shared<ProgramNode>(extern_list, decl_list);
  } else if(CurTok.type == VOID_TOK || CurTok.type == INT_TOK
    || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    //Parses decl_list
    auto decl_list = ParseDeclList();
    if(!decl_list) {
      return nullptr;
    }
    return std::make_shared<ProgramNode>(decl_list);
  }
  ParsingError("Expected \'extern\', \'void\', \'int\', \'float\' or \'bool\'.");
  return nullptr;
}

/*
  extern_list ::= extern extern_list_tail
  extern_list_tail ::= extern extern_list_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseExternList() {
  std::vector<std::shared_ptr<Node>> externs;
  do {
    //Parses extern
    auto extern_command = ParseExtern();
    if(!extern_command) {
      return nullptr;
    }
    //Appends extern
    externs.push_back(std::move(extern_command));
    getNextToken();
  } while(CurTok.type == EXTERN);

  if(CurTok.type == VOID_TOK || CurTok.type == INT_TOK
    || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
      return std::make_shared<SequenceNode>(std::move(externs));
  }
  ParsingError("Expected a \'void\', \'int\', \'bool\' or \'float\' type.");
  return nullptr;
}

/*
  "extern" type_spec IDENT "(" params ")" ";"
*/
std::shared_ptr<Node> parser::ParseExtern() {
  getNextToken();
  //Parses type_spec
  if(CurTok.type != VOID_TOK && CurTok.type != INT_TOK
    && CurTok.type != FLOAT_TOK && CurTok.type != BOOL_TOK) {
    ParsingError("Expected a \'void\', \'int\', \'bool\' or \'float\' type.");
    return nullptr;
  }
  int type_spec = CurTok.type;
  getNextToken();
  //Parses identifier
  if(CurTok.type != IDENT) {
    ParsingError("Expected an Indentifier.");
    return nullptr;
  }
  std::string identifier = CurTok.lexeme;
  getNextToken();
  //Parses "("
  if(CurTok.type != LPAR) {
    ParsingError("Expected \'(\'.");
    return nullptr;
  }
  getNextToken();
  //Parses params
  auto params = ParseParams();
  if(!params) {
    return nullptr;
  }
  getNextToken();
  //Parses ";"
  if(CurTok.type != SC) {
    ParsingError("Expected \';\'.");
    return nullptr;
  }
  return std::make_shared<ExternNode>(type_spec, identifier, params);
}

/*
  decl_list ::= decl decl_list_tail
  decl_list_tail ::= decl decl_list_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseDeclList() {
  std::vector<std::shared_ptr<Node>> decls;
  do {
    //Parses decl
    auto decl = ParseDecl();
    if(!decl) {
      return nullptr;
    }
    //Appends decl
    decls.push_back(std::move(decl));
    getNextToken();
  } while(CurTok.type == VOID_TOK || CurTok.type == INT_TOK
     || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK);
  if(CurTok.type == EOF_TOK) {
    return std::make_shared<SequenceNode>(std::move(decls));
  }
  ParsingError("Expected an EOF.");
  return nullptr;
}

/*
  decl ::= var_decl
      | fun_decl
*/
std::shared_ptr<Node> parser::ParseDecl() {
  if(CurTok.type == VOID_TOK) {
    //Parses fun_decl
    return ParseFunDecl();
  }
  TOKEN type = CurTok;
  getNextToken();
  if(CurTok.type != IDENT) {
    ParsingError("Expected an identifier.");
    return nullptr;
  }
  TOKEN identifier = CurTok;
  getNextToken();
  if(CurTok.type == LPAR) {
    //Backtracks lookahead
    putBackToken(identifier);
    putBackToken(type);
    //Parse fun_decl
    return ParseFunDecl();
  } else if(CurTok.type == SC) {
    //Backtracks lookahead
    putBackToken(identifier);
    putBackToken(type);
    //Parse var_decl
    return ParseVarDecl();
  }
  ParsingError("Expected a function or variable declaration.");
  return nullptr;
}

/*
  var_decl ::= var_type IDENT ";"
*/
std::shared_ptr<Node> parser::ParseVarDecl() {
  int var_type = CurTok.type;
  getNextToken();
  //Parses IDENT
  if(CurTok.type != IDENT) {
    ParsingError("Expected an identifier.");
    return nullptr;
  }
  std::string identifier = CurTok.lexeme;
  getNextToken();
  //Parses ";"
  if(CurTok.type != SC) {
    ParsingError("Expected a \';\'.");
    return nullptr;
  }
  return std::make_shared<VarDeclNode>(var_type, identifier);
}

/*
  fun_decl ::= type_spec IDENT "(" params ")" block
*/
std::shared_ptr<Node> parser::ParseFunDecl() {
  int type_spec = CurTok.type;
  getNextToken();
  if(CurTok.type != IDENT) {
    ParsingError("Expected an identifier.");
    return nullptr;
  }
  std::string identifier = CurTok.lexeme;
  getNextToken();
  if(CurTok.type != LPAR) {
    ParsingError("Expected a \'(\'.");
    return nullptr;
  }
  getNextToken();
  auto params = ParseParams();
  if(!params) {
    return nullptr;
  }
  getNextToken();
  if(CurTok.type != LBRA) {
    ParsingError("Expected \'{\'.");
    return nullptr;
  }
  auto block = ParseBlock();
  if(!block) {
    return nullptr;
  }
  return std::make_shared<FunDeclNode>(type_spec, identifier, params, block);
}

/*
  params ::= param_list
      | "void"
      | epsilon
*/
std::shared_ptr<Node> parser::ParseParams() {
  if(CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK
      || CurTok.type == BOOL_TOK) {
    return ParseParamList();
  }
  if(CurTok.type == VOID_TOK) {
    getNextToken();
    if(CurTok.type != RPAR) {
      ParsingError("Expected \')\'.");
      return nullptr;
    }
    return std::make_shared<SequenceNode>();
  } else if(CurTok.type == RPAR) {
    return std::make_shared<SequenceNode>();
  }
  ParsingError("Expected \')\' or parameter(s).");
  return nullptr;
}

/*
  param_list ::= param param_list_tail
  param_list_tail ::= "," param param_list_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseParamList() {
  std::vector<std::shared_ptr<Node>> params;
  do {
    auto param = ParseParam();
    if(!param) {
      return nullptr;
    }
    params.push_back(std::move(param));
    getNextToken();
    if(CurTok.type != COMMA) {
      break;
    }
    getNextToken();
  } while(CurTok.type == VOID_TOK || CurTok.type == INT_TOK
     || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK);
  if(CurTok.type == RPAR) {
    return std::make_shared<SequenceNode>(std::move(params));
  }
  ParsingError("Expected an \')\'.");
  return nullptr;
}

/*
  param ::= var_type IDENT
*/
std::shared_ptr<Node> parser::ParseParam() {
  int var_type = CurTok.type;
  getNextToken();
  if(CurTok.type != IDENT) {
    ParsingError("Expected an identifier.");
    return nullptr;
  }
  return std::make_shared<ParamNode>(var_type, CurTok.lexeme);
}

/*
  block ::= "{" local_decls stmt_list "}"
*/
std::shared_ptr<Node> parser::ParseBlock() {
  if(CurTok.type != LBRA) {
    ParsingError("Expected a \'{\'");
    return nullptr;
  }
  getNextToken();
  if(CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK
    || CurTok.type == BOOL_TOK) {
    auto local_decls = ParseLocalDecls();
    if(!local_decls) {
      return nullptr;
    }
    auto stmt_list = ParseStmtList();
    if(!stmt_list) {
      return nullptr;
    }
    return  std::make_shared<BlockNode>(local_decls, stmt_list);
  }
  auto stmt_list = ParseStmtList();
  if(!stmt_list) {
    return nullptr;
  }
  return std::make_shared<BlockNode>(stmt_list);
}


/*
  local_decls ::= local_decl local_decls
      | epsilon
*/
std::shared_ptr<Node> parser::ParseLocalDecls() {
  std::vector<std::shared_ptr<Node>> local_decls;
  do {
    auto local_decl = ParseLocalDecl();
    if(!local_decl) {
      return nullptr;
    }
    local_decls.push_back(std::move(local_decl));
    getNextToken();
  }  while(CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK
      || CurTok.type == BOOL_TOK);
  if(CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE
      || CurTok.type == RETURN || CurTok.type == SC || CurTok.type == MINUS
      || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == IDENT
      || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT
      || CurTok.type == BOOL_LIT) {
    return std::make_shared<SequenceNode>(std::move(local_decls));
  }

  ParsingError("Expected a \'}\', \'if\', \'while\', \'return\', \';\', \'-\', \'!\', \'{\', an identifier, integer, float or boolean literal.");
  return nullptr;
}

/*
  local_decl ::= var_type IDENT ";"
*/
std::shared_ptr<Node> parser::ParseLocalDecl() {
  int var_type = CurTok.type;
  getNextToken();
  if(CurTok.type != IDENT) {
    ParsingError("Expected an identifier.");
    return nullptr;
  }
  std::string identifier = CurTok.lexeme;
  getNextToken();
  if(CurTok.type != SC) {
    ParsingError("Expected a \';\'.");
    return nullptr;
  }

  return std::make_shared<VarDeclNode>(var_type, identifier);
}

/*
  stmt_list ::= stmt stmt_list
      | epsilon
*/
std::shared_ptr<Node> parser::ParseStmtList () {
  std::vector<std::shared_ptr<Node>> stmts;
  if(CurTok.type == RBRA) {
    return std::make_shared<SequenceNode>(std::move(stmts));
  }
  do {
    auto stmt = ParseStmt();
    if(!stmt) {
      return nullptr;
    }
    stmts.push_back(std::move(stmt));
    getNextToken();
  } while(CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE
      || CurTok.type == RETURN || CurTok.type == SC || CurTok.type == MINUS
      || CurTok.type == NOT || CurTok.type == LPAR || CurTok.type == IDENT
      || CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT
      || CurTok.type == BOOL_LIT);
  if(CurTok.type == RBRA) {
    return std::make_shared<SequenceNode>(std::move(stmts));
  }

  ParsingError("Expected a \'}\'.");
  return nullptr;
}

/*
  stmt ::= expr_stmt
      | block
      | if_stmt
      | while_stmt
      | return_stmt
*/
std::shared_ptr<Node> parser::ParseStmt() {
  switch(CurTok.type) {
    case LBRA:
      return ParseBlock();
    case IF:
      return ParseIfStmt();
    case WHILE:
      return ParseWhileStmt();
    case RETURN:
      return ParseReturnStmt();
    case SC:
    case MINUS:
    case NOT:
    case LPAR:
    case IDENT:
    case INT_LIT:
    case FLOAT_LIT:
    case BOOL_LIT:
      return ParseExprStmt();
  }
  ParsingError("Expected a \'}\', \'if\', \'while\', \'return\', \';\', \'-\', \'!\', \'{\', an identifier, integer, float or boolean literal.");
  return nullptr;
}

/*
  expr_stmt ::= expr ";"
      | ";"
*/
std::shared_ptr<Node> parser::ParseExprStmt() {
  if(CurTok.type == SC) {
    return std::make_shared<EmptyNode>();
  }
  auto expr = ParseExpr();
  if(!expr) {
    return nullptr;
  }
  if(CurTok.type != SC) {
    ParsingError("Expected a \';\'.");
    return nullptr;
  }
  return expr;
}

/*
  while_stmt ::= "while" "(" expr ")" stmt
*/
std::shared_ptr<Node>  parser::ParseWhileStmt() {
  getNextToken();
  if(CurTok.type != LPAR) {
    ParsingError("Expected a \'(\'.");
    return nullptr;
  }
  getNextToken();
  auto expr = ParseExpr();
  if(!expr) {
    return nullptr;
  }
  if(CurTok.type != RPAR) {
    ParsingError("Expected a \')\'.");
    return nullptr;
  }
  getNextToken();
  auto stmt = ParseStmt();
  if(!stmt) {
    return nullptr;
  }

  return std::make_shared<WhileNode>(expr, stmt);
}

/*
  if_stmt ::= "if" "(" expr ")" block else_stmt
  else_stmt ::= "else" block
      | epsilon
*/
std::shared_ptr<Node> parser::ParseIfStmt() {
  getNextToken();
  if(CurTok.type != LPAR) {
    ParsingError("Expected \'(\'.");
    return nullptr;
  }
  getNextToken();
  auto expr = ParseExpr();
  if(!expr) {
    return nullptr;
  }
  if(CurTok.type != RPAR) {
    return nullptr;
  }
  getNextToken();
  auto block = ParseBlock();
  if(!block) {
    return nullptr;
  }
  getNextToken();
  if(CurTok.type == ELSE) {
    getNextToken();
    auto else_block = ParseBlock();
    if(!else_block) {
      return nullptr;
    }

    return std::make_shared<ConditionalNode>(expr, block, else_block);
  }
  return std::make_shared<ConditionalNode>(expr, block);
}

/*
  return_stmt ::= "return" ";"
      | "return" expr ";"
*/
std::shared_ptr<Node> parser::ParseReturnStmt() {
  getNextToken();
  if(CurTok.type == SC) {
    return std::make_shared<ReturnNode>(nullptr);
  }
  auto expr = ParseExpr();
  if(!expr) {
    return nullptr;
  }
  if(CurTok.type != SC) {
    ParsingError("Expected a \';\'.");
    return nullptr;
  }

  return std::make_shared<ReturnNode>(expr);
}

/*
  expr ::= IDENT "=" expr
      | rval
*/
std::shared_ptr<Node> parser::ParseExpr() {
  if(CurTok.type == IDENT) {
    TOKEN identifier = CurTok;
    getNextToken();
    if(CurTok.type == ASSIGN) {
      //Recursion is finite
      getNextToken();
      auto expr = ParseExpr();
      if(!expr) {
        return nullptr;
      }

      return std::make_shared<AssignNode>(identifier.lexeme, expr);
    }
    //Backtracks lookahead
    putBackToken(identifier);
  }
  return ParseRval();
}

/*
  rval ::= term rval_tail
  rval_tail ::= "||" term rval_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseRval() {
  auto left = ParseTerm();
  if(!left) {
    return nullptr;
  }
  if(CurTok.type == OR) {
    int op = CurTok.type;
    getNextToken();
    auto right = ParseRval();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  term ::= equiv term_tail
  term_tail ::= "&&" equiv term_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseTerm() {
  auto left = ParseEquiv();
  if(!left) {
    return nullptr;
  }
  if(CurTok.type == AND) {
    int op = CurTok.type;
    getNextToken();
    auto right = ParseTerm();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == OR || CurTok.type == SC
      || CurTok.type == RPAR || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \'||\', \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  equiv ::= rel equiv_tail
  equiv_tail ::= "==" rel equiv_tail
      | "!=" rel equiv_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseEquiv() {
  auto left = ParseRel();
  if(!left) {
    return nullptr;
  }
  if(CurTok.type == EQ || CurTok.type == NE) {
    int op = CurTok.type;
    getNextToken();
    auto right = ParseEquiv();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == AND || CurTok.type == OR || CurTok.type == SC
      || CurTok.type == RPAR || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \'&&\', \'||\', \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  rel ::= subexpr rel_tail
  rel_tail ::= "<=" subexpr rel_tail
      | "<" subexpr rel_tail
      | ">=" subexpr rel_tail
      | ">" subexpr rel_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseRel() {
  auto left = ParseSubExpr();
  if(!left) {
    return nullptr;
  }
  if(CurTok.type == LE || CurTok.type == LT || CurTok.type == GE
      || CurTok.type == GT) {
    int op = CurTok.type;
    getNextToken();
    auto right = ParseRel();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == EQ || CurTok.type == NE || CurTok.type == AND
      || CurTok.type == OR || CurTok.type == SC || CurTok.type == RPAR
      || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \'==\', \'!=\', \'&&\', \'||\', \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  subexpr ::= factor subexpr_tail
  subexpr_tail ::= "+" factor subexpr_tail
      | "-" factor subexpr_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseSubExpr() {
  auto left = ParseFactor();
  if(!left) {
    return nullptr;
  }
  if(CurTok.type == PLUS || CurTok.type == MINUS) {
    int op = CurTok.type;
    getNextToken();
    auto right = ParseSubExpr();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == LE || CurTok.type == LT || CurTok.type == GE
      || CurTok.type == GT || CurTok.type == EQ || CurTok.type == NE
      || CurTok.type == AND || CurTok.type == OR || CurTok.type == SC
      || CurTok.type == RPAR || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \'>=\', \'>\', \'<=\', \'<\', \'==\', \'!=\', \'&&\', \'||\', \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  factor ::= element factor_tail
  factor_tail ::= "*" element factor_tail
      | "/" element factor_tail
      | "%" element factor_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseFactor() {
  auto left = ParseElement();
  if(!left) {
    return nullptr;
  }
  getNextToken();
  if(CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
    int op = CurTok.type;
    // Must be an expression on same level
    getNextToken();
    auto right = ParseFactor();
    if(!right) {
      return nullptr;
    }
    return std::make_shared<ExprNode>(left, op, right);
  } else if(CurTok.type == PLUS || CurTok.type == MINUS || CurTok.type == LE
      || CurTok.type == LT || CurTok.type == GE
      || CurTok.type == GT || CurTok.type == EQ || CurTok.type == NE
      || CurTok.type == AND || CurTok.type == OR || CurTok.type == SC
      || CurTok.type == RPAR || CurTok.type == COMMA) {
    return left;
  }

  ParsingError("Expected a \'+\', \'-\', \'>=\', \'>\', \'<=\', \'<\', \'==\', \'!=\', \'&&\', \'||\', \';\', \')\' or \',\'.");
  return nullptr;
}

/*
  element ::= "-" element
      | "!" element
      | "(" expr ")"
      | IDENT
      | IDENT "(" args ")"
      | INT_LIT
      | FLOAT_LIT
      | BOOL_LIT
*/
std::shared_ptr<Node> parser::ParseElement() {
  if(CurTok.type == MINUS || CurTok.type == NOT) {
    int type = CurTok.type;
    getNextToken();
    auto element = ParseElement();
    if(!element) {
      return nullptr;
    }
    if(type == MINUS) {
      return std::make_shared<NegNode>(element);
    }
    return std::make_shared<NotNode>(element);
  }
  if(CurTok.type == LPAR) {
    getNextToken();
    auto expr = ParseExpr();
    if(!expr) {
      return nullptr;
    }
    if(CurTok.type != RPAR) {
      ParsingError("Expected a \')\'.");
      return nullptr;
    }
    return expr;
  }
  if(CurTok.type == IDENT) {
    TOKEN identifier = CurTok;
    getNextToken();
    if(CurTok.type == LPAR) {
      getNextToken();
      if(CurTok.type == RPAR) {
        return std::make_shared<CallNode>(identifier.lexeme);
      } else {
        auto args = ParseArgs();
        if(!args) {
          return nullptr;
        }
        return std::make_shared<CallNode>(identifier.lexeme, args);
      }
    }
    //Backtracks lookahead
    putBackToken(CurTok);
    return std::make_shared<IdentifierNode>(identifier.lexeme);
  }
  if(CurTok.type == INT_LIT) {
    return std::make_shared<NatNode>(std::stoi(CurTok.lexeme));
  }
  if(CurTok.type == FLOAT_LIT) {
    return std::make_shared<FloatNode>(std::stof(CurTok.lexeme));
  }
  if(CurTok.type == BOOL_LIT) {
    bool value = false;
    if(CurTok.lexeme == "true") {
      value = true;
    }
    return std::make_shared<BoolNode>(value);
  }
  ParsingError("Expected \'-\', \'!\', \'(\', an identifier, float, integer or boolean literal.");
  return nullptr;
}

/*
  args ::= arg_list
      | epsilon
  arg_list ::= expr arg_list_tail
  arg_list_tail ::= "," expr arg_list_tail
      | epsilon
*/
std::shared_ptr<Node> parser::ParseArgs() {
  /*
    Although this args is technically nullable,
    that was checked for in ParseElement.
    So, only args_list can be nullable here.
  */
  std::vector<std::shared_ptr<Node>> args;
  do {
    auto expr = ParseExpr();
    if(!expr) {
      return nullptr;
    }
    args.push_back(std::move(expr));
  } while(CurTok.type == COMMA);
  if(CurTok.type == RPAR) {
    return std::make_shared<SequenceNode>(std::move(args));
  }
  ParsingError("Expected a \')\'.");
  return nullptr;
}
