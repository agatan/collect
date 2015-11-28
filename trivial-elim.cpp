#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_os_ostream.h"

#include <string>
#include <iostream>

static clang::Rewriter rewriter;

std::string createPanicStmt(std::string const& file_name, std::string const& func_name)
{
  std::string ret = R"str({
#ifdef _LINUX_KERNEL_H
  panic(")str";
  ret += file_name;
  ret += R"str(: Execute eliminated function()str";
  ret += func_name;
  ret += R"str().");
#endif
})str";
  return ret;
}

class FindTrivialFuncVisitor : public clang::RecursiveASTVisitor<FindTrivialFuncVisitor>
{
public:
  explicit FindTrivialFuncVisitor(clang::ASTContext *context)
    : context(context)
  {
    rewriter.setSourceMgr(context->getSourceManager(), context->getLangOpts());
  }

  bool VisitFunctionDecl(clang::FunctionDecl *function)
  {
    auto name = function->getNameInfo().getName().getAsString();
    if (function->getBody() == nullptr) {
      return true;
    }
    clang::SourceLocation start = function->getBody()->getLocStart();
    clang::SourceLocation end = function->getBody()->getLocEnd();

    auto file_name = context->getSourceManager().getFilename(start);
    if (!file_name.endswith(".c")) {
      return true;
    }
    auto range = clang::SourceRange(start, end);
    llvm::StringRef replace(createPanicStmt(file_name, name));
    unsigned length = rewriter.getRangeSize(range);
//     rewriter.ReplaceText(range.getBegin(), length, replace);
    std::cout << name << "," << start.printToString(context->getSourceManager()) << "," << end.printToString(context->getSourceManager()) << std::endl;
    return true;
  }

private:
  clang::ASTContext *context;
};

class FindTrivialFuncConsumer : public clang::ASTConsumer
{
public:
  explicit FindTrivialFuncConsumer(clang::ASTContext *context)
    : visitor(context) {}

  virtual void HandleTranslationUnit(clang::ASTContext& context)
  {
    visitor.TraverseDecl(context.getTranslationUnitDecl());
  }

private:
  FindTrivialFuncVisitor visitor;
};

class FindTrivialFuncAction : public clang::ASTFrontendAction
{
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &ci,
                    llvm::StringRef)
  {
    return std::unique_ptr<clang::ASTConsumer>(new FindTrivialFuncConsumer(&ci.getASTContext()));
  }
};

static llvm::cl::OptionCategory my_tool_category("trivial function elimination");

int main(int argc, const char** argv)
{
  clang::tooling::CommonOptionsParser options_parser(argc, argv, my_tool_category);
  clang::tooling::ClangTool tool(options_parser.getCompilations(),
                                 options_parser.getSourcePathList());
  int result = tool.run(clang::tooling::newFrontendActionFactory<FindTrivialFuncAction>().get());

//     std::cerr << "overwrite error" << std::endl;
//   }
//   rewriter.getEditBuffer(rewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
  return result;
}
