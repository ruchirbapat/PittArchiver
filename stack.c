#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stack.h"

void push(STACK **head, DATA *data)
{
    STACK *node = malloc(sizeof(STACK));

    if (node == NULL)
    {
        printf("Malloc failed");
        exit(EXIT_FAILURE);
    } else {
        memcpy(&node->data,data,sizeof(DATA));
        if(empty(*head))
        {
            node->next = NULL;
        } else {
            node->next = *head;
        }
        *head = node;
    }
}

DATA *pop(STACK **head)
{
    DATA *ret = malloc(sizeof(*ret));
    if (empty(*head))
    {
        return NULL;
    } else {
        STACK *top = *head;
        memcpy(ret,&top->data,sizeof(DATA));
        *head = top->next;
        free(top);
        return ret;
    }
}

int empty(STACK *head)
{
    if(head == NULL)
    {
        return 1;
    } else {
        return 0;
    }
}
