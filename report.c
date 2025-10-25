#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"

static void mergeparts(record *a, record *t, int l, int m, int r) {
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r) {
        if (a[i].amount >= a[j].amount) t[k++] = a[i++];
        else t[k++] = a[j++];
    }
    while (i <= m) t[k++] = a[i++];
    while (j <= r) t[k++] = a[j++];
    for (int x = l; x <= r; x++) a[x] = t[x];
}

static void mergerec(record *a, record *t, int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergerec(a, t, l, m);
    mergerec(a, t, m + 1, r);
    mergeparts(a, t, l, m, r);
}

void sortrecords(finance *f) {
    if (f->count <= 1) return;
    record *temp = (record *)malloc(sizeof(record) * f->count);
    if (!temp) return;
    mergerec(f->list, temp, 0, f->count - 1);
    free(temp);
}

void printcategory(finance *f) {
    printf("\ncategory totals\n");
    for (int i = 0; i < f->tablesize; i++) {
        catnode *t = f->table[i];
        while (t) {
            printf("%-15s : %.2f\n", t->name, t->total);
            t = t->next;
        }
    }
}

void printtop(finance *f) {
    printf("\ntop 5 expenses\n");
    sortrecords(f);
    int n = f->count < 5 ? f->count : 5;
    for (int i = 0; i < n; i++) {
        printf("%-10s %-12s %-20s %.2f\n",
               f->list[i].date, f->list[i].category, f->list[i].text, f->list[i].amount);
    }
}

void printmonth(finance *f) {
    printf("\nmonthly summary\n");
    for (int i = 0; i < f->count; i++) {
        char m[8];
        strncpy(m, f->list[i].date, 7);
        m[7] = '\0';
        double sum = 0;
        for (int j = 0; j < f->count; j++) {
            if (strncmp(f->list[j].date, m, 7) == 0) sum += f->list[j].amount;
        }
        int done = 0;
        for (int k = 0; k < i; k++) {
            if (strncmp(f->list[k].date, m, 7) == 0) { done = 1; break; }
        }
        if (!done) printf("%s : %.2f\n", m, sum);
    }
}

void makereport(finance *f) {
    printcategory(f);
    printtop(f);
    printmonth(f);
}
