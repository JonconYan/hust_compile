%error-verbose
%locations
%{
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "def.h"
extern int yylineno;
extern char *yytext;
extern FILE *yyin;
void yyerror(const char* fmt, ...);
void display(struct ASTNode *,int);
// int flag = 1;
%}

%union {
	int    type_int;
	float  type_float;
        char   type_char;
	char   type_id[32];
	struct ASTNode *ptr;
};

//  %type 定义非终结符的语义值类型
%type  <ptr> program ExtDefList ExtDef Specifier StructSpecifier ExtDecList FuncDec CompSt VarList VarDec ParamDec Stmt StmList DefList Def DecList Dec Exp Args

//% token 定义终结符的语义值类型
%token <type_int> INT              /*指定INT的语义值是type_int，有词法分析得到的数值*/
%token <type_id> ID  RELOP TYPE ARRAY  /*指定ID,RELOP 的语义值是type_id，有词法分析得到的标识符字符串*/
%token <type_float> FLOAT          /*指定ID的语义值是type_id，有词法分析得到的标识符字符串*/
%token <type_char> CHAR

%token LP RP LB RB LC RC SEMI COMMA FOR_1
%token bool STRING ASSIGNOP PLUS MINUS STAR DIV AND OR DOT NOT STRUCT RETURN BREAK CONTINUE IF ELSE WHILE FOR SELFADD SELFDEC ADD_ASSIGNOP MINUS_ASSIGNOP STAR_ASSIGNOP DIV_ASSIGNOP
/*以下为接在上述token后依次编码的枚举常量，作为AST结点类型标记*/
%token EXT_DEF_LIST EXT_VAR_DEF FUNC_DEF FUNC_DEC EXT_DEC_LIST PARAM_LIST PARAM_DEC VAR_DEF DEC_LIST DEF_LIST COMP_STM STM_LIST EXP_STMT IF_THEN IF_THEN_ELSE
%token FUNC_CALL ARGS FUNCTION PARAM ARG CALL LABEL GOTO JLT JLE JGT JGE EQ NEQ STRUCT_ACCESS


%right ASSIGNOP ADD_ASSIGNOP MINUS_ASSIGNOP STAR_ASSIGNOP DIV_ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right UMINUS NOT SELFADD SELFDEC

%nonassoc LOWER_THEN_ELSE
%left DOT LP RP LB RB 
%nonassoc ELSE
%nonassoc ARRAY_FIRST

%%
//display($1,0);
program: ExtDefList    { display($1,0);semantic_Analysis0($1); }     //显示语法树,语义分析
         ; 
ExtDefList: {$$=NULL;}
          | ExtDef ExtDefList {$$=mknode(2,EXT_DEF_LIST,yylineno,$1,$2);}   //每一个EXTDEFLIST的结点，其第1棵子树对应一个外部变量声明或函数
          ;  
ExtDef:   Specifier ExtDecList SEMI   {$$=mknode(2,EXT_VAR_DEF,yylineno,$1,$2);}   //该结点对应一个外部变量声明
         |Specifier FuncDec CompSt    {$$=mknode(3,FUNC_DEF,yylineno,$1,$2,$3);}         //该结点对应一个函数定义
         | error SEMI   {$$=NULL;}
         |Specifier SEMI {$$=mknode(1,EXT_VAR_DEF,yylineno,$1);}
         ;
Specifier:  TYPE    {
                        $$=mknode(0,TYPE,yylineno);strcpy($$->type_id,$1);
                        if(!strcmp($1,"int")) $$->type = INT;
                        else if(!strcmp($1,"float")) $$->type = FLOAT;
                        else if(!strcmp($1,"char")) $$->type = CHAR;
                        else if(!strcmp($1,"string")) $$->type = STRING;
                    }
        | StructSpecifier {$$ = $1;}
                    ;
StructSpecifier: STRUCT ID LC DefList RC {$$=mknode(1,STRUCT,yylineno,$4);strcpy($$->type_id,$2);}
        |        STRUCT ID {$$=mknode(1,STRUCT,yylineno,$2);strcpy($$->type_id,$2);}
        |        STRUCT LC DefList RC {$$=mknode(1,STRUCT,yylineno,$3);}
        ;
