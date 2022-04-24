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
  void loadModule();
  bool printIR();

  // Abstract/General class for any node in the AST.
  class Node {
    public:
      virtual ~Node();
      // Returns the string conversion of the node.
      virtual std::string to_string() const;
      // Generates code given the AST.
      virtual Value *codegen() = 0;
  };

  // Series of nodes, which are evaluated sequentially (left to right).
  class Series : public Node {
    private:
      std::vector<std::shared_ptr<Node>> nodes;
    public:
      Series(
        std::vector<std::shared_ptr<Node>> nodes
      ) : nodes(std::move(nodes)) {};
      Series() {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  // Represents the program itself.
  class Program : public Node {
    private:
      std::shared_ptr<Node> models;
      std::shared_ptr<Node> neighbourhoods;
    public:
      Program(
        std::shared_ptr<Node> models,
        std::shared_ptr<Node> neighbourhoods
      ) : models(std::move(models)),
          neighbourhoods(std::move(neighbourhoods)) {};
      Program() : {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  // Defines CA formal definition.
  class Model : public Node {
    private:
      std::string model_id;
      std::string neighbourhood_id;
      std::shared_ptr<Node> states;
    public:
      Model(
        const std::string &model_id,
        const std::string &neighbourhood_id,
        std::shared_ptr<Node> states
      ) : model_id(model_id),
          neighbourhood_id(neighbourhood_id),
          states(std::move(states)) {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  // Represents the possible state of a cell in the model.
  class State : public Node {
    private:
      bool is_default = false;
      std::string id;
      std::shared_ptr<Node> predicate;
    public:
      State(
        bool is_default,
        const std::string &id
      ) : is_default(is_default),
          id(id) {};
      State(
        const std::string &id,
        std::shared_ptr<Node> predicate
      ) : id(id),
          predicate(std::move(predicate)) {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  // All neighbours stored here, to be used in multiple models.
  class Neighbourhood : public Node {
    private:
      std::string id;
      int dimensions; // Cannot be Zero.
      std::shared_ptr<Node> neighbours;
    public:
      Neighbourhood(
        const std::string &id,
        int dimensions,
        std::shared_ptr<Node> neighbours
      ) : id(id),
          dimensions(dimensions),
          neighbours(std::move(neighbours)) {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  class Neighbour : public Node {
    private:
      std::string id;
      std::shared_ptr<Node> coordinate;
    public:
      Neighbour(
        const std::string &id,
        std::shared_ptr<Node> coordinate
      ) : id(id).
          vector(std::move(vecctor)) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Binary Expression with a given operation.
  class Expression : public Node {
    private:
      std::shared_ptr<Node> left, right;
      TOKEN_TYPE operation;
    public:
      Expression(
        std::shared_ptr<Node> left,
        int op,
        std::shared_ptr<Node> right
      ) : left(std::move(left)),
          operation(op),
          right(std::move(right)) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Represents a cell relative to THIS.
  class Coordinate : public Node {
    private:
      std::share_ptr<Node> vector;
    public:
      Coordinate(
        std::shared_ptr<Node> vector
      ) : vector(std::move(vector)) {};
      virtual std::string to_string() const;
      virtual Value *codegen();
  };

  // Represents the integer literal.
  class Integer : public Node  {
    private:
      int16_t value;
    public:
      Integer(
        int16_t value
      ) : value(value) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };
  
  // Represents the decimal literal.
  class Decimal : public Node {
    private:
      float value;
    public:
      Decimal(
        float value
      ) : value(value) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Represents the ID token.
  class Identifier : public Node {
    private:
      std::string id;
    public:
      Identifier(
        const std::string &id
      ) : id(id) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Represents the negation unary operation.
  class Negation : public Node {
    private:
      std::shared_ptr<Node> value;
    public:
      Negation(
        std::shared_ptr<Node> value
      ) : value(std::move(value)) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Represents the negative unary operation.
  class Negative : public Node {
    private:
      std::shared_ptr<Node> value;
    public:
      Negative(
        std::shared_ptr<Node> value
      ) : value(std::move(value)) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };

  // Counts the amount of returned cells in set.
  class Cardinality : public Node {
    private:
      // If empty, ANY keyword was used.
      std::string variable;
      std::shared_ptr<Node> coords;
      std::shared_ptr<Node> predicate;
    public:
      Cardinality(
        const std::string &variable,
        std::shared_ptr<Node> coords,
        std::shared_ptr<Node> predicate
      ) : variable(variable),
          coords(std::move(coords)),
          predicate(std::move(predicate)) {};
      virtual std::string to_string() const;
      virtual Value * codegen();
  };
};
