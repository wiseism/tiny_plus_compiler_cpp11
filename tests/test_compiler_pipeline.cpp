#include <algorithm>
#include <sstream>

#include "gtest/gtest.h"
#include "grammar_analysis.h"
#include "lexical_analysis.h"
#include "to_four_element_exp.h"

namespace {

TreeNode *parse_program(const std::string &source, ErrorMsgs &errors) {
    std::istringstream input(source);
    LexicalAnalysis lexer;
    Tokens tokens;
    errors = lexer.transfer_token(input, tokens);
    if (!errors.empty()) {
        return nullptr;
    }
    tokens.to_thin();
    GrammarAnalysis grammar(tokens);
    return grammar.program(errors);
}

}  // namespace

TEST(CompilerPipeline, TokenizesLongestOperatorsStringsAndMultilineComments) {
    std::istringstream input("int value; { a\ncomment } value:=12<=13; write 'ok';");
    LexicalAnalysis lexer;
    Tokens tokens;
    ErrorMsgs errors = lexer.transfer_token(input, tokens);

    ASSERT_TRUE(errors.empty());
    ASSERT_EQ(12U, tokens.size());
    EXPECT_EQ(":=", tokens[4].value);
    EXPECT_EQ("<=", tokens[6].value);
    EXPECT_EQ("ok", tokens[10].value);
    EXPECT_EQ(2, tokens[4].line);
}

TEST(CompilerPipeline, ParsesChecksAndGeneratesThreeAddressCode) {
    const std::string source =
            "int x; bool ready; "
            "x:=1; ready:=x<2; "
            "if ready then write x else write 0 end";
    ErrorMsgs errors;
    TreeNode *root = parse_program(source, errors);

    ASSERT_TRUE(errors.empty());
    ASSERT_NE(nullptr, root);

    ToFourElementExp generator;
    const std::vector<std::string> code = generator.convert(root);
    EXPECT_NE(code.end(), std::find(code.begin(), code.end(), "write x"));
    EXPECT_NE(code.end(), std::find(code.begin(), code.end(), "write 0"));
}

TEST(CompilerPipeline, ReportsSemanticAndSyntaxErrors) {
    ErrorMsgs errors;
    EXPECT_EQ(nullptr, parse_program("int x; y:=1", errors));
    ASSERT_EQ(1U, errors.size());
    EXPECT_NE(std::string::npos, errors[0].msg.find("未声明"));

    errors.clear();
    EXPECT_EQ(nullptr, parse_program("int x; x:=true", errors));
    ASSERT_EQ(1U, errors.size());
    EXPECT_NE(std::string::npos, errors[0].msg.find("同种类型"));

    errors.clear();
    EXPECT_EQ(nullptr, parse_program("int x x:=1", errors));
    ASSERT_EQ(1U, errors.size());
    EXPECT_NE(std::string::npos, errors[0].msg.find("';'"));
}
