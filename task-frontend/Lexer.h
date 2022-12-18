#pragma once

#ifndef yyFlexLexerOnce
#include <FlexLexer.h>
#endif

#undef YY_DECL
#define YY_DECL                                                                \
  yy::parser::token::yytokentype yy::Lexer::yylex(                             \
      yy::parser::semantic_type *yylval, yy::location *yyloc)

namespace yy {
struct Lexer : public yyFlexLexer {
  using FlexLexer::yylex;
  yy::parser::token::yytokentype yylex(yy::parser::semantic_type *yylval,
                                       yy::location *yyloc);
  Lexer(std::istream *in) : yyFlexLexer(in) {}
};
} // namespace yy
