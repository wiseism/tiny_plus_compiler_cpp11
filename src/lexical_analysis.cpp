//
// Created by JefungChan on 2018/10/26.
//

#include <cctype>
#include "lexical_analysis.h"

std::vector<ErrorMsg>
LexicalAnalysis::transfer_token(std::istream &is, Tokens &tokens) {
    int line = 1;
    unsigned long column = 1;
    std::vector<ErrorMsg> err_msgs;
    std::size_t line_token_start = 0;
    char c;
    while (is.get(c)) {
        const int token_line = line;
        const unsigned long token_column = column;
        if (c == '\n') { ++line; column = 1; line_token_start = tokens.size(); continue; }
        if (std::isspace(static_cast<unsigned char>(c))) { ++column; continue; }
        if (is_letter(c)) {
            std::string word(1, c);
            while (is.peek() != EOF && (is_letter(static_cast<char>(is.peek())) || is_number(static_cast<char>(is.peek())))) {
                word += static_cast<char>(is.get());
            }
            column += word.size();
            tokens.push(Token::is_KEY(word) ? Token::Kind::KEY : Token::Kind::ID, word, token_line, token_column);
            continue;
        }
        if (is_number(c)) {
            std::string word(1, c);
            bool invalid = false;
            while (is.peek() != EOF && (is_number(static_cast<char>(is.peek())) || is_letter(static_cast<char>(is.peek())))) {
                char next = static_cast<char>(is.get());
                invalid = invalid || is_letter(next);
                word += next;
            }
            column += word.size();
            if (invalid) {
                err_msgs.emplace_back(token_line, token_column, "数字加字母是不合法的组合");
                tokens.resize(line_token_start);
            }
            else tokens.push(Token::Kind::NUM, word, token_line, token_column);
            continue;
        }
        if (c == '{') {
            bool closed = false;
            ++column;
            while (is.get(c)) {
                if (c == '}') { ++column; closed = true; break; }
                if (c == '\n') { ++line; column = 1; line_token_start = tokens.size(); } else ++column;
            }
            if (!closed) err_msgs.emplace_back(token_line, token_column, "注释标识符缺少匹配的'}'");
            continue;
        }
        if (c == '}') {
            err_msgs.emplace_back(token_line, token_column, "注释标识符缺少匹配的'{'");
            tokens.resize(line_token_start);
            ++column;
            continue;
        }
        if (c == '\'') {
            std::string value;
            bool closed = false;
            ++column;
            while (is.get(c)) {
                if (c == '\'') { ++column; closed = true; break; }
                if (c == '\n') { ++line; column = 1; break; }
                value += c; ++column;
            }
            if (closed) tokens.push(Token::Kind::STR, value, token_line, token_column);
            else {
                err_msgs.emplace_back(token_line, token_column, "字符串缺少单引号匹配");
                tokens.resize(line_token_start);
            }
            continue;
        }
        std::string symbol(1, c);
        if ((c == ':' || c == '<' || c == '>') && is.peek() == '=') { symbol += static_cast<char>(is.get()); ++column; }
        if (symbol == ":") {
            err_msgs.emplace_back(token_line, token_column, "':'必须与'='组成':='");
            tokens.resize(line_token_start);
        }
        else if (symbol == ":=" || std::string(",;<>=+-*/()").find(c) != std::string::npos)
            tokens.push(Token::Kind::SYM, symbol, token_line, token_column);
        else {
            err_msgs.emplace_back(token_line, token_column, "非法字符");
            tokens.resize(line_token_start);
        }
        ++column;
    }
    return err_msgs;
}
