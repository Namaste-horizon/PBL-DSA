#ifndef SPLITTER_H
#define SPLITTER_H

typedef struct splitmember {
    char name[50];
    double balance;
} splitmember;

typedef struct expense_share {
    char name[50];
    double amount;
} expense_share;

typedef struct expense_entry {
    char date[11];
    char desc[80];
    double amount;
    char payer[50];
    int sharecount;
    expense_share *shares;
} expense_entry;

typedef struct settlement_entry {
    char date[11];
    char from[50];
    char to[50];
    double amount;
} settlement_entry;

typedef struct splitter {
    splitmember *members;
    int membercount;
    int membercap;
    expense_entry *expenses;
    int expensecount;
    int expensecap;
    settlement_entry *settlements;
    int settlementcount;
    int settlementcap;
} splitter;

void initsplitter(splitter *s);
void freesplitter(splitter *s);
void splittermenu(splitter *s);

#endif

