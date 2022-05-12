#include <iostream>
#include <string.h>
#include <string>
#include <system_error>
#include <vector>
#include <algorithm>
#include <memory>
struct point { int x, y; };

int steps = 0;
std::string name;
std::vector<char> prev;
int width = 0;
int height = 0;


int coordinate1d(int x) {
    return x % width;
}

int coordinate2d(point p) {
    return (p.x % height) + (width * (p.y % height));
}

std::vector<int> neighboursA = std::vector<int> { 
    1, -1
};

std::vector<point> neighboursB = std::vector<point> {
    { 1, 0}, { 0, 1}
};

const char* modelA() {
    if(height > 1) {
        return "Error: Expected 1 Dimension for INPUT.";
    }

    
    std::vector<char> next(width);

    for(int t = 0; t < steps; t++) {
        for(int x = 0; x < width; x++) {
            
        }
        std::copy(next.begin(), next.end(), prev.begin());
    }
    
    return "";
}

const char* modelB() {
    std::vector<char> next(width * height);
    for(int t = 0; t < steps; t++) {
        for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
                int current = coordinate2d({x,y});
                if(...) {
                    next[current] = ;
                } else {
                    next[current] = ;//Default State
                }
            }
        }
        std::copy(next.begin(), next.end(), prev.begin());
    }
    return "";
}

int main(int argc, char **argv) {
    name = std::string(argv[0]);
    if(argc != 5) {
        std::cout << "Error: Missing operands\nUsage: ./" +  name + " INPUT MODEL STEPS OUTPUT";
        return 1;
    }

    steps = std::atoi(argv[3]);
    if(steps == 0) {
        std::cout << "Error: Incorrect 3rd operand STEPS must be > 0";
        return 1;
    } 

    FILE *input = fopen(argv[1], "r");

    if(input == NULL) {
        perror("Error: Unable to open input file.");
        return 1;
    }

    int pos = 0;
    char c;
    while((c = getc(input)) != EOF) {
        if(c == '\n') {
            height++;
            pos = 0;
        } else {
            prev.push_back(c);
        }
        pos++;
        if(height == 0) {
            width == pos;
        } else if(pos > width) {
            std::cout << "Error: Contradicing dimensions within INPUT file.";
            return 1;
        }
    }    

    std::string error;
    if(argv[2] == model_id) {
        if((error = modelA()) != "") {
            std::cout << error;
            return 1;
        }
    } else if(argv[2] == model_id) {
        if((error = modelB()) != "") {
            std::cout << error;
            return 1;
        }
    } else {
        std::cout << "Error: Incorrect 2nd operand MODEL must be a name of a model";
        return 1;
    }

    fclose(input);
    FILE *output = fopen(argv[4], "w");
    int pos = 0;
    while(pos < prev.size()) {
        putc(prev.at(pos), output);
        pos++;
        if(pos % width == 0) {
            putc('\n', output);
        }
    }
    return 0;
}