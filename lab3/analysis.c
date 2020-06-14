#include "def.h"
#define DEBUG 1
int is_loop = 0;
//int array_num = 0;
char loop_label[15];
char break_label[15];
char *strcat0(char *s1,char *s2){
    static char result[10];
    strcpy(result,s1);
    strcat(result,s2);
    return result;
}

char *newAlias() {
    static int no=1;
    char s[10];
    sprintf(s,"%d",no++);
    //itoa(no++,s,10);
    return strcat0("v",s);
}

char *newLabel() {
    static int no=1;
    char s[10];
    //itoa(no++,s,10);
    sprintf(s,"%d",no++);
    return strcat0("label",s);
}

char *newTemp(){
    static int no=1;
    char s[10];
    //itoa(no++,s,10);
    sprintf(s,"%d",no++);
    return strcat0("temp",s);
}

//生成一条TAC代码的结点组成的双向循环链表，返回头指针
struct codenode *genIR(int op,struct opn opn1,struct opn opn2,struct opn result){
    struct codenode *h=(struct codenode *)malloc(sizeof(struct codenode));
    h->op=op;
    h->opn1=opn1;
    h->opn2=opn2;
    h->result=result;
    h->next=h->prior=h;
    return h;
}

//生成一条标号语句，返回头指针
struct codenode *genLabel(char *label){
    struct codenode *h=(struct codenode *)malloc(sizeof(struct codenode));
    h->op=LABEL;
    strcpy(h->result.id,label);
    h->next=h->prior=h;
    return h;
}

//生成GOTO语句，返回头指针
struct codenode *genGoto(char *label){
    struct codenode *h=(struct codenode *)malloc(sizeof(struct codenode));
    h->op=GOTO;
    strcpy(h->result.id,label);
    h->next=h->prior=h;
    return h;
}

//合并多个中间代码的双向循环链表，首尾相连
struct codenode *merge(int num,...){
    struct codenode *h1,*h2,*p,*t1,*t2;
    va_list ap;
    va_start(ap,num);
    h1=va_arg(ap,struct codenode *);
    while (--num>0) {
        h2=va_arg(ap,struct codenode *);
        if (h1==NULL) h1=h2;
        else if (h2){
            t1=h1->prior;
            t2=h2->prior;
            t1->next=h2;
            t2->next=h1;
            h1->prior=t2;
            h2->prior=t1;
            }
        }
    va_end(ap);
    return h1;
}
struct array_info* array_check(struct ASTNode* root,int type)
{
    int each_dim[4];//记录每一层的数据是多少
    struct ASTNode *temp = root;
    struct array_info *a = (struct array_info *)malloc(sizeof(struct array_info));
    a->dim = 0;
    a->type = type;
    while(temp->kind==ARRAY)
    {
        a->dim++;
        if(temp->ptr[1]->kind!=INT)
        {   
            semantic_error(temp->pos,"", "数组大小必须为int");
            return NULL;
        }
        a->diminfo[a->dim-1]=temp->ptr[1]->type_int;
        temp=temp->ptr[0];
    }
    //printf("%s",temp->type_id);
    strcpy(a->id,temp->type_id);
    //printf("%s",a->id);
    //printf("dim:%d\n",a->dim);
    return a;
}

//填写临时变量到符号表，返回临时变量在符号表中的位置
int fill_Temp(char *name,int level,int type,char flag,int offset) {
    strcpy(symbolTable.symbols[symbolTable.index].name,"");
    strcpy(symbolTable.symbols[symbolTable.index].alias,name);
    symbolTable.symbols[symbolTable.index].level=level;
    symbolTable.symbols[symbolTable.index].type=type;
    symbolTable.symbols[symbolTable.index].flag=flag;
    symbolTable.symbols[symbolTable.index].offset=offset;
    return symbolTable.index++; //返回的是临时变量在符号表中的位置序号
}


int array_access(struct ASTNode* root)//返回数组类型
{
    struct ASTNode *T1 = root;
    //prnIR(T1->code);
    int this_dim = 0;
    int tmp;
    int this_each_dim[10];//记录此次访问每个维数的大小
    struct ASTNode* Node_Array[MAXLENGTH];
    while(root->kind==ARRAY)
    {
        Exp(root->ptr[1]);
        //prnIR(root->ptr[1]->code);
        //if(!root->ptr[1]) printf("!!!!!!!!!!!!");
        if(root->ptr[1]->type!=INT) {   semantic_error(root->pos,"","数组大小必须为int"); return -1;}
        this_each_dim[this_dim] = root->ptr[1]->type_int;
        Node_Array[this_dim] = root->ptr[1];
        this_dim++;
        root = root->ptr[0];
    }
    int value = searchSymbolTable(root->type_id);
    if(value == -1)
    {
        semantic_error(root->pos,"","未找到相关定义"); 
        return -1;
    }
    else if(symbolTable.symbols[value].flag!='A')
    {
        semantic_error(root->pos,"","未找到相关数组"); 
        return -1;
    }
    struct array_info* p = symbolTable.symbols[value].info;
    if(p->dim != this_dim)
    {
        // printf("!!!!%d\n",this_dim);
        // printf("!!!!!%d\n",p->dim);
        semantic_error(root->pos,"","数组维数错误"); 
        return -1;
    }
    //p->array_num = this_each_dim[0];
    //printf("!!!!!%d\n",array_num);
    //int sum = p->diminfo[0];//末尾 如果是[10][20]则表示20
    //printf("!!!!%d\n",Node_Array[this_dim - 1]->kind);
    Exp(Node_Array[this_dim - 1]);
    //printf("!!!!!!!!%d",this_dim);
    //prnIR(Node_Array[this_dim - 1]->code);
    //printf("!!!!!!!!");
    T1->code = merge(2,T1->code,Node_Array[this_dim - 1]->code);
    struct opn opn1,opn2,result;
    //printf("!!!%d\n",p->dim);
    for(int i = this_dim - 2;i >= 0;i--)//循环dim-1次
    {
        //先计算乘法,乘上前一位的元素上限
        root->place = fill_Temp(newTemp(), LEV, INT, 'T', Node_Array[0]->offset);
        strcpy(result.id, symbolTable.symbols[root->place].alias);
        result.kind = ID;

        opn2.kind = INT;
        opn2.const_int = symbolTable.symbols[value].info->diminfo[i];

        opn1.kind = ID;
        strcpy(opn1.id, symbolTable.symbols[Node_Array[i + 1]->place].alias);

        //if(T1->code==NULL) printf("~!!!!!!!!!!!!!!!!!\n");
        T1->code = merge(2, T1->code, genIR(STAR, opn1, opn2, result));
        //prnIR(T1->code);
        //sum *= this_each_dim[this_dim-i];
        //生成右半部分临时变量
        Exp(Node_Array[i]);
        
        tmp = fill_Temp(newTemp(),LEV,INT,'T',0);

        strcpy(result.id,symbolTable.symbols[tmp].alias);
        result.kind = ID;

        opn2.kind = ID;
        strcpy(opn2.id,symbolTable.symbols[Node_Array[i]->place].alias);

        opn1.kind = ID;
        strcpy(opn1.id,symbolTable.symbols[root->place].alias);

        T1->code = merge(3,T1->code,Node_Array[i]->code,genIR(PLUS,opn1,opn2,result));
        //printf("!!!!!!!!!!!!!!!!!\n");
    }
        //计算总的偏移数，以字节为基本单位

    int tmp1 = fill_Temp(newTemp(), LEV, INT, 'T', 0);

    strcpy(result.id, symbolTable.symbols[tmp1].alias);
    result.kind = ID;

    opn2.kind = INT;
    opn2.const_int = (symbolTable.symbols[value].info->type == CHAR ? 1 : 4);

    opn1.kind = ID;
    if(this_dim==1)
        strcpy(opn1.id, symbolTable.symbols[Node_Array[this_dim - 1]->place].alias);
    else
        strcpy(opn1.id, symbolTable.symbols[tmp].alias);

    T1->code = merge(2, T1->code, genIR(STAR, opn1, opn2, result));

    //prnIR(T1->code);
    //symbolTable.symbols[T1->place].info->target = tmp1;
    T1->place = searchSymbolTable(root->type_id);
    symbolTable.symbols[T1->place].info->target = tmp1;
    //printf("!!!!!%d\n",p->array_num);
    return value;
} 




