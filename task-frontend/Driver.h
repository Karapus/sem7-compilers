#pragma once
#include "AST.h"
#include "Grammar.tab.hh"
#include "Lexer.h"

namespace yy {
struct Driver final {
  Lexer lexer;
  AST::INode *yylval;
  Driver(std::istream *is) : lexer(is), yylval(nullptr) {}
  AST::ASTModule *parse() {
    yy::parser parser{*this};
    if (parser())
      return nullptr;
    return static_cast<AST::ASTModule *>(yylval);
  }
};
} // namespace yy
