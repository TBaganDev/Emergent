#pragma once
#include "ast.hpp"
#include "lexer.hpp"
#include <memory>
using namespace lexer;
using namespace ast;

namespace parser {
   // Opens the source file.
   bool openFile(char * filename);

   // Closes the source file.
   void closeFile();

   /*
      Returns the AST which represents the Program as a whole.
      program -> model program_tail
      program -> neighbourhood program_tail
      program_tail -> model program_tail
      program_tail -> neighbourhood program_tail
      program_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseProgram();

   /*
      model -> MODEL ID COLON ID LBRACE states RBRACE
   */
   std::shared_ptr<Node> ParseModel();

   /*
      neighbourhood -> NEIGHBOURHOOD ID COLON NAT_LIT LBRACE neighbours RBRACE
   */
   std::shared_ptr<Node> ParseNeighbourhood();

   /*
      neighbours ->  neighbour neighbours_tail
      neighbours_tail -> COMMA neighbour neighbours_tail
      neighbours_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseNeighbours();

   /*
      neighbour -> ID coord 
      neighbour -> coord
   */
   std::shared_ptr<Node> ParseNeighbour();

   /*
      states -> state states
      states -> state
   */
   std::shared_ptr<Node> ParseStates();

   /*
      state -> DEFAULT STATE ID
      state -> STATE ID LBRACE pred RBRACE
      state -> STATE ID LBRACE RBRACE
   */
   std::shared_ptr<Node> ParseState();

   /*
      pred -> ex_disj pred_tail
      pred_tail -> OR ex_disj pred_tail
      pred_tail -> Ɛ
   */
   std::shared_ptr<Node> ParsePredicate();

   /*
      ex_disj -> conj ex_disj_tail
      ex_disj_tail -> XOR conj ex_disj_tail
      ex_disj_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseExDisjunction();

   /*
      conj -> equiv conj_tail
      conj_tail -> AND equiv conj_tail
      conj_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseConjucation();

   /*
      equiv -> rel equiv_tail
      equiv_tail -> EQ rel equiv_tail
      equiv_tail -> NE rel equiv_tail
      equiv_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseEquivalence();

   /*
      rel -> trans rel_tail
      rel_tail -> LE trans rel_tail
      rel_tail -> LT trans rel_tail
      rel_tail -> GE trans rel_tail
      rel_tail -> GT trans rel_tail
      rel_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseRelation();

   /*
      trans -> scale trans_tail
      trans_tail -> ADD scale trans_tail
      trans_tail -> SUB scale trans_tail
      trans_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseTranslation();

   /*
      scale -> element scale_tail
      scale_tail -> MULT element scale_tail
      scale_tail -> DIV element scale_tail
      scale_tail -> MOD element scale_tail
      scale_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseScaling();

   /*
      element -> SUB element
      element -> NOT element
      element -> LPAREN pred RPAREN
      element -> NAT_LIT
      element -> DEC_LIT
      element -> PIPE set PIPE
      element -> THIS
      element -> ID
      element -> coord
   */
   std::shared_ptr<Node> ParseElement();

   /*
      set -> SET ID IN ANY COLON pred
      set -> SET ID IN coords COLON pred
   */
   std::shared_ptr<Node> ParseSet();

   /*
      coord -> LSQUAR vector RSQUAR
   */
   std::shared_ptr<Node> ParseCoordinate();

   /*
      int -> NAT_LIT
      int -> SUB NAT_LIT
   */
   std::shared_ptr<Node> ParseInteger();

   /*
      vector -> NAT_LIT vector_tail
      vector_tail -> COMMA NAT_LIT vector_tail
      vector_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseVector();

   /*
      coords -> coord coords_tail
      coords_tail -> COMMA coord coords_tail
      coords_tail -> Ɛ
   */
   std::shared_ptr<Node> ParseCoordinates();
   
   // Performs the common binary parsing pattern, useful for abstracting binary operations.
   // Sends an error message if a token in the follow_set and first_set isn't found.
   std::shared_ptr<Node> BinaryParsing(
      std::shared_ptr<Node> (parse_function)(), 
      std::vector<TOKEN_TYPE> first_set, 
      std::vector<TOKEN_TYPE> follow_set,
      std::string error_message
   );

   // Performs the array/series parsing pattern for representing lists of objects.
   // Sends an error message if a token in the follow_set isn't found.
   // NOTE: An ERROR token seperator means no seperator is decided.
   std::shared_ptr<Node> SeriesParsing(
      std::shared_ptr<Node> (parse_function)(), 
      std::vector<TOKEN_TYPE> first_set, 
      std::vector<TOKEN_TYPE> follow_set,
      std::string error_message,
      TOKEN_TYPE seperator
   );
}
