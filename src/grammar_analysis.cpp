#include "grammar_analysis.h"

namespace {

bool is_type_specifier(Token::Kind kind) {
    return kind == Token::Kind::TK_INT || kind == Token::Kind::TK_BOOL || kind == Token::Kind::TK_STRING;
}

ValType type_from_token(Token::Kind kind) {
    if (kind == Token::Kind::TK_INT) return ValType::VT_INT;
    if (kind == Token::Kind::TK_BOOL) return ValType::VT_BOOL;
    return ValType::VT_STRING;
}

TreeNode::Type comparison_type(Token::Kind kind) {
    switch (kind) {
        case Token::Kind::TK_GTR: return TreeNode::Type::GTR_EXP;
        case Token::Kind::TK_GEQ: return TreeNode::Type::GEQ_EXP;
        case Token::Kind::TK_LSS: return TreeNode::Type::LSS_EXP;
        case Token::Kind::TK_LEQ: return TreeNode::Type::LEQ_EXP;
        default: return TreeNode::Type::EQU_EXP;
    }
}

}  // namespace

GrammarAnalysis::GrammarAnalysis(Tokens tokens) : tokens_(tokens) {}

bool GrammarAnalysis::at_end() const { return position_ >= tokens_.size(); }

Token &GrammarAnalysis::current() { return tokens_[position_]; }

const Token &GrammarAnalysis::current() const { return tokens_[position_]; }

bool GrammarAnalysis::match(KindSet kind_set, std::string error_message) throw(ErrorMsg) {
    if (!at_end() && kind_set.find(current().kind) != kind_set.end()) return true;
    if (error_message.empty()) return false;
    if (at_end()) {
        Token last = tokens_.back();
        throw ErrorMsg(last.line, last.column + last.value.size(), error_message);
    }
    throw ErrorMsg(current().line, current().column, error_message);
}

bool GrammarAnalysis::match_and_forward(KindSet kind_set, std::string error_message) throw(ErrorMsg) {
    if (!match(kind_set, error_message)) return false;
    ++position_;
    return true;
}

Token GrammarAnalysis::take(KindSet kind_set, const std::string &error_message) {
    match(kind_set, error_message);
    return tokens_[position_++];
}

bool GrammarAnalysis::is_statement_start() const {
    if (at_end()) return false;
    const Token::Kind kind = current().kind;
    return kind == Token::Kind::TK_IF || kind == Token::Kind::TK_WHILE || kind == Token::Kind::TK_REPEAT ||
           kind == Token::Kind::TK_READ || kind == Token::Kind::TK_WRITE || kind == Token::Kind::ID;
}

void GrammarAnalysis::require_bool(TreeNode *node, const std::string &context) {
    if (node->val_type_ != ValType::VT_BOOL) {
        throw ErrorMsg(at_end() ? tokens_.back().line : current().line,
                       at_end() ? tokens_.back().column : current().column,
                       context + "必须是bool类型");
    }
}

TreeNode *GrammarAnalysis::program(ErrorMsgs &error_msgs) {
    if (tokens_.size() == 0) {
        error_msgs.emplace_back(1, 1, "程序不能为空");
        return nullptr;
    }
    try {
        declaration();
        TreeNode *root = stmt_sequence();
        if (!root) return nullptr;
        if (!at_end()) throw ErrorMsg(current().line, current().column, "无法识别的语句或缺少分号");
        return root;
    } catch (const ErrorMsg &msg) {
        error_msgs.emplace_back(msg);
        return nullptr;
    }
}

void GrammarAnalysis::declaration() {
    while (!at_end() && is_type_specifier(current().kind)) {
        const ValType val_type = type_from_token(current().kind);
        ++position_;
        do {
            Token identifier = take({Token::Kind::ID}, "类型说明后需要变量名");
            Sym *sym = sym_table_.insert(identifier.value);
            if (!sym) throw ErrorMsg(identifier.line, identifier.column, "标识符已经声明: " + identifier.value);
            sym->obj_type = ObjType::OT_VAR;
            sym->val_type = val_type;
            sym->tk = identifier;
            if (!match({Token::Kind::TK_COMMA})) break;
            ++position_;
        } while (true);
        take({Token::Kind::TK_SEMICOLON}, "变量声明最后应该加';'");
    }
}

