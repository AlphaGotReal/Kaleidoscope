#include <unordered_map>
#include <memory>
#include <vector>

#include "llvm/IR/Value.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

namespace ast {

// base class for the AST
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual llvm::Value *codegen() = 0;
};

/* token node definition */

// number expr ast class to hold numbers
class NumberExprAST: public ExprAST {
public:
  NumberExprAST(double value)
    : value(value) {}

  llvm::Value *codegen() override;

private:
  double value;
};

// identifier expr ast class for holding variables 
class VariableExprAST: public ExprAST {
public:
  VariableExprAST(const std::string& name)
    : name(name) {}

  llvm::Value *codegen() override;

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

  llvm::Value *codegen() override;

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

  llvm::Value *codegen() override;

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

  llvm::Function *codegen();

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

  llvm::Function *codegen();

private:
  std::unique_ptr<PrototypeAST> proto;
  std::unique_ptr<ExprAST> body;
};

};

// override IR code generation 

namespace ast {

}

