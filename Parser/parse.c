#define _CRT_SECURE_NO_WARNINGS // for VS2019
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define TOKENSIZE 100
#define BUFFERSIZE 200
#define MAXRESERVED 6
#define MAXCHILDREN 3

static int indentno = 0;

#define INDENT indentno+=2
#define UNINDENT indentno-=2

typedef enum
{
    NONE, START, IN_NUM, IN_ID, IN_LESS, IN_GREATER, IN_ASSIGN, IN_INEQUATION, IN_COMMENT, COMMENT, OUT_COMMENT, FINISH
} StateType;

typedef enum
{
    ERROR, ENDFILE,
    // keyword
    ELSE, IF, INT, RETURN, VOID, WHILE,
    // special symbol
    PLUS, MINUS, TIMES, OVER, LT, LE, GT, GE, EQ, INEQ, ASSIGN, SEMI, COMMA, LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK,
    // token
    ID, NUM
} TokenType;

static struct
{
    char *str;
    TokenType tok;
} reservedWords[MAXRESERVED] = {{"else", ELSE}, {"if", IF}, {"int", INT}, {"return", RETURN}, {"void", VOID}, {"while", WHILE}};

typedef enum { StmtK, ExpK } NodeKind;
typedef enum { CompStmtK, SelStmtK, IterStmtK, RetStmtK, CallK } StmtKind;
typedef enum { VarDeclK, VarArrayDeclK, FuncDeclK, AssignK, OpK, IdK, ConstK } ExpKind;
typedef enum {Void,Integer,Boolean, IntegerArray} ExpType;

typedef struct treeNode
{
	struct treeNode * child[MAXCHILDREN];
	struct treeNode * sibling;
	NodeKind nodekind;
	union { StmtKind stmt; ExpKind exp; } kind;
	union {
		TokenType op;
		int val;
		char * name;
		struct ScopeListRec *scope;
	} attr;
	ExpType type;
	int isParam;
	int arraysize;
} TreeNode;

char tokenStringArr[1000][100];
TokenType tokenTypeArr[1000];
int tokenPoint = 0;
int tokenParseNow = 0;
TokenType token;
char tokenString[100];

void scan(FILE *fp);
int isDigit(char c);
int isLetter(char c);
TokenType checkReservedWord(char *t);
void saveToken(TokenType tokenType, char *tokenString);
void printToken(TokenType curToken, char *tokenString);
TreeNode *newStmtNode(StmtKind kind);
TreeNode * newExpNode(ExpKind kind);
TokenType getToken();
static void syntaxError(char * message);
static void match(TokenType expected, char * from);
static TreeNode * declaration_list(void);
static TreeNode * declaration(void);
static TreeNode * var_declaration(void);
static ExpType type_specifier(void);
static TreeNode * params(void);
static TreeNode * param_list(ExpType type);
static TreeNode * param(ExpType type);
static TreeNode * compound_stmt(void);
static TreeNode * local_declaration(void);
static TreeNode * statement_list(void);
static TreeNode * statement(void);
static TreeNode * expression_stmt(void);
static TreeNode * selection_stmt(void);
static TreeNode * iteration_stmt(void);
static TreeNode * return_stmt(void);
static TreeNode * expression(void);
static TreeNode * simple_expression(TreeNode *f);
static TreeNode * additive_expression(TreeNode *f);
static TreeNode * term(TreeNode *f);
static TreeNode * factor(TreeNode *f);
static TreeNode * call(void);
static TreeNode * args(void);
static TreeNode * args_list(void);
static TreeNode * parse(void);
static void printSpaces(void);
void printTree(TreeNode * tree);
char *typeName(ExpType type);
void testPrint(char *s);
char * copyString(char * s);

FILE *listing;

