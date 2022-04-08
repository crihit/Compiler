#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void scan(FILE *fp);
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
        strcat(sourcefilename,".c");
    
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
    char token[100];
    int tPoint = 0;
    char c;

    c = fgetc(fp);
    while(c!=EOF)
    {
        printf("%c", c);
        c = fgetc(fp);
    }

    if(tPoint != 0) // comment error
    {
        fprintf(stderr, "Stop before ending\n");
    }
}