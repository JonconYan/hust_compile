%{
#include "parser.tab.h"
#include "string.h"
#include "def.h"
int yycolumn=1;
#define YY_USER_ACTION    yylloc.first_line=yylloc.last_line=yylineno; \
	yylloc.first_column=yycolumn;\
    yylloc.last_column=yycolumn+yyleng-1;\ 
    yycolumn+=yyleng;
typedef union {
	int type_int;
	int type_float;
    char type_char;
	char type_id[32];
	struct node *ptr;
} YYLVAL;
#define YYSTYPE YYLVAL

%}
%option yylineno

id   [A-Za-z][A-Za-z0-9]*  
int  ([1-9][0-9]*)|0
float  ([0-9]*\.[0-9]+)|([0-9]+\.)
char 	\'[0-9a-zA-Z]\'
string 	(\".*\")


%%
{int}        {printf("(%s , INT)\n",yytext);yylval.type_int=atoi(yytext); return INT;}
{float}      {printf("(%s , FLOAT)\n",yytext);yylval.type_float=atof(yytext); return FLOAT;}
{char}       {printf("(%s  ,CHAR)\n",yytext);yylval.type_char=yytext[1];return CHAR;}
"int"        {printf("(int , TYPE)\n");strcpy(yylval.type_id,  yytext);return TYPE;}
"float"      {printf("(float , TYPE)\n");strcpy(yylval.type_id,  yytext);return TYPE;}
"char"       {printf("(char , TYPE)\n");strcpy(yylval.type_id,  yytext);return TYPE;}
"string"     {printf("(string , TYPE)\n");strcpy(yylval.type_id,  yytext);return TYPE;}

"struct"     {printf("(struct , STRUCT)\n");return STRUCT;}

"return"     {printf("(return , RETURN)\n");return RETURN;}
"if"         {printf("(if , IF)\n");return IF;}
"else"       {printf("(else , ELSE)\n");return ELSE;}
"while"      {printf("(while , WHILE)\n");return WHILE;}
"break"      {printf("(break , BREAK)\n");return BREAK;}
"continue"   {printf("(continue , CONTINUE)\n");return CONTINUE;}
"for"        {printf("(for , FOR)\n");return FOR;}

{id}       {printf("(%s , ID)\n",yytext);strcpy(yylval.type_id,  yytext); return ID;/*由于关键字的形式也符合标识符的规则，所以把关键字的处理全部放在标识符的前面，优先识别*/}
";"			{printf("(; , SEMI)\n");return SEMI;}
","			{printf("(, , COMMA)\n");return COMMA;}
">"|"<"|">="|"<="|"=="|"!=" {printf("(%s , RELOP)\n",yytext);strcpy(yylval.type_id, yytext);return RELOP;}
"="			{printf("(= , ASSIGNOP)\n");return ASSIGNOP;}
"+"			{printf("(+ , PLUS)\n");return PLUS;}
"-"			{printf("(- , MINUS)\n");return MINUS;}
"*"			{printf("(* , STAR)\n");return STAR;}
"/"			{printf("(/ , DIV)\n");return DIV;}
"++"        {printf("(++ , SELFADD)\n");return SELFADD;}
"--"        {printf("(-- , SELFDEC)\n");return SELFDEC;}
"+="        {printf("(+= , ADD_ASSIGNOP)\n");return ADD_ASSIGNOP;}
"-="        {printf("(-= , MINUS_ASSIGNOP)\n");return MINUS_ASSIGNOP;}
"*="        {printf("(*= , STAR_ASSIGNOP)\n");return STAR_ASSIGNOP;}
"/="        {printf("(/= , DIV_ASSIGNOP)\n");return DIV_ASSIGNOP;}
"&&"        {printf("(&& , AND)\n");return AND;}
"||"        {printf("(|| , OR)\n");return OR;}
"!"			{printf("(! , NOT)\n");return NOT;}
"("			{printf("(( , LP)\n");return LP;}
")"			{printf("() , RP)\n");return RP;}
"{"			{printf("({ , LC)\n");return LC;}
"}"			{printf("(} , RC)\n");return RC;}
"."         {printf("(. , DOT)\n");return DOT;}
"["         {printf("([ , LB)\n");return LB;}
"]"         {printf("(] , RB)\n");return RB;}
[\n]           {yycolumn=1;}   
[ \r\t]          {}   
"//"		{char c=input();while(c!='\n') c=input();}
"/*"		{
            char c = input();
            loop:
            while(c!='*') c = input();
            c = input();
            if(c != '/') goto loop;
            }
.			{printf("Error type A :Mysterious character \"%s\"\n\t at Line %d\n",yytext,yylineno);}
%%

/* 和bison联用时，不需要这部分
void main()
{
yylex();
return 0;
}

*/
int yywrap()
{
return 1;
}
   
// "//"		{char c=input();while(c!='\n') c=input();}
// "/*"		{
//             char c = input();
//             loop:
//             while(c!='*') c = input();
//             c = input();
//             if(c != '/') goto loop;
//             }
// .			{printf("Error type A :Mysterious character \"%s\"\n\t at Line %d\n",yytext,yylineno);}

// %%
// int yywrap()
// {
// return 1;
// }

