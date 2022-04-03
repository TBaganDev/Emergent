#pragma once
#include <string>
#include <vector>
#include "stdint.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "lexer.hpp"

using namespace llvm;
using namespace lexer;

namespace ast {
  void load_module();
  bool print_ir();

  class Node {
  public:
    virtual ~Node();
    virtual std::string to_string() const;
    virtual Value *codegen() = 0;
  };

  // This acts as a wrapper for all nodes that are generated in sequence
  // Means there's no need to have a std::vector attribute for all classes
  // with this property
  class SequenceNode : public Node {
  private:
    std::vector<std::shared_ptr<Node>> nodes;
  public:
    SequenceNode(
      std::vector<std::shared_ptr<Node>> nodes
    ) : nodes(std::move(nodes)) {};
    SequenceNode() {};
    virtual std::string to_string() const;
    virtual Value *codegen();
  };

  class ProgramNode : public Node {
  private:
    std::shared_ptr<Node> externs;
    std::shared_ptr<Node> local_decls;
  public:
    ProgramNode(
      std::shared_ptr<Node> externs,
      std::shared_ptr<Node> local_decls
    ) : externs(std::move(externs)),
        local_decls(std::move(local_decls)) {};
    ProgramNode(
      std::shared_ptr<Node> local_decls
    ) : local_decls(std::move(local_decls)) {};
    virtual std::string to_string() const;
    virtual Value *codegen();
  };

  class NatNode : public Node  {
  private:
    int16_t value;
  public:
    NatNode(
      int16_t value
    ) : value(value) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };
  

  class FloatNode : public Node {
  private:
    float value;
  public:
    FloatNode(
      float value
    ) : value(value) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class BoolNode : public Node {
  private:
    bool value;
  public:
    BoolNode(
      bool value
    ) : value(value) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class IdentifierNode : public Node {
  private:
    std::string id;
  public:
    IdentifierNode(
      const std::string &id
    ) : id(id) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class NotNode : public Node {
  private:
    std::shared_ptr<Node> value;
  public:
    NotNode(
      std::shared_ptr<Node> value
    ) : value(std::move(value)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class NegNode : public Node {
  private:
    std::shared_ptr<Node> value;
  public:
    NegNode(
      std::shared_ptr<Node> value
    ) : value(std::move(value)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class CallNode : public Node {
  private:
    std::string id;
    std::shared_ptr<Node> args;
  public:
    CallNode(
      const std::string &id,
      std::shared_ptr<Node> args
    ) : id(id), args(std::move(args)) {};
    CallNode(
      const std::string &id
    ) : id(id) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class ExprNode : public Node {
  private:
    std::shared_ptr<Node> lhs, rhs;
    int op;
  public:
    ExprNode(
      std::shared_ptr<Node> lhs,
      int op,
      std::shared_ptr<Node> rhs
    ) : lhs(std::move(lhs)),
        op(op),
        rhs(std::move(rhs)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class AssignNode : public Node {
  private:
    std::string id;
    std::shared_ptr<Node> expr;
  public:
    AssignNode(
      const std::string &id,
      std::shared_ptr<Node> expr
    ) : id(id), expr(std::move(expr)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class ReturnNode : public Node {
  private:
    std::shared_ptr<Node> expr;
  public:
    ReturnNode(
      std::shared_ptr<Node> expr
    ) : expr(std::move(expr)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class ConditionalNode : public Node {
  private:
    std::shared_ptr<Node> condition;
    std::shared_ptr<Node> if_block;
    std::shared_ptr<Node> else_block;
  public:
    ConditionalNode(
      std::shared_ptr<Node> condition,
      std::shared_ptr<Node> if_block,
      std::shared_ptr<Node> else_block
    ) : condition(std::move(condition)),
        if_block(std::move(if_block)),
        else_block(std::move(else_block)) {};
    ConditionalNode(
      std::shared_ptr<Node> condition,
      std::shared_ptr<Node> if_block
    ) : condition(std::move(condition)),
        if_block(std::move(if_block)) {};

    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class WhileNode : public Node {
  private:
    std::shared_ptr<Node> condition;
    std::shared_ptr<Node> stmt;
  public:
    WhileNode(
      std::shared_ptr<Node> condition,
      std::shared_ptr<Node> stmt
    ) : condition(std::move(condition)),
        stmt(std::move(stmt)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  // Needed to represent semi colons returned from ParseExprStmt
  class EmptyNode : public Node {
  public:
    EmptyNode() {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class VarDeclNode : public Node {
  private:
    int type;
    std::string id;
  public:
    VarDeclNode(
      int type,
      const std::string &id
    ) : type(type), id(id) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class ParamNode : public Node {
  private:
    int type;
    std::string id;
  public:
    ParamNode(
      int type,
      const std::string &id
    ) : type(type), id(id) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class BlockNode : public Node {
  private:
    std::shared_ptr<Node> decls;
    std::shared_ptr<Node> stmts;
  public:
    BlockNode(
      std::shared_ptr<Node> decls,
      std::shared_ptr<Node> stmts
    ) : decls(std::move(decls)),
        stmts(std::move(stmts)) {};
    BlockNode(
      std::shared_ptr<Node> stmts
    ) : stmts(std::move(stmts)) {}
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class FunDeclNode : public Node {
  private:
    int type;
    std::string id;
    std::shared_ptr<Node> params;
    std::shared_ptr<Node> block;
  public:
    FunDeclNode(
      int type,
      const std::string &id,
      std::shared_ptr<Node> params,
      std::shared_ptr<Node> block
    ) : type(type), id(id),
        params(std::move(params)),
        block(std::move(block)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };

  class ExternNode : public Node {
  private:
    int type;
    std::string id;
    std::shared_ptr<Node> params;
  public:
    ExternNode(
      int type,
      const std::string &id,
      std::shared_ptr<Node> params
    ) : type(type), id(id),
        params(std::move(params)) {};
    virtual std::string to_string() const;
    virtual Value * codegen();
  };
};
