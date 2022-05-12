#include "ast.hpp"
#include "lexer.hpp"
#include <map>
#include <algorithm>

using namespace ast;

std::map<std::string, std::shared_ptr<ast::Node>> globals;
std::map<std::string, std::map<std::string, std::shared_ptr<Coordinate>>> neighbour_ids;
std::shared_ptr<Neighbourhood> current_neighbourhood = nullptr;
std::map<std::string, std::shared_ptr<ast::State>> local_states;
std::vector<std::string> variables;

std::string ast::Binary::codegen() {
    std::string l = left->codegen();
    if(l == "") {
        return "";
    }
    std::string r = right->codegen();
    if(r == "") {
        return "";
    }

    switch(operation) {
        case AND: return "(" + l + " && " + r + ")";
        case OR: return "(" + l + " || " + r + ")";
        case XOR: return "((" + l + " && !" + r + ") || (!" + l + " && "+ r + "))";
        case EQ: return "(" + l + " == " + r + ")";
        case NE: return "(" + l + " != " + r + ")";
        case LE: return "(" + l + " <= " + r + ")";
        case LT: return "(" + l + " < " + r + ")";
        case GE: return "(" + l + " >= " + r + ")";
        case GT: return "(" + l + " > " + r + ")";
        case ADD: return "(" + l + " + " + r + ")";
        case SUB: return "(" + l + " - " + r + ")";
        case MULT: return "(" + l + " * " + r + ")";
        case DIV: return "(" + l + " / " + r + ")";
        case MOD: return "(" + l + " % " + r + ")";
    }

    SemanticError("Binary", "Unrecognised operation.");
    return "";
}

std::string ast::Integer::codegen() {
    return std::to_string(value);
}

std::string ast::Coordinate::codegen() {
    std::string code = codegen_restricted();
    if(code == "") {
        return "";
    }
    if(current_neighbourhood->dimensions == 1) {
        return "coordinate1d(x +" + code + ")";
    }
    return "coordinate2d(add_point(" + code + ", x, y))";
}

std::string ast::Coordinate::codegen_restricted() {
    if(current_neighbourhood->dimensions != vector->items.size()) {
        SemanticError("Coordinate", "Dimension don't match neighbourhood.");
        return "";
    }
    if(current_neighbourhood->dimensions == 1) {
        return vector->codegen();
    }
    return "{" + vector->codegen() + "}";
}
std::string ast::Decimal::codegen() {
    return std::to_string(value);
}
std::string ast::Identifier::codegen() {
    if(id == "this") {
        return "prev[current]";
    }

    auto coordinate = neighbour_ids[current_neighbourhood->id][id];
    if(!coordinate) {
        auto state = local_states[id];
        if(!state) {
            if(std::count(variables.begin(), variables.end(), id)) {
                if(current_neighbourhood->dimensions == 1) {
                    return "prev[coordinate1d(x + " + id + ")]";
                }
                return "prev[coordinate2d(add_point(" + id + ", x, y))]";
            }

            SemanticError("Idenitifier", "Unrecognised name");
            return "";
        }
        // Is a reference to a state
        std::string char_string(1, state->character);
        return "\'" + char_string + "\'";
    }
    return coordinate->codegen();
}
std::string ast::Negation::codegen() {
    return "!" + value->codegen();
}
std::string ast:: Negative::codegen() {
    return "-" + value->codegen();
}
std::string ast::Cardinality::codegen() {
    variables.push_back(variable);
    
    std::string type = "int";
    std::string d = "1d";
    if(current_neighbourhood->dimensions == 2) {
        type = "std::pair<int, int>";
        d = "2d";
    }

    std::string list;
    if(!coords) {
        //Any
        list = current_neighbourhood->id;
    } else { 
        list = coords->codegen();
        if(list == "") {
            return "";
        }
        list = "vec" +  d + "("+  list + ")";
    }

    std::string condition = predicate->codegen();
    if(condition == "") {
        return "";
    }
    variables.erase(std::remove(variables.begin(), variables.end(), variable), variables.end());
    return 
        "std::count_if(" + list + ".begin(), " + list + ".end(), [=](" + type + " " + variable + ") { return" + condition + ";})"
        ;
}
std::string ast::State::codegen() {
    std::string char_string(1, character);
    
    if(is_default) {
        return
            "{\n"
            "              next[current] = \'" + char_string + "\';\n"
            "           }\n";
    }

    if(!predicate) {
        return 
            "if(false) {\n"
            "           } else ";
    }
    std::string code = predicate->codegen();
    if(code == "") {
        return "";
    }

    return
        "if(" + code + ") {\n"
        "               next[current] = \'" + char_string + "\';\n"
        "           } else ";
}

