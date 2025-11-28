#pragma once

#ifndef TRANSACTION_H
#define TRANSACTION_H

typedef struct transaction {
    char *id;
    char *owner;
    char *date;
    char *cat;
    char *det;
    double amt;
} transaction;

int loadtransactions(transaction **out, int *n);
void freetransactions(transaction *arr, int n);

#endif

void menufinance(finance *f);