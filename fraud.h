#ifndef FRAUD_H
#define FRAUD_H
#include "transaction.h"
#include "report.h"

int detect_fraud(transaction *list, int n);
int detectfraudforuser(const char *owner, transaction *all, int total);
#endif