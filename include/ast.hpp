#include <unordered_map>
#include <memory>
#include <vector>

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

class IfExprAST: public ExprAST {
public:
  IfExprAST(std::unique_ptr<ExprAST> cond, 
      std::unique_ptr<ExprAST> then,
      std::unique_ptr<ExprAST> else_) 
    : cond(cond), then(then), else_(else_) {}

  llvm::Value *codegen() override;

private:
  std::unique_ptr<ExprAST> cond, then, else_;
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

// define the overriden IR generation code

namespace ast {

static std::unique_ptr<llvm::LLVMContext> _context;
static std::unique_ptr<llvm::Module> _module;
static std::unique_ptr<llvm::IRBuilder<>> _builder;
static std::unordered_map<std::string, llvm::Value *> named_values;

static void init_ir() {
  _context = std::make_unique<llvm::LLVMContext>();
  _module = std::make_unique<llvm::Module>("module", *_context);
  _builder = std::make_unique<llvm::IRBuilder<>>(*_context);
}

llvm::Value *NumberExprAST::codegen() {
  return llvm::ConstantFP::get(*_context, llvm::APFloat(this->value));
}

llvm::Value *VariableExprAST::codegen() {
  llvm::Value *_value = named_values[this->name];   
  if (!_value) {
    std::cerr << "name '" << this->name << "' is not defined" << std::endl;
  }return _value;
}

llvm::Value *IfExprAST::codegen() {
  return nullptr;
}

llvm::Value *BinaryExprAST::codegen() {
  llvm::Value *_right = this->rhs->codegen();
  llvm::Value *_left = this->lhs->codegen();

  if (!_right||!_left) {
    return nullptr;
  }

  switch(this->operation) {
    case '*':
      return _builder->CreateFMul(_left, _right, "multmp");
    case '+':
      return _builder->CreateFAdd(_left, _right, "addtmp");
    case '-':
      return _builder->CreateFSub(_left, _right, "subtmp");
    case '<':
      _left = _builder->CreateFCmpULT(_left, _right, "cmptmp");
      return _builder->CreateUIToFP(_left, 
          llvm::Type::getDoubleTy(*_context),
          "booltmp");
    default:
      std::cerr << "operation '" << this->operation << "' not recognised" << std::endl;
      return nullptr;
  }

}

llvm::Value *CallExprAST::codegen() {
 
  // first finding the function definition in the code somewhere.
  llvm::Function *function_call = _module->getFunction(this->callee);
 
  // checking for validity of the function arguments
  if (function_call->arg_size() != this->args.size()) {
    std::cerr << "number of arguments don't match" << std::endl;
    return nullptr;
  }

  std::vector<llvm::Value *> arg_code;
  for (unsigned i = 0; i < this->args.size(); ++i) {
    arg_code.push_back(this->args[i]->codegen());
    if (!arg_code.back()) {
      std::cerr << "invalid argument '" << arg_code.back() << "'" << std::endl;
      return nullptr;
    }
  }

  return _builder->CreateCall(function_call, arg_code, "calltmp");
}

llvm::Function *PrototypeAST::codegen() {

  // defining the argument types --> <function_return_type>(double, double.....);
  std::vector<llvm::Type *> arg_types(this->args.size(), 
      llvm::Type::getDoubleTy(*_context)); 

  // defining the fucntion return type --> double for this language
  llvm::FunctionType *_function_type = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(*_context), 
      arg_types, false); // isVarArgs=false

  llvm::Function *_function = llvm::Function::Create(
      _function_type, llvm::Function::ExternalLinkage, this->name,
      _module.get());

  // setting names to all the function args
  // this is unnecessay, the IRBuilder automatically sets default names to the args
  uint8_t t = 0;
  for (auto& arg: _function->args()) {
    arg.setName(this->args[t++]);
  }

  return _function;
}

llvm::Function *FunctionAST::codegen() {

  // checking for previously defined prototype by the 'extern' keyword
  llvm::Function *_function = _module->getFunction(this->proto->get_name());
  if (!_function) {
    // if the function prototype was never defined then get the defintion directly
    _function = this->proto->codegen();
  }

  if (!_function) {
    std::cerr << "syntax error in the definition of the function prototype" << std::endl;
    return nullptr;
  }

  llvm::BasicBlock *_basic_block = llvm::BasicBlock::Create(*_context, 
      "entrypoint", _function);
  _builder->SetInsertPoint(_basic_block);

  // named_values.clear(); // clearning the named_values to store named values of a different scope
  for (auto& arg: _function->args()) {
    named_values[std::string(arg.getName())] = &arg;
  }

  if (llvm::Value *return_value = this->body->codegen()) {
    _builder->CreateRet(return_value);
    llvm::verifyFunction(*_function);
    return _function;
  }

  // a function with no return type will raise an error
  _function->eraseFromParent(); 
  return nullptr;
}

};

