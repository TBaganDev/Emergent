#include <iostream>
#include <string.h>
#include <string>
#include <system_error>
#include <algorithm>
#include "ast.hpp"
#include "parser.hpp"

bool verbose = false;

// Only outputs a message iff a verbose option is given.
void spit(std::string text) {
  if(verbose) {
    std::cout << text + "\n";
  }
}

int main(int argc, char **argv) {
  if(argc < 2) {
    std::cout << "Error: Missing operand\nUsage: ./emergent [OPTION]... SOURCE.emg\n";
    return 1;
  }
  int top = argc - 1;
  bool ast = false;
  for(int i = 1; i < argc; i++) {
    std::string option(argv[i]);
    if(option == "-t") {
      ast = true;
    } else if(option == "-v") {
      verbose = true;
    } else if(option == "--help") {
      std::cout << "Usage: ./emergent [OPTION]... SOURCE.emg\n"
        "Compiles any *.emg Emergent source code into C++.\n\n" 
        "All possible options:\n"
        "   -t      Prints the parsed syntax tree.\n"
        "   -v      Prints all the stages of the compiler\n"
        "   --help  Displays this message.\n";
      return 0;
    } else if(i < top) {
      std::cout << "Error: Unknown operand " + option + "\nUsage: ./emergent [OPTION]... SOURCE.emg\n";
      return 1;
    }
  }

  spit("Opening file...");
  std::string name(argv[top]);
  int i = name.find_last_of('.');
  std::string type = name.substr(i, name.length() - i);
  if(type != ".emg") {
    std::cout << "Error: SOURCE file extension must be .emg\n";
    return 1;
  }

  if(parser::openFile(argv[top])) {
    perror("Error: Unable to open SOURCE file\n"); // Returns error for open() also
    return 1;
  }
  spit("File has been opened!");

  // Parser is ran.
  spit("Parsing Source...\n");
  auto program = parser::ParseProgram();
  //Print AST using post order traversal
  if(!program) {
    parser::closeFile();
    return 0;
  }
  spit("Parsing Finished!\n");
  if(ast) {
    spit("Printing AST...\n");
    std::cout << program->ast();
    spit("AST Printed!\n");
  }
  parser::closeFile();
  spit("Source file closed Successfully!");
  spit("Code Generating...\n");
  std::string code = program->codegen();
  spit("Code Generation Successful!\n");
  spit("Outputting object...\n");

  std::string target = name.substr(0, i) + ".cpp";
  FILE *object = fopen(target.c_str(), "w");
  
  if(object == NULL) {
    perror("Error: Couldn't create object.cpp file.");
    return 1;
  }

  if(false) {
    
  }

  fputs(code.c_str(), object);
  fclose(object);
  spit("Object file Successful!\n");
  return 0;
}
