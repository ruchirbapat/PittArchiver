#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

typedef struct data
{
    char f_name[200];   //file name
    long int file_offset;    //offset, location of file
    int uid;            //user id
    int gid;            //group id
    int size;           //file size
    char mtime[13];     //time of last modification
    mode_t perms;
    char type;         //file type
    char c_flag; //indicates if file is compressed
} DATA;

typedef struct stack
{
    DATA data;
    struct stack *next;
} STACK;

void push(STACK **head, DATA *data);

DATA *pop(STACK **head);

int empty(STACK *head);
#endif // STACK_H_INCLUDED
