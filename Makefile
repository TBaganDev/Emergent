CXX=clang++ -std=c++17
DCS_FLAGS=-stdlib=libstdc++ -cxx-isystem /local/java/gcc-9.2.0/include/c++/9.2.0/ -cxx-isystem /local/java/gcc-9.2.0/include/c++/9.2.0/x86_64-pc-linux-gnu/ -L/local/java/gcc-9.2.0/lib64 -L/local/java/gcc-9.2.0/lib/gcc/x86_64-pc-linux-gnu/9.2.0/

SRC=./src
BIN=./bin

emergent: $(BIN)/ast.o $(BIN)/codegen.o $(BIN)/lexer.o $(BIN)/parser.o $(SRC)/main.cpp 
	$(CXX) $(SRC)/main.cpp $(BIN)/ast.o  $(BIN)/codegen.o $(BIN)/parser.o $(BIN)/lexer.o $(DCS_FLAGS) -o $(BIN)/emergent

$(BIN)/codegen.o: $(SRC)/codegen.cpp $(SRC)/ast.cpp $(SRC)/ast.hpp $(SRC)/lexer.cpp $(SRC)/lexer.hpp
	$(CXX) -c -o $(BIN)/codegen.o $(SRC)/codegen.cpp

$(BIN)/ast.o: $(SRC)/ast.cpp $(SRC)/ast.hpp $(SRC)/lexer.cpp $(SRC)/lexer.hpp
	$(CXX) -c -o $(BIN)/ast.o $(SRC)/ast.cpp

$(BIN)/lexer.o: $(SRC)/lexer.cpp $(SRC)/lexer.hpp
	$(CXX) -c -o $(BIN)/lexer.o $(SRC)/lexer.cpp

$(BIN)/parser.o: $(SRC)/parser.cpp $(SRC)/parser.hpp $(SRC)/lexer.cpp $(SRC)/lexer.hpp $(SRC)/ast.cpp $(SRC)/ast.hpp
	$(CXX) -c -o $(BIN)/parser.o $(SRC)/parser.cpp

clean:
	rm -rf $(BIN)/*.o
	rm -rf $(BIN)/emergent
