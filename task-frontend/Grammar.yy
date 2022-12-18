%language "c++"
%require "3.2"
%defines
%locations

%code requires {
	namespace yy {
		class Driver;
	}
	#include "AST.h"
}

%define api.token.raw
%define api.value.type {AST::INode *}
%parse-param { Driver &driver }
%code {
	#include "Driver.h"
	#undef	yylex
	#define	yylex driver.lexer.yylex
	using namespace AST;
}

%define parse.trace
%define parse.error verbose
%define parse.lac full

%define api.token.prefix {TOK_}
%token
	END	0
	PLUS
	MINUS
	STAR
	SLASH
	PERCNT
	EQ
	ASSIGN
        LET
	EXCL
	NEQ
	LE
	GE
	LT
        GT
	AND
	OR
	QMARK
	PRINT
	WHILE
	IF
	ELSE
	LBRACE
	RBRACE
	LPAR
	RPAR
	SEMICOLON
	COLON
	COMA
	NUM
	ID
	FUNC
	RETURN

%destructor { delete $$; } ID NUM funcs func declist decls scope blocks block stm expr applist exprs

%right ELSE THEN
%precedence PRINT
%left OR
%left AND
%precedence ASSIGN
%left EQ NEQ LE GE LT GT
%left PLUS MINUS
%left STAR SLASH PERCNT
%precedence UNOP

%start program
%%
program : funcs END { driver.yylval = $1; };

funcs : funcs func { $$ = static_cast<ASTModule *>($1)->addFunction($2); }
| func { $$ = (new ASTModule)->addFunction($1); }

func : FUNC ID declist scope { $$ = static_cast<ExprFunc *>($3)->addBody($4)->addId($2)->addLocation(@$); };

declist : LPAR decls RPAR { $$ = $2; }
| LPAR RPAR { $$ = new ExprFunc(); };

decls : decls COMA ID { $$ = static_cast<ExprFunc *>($1)->addArgDecl($3); }
| ID { $$ = (new ExprFunc)->addArgDecl($1); };

scope : LBRACE blocks RBRACE { $$ = static_cast<Scope *>($2)->addLocation(@$); };

blocks : blocks block { $$ = static_cast<Scope *>($1)->addBlock($2); }
| %empty { $$ = new Scope(); };

block : stm { $$ = $1; }
| expr SEMICOLON { $$ = $1; }
| SEMICOLON { $$ = new Empty(@$); }
| error { $$ = new Empty(@$); };

stm : scope { $$ = $1; }
| WHILE LPAR expr RPAR block { $$ = new While(@$, $3, $5); }
| IF LPAR expr RPAR block ELSE block { $$ = new If(@$, $3, $5, $7); }
| IF LPAR expr RPAR block %prec THEN { $$ = new If(@$, $3, $5); }
| LET ID expr SEMICOLON { $$ = new Let(@$, $2, $3); }
| RETURN expr SEMICOLON { $$ = new Return(@$, $2); };

expr : LPAR expr RPAR { $$ = $2; }
| NUM { $$ = $1; }
| ID { $$ = $1; }
| QMARK { $$ = new ExprQmark(@$); }
| PRINT expr { $$ = new ExprPrint(@$, $2); }
| ID ASSIGN expr { $$ = new ExprAssign(@$, $1, $3); }
| ID applist { $$ = static_cast<ExprApply *>($2)->addId($1)->addLocation(@$); }
| PLUS expr %prec UNOP { $$ = new ExprUnOp<UnOpPlus>(@$, $2); }
| MINUS expr %prec UNOP { $$ = new ExprUnOp<UnOpMinus>(@$, $2); }
| EXCL expr %prec UNOP { $$ = new ExprUnOp<UnOpNot>(@$, $2); }
| expr PLUS expr { $$ = new ExprBinOp<BinOpPlus>(@$, $1, $3); }
| expr MINUS expr { $$ = new ExprBinOp<BinOpMinus>(@$, $1, $3); }
| expr STAR expr { $$ = new ExprBinOp<BinOpMul>(@$, $1, $3); }
| expr SLASH expr { $$ = new ExprBinOp<BinOpDiv>(@$, $1, $3); }
| expr PERCNT expr { $$ = new ExprBinOp<BinOpMod>(@$, $1, $3); }
| expr LT expr { $$ = new ExprBinOp<BinOpLess>(@$, $1, $3); }
| expr GT expr { $$ = new ExprBinOp<BinOpGrtr>(@$, $1, $3); }
| expr LE expr { $$ = new ExprBinOp<BinOpLessOrEq>(@$, $1, $3); }
| expr GE expr { $$ = new ExprBinOp<BinOpGrtrOrEq>(@$, $1, $3); }
| expr EQ expr { $$ = new ExprBinOp<BinOpEqual>(@$, $1, $3); }
| expr NEQ expr { $$ = new ExprBinOp<BinOpNotEqual>(@$, $1, $3); }
| expr AND expr { $$ = new ExprBinOp<BinOpAnd>(@$, $1, $3); }
| expr OR expr { $$ = new ExprBinOp<BinOpOr>(@$, $1, $3); };

applist : LPAR exprs RPAR { $$ = $2; }
| LPAR RPAR { $$ = new ExprApply(); };

exprs : exprs COMA expr { $$ = static_cast<ExprApply *>($1)->addArg($3); }
| expr { $$ = (new ExprApply())->addArg($1); };

%%

void yy::parser::error(const location_type &loc, const std::string &err_message) {
  std::cerr << "Error: " << err_message << " at " << loc << std::endl;
}
