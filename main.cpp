#include "util/errors.h"
#include "core/main/parser.h"
#include <iostream>
#include <fstream>
#include <iterator>

#define RST  "\x1B[0m"
#define RED  "\x1B[31m"


int interpret(bool shell, const std::string &filename) {
    std::shared_ptr<Scope> globalScope = std::make_shared<Scope>();
    Lexer lexer("");
    Parser parser(lexer);

    std::ifstream file;
    if (!shell) {
        file = std::ifstream(filename);
        if (!file.is_open()) {
            std::cout << RED << "Error: Couldn't open file " << filename << RST << std::endl;
            return 1;
        }
    }
    std::string multiline, line;
    bool continuation;
    if (shell) std::cout << "Type 'exit' to quit" << std::endl;

    while (true) {
        if (shell) std::cout << "> ";
        multiline.clear();

        try {
            do {
                if (shell) {
                    std::getline(std::cin, line);
                } else {
                    std::getline(file, line);
                }
                line.erase(line.find_last_not_of(" \t") + 1);

                if (shell && line == "exit" && multiline.empty()) {
                    return 0;
                }

                if (!line.empty() && line.back() == '\\') {
                    line.pop_back();
                    continuation = true;
                } else {
                    continuation = false;
                }

                multiline += line;
                if (!continuation) {
                    multiline += "\n";
                }

                lexer.reset(multiline);
                parser.advanceToken();
            } while (continuation || !parser.isStatementComplete());

            auto statements = parser.parse();
            Value result;

            for (const auto &statement: statements) {
                result = statement->evaluate(globalScope);
                if (shell) {
                    printValue(result, true);
                    std::cout << std::endl;
                }
            }
        } catch (const ControlFlowException &e) {
            std::cout << RED << "Control flow error: Use of " << e.what() << " outside of a loop" << RST << std::endl;
        } catch (const ReturnException &e) {
            std::cout << RED << "Control flow error: Use of RETURN outside of a function" << RST << std::endl;
        } catch (const BaseError &e) {
            std::cout << RED << e.what() << RST << std::endl;
        } catch (const std::exception &e) {
            std::cout << RED << "Unexpected error: " << e.what() << RST << std::endl;
        }
        if (!shell && file.eof()) break;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cout << "Usage: <program> [<filename>]" << std::endl;
        return 1;
    }
    std::cout << std::boolalpha << std::fixed;

    return argc == 2 ? interpret(false, argv[1]) : interpret(true, {});
}
