#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define TOKENSIZE 100
#define BUFFERSIZE 200

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
void scan(FILE *fp, FILE *outputfp);
int isDigit(char c);
int isLetter(char c);
void printToken(int lineIndex, TokenType curToken, char *tokenString, FILE *fp);

int main(int argc, char *argv[])
{
    char sourcefilename[120];
    char outputfilename[120];
    FILE *fp;
    FILE *outputfp;

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

    outputfp = stdout; // for test
    printf("%s %s\n", sourcefilename, outputfilename);
    scan(fp, outputfp);
    fclose(fp);
    return 0;
}

void scan(FILE *fp, FILE *outputfp)
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

    eof = fgets(lineBuffer, sizeof(lineBuffer), fp); // read 1 line
    while (eof != NULL)
    {
        fprintf(outputfp, "%d: %s", lineIndex, lineBuffer); // print line
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
                    else if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\0'))
                    {
                        state = START;
                        save = FALSE;
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
                        currentToken = NUM; // TODO : TOKEN SAVE
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
                    // if(currentToken == ID) // check reserved keyword
                    // {
                        
                    // }
                    printToken(lineIndex,currentToken,token,outputfp);
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
        bIndex = 0;
    }

    if(state == COMMENT || state == OUT_COMMENT) // comment error
    {
        fprintf(stderr, "Stop before ending\n");
    }
}

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
void printToken(int lineIndex, TokenType curToken, char *tokenString, FILE *fp)
{
    fprintf(fp, "\t%d: ",lineIndex);
    switch(curToken)
    {
        case ERROR: fprintf(fp,"error: %s\n",tokenString); break;
        case ENDFILE: fprintf(fp,"EOF\n"); break;
        case ELSE:
        case IF:
        case INT:
        case RETURN:
        case VOID:
        case WHILE: fprintf(fp, "reserved word: %s\n",tokenString); break;
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
        case RBRACK: fprintf(fp, "%s\n", tokenString); break;
        case ID: fprintf(fp, "ID, name= %s\n",tokenString); break;
        case NUM: fprintf(fp, "NUM, val= %s\n",tokenString); break;
        default:
            break;
    }
}