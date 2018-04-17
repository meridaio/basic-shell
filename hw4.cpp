#include <iostream>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <regex.h>
#include "hw4.h"



int main() {
    std::string line;
    std::vector<std::string> tokens;

    while(1) {
        // our prompt
        std::cout << ">";

        std::getline(std::cin, line);
        std::stringstream ss(line);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
    }
}


std::vector<std::string> parse_line(std::string line) {
    
}