//输出中间代码
void prnIR(struct codenode *head){
    char opnstr1[32],opnstr2[32],resultstr[32];
    struct codenode *h=head;
    do {
        if (h->opn1.kind==INT)
             sprintf(opnstr1,"#%d",h->opn1.const_int);
        if (h->opn1.kind==FLOAT)
             sprintf(opnstr1,"#%f",h->opn1.const_float);
        if (h->opn1.kind==CHAR)
             sprintf(opnstr1,"#%c",h->opn1.const_char);
        // if (h->opn1.kind==ARRAY)
        //      sprintf(opnstr1,"#error");
        if (h->opn1.kind==ID)
             sprintf(opnstr1,"%s",h->opn1.id);
        if (h->opn2.kind==INT)
             sprintf(opnstr2,"#%d",h->opn2.const_int);
        if (h->opn2.kind==FLOAT)
             sprintf(opnstr2,"#%f",h->opn2.const_float);
        if (h->opn2.kind==CHAR)
             sprintf(opnstr2,"#%c",h->opn2.const_char);
        if (h->opn2.kind==ID)
             sprintf(opnstr2,"%s",h->opn2.id);
        if(h->result.kind==ARRAY)
            sprintf(resultstr,"%s[%s]",h->result.id,h->opn2.id);
        else
            sprintf(resultstr,"%s",h->result.id);
        switch (h->op) {
            case ASSIGNOP:  printf("  %s := %s\n",resultstr,opnstr1);
                            break;
            case PLUS:
            case MINUS:
            case STAR:
            case DIV: printf("  %s := %s %c %s\n",resultstr,opnstr1, \
                      h->op==PLUS?'+':h->op==MINUS?'-':h->op==STAR?'*':'\\',opnstr2);
                      break;
            case SELFADD:
            case SELFDEC:
                           if(!strcpy(h->opn2.id,"LEFTADD")||!strcpy(h->opn2.id,"LEFTDEC"))
                           {
                               printf("  %s := %s\n",resultstr,opnstr1);
                               printf("  %s := %s %c 1\n",opnstr1,resultstr,h->op == SELFADD ? '+' : '-');
                               printf("  %s := %s\n",resultstr,opnstr1);
                           }
                           else{
                               printf("  %s := %s\n",resultstr,opnstr1);
                               printf("  %s := %s %c 1\n",opnstr1,resultstr,h->op == SELFADD ? '+' : '-');
                           }
                           break;
            case UMINUS:   printf("  %s := -%s\n",resultstr,opnstr1);
                           break;
            case FUNCTION: printf("\nFUNCTION %s :\n",h->result.id);
                           break;
            case PARAM:    printf("  PARAM %s\n",h->result.id);
                           break;
            case LABEL:    printf("LABEL %s :\n",h->result.id);
                           break;
            case GOTO:     printf("  GOTO %s\n",h->result.id);
                           break;
            case JLE:      printf("  IF %s <= %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case JLT:      printf("  IF %s < %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case JGE:      printf("  IF %s >= %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case JGT:      printf("  IF %s > %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case EQ:       printf("  IF %s == %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case NEQ:      printf("  IF %s != %s GOTO %s\n",opnstr1,opnstr2,resultstr);
                           break;
            case ARG:      printf("  ARG %s\n",h->result.id);
                           break;
            case CALL:     if (!strcmp(opnstr1,"write"))
                                printf("  CALL  %s\n", opnstr1);
                            else
                                printf("  %s := CALL %s\n",resultstr, opnstr1);
                           break;
            case RETURN:   if (h->result.kind)
                                printf("  RETURN %s\n",resultstr);
                           else
                                printf("  RETURN\n");
                           break;

        }
    h=h->next;
    } while (h!=head);
}
void semantic_error(int line,char *msg1,char *msg2){
    //这里可以只收集错误信息，最后一次显示
    printf("在%d行,%s %s\n",line,msg1,msg2);
}
void prn_symbol(){ //显示符号表
    int i=0;
    printf("%6s %6s %6s  %6s %4s %6s\n","变量名","别 名","层 号","类  型","标记","偏移量");
    for(i=0;i<symbolTable.index;i++)
        printf("%6s %6s %6d  %6s %4c %6d\n",symbolTable.symbols[i].name,\
                symbolTable.symbols[i].alias,symbolTable.symbols[i].level,\
                symbolTable.symbols[i].type==INT?"int":symbolTable.symbols[i].type==FLOAT?"float":"char",\
                symbolTable.symbols[i].flag,symbolTable.symbols[i].offset);
}

int searchSymbolTable(char *name) {
    int i,flag=0;
    for(i=symbolTable.index-1;i>=0;i--){
        if (symbolTable.symbols[i].level==0)
            flag=1;
        if (flag && symbolTable.symbols[i].level==1)
            continue;   //跳过前面函数的形式参数表项
        if (!strcmp(symbolTable.symbols[i].name, name))  return i;
    }
    return -1;
}
int fillSymbolTable(char *name,char *alias,int level,int type,char flag,int offset) {
    //首先根据name查符号表，不能重复定义 重复定义返回-1
    int i;
    /*符号查重，考虑外部变量声明前有函数定义，
    其形参名还在符号表中，这时的外部变量与前函数的形参重名是允许的*/
    for(i=symbolTable.index-1; i>=0 && (symbolTable.symbols[i].level==level||level==0); i--) {
        if (level==0 && symbolTable.symbols[i].level==1) continue;  //外部变量和形参不必比较重名
        if (!strcmp(symbolTable.symbols[i].name, name))  return -1;
        }
    //填写符号表内容
    strcpy(symbolTable.symbols[symbolTable.index].name,name);
    strcpy(symbolTable.symbols[symbolTable.index].alias,alias);
    symbolTable.symbols[symbolTable.index].level=level;
    symbolTable.symbols[symbolTable.index].type=type;
    symbolTable.symbols[symbolTable.index].flag=flag;
    symbolTable.symbols[symbolTable.index].offset=offset;
    return symbolTable.index++; //返回的是符号在符号表中的位置序号，中间代码生成时可用序号取到符号别名
}


void ext_var_list(struct ASTNode *T){  //处理变量列表
    int rtn,num=1;
    switch (T->kind){
        case EXT_DEC_LIST:
                T->ptr[0]->type=T->type;              //将类型属性向下传递变量结点
                T->ptr[0]->offset=T->offset;          //外部变量的偏移量向下传递
                T->ptr[1]->type=T->type;              //将类型属性向下传递变量结点
                T->ptr[1]->offset=T->offset+T->width; //外部变量的偏移量向下传递
                T->ptr[1]->width=T->width;
                ext_var_list(T->ptr[0]);
                ext_var_list(T->ptr[1]);
                T->num=T->ptr[1]->num+1;
                break;

        case ID:
            rtn=fillSymbolTable(T->type_id,newAlias(),LEV,T->type,'V',T->offset);  //最后一个变量名
            if (rtn==-1)
                semantic_error(T->pos,T->type_id, "变量重复定义");
            else T->place=rtn;
            T->num=1;
            break;

        case ARRAY:
            rtn = fillSymbolTable(T->ptr[0]->type_id, newAlias(), LEV, T->type, 'A', T->offset); //最后一个变量名
            if (rtn == -1)
                semantic_error(T->pos, T->ptr[0]->type_id, "变量名重复定义");
            else if(T->ptr[1]->type!=INT)
                semantic_error(T->pos,"","数组大小不能为float或char");
            else if (T->ptr[1]->type_int <= 0||!strcmp(T->ptr[1]->type_id,"UMINUS"))
            {
                semantic_error(T->pos, T->type_id, "数组大小不能为负值或0");
            }
            else
            {
                //printf("111111\n");
                T->place = rtn;
                T->num = 1;
            }
            break;
        }
    }

int  match_param(int i,struct ASTNode *T){
    int j,num=symbolTable.symbols[i].paramnum;
    int type1,type2,pos=T->pos;
    T=T->ptr[0];
    if (num==0 && T==NULL) return 1;
    for (j=1;j<=num;j++) {
        if (!T){
            semantic_error(pos,"", "函数调用参数太少!");
            return 0;
            }
        type1=symbolTable.symbols[i+j].type;  //形参类型
        type2=T->ptr[0]->type;
        if (type1!=type2){
            semantic_error(pos,"", "参数类型不匹配");
            return 0;
        }
        T=T->ptr[1];
    }
    if (T){ //num个参数已经匹配完，还有实参表达式
        semantic_error(pos,"", "函数调用参数太多!");
        return 0;
        }
    return 1;
    }

void boolExp(struct ASTNode *T){  //布尔表达式，参考文献[2]p84的思想
  struct opn opn1,opn2,result;
  int op;
  int rtn;
  if (T)
	{
	switch (T->kind) {
        case INT:  
        case FLOAT: 
        case ID:    Exp(T); 
                    //T->code=merge(2,T->code,genGoto(T->Efalse));
                    break;
        case RELOP: //处理关系运算表达式,2个操作数都按基本表达式处理
                    T->ptr[0]->offset=T->ptr[1]->offset=T->offset;
                    Exp(T->ptr[0]);
                    T->width=T->ptr[0]->width;
                    Exp(T->ptr[1]);
                    if (T->width<T->ptr[1]->width) T->width=T->ptr[1]->width;
                    opn1.kind=ID; strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
                    opn1.offset=symbolTable.symbols[T->ptr[0]->place].offset;
                    opn2.kind=ID; strcpy(opn2.id,symbolTable.symbols[T->ptr[1]->place].alias);
                    opn2.offset=symbolTable.symbols[T->ptr[1]->place].offset;
                    result.kind=ID; strcpy(result.id,T->Etrue);
                    if (strcmp(T->type_id,"<")==0)
                            op=JLT;
                    else if (strcmp(T->type_id,"<=")==0)
                            op=JLE;
                    else if (strcmp(T->type_id,">")==0)
                            op=JGT;
                    else if (strcmp(T->type_id,">=")==0)
                            op=JGE;
                    else if (strcmp(T->type_id,"==")==0)
                            op=EQ;
                    else if (strcmp(T->type_id,"!=")==0)
                            op=NEQ;
                    T->code=genIR(op,opn1,opn2,result);
                    T->code=merge(4,T->ptr[0]->code,T->ptr[1]->code,T->code,genGoto(T->Efalse));
                    break;
        case AND: 
        case OR:
                    if (T->kind==AND) {
                        strcpy(T->ptr[0]->Etrue,newLabel());
                        strcpy(T->ptr[0]->Efalse,T->Efalse);
                        }
                    else {
                        strcpy(T->ptr[0]->Etrue,T->Etrue);
                        strcpy(T->ptr[0]->Efalse,newLabel());
                        }
                    strcpy(T->ptr[1]->Etrue,T->Etrue);
                    strcpy(T->ptr[1]->Efalse,T->Efalse);
                    T->ptr[0]->offset=T->ptr[1]->offset=T->offset;
                    boolExp(T->ptr[0]);
                    T->width=T->ptr[0]->width;
                    boolExp(T->ptr[1]);
                    if (T->width<T->ptr[1]->width) T->width=T->ptr[1]->width;
                    if (T->kind==AND)
                        T->code=merge(3,T->ptr[0]->code,genLabel(T->ptr[0]->Etrue),T->ptr[1]->code);
                    else
                        T->code=merge(3,T->ptr[0]->code,genLabel(T->ptr[0]->Efalse),T->ptr[1]->code);
                    break;
        case NOT:   
                    strcpy(T->ptr[0]->Etrue,T->Efalse);
                    strcpy(T->ptr[0]->Efalse,T->Etrue);
                    boolExp(T->ptr[0]);
                    T->code=T->ptr[0]->code;
                    break;
        }
	}
}


void Exp(struct ASTNode *T)
{//处理基本表达式，参考文献[2]p82的思想
  int rtn,num,width;
  int op;
  struct ASTNode *T0;
  struct opn opn1,opn2,result,result2;
  char Label1[15],Label2[15];
  if (T)
	{
	switch (T->kind) {
	case ID:    //查符号表，获得符号表中的位置，类型送type
                rtn=searchSymbolTable(T->type_id);
                if (rtn==-1)
                    semantic_error(T->pos,T->type_id, "变量未定义");
                if (symbolTable.symbols[rtn].flag=='F')
                    semantic_error(T->pos,T->type_id, "是函数名，类型不匹配");
                else {
                    T->place=rtn;       //结点保存变量在符号表中的位置
                    T->code=NULL;       //标识符不需要生成TAC
                    T->type=symbolTable.symbols[rtn].type;
                    T->offset=symbolTable.symbols[rtn].offset;
                    T->width=0;   //未再使用新单元
                    }
                break;
    case INT:   T->place=fill_Temp(newTemp(),LEV,INT,'T',T->offset); //为整常量生成一个临时变量
                T->type=INT;
                opn1.kind=INT;opn1.const_int=T->type_int;
                result.kind=ID; strcpy(result.id,symbolTable.symbols[T->place].alias);
                result.offset=symbolTable.symbols[T->place].offset;
                T->code=genIR(ASSIGNOP,opn1,opn2,result);
                T->width=4;
                break;
    case FLOAT: T->place=fill_Temp(newTemp(),LEV,FLOAT,'T',T->offset);   //为浮点常量生成一个临时变量
                T->type=FLOAT;
                opn1.kind=FLOAT; opn1.const_float=T->type_float;
                result.kind=ID; strcpy(result.id,symbolTable.symbols[T->place].alias);
                result.offset=symbolTable.symbols[T->place].offset;
                T->code=genIR(ASSIGNOP,opn1,opn2,result);
                T->width=4;
                break;
    case CHAR:  T->place=fill_Temp(newTemp(),LEV,CHAR,'T',T->offset);   //为浮点常量生成一个临时变量
                T->type=CHAR;
                opn1.kind=CHAR; opn1.const_char=T->type_char;
                result.kind=ID; strcpy(result.id,symbolTable.symbols[T->place].alias);
                result.offset=symbolTable.symbols[T->place].offset;
                T->code=genIR(ASSIGNOP,opn1,opn2,result);
                T->width=4;
                break;
    case ARRAY:
                rtn = array_access(T);
                //T->place = fill_Temp(newTemp(),LEV,INT,)
                //opn1.kind = INT;
                //opn1.type_int = array_num;
                //T->code=genIR(ARRAY,op1,op2,result);
                T->place=rtn;       //结点保存变量在符号表中的位置
                //T->code=NULL;       //标识符不需要生成TAC
                //T->type=ARR;
                //T->offset=symbolTable.symbols[rtn].offset;
                if(rtn!=-1) T->type = symbolTable.symbols[rtn].type;
                break;
    case BREAK:
                if(!is_loop)
                    semantic_error(T->pos, "", "不在循环体内部，不能break");
                else
                {
                    T->code = genGoto(break_label);
                }
                break;
    case CONTINUE:
                if(!is_loop)
                    semantic_error(T->pos, "", "不在循环体内部，不能continue");
                else
                {
                    T->code = genGoto(loop_label);
                }
                break;
    case AND:   //按算术表达式方式计算布尔值，未写完
            T->type=INT;
            T->ptr[0]->offset=T->offset;
            Exp(T->ptr[0]);
            T->place=fill_Temp(newTemp(),LEV,T->type,'T',T->offset);
            T->code=T->ptr[0]->code;
            strcpy(Label1,newLabel());
            strcpy(Label2,newLabel());
            opn1.kind=ID;
            strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
            opn1.offset=symbolTable.symbols[T->ptr[0]->place].offset;
            opn1.type=T->ptr[0]->type;

            opn2.kind=INT;
            opn2.const_int=0;
            result.kind=ID;
            strcpy(result.id,Label1);
            op=EQ;
            T->code=merge(2,T->code,genIR(op,opn1,opn2,result));

            T->ptr[1]->offset=T->offset+T->ptr[0]->width;
            Exp(T->ptr[1]);
            T->code=merge(2,T->code,T->ptr[1]->code);
            opn1.kind=ID;
            strcpy(opn1.id,symbolTable.symbols[T->ptr[1]->place].alias);
            opn1.offset=symbolTable.symbols[T->ptr[1]->place].offset;
            opn1.type=T->ptr[1]->type;
            
            T->code=merge(2,T->code,genIR(op,opn1,opn2,result));
            opn1.kind=INT;
            opn1.const_int=1;
            result.kind=ID;
            strcpy(result.id,symbolTable.symbols[T->place].alias);
            result.type=T->type;
            result.offset=symbolTable.symbols[T->place].offset;
            T->code=merge(4,T->code,genIR(ASSIGNOP,opn1,opn2,result),genGoto(Label2),genLabel(Label1));
            opn1.kind=INT;
            opn1.const_int=0;
            opn1.type=INT;
            
            T->code=merge(3,T->code,genIR(ASSIGNOP,opn1,opn2,result),genLabel(Label2));
            break;
    case OR:    //按算术表达式方式计算布尔值，未写完
           T->type=INT;
            T->ptr[0]->offset=T->offset;
            Exp(T->ptr[0]);
            T->place=fill_Temp(newTemp(),LEV,T->type,'T',T->offset);
            T->code=T->ptr[0]->code;
            strcpy(Label1,newLabel());
            strcpy(Label2,newLabel());
            opn1.kind=ID;
            strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
            opn1.offset=symbolTable.symbols[T->ptr[0]->place].offset;
            opn1.type=T->ptr[0]->type;

            opn2.kind=INT;
            opn2.const_int=0;
            result.kind=ID;
            strcpy(result.id,Label1);
            op=NEQ;
            T->code=merge(2,T->code,genIR(op,opn1,opn2,result));

            T->ptr[1]->offset=T->offset+T->ptr[0]->width;
            Exp(T->ptr[1]);
            T->code=merge(2,T->code,T->ptr[1]->code);
            opn1.kind=ID;
            strcpy(opn1.id,symbolTable.symbols[T->ptr[1]->place].alias);
            opn1.offset=symbolTable.symbols[T->ptr[1]->place].offset;
            opn1.type=T->ptr[1]->type;
            
            T->code=merge(2,T->code,genIR(op,opn1,opn2,result));
            opn1.kind=INT;
            opn1.const_int=0;
            result.kind=ID;
            strcpy(result.id,symbolTable.symbols[T->place].alias);
            result.type=T->type;
            result.offset=symbolTable.symbols[T->place].offset;
            T->code=merge(4,T->code,genIR(ASSIGNOP,opn1,opn2,result),genGoto(Label2),genLabel(Label1));
            opn1.kind=INT;
            opn1.const_int=1;
            opn1.type=INT;
            
            T->code=merge(3,T->code,genIR(ASSIGNOP,opn1,opn2,result),genLabel(Label2));
            break;

    case ADD_ASSIGNOP:
    case MINUS_ASSIGNOP: 
    case STAR_ASSIGNOP: 
    case DIV_ASSIGNOP:  
	case ASSIGNOP:
                if (T->ptr[0]->kind!=ID&&T->ptr[0]->kind!=ARRAY){
                    semantic_error(T->pos,"", "赋值语句需要左值");
                    }
                else {
                    Exp(T->ptr[0]);   //处理左值，例中仅为变量
                    T->ptr[1]->offset=T->offset;
                    Exp(T->ptr[1]);
                    T->type=T->ptr[0]->type;
                    if(T->ptr[0]->type!=T->ptr[1]->type)
                    {
                        semantic_error(T->pos,"", "赋值语句类型不匹配");
                    }
                    //printf("!!!%d\n",T->ptr[0]->kind);
                    T->width=T->ptr[1]->width;
                    T->code=merge(2,T->ptr[0]->code,T->ptr[1]->code);
                    opn1.kind=ID;   strcpy(opn1.id,symbolTable.symbols[T->ptr[1]->place].alias);//右值一定是个变量或临时变量
                    opn1.offset=symbolTable.symbols[T->ptr[1]->place].offset;
                    if(T->ptr[0]->kind==ID)
                    {    
                        result.kind=ID; strcpy(result.id,symbolTable.symbols[T->ptr[0]->place].alias);
                    }
                    else
                    {
                        result.kind=ARRAY; strcpy(result.id,symbolTable.symbols[T->ptr[0]->place].alias);
                        int foo = symbolTable.symbols[T->ptr[0]->place].info->target;
                        //printf("!!!!!!!%d",foo);
                        opn2.kind = ID;strcpy(opn2.id,symbolTable.symbols[foo].alias);
                    }
                    result.offset=symbolTable.symbols[T->ptr[0]->place].offset;
                    T->code=merge(2,T->code,genIR(T->kind,opn1,opn2,result));
                    }
                break;
    case SELFDEC:
    case SELFADD:
                if((!strcmp("RIGHTADD",T->ptr[0]->type_id)||!strcmp("RIGHTDEC",T->ptr[0]->type_id)&&!strcmp("LEFTADD",T->type_id))||(!strcmp("LEFTADD",T->ptr[0]->type_id)||!strcmp("LEFTDEC",T->ptr[0]->type_id)&&!strcmp("RIGHTADD",T->type_id)))
                {
                    semantic_error(T->pos,"", "操作数左右值不匹配");
                }
                else if(!strcmp(T->ptr[0]->type_id,"PLUS")||!strcmp(T->ptr[0]->type_id,"MINUS")||!strcmp(T->ptr[0]->type_id,"STAR")||!strcmp(T->ptr[0]->type_id,"DIV"))
                {
                    semantic_error(T->pos,"", "操作数左右值不匹配");
                }
                else
                {
                    Exp(T->ptr[0]);
                    T->type = T->ptr[0]->type;
                    T->place = fill_Temp(newTemp(),LEV,T->type,'T',T->offset+T->ptr[0]->width);
                    opn1.kind=ID;
                    strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
                    opn1.type = T->ptr[0]->type;
                    opn1.offset = symbolTable.symbols[T->ptr[0]->place].offset;

                    result.kind = ID;
                    strcpy(result.id,symbolTable.symbols[T->place].alias);
                    result.type = T->type;
                    result.offset = symbolTable.symbols[T->place].offset;

                    strcpy(opn2.id,T->ptr[0]->type_id);
                    T->code = merge(2,T->ptr[0]->code,genIR(T->kind,opn1,opn2,result));
                }
                break;
    // case SELFDEC:
    //             if((!strcmp("RIGHTADD",T->ptr[0]->type_id)||!strcmp("RIGHTDEC",T->ptr[0]->type_id)&&!strcmp("LEFTDEC",T->type_id))||(!strcmp("LEFTADD",T->ptr[0]->type_id)||!strcmp("LEFTDEC",T->ptr[0]->type_id)&&!strcmp("RIGHTDEC",T->type_id)))
    //             {
    //                 semantic_error(T->pos,"", "操作数左右值不匹配");
    //             }
    //             else if(!strcmp(T->ptr[0]->type_id,"PLUS")||!strcmp(T->ptr[0]->type_id,"MINUS")||!strcmp(T->ptr[0]->type_id,"STAR")||!strcmp(T->ptr[0]->type_id,"DIV"))
    //             {
    //                 semantic_error(T->pos,"", "操作数左右值不匹配");
    //             }
    //             else
    //             {
    //                 Exp(T->ptr[0]);
    //                 T->type = T->ptr[0]->type;
    //             }
    //             break;
                
	case RELOP: //按算术表达式方式计算布尔值，未写完
            T->type=INT;
            T->ptr[0]->offset=T->offset;
            Exp(T->ptr[0]);
            T->ptr[1]->offset=T->offset+T->ptr[0]->width;
            Exp(T->ptr[1]);

            T->place=fill_Temp(newTemp(),LEV,T->type,'T',T->offset);
            T->code=merge(2,T->ptr[0]->code,T->ptr[1]->code);
            opn1.kind=ID;
            strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
            opn1.offset=symbolTable.symbols[T->ptr[0]->place].offset;
            opn2.kind =ID;
            strcpy(opn2.id,symbolTable.symbols[T->ptr[1]->place].alias);
            opn2.offset=symbolTable.symbols[T->ptr[1]->place].offset;
            result.kind=ID;
            strcpy(result.id,newLabel());
             if (strcmp(T->type_id, "<") == 0)
                op = JLT;
            else if (strcmp(T->type_id, "<=") == 0)
                op = JLE;
            else if (strcmp(T->type_id, ">") == 0)
                op = JGT;
            else if (strcmp(T->type_id, ">=") == 0)
                op = JGE;
            else if (strcmp(T->type_id, "==") == 0)
                op = EQ;
            else if (strcmp(T->type_id, "!=") == 0)
                op = NEQ;
            T->code=merge(2,T->code,genIR(op,opn1,opn2,result));
            opn1.kind=INT;
            opn1.const_int=0;
            result2.kind=ID;
            strcpy(result2.id,symbolTable.symbols[T->place].alias);
            result2.offset=symbolTable.symbols[T->place].offset;
            strcpy(Label2,newLabel());
            T->code=merge(4,T->code,genIR(ASSIGNOP,opn1,opn2,result2),genGoto(Label2),genLabel(result.id));
            opn1.const_int=1;
            T->code=merge(3,T->code,genIR(ASSIGNOP,opn1,opn2,result2),genLabel(Label2));
            break;

	case PLUS:
	case MINUS:
	case STAR:
	case DIV:   T->ptr[0]->offset=T->offset;
                Exp(T->ptr[0]);
                T->ptr[1]->offset=T->offset+T->ptr[0]->width;
                Exp(T->ptr[1]);
                //判断T->ptr[0]，T->ptr[1]类型是否正确，可能根据运算符生成不同形式的代码，给T的type赋值
                //下面的类型属性计算，没有考虑错误处理情况
                if (T->ptr[0]->type==FLOAT || T->ptr[1]->type==FLOAT)
                     T->type=FLOAT,T->width=T->ptr[0]->width+T->ptr[1]->width+4;
                else T->type=INT,T->width=T->ptr[0]->width+T->ptr[1]->width+2;
                T->place=fill_Temp(newTemp(),LEV,T->type,'T',T->offset+T->ptr[0]->width+T->ptr[1]->width);
                opn1.kind=ID; strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
                opn1.type=T->ptr[0]->type;opn1.offset=symbolTable.symbols[T->ptr[0]->place].offset;
                opn2.kind=ID; strcpy(opn2.id,symbolTable.symbols[T->ptr[1]->place].alias);
                opn2.type=T->ptr[1]->type;opn2.offset=symbolTable.symbols[T->ptr[1]->place].offset;
                result.kind=ID; strcpy(result.id,symbolTable.symbols[T->place].alias);
                result.type=T->type;result.offset=symbolTable.symbols[T->place].offset;
                T->code=merge(3,T->ptr[0]->code,T->ptr[1]->code,genIR(T->kind,opn1,opn2,result));
                T->width=T->ptr[0]->width+T->ptr[1]->width+(T->type==CHAR?1:4);
                break;
	case NOT:   Exp(T->ptr[0]);
                T->type = INT;
                break;
	case UMINUS://未写完整
                T->place = fill_Temp(newTemp(),LEV,T->ptr[0]->type,'T',T->offset);
                T->ptr[0]->offset = T->offset;
                Exp(T->ptr[0]);
                opn1.kind = ID;
                opn1.offset = symbolTable.symbols[T->ptr[0]->place].offset;
                strcpy(opn1.id,symbolTable.symbols[T->ptr[0]->place].alias);
                //printf("!!!%s",opn1.id);

                result.kind = ID;
                result.offset = symbolTable.symbols[T->place].offset;
                strcpy(result.id,symbolTable.symbols[T->place].alias);

                T->type = T->ptr[0]->type;
                T->code = merge(2,T->ptr[0]->code,genIR(UMINUS,opn1,opn2,result));
                break;
    case FUNC_CALL: //根据T->type_id查出函数的定义，如果语言中增加了实验教材的read，write需要单独处理一下
                rtn=searchSymbolTable(T->type_id);
                if (rtn==-1){
                    semantic_error(T->pos,T->type_id, "函数未定义");
                    break;
                    }
                if (symbolTable.symbols[rtn].flag!='F'){
                    semantic_error(T->pos,T->type_id, "不是一个函数");
                     break;
                    }
                T->type=symbolTable.symbols[rtn].type;
                width=T->type==CHAR?1:4;   //存放函数返回值的单数字节数
                if (T->ptr[0]){
                    T->ptr[0]->offset=T->offset;
                    Exp(T->ptr[0]);       //处理所有实参表达式求值，及类型
                    T->width=T->ptr[0]->width+width; //累加上计算实参使用临时变量的单元数
                    T->code=T->ptr[0]->code;
                    }
                else {T->width=width; T->code=NULL;}
                match_param(rtn,T);   //处理所有参数的匹配
                    //处理参数列表的中间代码
                T0=T->ptr[0];
                while (T0) {
                    result.kind=ID;  strcpy(result.id,symbolTable.symbols[T0->ptr[0]->place].alias);
                    result.offset=symbolTable.symbols[T0->ptr[0]->place].offset;
                    T->code=merge(2,T->code,genIR(ARG,opn1,opn2,result));
                    T0=T0->ptr[1];
                    }
                T->place=fill_Temp(newTemp(),LEV,T->type,'T',T->offset+T->width-width);
                opn1.kind=ID;     strcpy(opn1.id,T->type_id);  //保存函数名
                opn1.offset=rtn;  //这里offset用以保存函数定义入口,在目标代码生成时，能获取相应信息
                result.kind=ID;   strcpy(result.id,symbolTable.symbols[T->place].alias);
                result.offset=symbolTable.symbols[T->place].offset;
                T->code=merge(2,T->code,genIR(CALL,opn1,opn2,result)); //生成函数调用中间代码
                break;
    case ARGS:      //此处仅处理各实参表达式的求值的代码序列，不生成ARG的实参系列
                T->ptr[0]->offset=T->offset;
                Exp(T->ptr[0]);
                T->width=T->ptr[0]->width;
                T->code=T->ptr[0]->code;
                if (T->ptr[1]) {
                    T->ptr[1]->offset=T->offset+T->ptr[0]->width;
                    Exp(T->ptr[1]);
                    T->width+=T->ptr[1]->width;
                    T->code=merge(2,T->code,T->ptr[1]->code);
                    }
                break;
         }
      }
}

void semantic_Analysis(struct ASTNode *T)
{//对抽象语法树的先根遍历,按display的控制结构修改完成符号表管理和语义检查和TAC生成（语句部分）
  int rtn,num,width;
  struct ASTNode *T0;
  struct opn opn1,opn2,result;
  if (T)
	{
	switch (T->kind) {
	case EXT_DEF_LIST:
            if (!T->ptr[0]) break;
            T->ptr[0]->offset=T->offset;
            semantic_Analysis(T->ptr[0]);    //访问外部定义列表中的第一个
            T->code=T->ptr[0]->code;
            if (T->ptr[1]){
                T->ptr[1]->offset=T->ptr[0]->offset+T->ptr[0]->width;
                semantic_Analysis(T->ptr[1]); //访问该外部定义列表中的其它外部定义
                T->code=merge(2,T->code,T->ptr[1]->code);
                }
            break;
	case EXT_VAR_DEF:   //处理外部说明,将第一个孩子(TYPE结点)中的类型送到第二个孩子的类型域
            T->type=T->ptr[1]->type=!strcmp(T->ptr[0]->type_id,"int")?INT:FLOAT;
            T->ptr[1]->offset=T->offset;        //这个外部变量的偏移量向下传递
            T->ptr[1]->width=T->type==CHAR?1:4;  //将一个变量的宽度向下传递
            ext_var_list(T->ptr[1]);            //处理外部变量说明中的标识符序列
            T->width=(T->type==INT?4:8)* T->ptr[1]->num; //计算这个外部变量说明的宽度
            //T->code=merge(2,T->ptr[0]->code,T->ptr[1]->code);             //这里假定外部变量不支持初始化
            T->code=NULL;
            break;
	case FUNC_DEF:      //填写函数定义信息到符号表
            T->ptr[1]->type=!strcmp(T->ptr[0]->type_id,"int")?INT:FLOAT;//获取函数返回类型送到含函数名、参数的结点
            T->width=0;     //函数的宽度设置为0，不会对外部变量的地址分配产生影响
            T->offset=DX;   //设置局部变量在活动记录中的偏移量初值
            semantic_Analysis(T->ptr[1]); //处理函数名和参数结点部分，这里不考虑用寄存器传递参数
            T->offset+=T->ptr[1]->width;   //用形参单元宽度修改函数局部变量的起始偏移量
            T->ptr[2]->offset=T->offset;
            strcpy(T->ptr[2]->Snext,newLabel());  //函数体语句执行结束后的位置属性
            semantic_Analysis(T->ptr[2]);         //处理函数体结点
            //计算活动记录大小,这里offset属性存放的是活动记录大小，不是偏移
            symbolTable.symbols[T->ptr[1]->place].offset=T->offset+T->ptr[2]->width;
            T->code=merge(3,T->ptr[1]->code,T->ptr[2]->code,genLabel(T->ptr[2]->Snext));          //函数体的代码作为函数的代码
            break;
	case FUNC_DEC:      //根据返回类型，函数名填写符号表
            rtn=fillSymbolTable(T->type_id,newAlias(),LEV,T->type,'F',0);//函数不在数据区中分配单元，偏移量为0
            if (rtn==-1){
                semantic_error(T->pos,T->type_id, "函数重复定义");
                break;
                }
            else T->place=rtn;
            result.kind=ID;   strcpy(result.id,T->type_id);
            result.offset=rtn;
            T->code=genIR(FUNCTION,opn1,opn2,result);     //生成中间代码：FUNCTION 函数名
            T->offset=DX;   //设置形式参数在活动记录中的偏移量初值
            if (T->ptr[0]) { //判断是否有参数
                T->ptr[0]->offset=T->offset;
                semantic_Analysis(T->ptr[0]);  //处理函数参数列表
                T->width=T->ptr[0]->width;
                symbolTable.symbols[rtn].paramnum=T->ptr[0]->num;
                T->code=merge(2,T->code,T->ptr[0]->code);  //连接函数名和参数代码序列
                }
            else symbolTable.symbols[rtn].paramnum=0,T->width=0;
            break;
	case PARAM_LIST:    //处理函数形式参数列表
            T->ptr[0]->offset=T->offset;
            semantic_Analysis(T->ptr[0]);
            if (T->ptr[1]){
                T->ptr[1]->offset=T->offset+T->ptr[0]->width;
                semantic_Analysis(T->ptr[1]);
                T->num=T->ptr[0]->num+T->ptr[1]->num;        //统计参数个数
                T->width=T->ptr[0]->width+T->ptr[1]->width;  //累加参数单元宽度
                T->code=merge(2,T->ptr[0]->code,T->ptr[1]->code);  //连接参数代码
                }
            else {
                T->num=T->ptr[0]->num;
                T->width=T->ptr[0]->width;
                T->code=T->ptr[0]->code;
                }
            break;
	case  PARAM_DEC:
            rtn=fillSymbolTable(T->ptr[1]->type_id,newAlias(),1,T->ptr[0]->type,'P',T->offset);
            if (rtn==-1)
                semantic_error(T->ptr[1]->pos,T->ptr[1]->type_id, "参数名重复定义");
            else T->ptr[1]->place=rtn;
            T->num=1;       //参数个数计算的初始值
            T->width=T->ptr[0]->type==CHAR?1:4;  //参数宽度
            result.kind=ID;   strcpy(result.id, symbolTable.symbols[rtn].alias);
            result.offset=T->offset;
            T->code=genIR(PARAM,opn1,opn2,result);     //生成：FUNCTION 函数名
            break;
	case COMP_STM:
            LEV++;
            //设置层号加1，并且保存该层局部变量在符号表中的起始位置在symbol_scope_TX
            symbol_scope_TX.TX[symbol_scope_TX.top++]=symbolTable.index;
            T->width=0;
            T->code=NULL;
            if (T->ptr[0]) {
                T->ptr[0]->offset=T->offset;
                semantic_Analysis(T->ptr[0]);  //处理该层的局部变量DEF_LIST
                T->width+=T->ptr[0]->width;
                T->code=T->ptr[0]->code;
                }
            if (T->ptr[1]){
                T->ptr[1]->offset=T->offset+T->width;
                strcpy(T->ptr[1]->Snext,T->Snext);  //S.next属性向下传递
                semantic_Analysis(T->ptr[1]);       //处理复合语句的语句序列
                T->width+=T->ptr[1]->width;
                T->code=merge(2,T->code,T->ptr[1]->code);
                }
             #if (DEBUG)
                prn_symbol();       //c在退出一个符合语句前显示的符号表
			  getchar();
             #endif
             LEV--;    //出复合语句，层号减1
             symbolTable.index=symbol_scope_TX.TX[--symbol_scope_TX.top]; //删除该作用域中的符号
             break;
    case DEF_LIST:
            T->code=NULL;
            if (T->ptr[0]){
                T->ptr[0]->offset=T->offset;
                semantic_Analysis(T->ptr[0]);   //处理一个局部变量定义
                T->code=T->ptr[0]->code;
                T->width=T->ptr[0]->width;
                }
            if (T->ptr[1]) {
                T->ptr[1]->offset=T->offset+T->ptr[0]->width;
                semantic_Analysis(T->ptr[1]);   //处理剩下的局部变量定义
                T->code=merge(2,T->code,T->ptr[1]->code);
                T->width+=T->ptr[1]->width;
                }
                break;
    case VAR_DEF://处理一个局部变量定义,将第一个孩子(TYPE结点)中的类型送到第二个孩子的类型域
                 //类似于上面的外部变量EXT_VAR_DEF，换了一种处理方法
                T->code=NULL;
                T->ptr[1]->type=!strcmp(T->ptr[0]->type_id,"int")?INT:!strcmp(T->ptr[0]->type_id,"char")?CHAR:FLOAT;  //确定变量序列各变量类型
                T0=T->ptr[1]; //T0为变量名列表子树根指针，对ID、ASSIGNOP类结点在登记到符号表，作为局部变量
                num=0;
                T0->offset=T->offset;
                T->width=0;
                width=T->ptr[1]->type==CHAR?1:4;  //一个变量宽度
                while (T0) {  //处理所以DEC_LIST结点
                    num++;
                    T0->ptr[0]->type=T0->type;  //类型属性向下传递
                    if (T0->ptr[1]) T0->ptr[1]->type=T0->type;
                    T0->ptr[0]->offset=T0->offset;  //类型属性向下传递
                    if (T0->ptr[1]) T0->ptr[1]->offset=T0->offset+width;
                    if (T0->ptr[0]->kind==ID){
                        rtn=fillSymbolTable(T0->ptr[0]->type_id,newAlias(),LEV,T0->ptr[0]->type,'V',T->offset+T->width);//此处偏移量未计算，暂时为0
                        if (rtn==-1)
                            semantic_error(T0->ptr[0]->pos,T0->ptr[0]->type_id, "变量重复定义");
                        else T0->ptr[0]->place=rtn;
                        T->width+=width;
                        }
                    else if(T0->ptr[0]->kind==ARRAY){
                        // int each_dim[4];//记录每一层的数据是多少
                        // ASTNode *temp = T0->ptr[0];
                        // array_info a;
                        // a.dim = 0;
                        // a.type = T0->type;
                        // while(temp->kind==ARRAY)
                        // {
                        //     a.dim++;
                        //     if(temp->ptr[1]->kind!=INT)
                        //         semantic_error(temp->pos,"", "数组大小必须为int");
                        //     a.diminfo[a.dim-1]=temp->ptr[1]->type_int;
                        //     temp=temp->ptr[0];
                        // }
                        //printf("!!!!\n");
                        struct array_info* p = array_check(T0->ptr[0],T0->type);
                        //printf("!!!!dim:%d\n",p->dim);
                        //printf("!!!!!!\n");
                        if(p)
                        {
                            int value = fillSymbolTable(p->id,newAlias(),LEV,p->type,'A',T->offset+T->width);
                            if(value == -1)
                                semantic_error(T0->pos,p->id,"重复定义的数组变量");
                            else 
                            {
                                //printf("######%d\n",p->dim);
                                symbolTable.symbols[value].info = p;
                                T0->ptr[0]->place=rtn;
                            }
                            T->width+=width;
                        }

                    }
                    else if (T0->ptr[0]->kind==ASSIGNOP){
                            rtn=fillSymbolTable(T0->ptr[0]->ptr[0]->type_id,newAlias(),LEV,T0->ptr[0]->type,'V',T->offset+T->width);//此处偏移量未计算，暂时为0
                            if (rtn==-1)
                                semantic_error(T0->ptr[0]->ptr[0]->pos,T0->ptr[0]->ptr[0]->type_id, "变量重复定义");
                            else {
                                T0->ptr[0]->place=rtn;
                                T0->ptr[0]->ptr[1]->offset=T->offset+T->width+width;
                                Exp(T0->ptr[0]->ptr[1]);
                                opn1.kind=ID; strcpy(opn1.id,symbolTable.symbols[T0->ptr[0]->ptr[1]->place].alias);
                                result.kind=ID; strcpy(result.id,symbolTable.symbols[T0->ptr[0]->place].alias);
                                T->code=merge(3,T->code,T0->ptr[0]->ptr[1]->code,genIR(ASSIGNOP,opn1,opn2,result));
                                }
                            T->width+=width+T0->ptr[0]->ptr[1]->width;
                            }
                    T0=T0->ptr[1];
                    }
                break;
	case STM_LIST:
                if (!T->ptr[0]) { T->code=NULL; T->width=0; break;}   //空语句序列
                if (T->ptr[1]) //2条以上语句连接，生成新标号作为第一条语句结束后到达的位置
                    strcpy(T->ptr[0]->Snext,newLabel());
                else     //语句序列仅有一条语句，S.next属性向下传递
                    strcpy(T->ptr[0]->Snext,T->Snext);
                T->ptr[0]->offset=T->offset;
                semantic_Analysis(T->ptr[0]);
                T->code=T->ptr[0]->code;
                T->width=T->ptr[0]->width;
                if (T->ptr[1]){     //2条以上语句连接,S.next属性向下传递
                    strcpy(T->ptr[1]->Snext,T->Snext);
                    T->ptr[1]->offset=T->offset;  //顺序结构共享单元方式
//                  T->ptr[1]->offset=T->offset+T->ptr[0]->width; //顺序结构顺序分配单元方式
                    semantic_Analysis(T->ptr[1]);
                    //序列中第1条为表达式语句，返回语句，复合语句时，第2条前不需要标号
                     if (T->ptr[0]->kind==RETURN ||T->ptr[0]->kind==EXP_STMT ||T->ptr[0]->kind==COMP_STM)
                         T->code=merge(2,T->code,T->ptr[1]->code);
                     else
                         T->code=merge(3,T->code,genLabel(T->ptr[0]->Snext),T->ptr[1]->code);
                    if (T->ptr[1]->width>T->width) T->width=T->ptr[1]->width; //顺序结构共享单元方式
//                        T->width+=T->ptr[1]->width;//顺序结构顺序分配单元方式
                    }
                break;
	case IF_THEN:
                strcpy(T->ptr[0]->Etrue,newLabel());  //设置条件语句真假转移位置
                strcpy(T->ptr[0]->Efalse,T->Snext);
                T->ptr[0]->offset=T->ptr[1]->offset=T->offset;
                boolExp(T->ptr[0]);
                T->width=T->ptr[0]->width;
                strcpy(T->ptr[1]->Snext,T->Snext);
                semantic_Analysis(T->ptr[1]);      //if子句
                if (T->width<T->ptr[1]->width) T->width=T->ptr[1]->width;
                T->code=merge(3,T->ptr[0]->code, genLabel(T->ptr[0]->Etrue),T->ptr[1]->code);
                break;  //控制语句都还没有处理offset和width属性
	case IF_THEN_ELSE:
                strcpy(T->ptr[0]->Etrue,newLabel());  //设置条件语句真假转移位置
                strcpy(T->ptr[0]->Efalse,newLabel());
                T->ptr[0]->offset=T->ptr[1]->offset=T->ptr[2]->offset=T->offset;
                boolExp(T->ptr[0]);      //条件，要单独按短路代码处理
                T->width=T->ptr[0]->width;
                strcpy(T->ptr[1]->Snext,T->Snext);
                semantic_Analysis(T->ptr[1]);      //if子句
                if (T->width<T->ptr[1]->width) T->width=T->ptr[1]->width;
                strcpy(T->ptr[2]->Snext,T->Snext);
                semantic_Analysis(T->ptr[2]);      //else子句
                if (T->width<T->ptr[2]->width) T->width=T->ptr[2]->width;
                T->code=merge(6,T->ptr[0]->code,genLabel(T->ptr[0]->Etrue),T->ptr[1]->code,\
                              genGoto(T->Snext),genLabel(T->ptr[0]->Efalse),T->ptr[2]->code);
                break;
    case FOR:   strcpy(T->ptr[1]->Etrue,newLabel());
                strcpy(T->ptr[1]->Efalse,T->Snext);
                Exp(T->ptr[0]);
                boolExp(T->ptr[1]);
                strcpy(T->ptr[1]->Snext,newLabel());
                Exp(T->ptr[2]);
                strcpy(T->ptr[2]->Snext,newLabel());//生成label continue要到这个节点

                is_loop++;
                strcpy(loop_label,T->ptr[2]->Snext);//维护loop_label 用于配合continue
                strcpy(break_label,T->ptr[1]->Efalse);//维护break_label 用于配合break
                semantic_Analysis(T->ptr[3]);
                is_loop--;
                T->code = merge(8,T->ptr[0]->code,genLabel(T->ptr[1]->Snext),T->ptr[1]->code,\
                genLabel(T->ptr[1]->Etrue),T->ptr[3]->code,genLabel(T->ptr[2]->Snext),T->ptr[2]->code,genGoto(T->ptr[1]->Snext));
                break;
	case WHILE: strcpy(T->ptr[0]->Etrue,newLabel());  //子结点继承属性的计算
                //printf("!!!!!%s\n",T->ptr[0]->Etrue);
                //strcpy(loop_label,T->ptr[0]->Etrue); //记录位置用于跟continue配合
                strcpy(T->ptr[0]->Efalse,T->Snext);
                T->ptr[0]->offset=T->ptr[1]->offset=T->offset;
                boolExp(T->ptr[0]);      //循环条件，要单独按短路代码处理
                T->width=T->ptr[0]->width;
                strcpy(T->ptr[1]->Snext,newLabel());
                is_loop++;
                strcpy(loop_label,T->ptr[1]->Snext);//维护loop_label 用于配合continue
                strcpy(break_label,T->ptr[0]->Efalse);//维护break_label 用于配合break
                semantic_Analysis(T->ptr[1]);      //循环体
                is_loop--;
                if (T->width<T->ptr[1]->width) T->width=T->ptr[1]->width;
                printf("*******\n");
                prnIR(T->ptr[0]->code);
                printf("*******\n");
                T->code=merge(5,genLabel(T->ptr[1]->Snext),T->ptr[0]->code, \
                genLabel(T->ptr[0]->Etrue),T->ptr[1]->code,genGoto(T->ptr[1]->Snext));
                break;
    case EXP_STMT:
                T->ptr[0]->offset=T->offset;
                semantic_Analysis(T->ptr[0]);
                T->code=T->ptr[0]->code;
                T->width=T->ptr[0]->width;
                break;
	case RETURN:if (T->ptr[0]){
                    T->ptr[0]->offset=T->offset;
                    Exp(T->ptr[0]);

				 /*需要判断返回值类型是否匹配*/
                    int p = symbolTable.index;
                    do
                    {
                        p--;
                    } while (symbolTable.symbols[p].flag != 'F');
                    if (T->ptr[0]->type != symbolTable.symbols[p].type)
                    {
                        semantic_error(T->pos, "返回值类型错误", "");
                        break;
                    }

                    T->width=T->ptr[0]->width;
                    result.kind=ID; strcpy(result.id,symbolTable.symbols[T->ptr[0]->place].alias);
                    result.offset=symbolTable.symbols[T->ptr[0]->place].offset;
                    T->code=merge(2,T->ptr[0]->code,genIR(RETURN,opn1,opn2,result));
                    }
                else{
                    T->width=0;
                    result.kind=0;
                    T->code=genIR(RETURN,opn1,opn2,result);
                    }
                break;
    case BREAK:
    case CONTINUE:
	case ID:
    case ARRAY:
    case INT:
    case CHAR:
    case FLOAT:
	case ASSIGNOP:
	case AND:
	case OR:
	case RELOP:
	case PLUS:
	case MINUS:
	case STAR:
	case DIV:
	case NOT:
	case UMINUS:
    case SELFADD:
    case SELFDEC:
    case ADD_ASSIGNOP:
    case MINUS_ASSIGNOP:
    case STAR_ASSIGNOP:
    case DIV_ASSIGNOP:
    case FUNC_CALL:
                    Exp(T);          //处理基本表达式
                    break;
    }
    }
}

void semantic_Analysis0(struct ASTNode *T) {
    symbolTable.index=0;
    fillSymbolTable("read","",0,INT,'F',4);
    symbolTable.symbols[0].paramnum=0;//read的形参个数
    fillSymbolTable("write","",0,INT,'F',4);
    symbolTable.symbols[1].paramnum=1;
    fillSymbolTable("x","",1,INT,'P',12);
    symbol_scope_TX.TX[0]=0;  //外部变量在符号表中的起始序号为0
    symbol_scope_TX.top=1;
    T->offset=0;              //外部变量在数据区的偏移量
    semantic_Analysis(T);
    //prn_symbol();
    prnIR(T->code);
    // objectCode(T->code);
 }

