#pragma once
typedef struct record {
    char date[11];
    char category[30];
    char text[50];
    double amount;
} record;

typedef struct catnode {
    char name[30];
    double total;
    struct catnode *next;
} catnode;

typedef struct finance {
    record *list;
    int count;
    int size;
    catnode **table;
    int tablesize;
} finance;

void sortrecords(finance *f);
void printcategory(finance *f);
void printtop(finance *f);
void printmonth(finance *f);
void makereport(finance *f);