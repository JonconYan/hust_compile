int array_read(struct ASTNode *T) //返回数据类型
{
    // printf("offset:%d",T->offset);
    struct opn opn1, opn2, result;
    struct ASTNode *T1 = T; //记录数组的开始结点

    int i = 0,
        j = 0, k;
    int rtn, tmp, tmp1;
    int record[MAXLENGTH]; //记录访问的每一维数的大小
    struct ASTNode *Node_Array[MAXLENGTH];
    int total = 0;
    while (T->kind == ARRAY_READ)
    {
        // semantic_Analysis(T->ptr[1]);
        // printf("IR:");
        // prnIR(T->ptr[1]->code);
        switch (T->ptr[1]->type)
        {
        case FLOAT:
        case CHAR:
            semantic_error(T->pos, "", "数组下标表达式非法");
            break;
        }
        Node_Array[i] = T->ptr[1];
        T = T->ptr[0];
        i++;
    }

    rtn = searchSymbolTable(T->type_id);

    if (rtn == -1)
    {
        semantic_error(T->pos, T->type_id, "数组未定义");
        return -1;
    }
    else if (symbolTable.symbols[rtn].flag == 'F')
    {
        semantic_error(T->pos, T->type_id, "函数名不能用下标访问");
        return -1;
    }
    else if (symbolTable.symbols[rtn].flag == 'V')
    {
        semantic_error(T->pos, T->type_id, "非数组变量，不能用下标访问");
        return -1;
    }
    else if (symbolTable.symbols[rtn].flag == 'P')
    {
        semantic_error(T->pos, T->type_id, "非数组变量，不能用下标访问");
        return -1;
    }
    else if (symbolTable.symbols[rtn].array_info->dimension != i)
    {
        semantic_error(T->pos, "", "数组维数错误");
        return -1;
    }

    for (j = 0; j < i; j++)
    {
        if (record[j] >= symbolTable.symbols[rtn].array_info->diminfo[j])
        {
            semantic_error(T->pos, "", "数组越界");
            return -1;
        }
        // int temp = 1;
        // for (k = j; k > 0; k--)
        // {
        //     temp *= symbolTable.symbols[rtn].array_info->diminfo[k - 1];
        // }
        // total += temp * record[j];
        // temp = 0;

        //record 3 2 1  k
        //diminfo 4 3 2  j
        //real 2 3 4
        // total += record[]
    }
    // prn_symbol();
    // printf("%d\n", Node_Array[i - 1]->kind);

    semantic_Analysis(Node_Array[i - 1]);

    for (j = i - 2; j >= 0; j--)
    {
        //先计算乘法,乘上前一位的元素上限
        T->place = fill_Temp(newTemp(), LEV, INT, 'T', Node_Array[j]->offset);
        strcpy(result.id, symbolTable.symbols[T->place].alias);
        result.kind = ID;

        opn2.kind = INT;
        opn2.const_int = symbolTable.symbols[rtn].array_info->diminfo[j];

        opn1.kind = ID;
        strcpy(opn1.id, symbolTable.symbols[Node_Array[j + 1]->place].alias);

        T1->code = merge(2, T1->code, genIR(STAR, opn1, opn2, result));

        //分析树的右半部分生成临时变量
        semantic_Analysis(Node_Array[j]);

        //再计算加法
        tmp = fill_Temp(newTemp(), LEV, INT, 'T', Node_Array[j]->offset);

        strcpy(result.id, symbolTable.symbols[tmp].alias);
        result.kind = ID;

        opn2.kind = ID;
        strcpy(opn2.id, symbolTable.symbols[Node_Array[j]->place].alias);

        opn1.kind = ID;
        strcpy(opn1.id, symbolTable.symbols[T->place].alias);

        // printf("tmp name : %s = %s + %s \n ", symbolTable.symbols[tmp].alias, symbolTable.symbols[T->place].alias, symbolTable.symbols[Node_Array[j]->place].alias);

        T1->code = merge(3, T1->code, Node_Array[j]->code, genIR(PLUS, opn1, opn2, result));
    }

    //计算总的偏移数，以字节为基本单位

    tmp1 = fill_Temp(newTemp(), LEV, INT, 'T', T->offset);

    strcpy(result.id, symbolTable.symbols[tmp1].alias);
    result.kind = ID;

    opn2.kind = INT;
    opn2.const_int = (symbolTable.symbols[rtn].array_info->type == CHAR ? 1 : 4);

    opn1.kind = ID;
    if(i==1)
        strcpy(opn1.id, symbolTable.symbols[Node_Array[i - 1]->place].alias);
    else
        strcpy(opn1.id, symbolTable.symbols[tmp].alias);

    T1->code = merge(2, T1->code, genIR(STAR, opn1, opn2, result));

    // prnIR(T1->code);

    T1->place = searchSymbolTable(T->type_id);
    symbolTable.symbols[T1->place].array_info->tartget = tmp1;

    // prn_symbol();

    return symbolTable.symbols[rtn].array_info->type;
}