ExtDecList:  VarDec      {$$=$1;}       /*每一个EXT_DECLIST的结点，其第一棵子树对应一个变量名(ID类型的结点),第二棵子树对应剩下的外部变量名*/
           | VarDec COMMA ExtDecList {$$=mknode(2,EXT_DEC_LIST,yylineno,$1,$3);}
           ;  
VarDec:  ID          {$$=mknode(0,ID,yylineno);strcpy($$->type_id,$1);}   //ID结点，标识符符号串存放结点的type_id
        | VarDec LB Exp RB    {$$=mknode(2,ARRAY,yylineno,$1,$3);}
         ;
FuncDec: ID LP VarList RP   {$$=mknode(1,FUNC_DEC,yylineno,$3);strcpy($$->type_id,$1);}//函数名存放在$$->type_id
	|ID LP  RP   {$$=mknode(0,FUNC_DEC,yylineno);strcpy($$->type_id,$1);$$->ptr[0]=NULL;}//函数名存放在$$->type_id

        ;  
VarList: ParamDec  {$$=mknode(1,PARAM_LIST,yylineno,$1);}
        | ParamDec COMMA  VarList  {$$=mknode(2,PARAM_LIST,yylineno,$1,$3);}
        ;
ParamDec: Specifier VarDec         {$$=mknode(2,PARAM_DEC,yylineno,$1,$2);}
         ;

CompSt: LC DefList StmList RC    {$$=mknode(2,COMP_STM,yylineno,$2,$3);}
       ;
StmList: {$$=NULL; }  
        | Stmt StmList  {$$=mknode(2,STM_LIST,yylineno,$1,$2);}
        ;
Stmt:   Exp SEMI    {$$=mknode(1,EXP_STMT,yylineno,$1);}
      | CompSt      {$$=$1;}      //复合语句结点直接最为语句结点，不再生成新的结点
      | RETURN Exp SEMI   {$$=mknode(1,RETURN,yylineno,$2);}
      | IF LP Exp RP Stmt %prec LOWER_THEN_ELSE   {$$=mknode(2,IF_THEN,yylineno,$3,$5);}
      | IF LP Exp RP Stmt ELSE Stmt   {$$=mknode(3,IF_THEN_ELSE,yylineno,$3,$5,$7);}
      | FOR LP Exp SEMI Exp SEMI Exp RP Stmt {$$=mknode(4,FOR,yylineno,$3,$5,$7,$9);}
      | FOR LP Exp COMMA Exp SEMI SEMI Exp RP Stmt {$$=mknode(4,FOR_1,yylineno,$3,$5,$8,$10);}
      | WHILE LP Exp RP Stmt {$$=mknode(2,WHILE,yylineno,$3,$5);}
      ;
DefList: {$$=NULL; }
        | Def DefList {$$=mknode(2,DEF_LIST,yylineno,$1,$2);}
        | error SEMI   {$$=NULL;}
        ;
Def:    Specifier DecList SEMI {$$=mknode(2,VAR_DEF,yylineno,$1,$2);}
        ;
DecList: Dec  {$$=mknode(1,DEC_LIST,yylineno,$1);}
       | Dec COMMA DecList  {$$=mknode(2,DEC_LIST,yylineno,$1,$3);}
	   ;
Dec:     VarDec  {$$=$1;}
       | VarDec ASSIGNOP Exp  {$$=mknode(2,ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"ASSIGNOP");}
       ;
