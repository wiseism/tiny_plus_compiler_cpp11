#include <fstream>
#include <iostream>
#include <string>

#include "data_types.h"
#include "grammar_analysis.h"
#include "lexical_analysis.h"
#include "to_four_element_exp.h"

namespace {

void print_errors(const std::string &title, const ErrorMsgs &errors) {
    if (errors.empty()) {
        return;
    }

    std::cout << title << ':' << std::endl;
    for (const ErrorMsg &error : errors) {
        std::cout << "  " << error.to_string() << std::endl;
    }
}

void print_tokens(Tokens &tokens) {
    std::cout << "Tokens:" << std::endl;
    for (std::vector<Token>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        std::cout << "  (" << it->kind_name() << ", " << it->value << ")"
                  << " @ " << it->line << ':' << it->column << std::endl;
    }
}

void print_syntax_tree(const TreeNode *node, int depth = 0) {
    if (!node) {
        return;
    }

    std::cout << std::string(static_cast<std::size_t>(depth) * 2, ' ')
              << node->get_type_name();
    if (node->type_ == TreeNode::Type::FACTOR || node->type_ == TreeNode::Type::READ_STMT) {
        std::cout << " (" << node->tk_.kind_name() << ", " << node->tk_.value << ')';
    }
    std::cout << std::endl;

    for (TreeNode *child : node->child_) {
        print_syntax_tree(child, depth + 1);
    }
}

void print_usage(const char *program_name) {
    std::cerr << "Usage: " << program_name << " <source-file | ->" << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
    if (argc != 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        print_usage(argv[0]);
        return argc == 2 ? 0 : 64;
    }

    std::ifstream source_file;
    std::istream *source = &std::cin;
    if (std::string(argv[1]) != "-") {
        source_file.open(argv[1]);
        if (!source_file) {
            std::cerr << "Cannot open source file: " << argv[1] << std::endl;
            return 66;
        }
        source = &source_file;
    }

    LexicalAnalysis lexer;
    Tokens tokens;
    ErrorMsgs lexical_errors = lexer.transfer_token(*source, tokens);
    print_tokens(tokens);
    print_errors("Lexical errors", lexical_errors);
    if (!lexical_errors.empty()) {
        return 1;
    }

    tokens.to_thin();
    GrammarAnalysis grammar(tokens);
    ErrorMsgs analysis_errors;
    TreeNode *root = grammar.program(analysis_errors);
    print_errors("Syntax/semantic errors", analysis_errors);
    if (!analysis_errors.empty()) {
        return 1;
    }
    if (!root) {
        std::cerr << "Syntax/semantic errors:" << std::endl;
        std::cerr << "  Program must contain at least one statement." << std::endl;
        return 1;
    }

    std::cout << "Syntax tree:" << std::endl;
    print_syntax_tree(root);

    ToFourElementExp generator;
    std::cout << "Three-address code:" << std::endl;
    for (const std::string &instruction : generator.convert(root)) {
        std::cout << "  " << instruction << std::endl;
    }
    return 0;
}
