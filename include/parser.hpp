#include "lexer.hpp"
#include "ast.hpp"

namespace parser {
  static std::unique_ptr<ast::ExprAST> parse_expression();
  static std::unique_ptr<ast::ExprAST> parse_paren_expr();
  static std::unique_ptr<ast::ExprAST> parse_number_expr();
  static std::unique_ptr<ast::ExprAST> parse_identifier_expr();
  static std::unique_ptr<ast::ExprAST> prase_primary();
  static std::unique_ptr<ast::ExprAST> parse_bin_op_rhs();
  static std::unique_ptr<ast::PrototypeAST> parse_prototype();
  static std::unique_ptr<ast::PrototypeAST> parse_extern();
  static std::unique_ptr<ast::FunctionAST> parse_function();
  static std::unique_ptr<ast::FunctionAST> parse_top_level_expr();
};

extern char *file_content;
extern int ptr;

namespace parser {

static int curr_token;

static int get_next_token() {
  return curr_token = lexer::get_token(file_content, ptr);
}

// number_expr := number 
static std::unique_ptr<ast::ExprAST> parse_number_expr() {
  auto res = std::make_unique<ast::NumberExprAST>(lexer::num_value);
  get_next_token(); // consume the number
  return std::move(res);
}

// paren_expr := ( `expr` )
static std::unique_ptr<ast::ExprAST> parse_paren_expr() {
  get_next_token(); // eat '('
  auto expr = parse_expression();
  
  if (!expr) {
    return nullptr;
  }
  
  if (curr_token != ')') {
    std::cerr << "parenthesis mismatch" << std::endl;
    return nullptr;
  }

  get_next_token(); // eat ')'
  return expr;
}

// identifier_expr := another_identifier_expr;
// identifier_expr := function(args..);
static std::unique_ptr<ast::ExprAST> parse_identifier_expr() {
  
  std::string identifier_name = lexer::identifier_str;
  get_next_token(); // eat identifier

  if (curr_token != '(') {
    // this if for just assigning another identifier expr
    return std::make_unique<ast::VariableExprAST>(
        identifier_name);
  }

  // this is for the function call
  get_next_token(); // eat '('
  std::vector<std::unique_ptr<ast::ExprAST>> args;
  if (curr_token != ')') {
    while (true) {
      if (auto arg = parse_expression()) {
        args.push_back(std::move(arg));
      }else {
        return nullptr;
      }

      if (curr_token == ')'){
        break;
      }

      if (curr_token != ',') {
        std::cerr << "syntax error: expected a ','  or ')' in the arguments" << std::endl;
        return nullptr;
      }

      get_next_token(); // eat ','
    }
  }

  get_next_token(); // eat ')'

  return std::make_unique<ast::CallExprAST>(
      identifier_name, std::move(args));
}

// merge the upper functions
// now this method is capable to handling the following
// primary := identifier_expr
// primary := function(args..)
// primary := number_expr
// primary := paren_expr
static std::unique_ptr<ast::ExprAST> prase_primary() {
  switch (curr_token) {
    case tok_identifier:
      return parse_identifier_expr();
    case tok_number:
      return parse_number_expr();
    case '(':
      return parse_paren_expr();
    default:
      std::cerr << "unknown error" << std::endl;
      return nullptr;
  }
}

/* binary expression parsing */
// binary operation precedence
static std::unordered_map<char, int> binary_op_precedence;

static int get_operation_precedence() {
  if (!isascii(curr_token)) {
    return -1;
  }

  return ((binary_op_precedence[curr_token] <= 0) ? 
      -1 : binary_op_precedence[curr_token]);
}

// parses a binsry operation from left to right
/*
  e.g.
  a = b * c + d
  the expression is b * c + d
  first lhs is popped and put the tree 'T' (this is not a part of this function)
  so 'T' contains b 
  now we run the funtion of the rest of the expression
  T(b) , '* c + d' which is the rhs
  now c is also processing along with the operation *
  and added to tree 'T'
  T(b * c), '+ d' which is the new rhs
*/
static std::unique_ptr<ast::ExprAST> parse_bin_op_rhs(
    int expr_prec,
    std::unique_ptr<ast::ExprAST> lhs) {

  while (true) {
    int token_prec = get_operation_precedence();
    
    if (token_prec < expr_prec) {
      return lhs;
    }

    int bin_op = curr_token;
    get_next_token(); // consume the binary operator

    auto rhs = prase_primary();
    if (!rhs) {
      return nullptr;
    }

    int next_prec = get_operation_precedence();
    if (token_prec < next_prec) {
      rhs = parse_bin_op_rhs(token_prec + 1, std::move(rhs));
      if (!rhs) {
        return nullptr;
      }
    }
    
    lhs = std::make_unique<ast::BinaryExprAST>(bin_op, 
        std::move(rhs), std::move(lhs));
  }
}

static std::unique_ptr<ast::ExprAST> parse_expression() {
  auto lhs = prase_primary();
  if (!lhs) {
    return nullptr;
  }

  return parse_bin_op_rhs(0, std::move(lhs));
}

// prototype parsing
static std::unique_ptr<ast::PrototypeAST> parse_prototype() {
  if (curr_token != tok_identifier) {
    std::cerr << "expected function name in the prototype" << std::endl;
    return nullptr;
  }

  std::string function_name = lexer::identifier_str;
  get_next_token(); 

  if (curr_token != '(') {
    std::cerr << "syntax error: function prototype must contain '('" << std::endl;
    return nullptr;
  }

  std::vector<std::string> arg_names;
  while (get_next_token() == tok_identifier) {
    arg_names.push_back(lexer::identifier_str);
  }

  if (curr_token != ')') {
    std::cerr << "syntax error: function prototype must end with a ')'" << std::endl;
    return nullptr;
  }

  get_next_token(); // eat ')'
  return std::make_unique<ast::PrototypeAST>(function_name, 
      std::move(arg_names));
}

// parse external
static std::unique_ptr<ast::PrototypeAST> parse_extern() {
  get_next_token(); // eat 'extern'
  return parse_prototype();
}

// funcition parsing
static std::unique_ptr<ast::FunctionAST> parse_function() {
  get_next_token(); // eat 'def'
  auto proto = parse_prototype();
  if (!proto) {
    return nullptr;
  }

  while (curr_token == '\n') {
    get_next_token(); // consume all the new line characters
  }

  if (auto body = parse_expression()) {
    return std::make_unique<ast::FunctionAST>(std::move(proto), 
        std::move(body));
  }

  return nullptr;
}

// top level expressions
static std::unique_ptr<ast::FunctionAST> parse_top_level_expr() {
  if (auto body = parse_expression()) {
    auto proto = std::make_unique<ast::PrototypeAST>("__anon_expr", 
        std::vector<std::string>());
    return std::make_unique<ast::FunctionAST>(
        std::move(proto), std::move(body));
  }
  return nullptr;
}

static void handle_definition() {
  if (parse_function()) {
    std::cerr << "Parsed a function definition" << std::endl;
  } else {
    get_next_token();
  }
}

static void handle_extern() {
  if (parse_extern()) {
    std::cerr << "Parsed an extern" << std::endl;
  } else {
    get_next_token();
  }
}

static void handle_top_level_expr() {
  if (parse_top_level_expr()) {
    std::cerr << "Parsed a top-level expr" << std::endl;
  } else {
    get_next_token();
  }
}

};