TreeNode *GrammarAnalysis::stmt_sequence() {
    TreeNode *sequence = nullptr;
    while (is_statement_start()) {
        TreeNode *statement = nullptr;
        switch (current().kind) {
            case Token::Kind::TK_IF: statement = if_stmt(); break;
            case Token::Kind::TK_WHILE: statement = while_stmt(); break;
            case Token::Kind::TK_REPEAT: statement = repeat_stmt(); break;
            case Token::Kind::TK_READ: statement = read_stmt(); break;
            case Token::Kind::TK_WRITE: statement = write_stmt(); break;
            case Token::Kind::ID: statement = assign_stmt(); break;
            default: break;
        }
        sequence = sequence ? new TreeNode(TreeNode::Type::STMT_SEQUENCE, sequence, statement) : statement;
        const int last_statement_line = tokens_[position_ - 1].line;
        if (match({Token::Kind::TK_SEMICOLON})) {
            ++position_;
        } else if (is_statement_start()) {
            if (current().line == last_statement_line)
                throw ErrorMsg(current().line, current().column, "语句之间需要用';'分隔");
        }
    }
    return sequence;
}

TreeNode *GrammarAnalysis::if_stmt() {
    take({Token::Kind::TK_IF}, "期望if关键词");
    TreeNode *condition = log_or_exp();
    require_bool(condition, "if条件");
    take({Token::Kind::TK_THEN}, "if条件后需要then关键词");
    TreeNode *then_branch = stmt_sequence();
    if (!then_branch) {
        Token location = at_end() ? tokens_.back() : current();
        throw ErrorMsg(location.line, location.column, "then后需要语句");
    }
    TreeNode *else_branch = nullptr;
    if (match({Token::Kind::TK_ELSE})) {
        ++position_;
        else_branch = stmt_sequence();
        if (!else_branch) {
            Token location = at_end() ? tokens_.back() : current();
            throw ErrorMsg(location.line, location.column, "else后需要语句");
        }
    }
    take({Token::Kind::TK_END}, "if控制块缺少end结束符");
    return new TreeNode(TreeNode::Type::IF_STMT, condition, then_branch, else_branch);
}

TreeNode *GrammarAnalysis::repeat_stmt() {
    take({Token::Kind::TK_REPEAT}, "repeat块缺少repeat关键词");
    TreeNode *body = stmt_sequence();
    if (!body) {
        Token location = at_end() ? tokens_.back() : current();
        throw ErrorMsg(location.line, location.column, "repeat后需要语句");
    }
    take({Token::Kind::TK_UNTIL}, "repeat块缺少until关键词");
    TreeNode *condition = log_or_exp();
    require_bool(condition, "until条件");
    return new TreeNode(TreeNode::Type::REPEAT_STMT, body, condition);
}

TreeNode *GrammarAnalysis::assign_stmt() {
    Token identifier = take({Token::Kind::ID}, "赋值语句左边需要变量名");
    Sym *sym = sym_table_.find(identifier.value);
    if (!sym) throw ErrorMsg(identifier.line, identifier.column, "使用了未声明的变量: " + identifier.value);
    take({Token::Kind::TK_ASSIGN}, "赋值语句应该有:=");
    TreeNode *expression = log_or_exp();
    if (sym->val_type != expression->val_type_)
        throw ErrorMsg(identifier.line, identifier.column, "赋值语句左右两边应该是同种类型");
    TreeNode *target = new TreeNode(TreeNode::Type::FACTOR, identifier);
    target->val_type_ = sym->val_type;
    return new TreeNode(TreeNode::Type::ASSIGN_STMT, target, expression);
}

TreeNode *GrammarAnalysis::read_stmt() {
    take({Token::Kind::TK_READ}, "期望read关键词");
    Token identifier = take({Token::Kind::ID}, "read后需要变量名");
    Sym *sym = sym_table_.find(identifier.value);
    if (!sym) throw ErrorMsg(identifier.line, identifier.column, "使用了未声明的变量: " + identifier.value);
    return new TreeNode(TreeNode::Type::READ_STMT, identifier);
}

TreeNode *GrammarAnalysis::write_stmt() {
    take({Token::Kind::TK_WRITE}, "期望write关键词");
    return new TreeNode(TreeNode::Type::WRITE_STMT, log_or_exp());
}

TreeNode *GrammarAnalysis::while_stmt() {
    take({Token::Kind::TK_WHILE}, "期望while关键词");
    TreeNode *condition = log_or_exp();
    require_bool(condition, "while条件");
    take({Token::Kind::TK_DO}, "while条件后需要do关键词");
    TreeNode *body = stmt_sequence();
    if (!body) {
        Token location = at_end() ? tokens_.back() : current();
        throw ErrorMsg(location.line, location.column, "do后需要语句");
    }
    take({Token::Kind::TK_END}, "while控制块缺少end结束符");
    return new TreeNode(TreeNode::Type::WHILE_STMT, condition, body);
}