int main(int argc, char *argv[])
{
    char sourcefilename[120];
    char outputfilename[120];
    FILE *fp;
    TreeNode *syntaxTree;

    // argument check
    if(argc != 3)
    {
        fprintf(stderr, "usage: %s <source name> <output file name>\n", argv[0]);
        exit(1);
    }

    // check file name, open file.
    strcpy(sourcefilename, argv[1]);
    strcpy(outputfilename, argv[2]);

    if(strchr(sourcefilename, '.') == NULL) // if filename without extension
        strcat(sourcefilename,".c");        // Only work on same folder
    
    if(strchr(outputfilename, '.') == NULL)
        strcat(outputfilename,".txt");

    fp = fopen(sourcefilename, "r");
    if(fp==NULL)
    {
        fprintf(stderr, "File %s not found\n", sourcefilename);
        exit(1);
    }

    // listing = stdout; // for test
    printf("%s %s\n", sourcefilename, outputfilename);
    scan(fp);
    fclose(fp);

    listing = fopen(outputfilename, "w");
    syntaxTree = parse();
    printTree(syntaxTree);
    fclose(listing);
    return 0;
}

void scan(FILE *fp)
{
    int lineIndex = 1; // line number
    char lineBuffer[BUFFERSIZE];
    int bIndex = 0; // index of buffer

    char token[TOKENSIZE];
    int tIndex = 0; // index of token

    char *eof; // for check EOF
    StateType state = START;
    TokenType currentToken;
    int save; // check token save

    int breaking = FALSE; // check break
    char c;
    int i;
    eof = fgets(lineBuffer, sizeof(lineBuffer), fp); // read 1 line
    while (eof != NULL)
    {
        for(i=0;i<BUFFERSIZE;i++)
        {
            if(lineBuffer[i]=='\0' && lineBuffer[i-1]!='\n')
            {
                lineBuffer[i] = '\n';
                lineBuffer[i+1] = '\0';
                break;
            }
        }
        bIndex = 0;
        breaking = FALSE;
        while (lineBuffer[bIndex] != '\n' && breaking == FALSE)
        {
            tIndex = 0;
            breaking = FALSE;
            if(!(state == COMMENT || state == OUT_COMMENT))
                state = START;
            while (state != FINISH)
            {
                c = lineBuffer[bIndex++]; // get one char
                save = TRUE;
                // printf("state: %d, c: %c\n",state,c); // test code for checking state
                switch (state)
                {
                case START:
                    if (isDigit(c))
                        state = IN_NUM;
                    else if (isLetter(c))
                        state = IN_ID;
                    else if (c == '<')
                        state = IN_LESS;
                    else if (c == '>')
                        state = IN_GREATER;
                    else if (c == '=')
                        state = IN_ASSIGN;
                    else if (c == '!')
                        state = IN_INEQUATION;
                    else if (c == '/')
                    {
                        state = IN_COMMENT;
                        save = FALSE; // If not comment, save token OVER
                    }
                    else if ((c == ' ') || (c == '\t') || (c == '\n'))
                    {
                        state = START;
                        save = FALSE;
                    }
                    else if((c == EOF))
                    {
                        currentToken = ENDFILE;
                        state = FINISH;
                        printf("eof");
                    }
                    else
                    {
                        state = FINISH;
                        switch (c)
                        {
                        case EOF:
                            currentToken = ENDFILE;
                            break;
                        case '+':
                            currentToken = PLUS;
                            break;
                        case '-':
                            currentToken = MINUS;
                            break;
                        case '*':
                            currentToken = TIMES;
                            break;
                        case ';':
                            currentToken = SEMI;
                            break;
                        case ',':
                            currentToken = COMMA;
                            break;
                        case '(':
                            currentToken = LPAREN;
                            break;
                        case ')':
                            currentToken = RPAREN;
                            break;
                        case '{':
                            currentToken = LBRACE;
                            break;
                        case '}':
                            currentToken = RBRACE;
                            break;
                        case '[':
                            currentToken = LBRACK;
                            break;
                        case ']':
                            currentToken = RBRACK;
                            break;
                        default:
                            currentToken = ERROR;
                            break;
                        }
                    }
                    break;
                case IN_NUM:
                    if(!isDigit(c))
                    {
                        save = FALSE;
                        bIndex--; // lookahead
                        state = FINISH;
                        currentToken = NUM;
                    }
                    break;
                case IN_ID:
                    if(!isLetter(c))
                    {
                        save = FALSE;
                        bIndex--;
                        state = FINISH;
                        currentToken = ID;
                    }
                    break;
                case IN_LESS:
                    state = FINISH;
                    if(c=='=')
                    {
                        currentToken = LE;
                    }
                    else
                    {
                        save = FALSE;
                        bIndex--;
                        currentToken = LT;
                    }
                    break;
                case IN_GREATER:
                    state = FINISH;
                    if(c=='=')
                    {
                        currentToken = GE;
                    }
                    else
                    {
                        save = FALSE;
                        bIndex--;
                        currentToken = GT;
                    }
                    break;
                case IN_ASSIGN:
                    state = FINISH;
                    if(c=='=')
                    {
                        currentToken = EQ;
                    }
                    else
                    {
                        save = FALSE;
                        bIndex--;
                        currentToken = ASSIGN;
                    }
                    break;
                case IN_INEQUATION:
                    state = FINISH;
                    if(c=='=')
                    {
                        currentToken = INEQ;
                    }
                    else
                    {
                        save = FALSE;
                        bIndex--;
                        currentToken = ERROR;
                    }
                    break;
                case IN_COMMENT:
                    if(c=='*')
                    {
                        save = FALSE;
                        state = COMMENT;
                    }
                    else
                    {
                        save = TRUE;
                        bIndex--;
                        c = '/'; // save OVER
                        currentToken = OVER;
                        state = FINISH;
                    }
                    break;
                case COMMENT:
                    save = FALSE;
                    if(c=='*')
                    {
                        state = OUT_COMMENT;
                    }
                    else
                    {
                        state = COMMENT;
                    }
                    break;
                case OUT_COMMENT:
                    save = FALSE;
                    if(c=='/')
                    {
                        state = START;
                    }
                    else if(c=='*')
                    {
                        state = OUT_COMMENT;
                    }
                    else
                    {
                        state = COMMENT;
                    }
                    break;
                case FINISH:
                default:
                    break;
                }
                if(save==TRUE)
                {
                    token[tIndex++] = c;
                }
                if(state==FINISH)
                {
                    token[tIndex] = '\0';
                    tIndex = 0;
                    if(currentToken == ID) // check reserved keyword
                    {
                        currentToken = checkReservedWord(token);
                    }
                    saveToken(currentToken, token);
                    memset(token,'\0',TOKENSIZE);
                }
                if(c=='\n')
                {
                    breaking = TRUE;
                    break;
                }
            }
        }

        eof = fgets(lineBuffer, sizeof(lineBuffer), fp); // get next line
        lineIndex++;
        if(eof=='\0')
        {
            saveToken(ENDFILE, "");
        }
        bIndex = 0;
    }
    if(state == COMMENT || state == OUT_COMMENT) // comment error
    {
        fprintf(stderr, "Stop before ending\n");
    }
}