std::string ast::Model::codegen() {
    if(!globals.count(neighbourhood_id)) {
        SemanticError("Model", "Associated neighbourhood doesn't exist.");
        return "";
    }
    //Have to cast down from Node, as Models are Global too
    current_neighbourhood = std::static_pointer_cast<Neighbourhood, Node>(globals.find(neighbourhood_id)->second);
    
    std::string code = 
        "const char* " + model_id + "() {\n";
    std::string ending_brace;
    if(current_neighbourhood->dimensions == 1) {
        code = code +
            "   if(height > 1) {\n"
            "       return \"Error: Expected 1 Dimension for INPUT.\";\n"
            "   }\n"
            "   std::vector<char> next(width);\n"
            "   for(int t = 0; t < steps; t++) {\n"
            "       for(int x = 0; x < width; x++) {\n"
            "           int current = x;\n"
            "           ";
    } else {
        code = code +
            "   std::vector<char> next(width * height);\n"
            "   for(int t = 0; t < steps; t++) {\n"
            "       for(int x = 0; x < width; x++) {\n"
            "       for(int y = 0; y < height; y++) {\n"
            "           int current = coordinate2d({x,y});\n"
            "           ";
        ending_brace = 
            "       }\n";
    }

    std::shared_ptr<State> default_state;
    for(auto state : states->items) {
        auto it = local_states.insert({state->id, state});
        if(!it.second) {
            state->SemanticError("State", "Duplicate identifiers conflict.");
            return "";
        }

        if(state->is_default) {
            if(default_state) {
                state->SemanticError("State", "Multiple Default States.");
                return "";
            } else {
                default_state = state;
            }
        }
    }

    for(auto state : states->items) {
        if(!state->is_default) {
            std::string state_string = state->codegen();
            if(state_string == "") {
                return "";
            }
            code = code + state_string;
        }
    }
    
    code = code + default_state->codegen();

    code = code + ending_brace +
        "       }\n"
        "       std::copy(next.begin(), next.end(), prev.begin());\n"
        "   }\n"
        "   return \"\";\n"
        "}\n";
    local_states.clear();
    current_neighbourhood = nullptr;
    return code;
}

std::string ast::Neighbour::codegen() {
    if(id != "" &&
            neighbour_ids.count(current_neighbourhood->id) && 
            neighbour_ids[current_neighbourhood->id].count(id)) {
        SemanticError("Neighbour", "Duplicate identifiers conflict.");
        return "";
    }
    neighbour_ids[current_neighbourhood->id][id] = coordinate;
    return coordinate->codegen_restricted();
}

std::string ast::Neighbourhood::codegen() {
    std::string code = neighbours->codegen();
    if(code == "") {
        return "";
    }
    if(dimensions == 1) {
        return 
            "std::vector<int> " + id + " = std::vector<int> {\n"
            "   " + code + "\n"
            "};\n"; 
    } else if(dimensions == 2) {
        return
            "std::vector<std::pair<int,int>> " + id + " = std::vector<std::pair<int,int>> {\n"
            "   " + code + "\n"
            "};\n";
    }
    SemanticError("Neighbourhood", "Neighbourhood's dimensions must be 1 or 2.");
    return "";
    
}