TreeNode *GrammarAnalysis::log_or_exp() {
    TreeNode *left = log_and_exp();
    while (match({Token::Kind::TK_OR})) {
        Token op = current(); ++position_;
        TreeNode *right = log_and_exp();
        require_bool(left, "or左操作数"); require_bool(right, "or右操作数");
        left = new TreeNode(TreeNode::Type::LOG_OR_EXP, left, right); left->val_type_ = ValType::VT_BOOL;
    }
    return left;
}

TreeNode *GrammarAnalysis::log_and_exp() {
    TreeNode *left = comparision_exp();
    while (match({Token::Kind::TK_AND})) {
        ++position_;
        TreeNode *right = comparision_exp();
        require_bool(left, "and左操作数"); require_bool(right, "and右操作数");
        left = new TreeNode(TreeNode::Type::LOG_AND_EXP, left, right); left->val_type_ = ValType::VT_BOOL;
    }
    return left;
}

TreeNode *GrammarAnalysis::comparision_exp() {
    TreeNode *left = add_exp();
    KindSet operators = {Token::Kind::TK_LEQ, Token::Kind::TK_GEQ, Token::Kind::TK_LSS, Token::Kind::TK_EQU, Token::Kind::TK_GTR};
    if (!match(operators)) return left;
    Token op = current(); ++position_;
    TreeNode *right = add_exp();
    if (left->val_type_ != right->val_type_)
        throw ErrorMsg(op.line, op.column, "比较运算两侧必须是同种类型");
    TreeNode *node = new TreeNode(comparison_type(op.kind), left, right);
    node->val_type_ = ValType::VT_BOOL;
    return node;
}

TreeNode *GrammarAnalysis::add_exp() {
    TreeNode *left = mul_exp();
    while (match({Token::Kind::TK_ADD, Token::Kind::TK_SUB})) {
        Token op = current(); ++position_;
        TreeNode *right = mul_exp();
        if (left->val_type_ != right->val_type_ || left->val_type_ != ValType::VT_INT)
            throw ErrorMsg(op.line, op.column, "加减运算的操作数必须是int类型");
        left = new TreeNode(op.kind == Token::Kind::TK_ADD ? TreeNode::Type::ADD_EXP : TreeNode::Type::SUB_EXP, left, right);
        left->val_type_ = ValType::VT_INT;
    }
    return left;
}

TreeNode *GrammarAnalysis::mul_exp() {
    TreeNode *left = factor();
    while (match({Token::Kind::TK_MUL, Token::Kind::TK_DIV})) {
        Token op = current(); ++position_;
        TreeNode *right = factor();
        if (left->val_type_ != ValType::VT_INT || right->val_type_ != ValType::VT_INT)
            throw ErrorMsg(op.line, op.column, "乘除运算的操作数必须是int类型");
        left = new TreeNode(op.kind == Token::Kind::TK_MUL ? TreeNode::Type::MUL_EXP : TreeNode::Type::DIV_EXP, left, right);
        left->val_type_ = ValType::VT_INT;
    }
    return left;
}

TreeNode *GrammarAnalysis::factor() {
    if (match({Token::Kind::TK_NOT})) {
        Token op = current(); ++position_;
        TreeNode *operand = factor(); require_bool(operand, "not操作数");
        TreeNode *node = new TreeNode(TreeNode::Type::LOG_NOT_EXP, operand); node->val_type_ = ValType::VT_BOOL; return node;
    }
    if (match({Token::Kind::TK_LP})) {
        ++position_; TreeNode *node = log_or_exp(); take({Token::Kind::TK_RP}, "'('缺少')'的匹配"); return node;
    }
    Token token = take({Token::Kind::NUM, Token::Kind::STR, Token::Kind::ID, Token::Kind::TK_TRUE, Token::Kind::TK_FALSE},
                       "期望数字、字符串、变量、true、false或括号表达式");
    TreeNode *node = new TreeNode(TreeNode::Type::FACTOR, token);
    if (token.kind == Token::Kind::NUM) node->val_type_ = ValType::VT_INT;
    else if (token.kind == Token::Kind::STR) node->val_type_ = ValType::VT_STRING;
    else if (token.kind == Token::Kind::TK_TRUE || token.kind == Token::Kind::TK_FALSE) node->val_type_ = ValType::VT_BOOL;
    else {
        Sym *sym = sym_table_.find(token.value);
        if (!sym) throw ErrorMsg(token.line, token.column, "使用了未声明的变量: " + token.value);
        node->val_type_ = sym->val_type;
    }
    return node;
}

void GrammarAnalysis::print_sym_table() { sym_table_.print(); }
int GrammarAnalysis::get_sym_table_size() { return sym_table_.size(); }