// for scan
int isDigit(char c)
{
    if(c>='0' && c<='9')
        return TRUE;
    else
        return FALSE;
}
int isLetter(char c)
{
    if((c>='A' && c<='Z') || (c>='a'&&c<='z'))
        return TRUE;
    else
        return FALSE;
}
void printToken(TokenType curToken, char *tokenString)
{
    switch(curToken)
    {
        case ERROR: fprintf(listing,"error: %s\n",tokenString); break;
        case ENDFILE: fprintf(listing,"EOF\n"); break;
        case ELSE:
        case IF:
        case INT:
        case RETURN:
        case VOID:
        case WHILE: fprintf(listing, "reserved word: %s\n",tokenString); break;
        case PLUS:
        case MINUS:
        case TIMES:
        case OVER:
        case LT:
        case LE:
        case GT:
        case GE:
        case EQ:
        case INEQ:
        case ASSIGN:
        case SEMI:
        case COMMA:
        case LPAREN:
        case RPAREN:
        case LBRACE:
        case RBRACE:
        case LBRACK:
        case RBRACK: fprintf(listing, "%s\n", tokenString); break;
        case ID: fprintf(listing, "ID, name= %s\n",tokenString); break;
        case NUM: fprintf(listing, "NUM, val= %s\n",tokenString); break;
        default:
            break;
    }
}
TokenType checkReservedWord(char *t)
{
    int i;
    for(i=0;i<MAXRESERVED;i++)
    {
        if(!strcmp(t,reservedWords[i].str))
        {
            return reservedWords[i].tok;
        }
    }
    return ID;
}
void saveToken(TokenType tokenType, char *tokenString)
{
    strcpy(tokenStringArr[tokenPoint], tokenString);
    tokenTypeArr[tokenPoint++] = tokenType;
}