std::string ast::Program::codegen() {  
    // Output preamble, neighbourhoods_gen, models_gen, main_a, cases, main_b

    std::string preamble = 
        "#include <iostream>\n"
        "#include <string.h>\n"
        "#include <string>\n"
        "#include <system_error>\n"
        "#include <vector>\n"
        "#include <algorithm>\n"
        "#include <memory>\n"
        "#include <utility>\n"

        "int steps = 0;\n"
        "std::string name;\n"
        "std::vector<char> prev;\n"
        "int width = 0;\n"
        "int height = 0;\n"
        "int coordinate1d(int x) {\n"
        " return x % width;\n"
        "}\n"
        "std::vector<int> vec1d(std::vector<int> l) { return l; };\n"
        "std::vector<std::pair<int,int>> vec2d(std::vector<std::pair<int,int>> l) { return l; };\n"
        "int coordinate2d(std::pair<int,int> p) {\n"
        "    return (p.first \% height) + (width * (p.second \% height));\n"
        "};\n"
        "std::pair<int,int> add_point(std::pair<int,int> l, int x, int y) {\n"
        "    return std::pair<int,int>{l.first + x, l.second + y};\n"
        "};\n"
        ;

    std::string neighbourhoods_gen;
    for(auto neighbourhood : neighbourhoods) {
        auto it = globals.insert({neighbourhood->id, neighbourhood});
        if(!it.second) {
            neighbourhood->SemanticError("Neighbourhood", "Duplicate identifiers conflict.");
            return "";
        }
        current_neighbourhood = neighbourhood;
        std::string code =  neighbourhood->codegen();
        current_neighbourhood = nullptr;
        if(code == "") {
            return "";
        }
        neighbourhoods_gen = neighbourhoods_gen + code;
    }

    std::string models_gen;
    for(auto model : models) {
        auto it = globals.insert({model->model_id, model});
        if(!it.second) {
            model->SemanticError("Model", "Duplicate identifiers conflict.");
            return "";
        }
        std::string code = model->codegen();
        if(code == "") {
            return "";
        }
        models_gen = models_gen + code;
    }
    
    std::string main_a =
        "int main(int argc, char **argv) {\n"
        "   name = std::string(argv[0]);\n"
        "   if(argc != 5) {\n"
        "   std::cout << \"Error: Missing operands\\nUsage: ./\" +  name + \" INPUT MODEL STEPS OUTPUT\\n\";"
        "   return 1;\n"
        "   }\n"
        "   steps = std::atoi(argv[3]);\n"
        "   if(steps == 0) {\n"
        "       std::cout << \"Error: Incorrect 3rd operand STEPS must be > 0\\n\";\n"
        "       return 1;\n"
        "   }\n" 
        "   FILE *input = fopen(argv[1], \"r\");\n"
        "   if(input == NULL) {\n"
        "       perror(\"Error: Unable to open input file.\\n\");\n"
        "       return 1;\n"
        "   }\n"
        "   int pos = 0;\n"
        "   char c;\n"
        "   while((c = getc(input)) != EOF) {\n"
        "       if(c == \'\\n\' || c == \'\\r\') {\n"
        "           height++;\n"
        "           pos = 0;\n"
        "       } else {\n"
        "           prev.push_back(c);\n"
        "       }\n"
        "       pos++;\n"
        "       if(height == 0) {\n"
        "           width = pos;\n"
        "       } else if(pos > width) {\n"
        "           std::cout << \"Error: Contradicing dimensions within INPUT file.\\n\";\n"
        "           return 1;\n"
        "       }\n"
        "   }\n"
        "   std::string model(argv[2]);"
        "   std::string error;\n    ";

    std::string cases;
    for(auto & model : models) {
        std::string id = model->model_id;
        cases = cases +
            "if(model == \"" + id + "\") {\n"
            "       if((error = " + id +"()) != \"\") {\n"
            "           std::cout << error + \"\\n\";\n"
            "           return 1;\n"
            "       }\n"
            "   } else ";
    }

    std::string main_b =
        " {\n"
        "       std::cout << \"Error: Incorrect 2nd operand MODEL must be a name of a model\\n\";\n"
        "       return 1;\n"
        "   }\n"
        "   fclose(input);\n"
        "   FILE *output = fopen(argv[4], \"w\");\n"
        "   pos = 0;\n"
        "   while(pos < prev.size()) {\n"
        "       putc(prev.at(pos), output);\n"
        "       pos++;\n"
        "       if(pos % width == 0) {\n"
        "           putc(\'n\', output);\n"
        "       }\n"
        "   }\n"
        "   return 0;\n"
        "}\n";

    return preamble + neighbourhoods_gen + models_gen + main_a + cases + main_b;
}