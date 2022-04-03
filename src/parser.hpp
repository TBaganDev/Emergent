#pragma once
#include "ast.hpp"
#include "lexer.hpp"
#include <memory>
using namespace lexer;
using namespace ast;

namespace parser {
   bool registerFile(char * filename);
   void closeFile();
   TOKEN getNextToken();
   void putBackToken(TOKEN tok);
   std::shared_ptr<Node> ParseProgram();
   std::shared_ptr<Node> ParseExternList();
   std::shared_ptr<Node> ParseExtern();
   std::shared_ptr<Node> ParseDeclList();
   std::shared_ptr<Node> ParseDecl();
   std::shared_ptr<Node> ParseVarDecl();
   std::shared_ptr<Node> ParseFunDecl();
   std::shared_ptr<Node> ParseParams();
   std::shared_ptr<Node> ParseParamList();
   std::shared_ptr<Node> ParseParam();
   std::shared_ptr<Node> ParseBlock();
   std::shared_ptr<Node> ParseLocalDecls();
   std::shared_ptr<Node> ParseLocalDecl();
   std::shared_ptr<Node> ParseStmtList ();
   std::shared_ptr<Node> ParseStmt();
   std::shared_ptr<Node> ParseExprStmt();
   std::shared_ptr<Node> ParseWhileStmt();
   std::shared_ptr<Node> ParseIfStmt();
   std::shared_ptr<Node> ParseReturnStmt();
   std::shared_ptr<Node> ParseExpr();
   std::shared_ptr<Node> ParseRval();
   std::shared_ptr<Node> ParseTerm();
   std::shared_ptr<Node> ParseEquiv();
   std::shared_ptr<Node> ParseRel();
   std::shared_ptr<Node> ParseSubExpr();
   std::shared_ptr<Node> ParseFactor();
   std::shared_ptr<Node> ParseElement();
   std::shared_ptr<Node> ParseArgs();
}
