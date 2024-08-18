#include <unordered_map>
#include <memory>
#include <vector>

namespace ast {

// base class for the AST
class ExprAST {
public:
  virtual ~ExprAST() = default;
};

/* token node definition */

// number expr ast class to hold numbers
class NumberExprAST: public ExprAST {
public:
  NumberExprAST(double value)
    : value(value) {}

private:
  double value;
};

// identifier expr ast class for holding variables 
class VariableExprAST: public ExprAST {
public:
  VariableExprAST(const std::string& name)
    : name(name) {}

private:
  std::string name;
};

/* expressions start here */

// binary expressions node
class BinaryExprAST: public ExprAST {
public:
  BinaryExprAST(char operation, 
      std::unique_ptr<ExprAST> lhs, 
      std::unique_ptr<ExprAST> rhs)
    : operation(operation), 
      lhs(std::move(lhs)), 
      rhs(std::move(rhs)) {}

private:
  char operation;
  std::unique_ptr<ExprAST> lhs, rhs;
};

// function call expression
class CallExprAST: public ExprAST {
public:
  CallExprAST(const std::string& callee,
      std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), 
      args(std::move(args)) {}

private:
  std::string callee;
  std::vector<std::unique_ptr<ExprAST>> args;
};

/* function definition nodes */

// function prototype node
class PrototypeAST {
public:
  PrototypeAST(const std::string& name,
      std::vector<std::string> args)
    : name(name), 
      args(std::move(args)) {}

  const std::string& get_name() const {
    return this->name;
  }

private:
  std::string name;
  std::vector<std::string> args;
};

// function node
class FunctionAST {
public:
  FunctionAST(std::unique_ptr<PrototypeAST> proto,
      std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), 
      body(std::move(body)) {}

private:
  std::unique_ptr<PrototypeAST> proto;
  std::unique_ptr<ExprAST> body;
};

};