// for parse

TreeNode *newStmtNode(StmtKind kind)
{
    TreeNode * t = (TreeNode *)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		fprintf(listing, "Out of memory error\n");
	else {
		for (i = 0;i<MAXCHILDREN;i++) t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = StmtK;
		t->kind.stmt = kind;
	}
	return t;
}
TreeNode * newExpNode(ExpKind kind)
{
	TreeNode * t = (TreeNode *)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		fprintf(listing, "Out of memory error\n");
	else {
		for (i = 0;i<MAXCHILDREN;i++) t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = ExpK;
		t->kind.exp = kind;
		t->type = Void;
		t->isParam = FALSE;
	}
	return t;
}

TokenType getToken()
{
    strcpy(tokenString, tokenStringArr[tokenParseNow]);
    return tokenTypeArr[tokenParseNow++];
}
static void syntaxError(char * message)
{ 
    fprintf(listing,"\n>>> ");
    fprintf(listing,"Syntax error : %s",message);
}
static void match(TokenType expected, char * from)
{
    if (token == expected)
    {
        token = getToken();
    }
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token,tokenString);
        fprintf(listing,"      ");
    }
}

TreeNode * declaration_list()
{
    testPrint("declaration_list");
    TreeNode * t = declaration();
    TreeNode * p = t;
    while(token != ENDFILE)
    {        
        TreeNode * q;
        q = declaration();
        if (q!=NULL)
        {
            if (t==NULL)
            {
                t = p = q;
            }
            else
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}
TreeNode * declaration()
{
    testPrint("declaration");

    TreeNode *t = NULL;
	ExpType type;
	char *name = NULL;

	type = type_specifier();

    name = copyString(tokenString);
	match(ID, "declaration");

	switch (token)
	{
	case SEMI:
		t = newExpNode(VarDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(SEMI, "declaration");
		break;
	case LBRACK:
		t = newExpNode(VarArrayDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LBRACK, "declaration");
		if (t != NULL)
			t->arraysize = atoi(tokenString);
		match(NUM, "declaration");
		match(RBRACK, "declaration");
		match(SEMI, "declaration");
		break;
	case LPAREN:
		t = newExpNode(FuncDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LPAREN, "declaration");
		if (t != NULL)
			t->child[0] = params();
		match(RPAREN, "declaration");
		if (t != NULL)
			t->child[1] = compound_stmt();
		break;
	default: syntaxError("unexpected token (decl) -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	}
	return t;
}
ExpType type_specifier()
{
    testPrint("type_specifier");
    switch (token)
    {
    case INT:
        token = getToken();
        return Integer;
        break;
    case VOID:
        token = getToken();
        return Void;
        break;
    default:
        syntaxError("unexpected token -> ");
        printToken(token,tokenString);
        token = getToken();
        return Void;
        break;
    }
}
TreeNode * var_declaration(void)
{
    testPrint("var_declaration");
	TreeNode *t = NULL;
	ExpType type;
	char *name = NULL;
	
	type = type_specifier();
    name = copyString(tokenString);
	match(ID, "var_declaration");
	switch (token)
	{
	case SEMI:
		t = newExpNode(VarDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(SEMI, "var_declaration");
		break;
	case LBRACK:
		t = newExpNode(VarArrayDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LBRACK, "var_declaration");
		if (t != NULL)
			t->arraysize = atoi(tokenString);
		match(NUM, "var_declaration");
		match(RBRACK, "var_declaration");
		match(SEMI, "var_declaration");
		break;
	default: syntaxError("unexpected token(var_decl) -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	}
	return t;
}
TreeNode * params(void)
{
    testPrint("params");
	ExpType type;
	TreeNode *t = NULL;

	type = type_specifier();
	if (type == Void && token == RPAREN)
	{
		t = newExpNode(VarDeclK);
		t->isParam = TRUE;
		t->type = Void;
	}
	else
		t = param_list(type);
	return t;
}
TreeNode * param_list(ExpType type)
{
    testPrint("param_list");
	TreeNode * t = param(type);
	TreeNode * p = t;
	TreeNode * q;
	while (token==COMMA)
	{
		match(COMMA, "param_list");
		q = param(type_specifier());
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else 
			{
				p->sibling = q;
				p = q;
			}
		}
	}
	return t;
}
TreeNode * param(ExpType type)
{
    testPrint("param");
	TreeNode *t = NULL;
	char *name = NULL;

    name = copyString(tokenString);
	match(ID, "param");
	if (token == LBRACK)
	{
		match(LBRACK, "param");
		match(RBRACK, "param");
		t = newExpNode(VarArrayDeclK);
	}
	else
		t = newExpNode(VarDeclK);
	if (t != NULL)
	{
		t->attr.name = name;
		t->type = type;
		t->isParam = TRUE;
	}
	return t;
}
TreeNode * compound_stmt(void)
{
    testPrint("compound_stmt");
	TreeNode *t = newStmtNode(CompStmtK);
	match(LBRACE, "compound_stmt");
	t->child[0] = local_declaration();
	t->child[1] = statement_list();
	match(RBRACE, "compound_stmt");
	return t;
}
TreeNode * local_declaration(void)
{
    testPrint("local_declaration");
	TreeNode * t=NULL;
	TreeNode * p;

	if (token == INT || token == VOID)
		t = var_declaration();
	p = t;
	if (t != NULL)
	{
		while (token == INT || token == VOID )
		{
			TreeNode * q;
			q = var_declaration();
			if (q != NULL) {
				if (t == NULL) t = p = q;
				else
				{
					p->sibling = q;
					p = q;
				}
			}
		}
	}
	return t;
}
TreeNode * statement_list(void)
{
    testPrint("statement_list");
	TreeNode * t = NULL;
	TreeNode * p;

	if (token == RBRACE)
		return NULL;
	t = statement();
	p = t;
	while (token != RBRACE)
	{
		TreeNode * q;
		q = statement();
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else
			{
				p->sibling = q;
				p = q;
			}
		}
	}
	
	return t;
}
TreeNode * statement(void)
{
    testPrint("statement");
	TreeNode *t = NULL;
	switch (token)
	{
	case LBRACE:
		t = compound_stmt();
		break;
	case IF:
		t = selection_stmt();
		break;
	case WHILE:
		t = iteration_stmt();
		break;
	case RETURN:
		t = return_stmt();
		break;
	case ID:
	case LPAREN:
	case NUM:
	case SEMI:
		t = expression_stmt();
		break;
	default: syntaxError("unexpected token(statement) -> ");
		printToken(token, tokenString);
		token = getToken();
		return t;
	}
	return t;
}
TreeNode * expression_stmt(void)
{
    testPrint("expression_stmt");
	TreeNode *t = NULL;
	
	if (token == SEMI)
		match(SEMI, "expression_stmt");
	else if (token != RBRACE)
	{
		t = expression();
		match(SEMI, "expression_stmt");
	}
	return t;
}
TreeNode * selection_stmt(void)
{
    testPrint("selection_stmt");
	TreeNode *t = newStmtNode(SelStmtK);

	match(IF, "selection_stmt");
	match(LPAREN, "selection_stmt");
	if(t!=NULL)
		t->child[0] = expression();
	match(RPAREN, "selection_stmt");
	if(t!=NULL)
		t->child[1] = statement();
	if (token == ELSE)
	{
		match(ELSE, "selection_stmt");
		if(t!=NULL)
			t->child[2] = statement();
	}
	
	return t;
}
TreeNode * iteration_stmt(void)
{
    testPrint("iteration_stmt");
	TreeNode *t = newStmtNode(IterStmtK);

	match(WHILE, "iteration_stmt");
	match(LPAREN, "iteration_stmt");
	if(t!=NULL)
		t->child[0] = expression();
	match(RPAREN, "iteration_stmt");
	if(t!=NULL)
		t->child[1] = statement();
	return t;
}
TreeNode * return_stmt(void)
{
    testPrint("return_stmt");
	TreeNode *t = newStmtNode(RetStmtK);

	match(RETURN, "return_stmt");
	if (token != SEMI && t!=NULL)
		t->child[0] = expression();
	match(SEMI, "return_stmt");
	return t;
}
TreeNode * expression(void)
{
    testPrint("expression");
	TreeNode *t = NULL;
	TreeNode *q = NULL;
	int flag = FALSE;

	if (token == ID)
	{
		q = call();
		flag = TRUE;
	}
	if (flag == TRUE && token == ASSIGN)
	{
		if (q != NULL && q->nodekind == ExpK && q->kind.exp == IdK)
		{
			match(ASSIGN, "expression");
			t = newExpNode(AssignK);
			if (t != NULL)
			{
				t->child[0] = q;
				t->child[1] = expression();
			}
		}
		else
		{
			syntaxError("attempt to assign to something not an lvalue\n");
			token = getToken();
		}
	}
	else
		t = simple_expression(q);
    // else if(token != SEMI && token != COMMA && token != RPAREN)
	// 	t = simple_expression(q);
	return t;
}
TreeNode * simple_expression(TreeNode *f)
{
    testPrint("simple_expression");
	TreeNode *t = NULL, *q;
	TokenType oper;
	q = additive_expression (f);
	if (token == LT || token == LE || token == GT || token == GE || token == EQ || token == INEQ)
	{
		oper = token;
		match(token, "simple_expression");
		t = newExpNode(OpK);
		if (t != NULL)
		{
			t->child[0] = q;
			t->child[1] = additive_expression(NULL);
			t->attr.op = oper;
		}
	}
	else
		t = q;
	return t;
}
TreeNode * additive_expression(TreeNode *f)
{
    testPrint("additive_expression");
	TreeNode *t = NULL;
	TreeNode *q;

	t = term(f);
	if (t != NULL)
	{
		while (token == PLUS || token==MINUS)
		{
			q = newExpNode(OpK);
			if (q != NULL) {
					q->child[0] = t;
					q->attr.op = token;
					t = q;
					match(token, "additive_expression");
					t->child[1] = term(NULL);
				
			}
		}
	}
	return t;
}
TreeNode * term(TreeNode *f)
{
    testPrint("term");
	TreeNode *t = NULL;
	TreeNode *q;

	t = factor(f);
	if (t != NULL)
	{
		while (token == TIMES || token == OVER)
		{
			q = newExpNode(OpK);
			if (q != NULL) {
					q->child[0] = t;
					q->attr.op = token;
					t = q;
					match(token, "term");
					t->child[1] = factor(NULL);
				
			}
		}
	}
	return t;
}
TreeNode * factor(TreeNode *f)
{
    testPrint("factor");
	TreeNode *t = NULL;
	
	if (f != NULL)
		return f;

	switch (token)
	{
	case LPAREN:
		match(LPAREN, "factor");
		t = expression();
		match(RPAREN, "factor");
		break;
	case ID:
		t = call();
		break;
	case NUM:
		t = newExpNode(ConstK);
		if (t != NULL)
		{
			t->attr.val = atoi(tokenString);
			t->type = Integer;
		}
		match(NUM, "factor");
		break;
	default: syntaxError("unexpected token(factor) -> ");
		printToken(token, tokenString);
		token = getToken();
		return t;
	}
	return t;
}
TreeNode * call(void)
{
    testPrint("call");
	TreeNode *t = NULL;
	char *name = NULL;

	if(token==ID)
        name = copyString(tokenString);
	match(ID, "call");

	if (token == LPAREN) // call
	{
		match(LPAREN, "call");
		t = newStmtNode(CallK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->child[0] = args();
		}
		match(RPAREN, "call");
	}
	else if(token==LBRACK) // var
	{
		t = newExpNode(IdK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = Integer;
			match(LBRACK, "call");
			t->child[0] = expression();
			match(RBRACK, "call");
		}
	}
	else
	{
		t = newExpNode(IdK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = Integer;
		}
	}
	return t;
}
TreeNode * args(void)
{
    testPrint("args");
	if (token == RPAREN)
		return NULL;
	else
		return args_list();
}
TreeNode * args_list(void)
{
    testPrint("args_list");
	TreeNode * t = NULL;
	TreeNode * p;

	t = expression();
	p = t;
	if (t != NULL)
	{
		while (token == COMMA)
		{
			match(COMMA, "args_list");
			TreeNode * q = expression();
			if (q != NULL) {
				if (t == NULL) t = p = q;
				else 
				{
					p->sibling = q;
					p = q;
				}
			}
		}
	}
	return t;
}
TreeNode * parse()
{
    testPrint("parse");
    TreeNode *t = NULL;
    token = getToken();
    t = declaration_list();
    if(token!=ENDFILE)
    {
        syntaxError("Code ends before file\n");
    }
    return t;
}

static void printSpaces(void)
{ int i;
  for (i=0;i<indentno;i++)
    fprintf(listing," ");
}
void printTree(TreeNode * tree)
{
	int i;
	INDENT;
	while (tree != NULL) {
		printSpaces();
		if (tree->nodekind == StmtK)
		{
			switch (tree->kind.stmt) {
			case CompStmtK:
				fprintf(listing, "Compound Statement :\n");
				break;
			case SelStmtK:
				if(tree->child[2]!=NULL)
					fprintf(listing, "If (condition) (body) (else) \n");
				else
					fprintf(listing, "If (condition) (body)\n");
				break;
			case IterStmtK:
				fprintf(listing, "While (condition) (body) \n");
				break;
			case RetStmtK:
				if(tree->child[0]==NULL)
					fprintf(listing, "Return Statement, with NOTHING\n");
				else
					fprintf(listing, "Return Statement, with below\n");
				break;
			case CallK:
				if(tree->child[0]!=NULL)
					fprintf(listing, "Call, name : %s, with arguments below\n",tree->attr.name);
				else
					fprintf(listing, "Call, name : %s, with NOTHING\n", tree->attr.name);
				break;
			default:
				fprintf(listing, "Unknown ExpNode kind\n");
				break;
			}
		}
		else if (tree->nodekind == ExpK)
		{
			switch (tree->kind.exp) {
			case VarDeclK:
				if(tree->isParam==TRUE)
					fprintf(listing, "Single Parameter, name : %s, type : %s\n", tree->attr.name, typeName(tree->type));
				else
					fprintf(listing, "Var Declaration, name : %s, type : %s\n",tree->attr.name, typeName(tree->type));
				break;
			case VarArrayDeclK:
				if(tree->isParam==TRUE)
					fprintf(listing, "Array Parameter, name : %s, type : %s\n", tree->attr.name, typeName(tree->type));
				else
					fprintf(listing, "Array Var Declaration, name : %s, type : %s, size : %d\n", tree->attr.name, typeName(tree->type), tree->arraysize);
				break;
			case FuncDeclK:
				fprintf(listing, "Function Declaration, name : %s, type : %s\n", tree->attr.name, typeName(tree->type));
				break;
			case AssignK:
				fprintf(listing, "Assign : (destination) (source) \n");
				break;
			case OpK:
				fprintf(listing, "Op : ");
				printToken(tree->attr.op, "\0");
				break;
			case IdK:
				fprintf(listing, "Id : %s\n",tree->attr.name);
				break;
			case ConstK:
				fprintf(listing, "Const : %d\n",tree->attr.val);
				break;
			default:
				fprintf(listing, "Unknown ExpNode kind\n");
				break;
			}
		}
		else fprintf(listing, "Unknown node kind\n");
		for (i = 0;i<MAXCHILDREN;i++)
			printTree(tree->child[i]);
		tree = tree->sibling;
	}
	UNINDENT;
}
char *typeName(ExpType type)
{
	static char i[] = "int";
	static char v[] = "void";
	static char invalid[] = "<<invalid type>>";

	switch (type)
	{
	case Integer: return i; break;
	case Void:    return v; break;
	default:      return invalid;
	}
}

void testPrint(char *s)
{
    // printf("%d %s %s\n", token, s, tokenString);
}

char * copyString(char * s)
{
	int n;
	char * t;
	if (s == NULL) return NULL;
	n = strlen(s) + 1;
	t = (char *)malloc(n);
	if (t == NULL)
		fprintf(listing, "Out of memory error\n");
	else strcpy(t, s);
	return t;
}