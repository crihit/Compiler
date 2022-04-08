#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    NONE, START, IN_NUM, IN_ID, IN_LESS, IN_GREATER, IN_ASSIGN, IN_INEQUATION, IN_COMMENT, COMMENT, OUT_COMMENT, FINISH
} StateType;

typedef enum
{
    ERROR,
    // keyword
    ELSE, IF, INT, RETURN, VOID, WHILE,
    // special symbol
    PLUS, MINUS, TIMES, OVER, LT, LE, GT, GE, EQ, INEQ, ASSIGN, SEMI, COMMA, LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK,
    // token
    ID, NUM
} TokenType;
void scan(FILE *fp);
int isDigit(char c);
int isLetter(char c);
int main(int argc, char *argv[])
{
    char sourcefilename[120];
    char outputfilename[120];
    FILE *fp;

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

    printf("%s %s\n", sourcefilename, outputfilename);
    scan(fp);
    fclose(fp);
    return 0;
}

void scan(FILE *fp)
{
    int lineIndex = 1; // line number
    char lineBuffer[200];
    int bIndex = 0; // index of buffer

    char token[100];
    int tIndex = 0; // index of token

    char *eof; // for check EOF
    StateType state = START;
    TokenType currentToken;
    char c;

    eof = fgets(lineBuffer, sizeof(lineBuffer), fp); // read 1 line
    while (eof != NULL)
    {
        printf("%d: %s", lineIndex, lineBuffer); // print line
        bIndex = 0;

        while (lineBuffer[bIndex] != '\n')
        {
            tIndex = 0;
            while (state != FINISH)
            {
                c = lineBuffer[bIndex++]; // get one char

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
                        state = IN_COMMENT;
                    else if ((c == ' ') || (c == '\t') || (c == '\n'))
                        state = START;
                    else
                    {
                        state = FINISH;
                        switch (c)
                        {
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
                        bIndex--; // lookahead
                        state = FINISH;
                        currentToken = NUM; // TODO : TOKEN SAVE
                    }
                    break;
                case IN_ID:
                    break;
                case IN_LESS:
                    break;
                case IN_GREATER:
                    break;
                case IN_ASSIGN:
                    break;
                case IN_INEQUATION:
                    break;
                case IN_COMMENT:
                    break;
                case COMMENT:
                    break;
                case OUT_COMMENT:
                    break;
                case FINISH:
                default:
                    break;
                }
            }
        }

        eof = fgets(lineBuffer, sizeof(lineBuffer), fp); // get next line
        lineIndex++;
    }

    if(tIndex != 0) // comment error
    {
        fprintf(stderr, "Stop before ending\n");
    }
}

int isDigit(char c)
{
    if(c>='0' && c<='9')
        return 1;
    else
        return 0;
}
int isLetter(char c)
{
    if((c>='A' && c<='Z') || (c>='a'&&c<='z'))
        return 1;
    else
        return 0;
}