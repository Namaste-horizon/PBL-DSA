#pragma once
typedef struct usernode {
    char name[100];
    char pass[100];
    int qindex;
    char answer[100];
    struct usernode *next;
} usernode;

typedef struct auth {
    usernode *start;
} auth;

int loginmenu(auth *a);
void freeauth(auth *a);