Exp:    Exp ASSIGNOP Exp {$$=mknode(2,ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"ASSIGNOP");}//$$结点type_id空置未用，正好存放运算符
      | Exp AND Exp   {$$=mknode(2,AND,yylineno,$1,$3);strcpy($$->type_id,"AND");}
      | Exp OR Exp    {$$=mknode(2,OR,yylineno,$1,$3);strcpy($$->type_id,"OR");}
      | Exp RELOP Exp {$$=mknode(2,RELOP,yylineno,$1,$3);strcpy($$->type_id,$2);}  //词法分析关系运算符号自身值保存在$2中
      | Exp PLUS Exp  {$$=mknode(2,PLUS,yylineno,$1,$3);strcpy($$->type_id,"PLUS");}
      | Exp MINUS Exp {$$=mknode(2,MINUS,yylineno,$1,$3);strcpy($$->type_id,"MINUS");}
      | Exp STAR Exp  {$$=mknode(2,STAR,yylineno,$1,$3);strcpy($$->type_id,"STAR");}
      | Exp DIV Exp   {$$=mknode(2,DIV,yylineno,$1,$3);strcpy($$->type_id,"DIV");}
      | LP Exp RP     {$$=$2;}
      | MINUS Exp %prec UMINUS   {$$=mknode(1,UMINUS,yylineno,$2);strcpy($$->type_id,"UMINUS");}
      | NOT Exp       {$$=mknode(1,NOT,yylineno,$2);strcpy($$->type_id,"NOT");}
      //| DPLUS  Exp      {$$=mknode(1,DPLUS,yylineno,$2);strcpy($$->type_id,"DPLUS");}
      //|   Exp DPLUS      {$$=mknode(1,DPLUS,yylineno,$1);strcpy($$->type_id,"DPLUS");}
      | Exp SELFADD   {$$=mknode(1,SELFADD,yylineno,$1);strcpy($$->type_id,"RIGHTADD");}
      | SELFADD Exp   {$$=mknode(1,SELFADD,yylineno,$2);strcpy($$->type_id,"LEFTADD");}
      | Exp SELFDEC   {$$=mknode(1,SELFDEC,yylineno,$1);strcpy($$->type_id,"RIGHTDEC");}
      | SELFDEC Exp   {$$=mknode(1,SELFDEC,yylineno,$2);strcpy($$->type_id,"LEFTDEC");}
      | Exp ADD_ASSIGNOP Exp  {$$=mknode(2,ADD_ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"ADD_ASSIGNOP");}
      | Exp MINUS_ASSIGNOP Exp {$$=mknode(2,MINUS_ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"MINUS_ASSIGNOP");}
      | Exp STAR_ASSIGNOP Exp  {$$=mknode(2,STAR_ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"STAR_ASSIGNOP");}
      | Exp DIV_ASSIGNOP Exp   {$$=mknode(2,DIV_ASSIGNOP,yylineno,$1,$3);strcpy($$->type_id,"DIV_ASSIGNOP");}
      | VarDec {$$ = $1;}

      | Exp DOT Exp   {$$=mknode(2,STRUCT_ACCESS,yylineno,$1,$3);}
      | ID LP Args RP {$$=mknode(1,FUNC_CALL,yylineno,$3);strcpy($$->type_id,$1);}
      | ID LP RP      {$$=mknode(0,FUNC_CALL,yylineno);strcpy($$->type_id,$1);}
      | ID            {$$=mknode(0,ID,yylineno);strcpy($$->type_id,$1);}
      | INT           {$$=mknode(0,INT,yylineno);$$->type_int=$1;$$->type=INT;}
      | FLOAT         {$$=mknode(0,FLOAT,yylineno);$$->type_float=$1;$$->type=FLOAT;}
      | CHAR          {$$=mknode(0,CHAR,yylineno);$$->type_char = $1;}
      | BREAK         {$$=mknode(0,BREAK,yylineno);strcpy($$->type_id,"BREAK");}
      | CONTINUE      {$$=mknode(0,CONTINUE,yylineno);strcpy($$->type_id,"CONTINUE");}
      ;
Args:    Exp COMMA Args    {$$=mknode(2,ARGS,yylineno,$1,$3);}
       | Exp               {$$=mknode(1,ARGS,yylineno,$1);}
       ;
       
%%

int main(int argc, char *argv[]){
	yyin=fopen(argv[1],"r");
	if (!yyin) return;
	yylineno=1;
	yyparse();
	return 0;
	}

#include<stdarg.h>
void yyerror(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Grammar Error at Line %d Column %d: ", yylloc.first_line,yylloc.first_column);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, ".\n");
}
