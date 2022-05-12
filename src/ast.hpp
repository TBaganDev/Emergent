#pragma once
#include <string>
#include <vector>
#include <memory>
#include "stdint.h"
#include "lexer.hpp"

using namespace lexer;

namespace ast {
  // Registers a pipe at the current depth,
  // so any added text in tree will show the pipe symbol.
  int startIndent();
  // Removes a pipe at a given depth.
  void endIndent(int level);
  // Returns a buffering string of the current indentation.
  std::string currentIndent();

  void incDepth();
  void decDepth();

  // Returns the AST for sequentially stored nodes of type T.
  template<typename T>
  std::string seriesAST(
    std::string top,
    std::vector<std::shared_ptr<T>> items
  ) {
    std::string text = top;
    if(items.empty()) {
      return text + " Ø";
    }
    text += ":";
    int indent = startIndent();
    incDepth();
    int size = items.size() - 1;
    
    for(int i = 0; i < size; i++) {
      auto item = items[i];
      text += currentIndent();
      text += "|-  " + item->ast();
    }
    endIndent(indent);
    text += currentIndent();
    text += "\\-  " + items[size]->ast();
    decDepth();
    return text;
  };

  // Abstract/General class for any node in the AST.
  class Node {
    private:
      // Token at the time of parsing.
      const TOKEN current_token;
    public:
      // Saves the token for reporting semantic errors.
      Node() : current_token(token) {};
      virtual ~Node() {};
      // Returns the string conversion of the AST.
      virtual std::string ast() const;
      // Generates code given the AST.
      virtual std::string codegen() = 0;
      // Outputs the semantic error to the terminal.
      void SemanticError(std::string title, std::string error_message);
  };

  // Series of nodes, which are evaluated sequentially (left to right).
  template<class T>
  class Series : public Node {
    public:
      std::vector<std::shared_ptr<T>> items; // Getter not needed
      Series(
        std::vector<std::shared_ptr<T>> items
      ) : items(std::move(items)) {};
      Series() {};
      /* 
        NOTE: Implementation has moved here, as:
        CREDDITED https://stackoverflow.com/questions/10632251/undefined-reference-to-template-function
        "The implementation of a non-specialized template 
        must be visible to a translation unit that uses it.""
      */
      std::string ast() const {
        return ast::seriesAST<T>("<series>", items);
      };
      std::string codegen() {
        return codegen_delimeter(", ");
      };
      std::string codegen_delimeter(std::string delimeter) {
        std::string list;
        bool flag = false;
        for(auto item : items) {
          if(flag) {
            list = list + delimeter;
          }
          std::string value = item->codegen();
          if(value == "") {
            return "";
          }
          list = list + value;
          flag = true;
        }
        return list;
      }
  };

  // Binary Expression with a given operation.
  class Binary : public Node {
    private:
      std::shared_ptr<Node> left, right;
      TOKEN_TYPE operation;
    public:
      Binary(
        std::shared_ptr<Node> left,
        TOKEN_TYPE operation,
        std::shared_ptr<Node> right
      ) : left(std::move(left)),
          operation(operation),
          right(std::move(right)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the integer literal.
  class Integer : public Node  {
    private:
      int value;
    public:
      Integer(
        int value
      ) : value(value) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents a cell relative to THIS.
  class Coordinate : public Node {
    private:
      std::shared_ptr<Series<Integer>> vector;
    public:
      Coordinate(
        std::shared_ptr<Series<Integer>> vector
      ) : vector(std::move(vector)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
      virtual std::string codegen_restricted();
  };

  // Represents the decimal literal.
  class Decimal : public Node {
    private:
      float value;
    public:
      Decimal(
        float value
      ) : value(value) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the ID token.
  class Identifier : public Node {
    public:
      const std::string id;
      Identifier(
        const std::string &id
      ) : id(id) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the negation unary operation.
  class Negation : public Node {
    private:
      std::shared_ptr<Node> value;
    public:
      Negation(
        std::shared_ptr<Node> value
      ) : value(std::move(value)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the negative unary operation.
  class Negative : public Node {
    private:
      std::shared_ptr<Node> value;
    public:
      Negative(
        std::shared_ptr<Node> value
      ) : value(std::move(value)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Counts the amount of returned cells in set.
  class Cardinality : public Node {
    private:
      std::string variable;
      // If nullptr, ANY keyword was used.
      std::shared_ptr<Series<Coordinate>> coords;
      std::shared_ptr<Node> predicate;
    public:
      Cardinality(
        const std::string &variable,
        std::shared_ptr<Series<Coordinate>> coords,
        std::shared_ptr<Node> predicate
      ) : variable(variable),
          coords(std::move(coords)),
          predicate(std::move(predicate)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the possible state of a cell in the model.
  class State : public Node {
    private:
      std::shared_ptr<Node> predicate;
    public:
      const std::string id;
      const char character;
      const bool is_default = false;
      State(
        bool is_default,
        const std::string &id,
        const char character
      ) : is_default(is_default),
          id(id),
          character(character) {};
      State(
        const std::string &id,
        const char character,
        std::shared_ptr<Node> predicate
      ) : id(id),
          character(character),
          predicate(std::move(predicate)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Defines CA formal definition.
  class Model : public Node {
    private:
      std::shared_ptr<Series<State>> states;
    public:
      const std::string neighbourhood_id;
      const std::string model_id;
      Model(
        const std::string &model_id,
        const std::string &neighbourhood_id,
        std::shared_ptr<Series<State>> states
      ) : model_id(model_id),
          neighbourhood_id(neighbourhood_id),
          states(std::move(states)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // A neighbour of the central cell.
  class Neighbour : public Node {
    private:
      std::string id;
      std::shared_ptr<Coordinate> coordinate;
    public:
      Neighbour(
        const std::string &id,
        std::shared_ptr<Coordinate> coordinate
      ) : id(id),
          coordinate(std::move(coordinate)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // All neighbours stored here, to be used in multiple models.
  class Neighbourhood : public Node {
    private:
      std::shared_ptr<Series<Neighbour>> neighbours;
    public:
      std::string id;
      int dimensions; // Cannot be Zero.
      Neighbourhood(
        const std::string &id,
        int dimensions,
        std::shared_ptr<Series<Neighbour>> neighbours
      ) : id(id),
          dimensions(dimensions),
          neighbours(std::move(neighbours)) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };

  // Represents the program itself.
  class Program : public Node {
    private:
      std::vector<std::shared_ptr<Model>> models;
      std::vector<std::shared_ptr<Neighbourhood>> neighbourhoods;
    public:
      Program(
        std::vector<std::shared_ptr<Model>> models,
        std::vector<std::shared_ptr<Neighbourhood>> neighbourhoods
      ) : models(models),
          neighbourhoods(neighbourhoods) {};
      virtual std::string ast() const;
      virtual std::string codegen();
  };


};
