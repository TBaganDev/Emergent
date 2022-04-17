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

#include "ast.hpp"
#include "parser.hpp"

using namespace llvm;
using namespace llvm::sys;

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ast::Node &ast) {
  os << ast.to_string();
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }
  bool flag = parser::registerFile(argv[1]);
  if(flag) {
    perror("Error opening file.");
  }

  ast::load_module();

  // Parser is ran.
  fprintf(stderr, "Parsing Source...\n");
  auto program = parser::ParseProgram();
  fprintf(stderr, "Parsing Finished!\n");

  //Print AST using post order traversal
  if(!program) {
    parser::closeFile();
    return 0;
  }
  fprintf(stderr, "Printing AST...\n");
  llvm::outs() << program.get()->to_string();
  fprintf(stderr, "AST Printed!\n");

  if(ast::print_ir()) {
    return 1;
  }


  parser::closeFile();
  return 0;
}
