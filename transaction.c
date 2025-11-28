#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"
#include "transaction.h"
#include "fraud.h"

void initfinance(finance *f, int s) {
    f->list = NULL;
    f->count = 0;
    f->size = 0;
    f->tablesize = s;
    f->table = (catnode **)calloc(s, sizeof(catnode *));
}

void freefinance(finance *f) {
    for (int i = 0; i < f->tablesize; i++) {
        catnode *t = f->table[i];
        while (t) {
            catnode *next = t->next;
            free(t);
            t = next;
        }
    }
    free(f->table);
    free(f->list);
    f->table = NULL;
    f->list = NULL;
    f->tablesize = 0;
    f->count = 0;
    f->size = 0;
}

static int hashtext(const char *key, int s) {
    int sum = 0;
    for (int i = 0; key[i]; i++) sum += key[i];
    return sum % s;
}

static void addcategory(finance *f, char *name, double amount) {
    int idx = hashtext(name, f->tablesize);
    catnode *t = f->table[idx];
    while (t) {
        if (strcmp(t->name, name) == 0) {
            t->total += amount;
            return;
        }
        t = t->next;
    }
    catnode *node = (catnode *)malloc(sizeof(catnode));
    if (!node) return;
    strcpy(node->name, name);
    node->total = amount;
    node->next = f->table[idx];
    f->table[idx] = node;
}

static void addrecord(finance *f, const char *date, const char *category, const char *text, double amount) {
    if (f->count == f->size) {
        int newsize = f->size == 0 ? 8 : f->size * 2;
        record *newlist = (record *)realloc(f->list, sizeof(record) * newsize);
        if (!newlist) return;
        f->list = newlist;
        f->size = newsize;
    }
    strcpy(f->list[f->count].date, date);
    strcpy(f->list[f->count].category, category);
    strcpy(f->list[f->count].text, text);
    f->list[f->count].amount = amount;
    f->count++;
    addcategory(f, f->list[f->count - 1].category, amount);
}

static void loadfile(finance *f, const char *name) {
    FILE *fp = fopen(name, "r");
    if (!fp) return;
    char line[256];
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return; }
    while (fgets(line, sizeof(line), fp)) {
        char d[11], c[30], t[50]; double a = 0;
        if (sscanf(line, "%10[^,],%29[^,],%49[^,],%lf", d, c, t, &a) == 4) {
            addrecord(f, d, c, t, a);
        }
    }
    fclose(fp);
}

static void savefile(finance *f, const char *name) {
    FILE *fp = fopen(name, "w");
    if (!fp) return;
    fprintf(fp, "date,category,text,amount\n");
    for (int i = 0; i < f->count; i++) {
        fprintf(fp, "%s,%s,%s,%.2f\n",
                f->list[i].date, f->list[i].category, f->list[i].text, f->list[i].amount);
    }
    fclose(fp);
}
void showcats(finance *f) {
    printf("\navailable categories\n");
    int count = 0;
    for (int i = 0; i < f->tablesize; i++) {
        catnode *t = f->table[i];
        while (t) {
            printf(" - %s\n", t->name);
            count++;
            t = t->next;
        }
    }
    if (count == 0) printf(" (none yet)\n");
}

void savecats(finance *f, const char *fname) {
    FILE *fp = fopen(fname, "w");
    if (!fp) return;
    for (int i = 0; i < f->tablesize; i++) {
        catnode *t = f->table[i];
        while (t) {
            fprintf(fp, "%s\n", t->name);
            t = t->next;
        }
    }
    fclose(fp);
}

void loadcats(finance *f, const char *fname) {
    FILE *fp = fopen(fname, "r");
    if (!fp) return;
    char line[64];
    while (fgets(line, sizeof(line), fp)) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (line[0] == '\0') continue;
        addcategory(f, line, 0.0);
    }
    fclose(fp);
}

void select_or_create_category(finance *f, char *out, int outsz, const char *storefname) {
    char c[30];
    showcats(f);
    printf("enter category name (or '+' to create new): ");
    scanf("%29s", c);
    if (strcmp(c, "+") == 0) {
        char newc[30];
        printf("new category name: ");
        scanf("%29s", newc);
        addcategory(f, newc, 0.0);
        savecats(f, storefname);
        strncpy(out, newc, outsz - 1);
        out[outsz - 1] = '\0';
        printf("category '%s' created.\n", out);
    } else {
        strncpy(out, c, outsz - 1);
        out[outsz - 1] = '\0';
    }
}

void add_record_flow(finance *f, const char *storefname) {
    char d[11], c[30], t[50];
    double a;
    printf("enter date (yyyy-mm-dd): ");
    scanf("%10s", d);
    select_or_create_category(f, c, sizeof(c), storefname);
    printf("enter text: ");
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
    fgets(t, sizeof(t), stdin);
    int len = (int)strlen(t);
    if (len > 0 && t[len - 1] == '\n') t[len - 1] = '\0';
    printf("enter amount: ");
    if (scanf("%lf", &a) != 1) {
        int cc; while ((cc = getchar()) != '\n' && cc != EOF) {}
        return;
    }
    addrecord(f, d, c, t, a);
    printf("added\n");
}

void menufinance(finance *f) {
    loadcats(f, "categories.txt");
    loadfile(f, "transactions.csv");
    while (1) {
        int choice;
        printf("\nfinance menu\n");
        printf("1 add record\n");
        printf("2 view reports\n");
        printf("3 show categories\n");
        printf("4 detect fraud\n");
        printf("5 save and exit\n");
        printf("enter choice: ");
        if (scanf("%d", &choice) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        if (choice == 1) {
            add_record_flow(f, "categories.txt");
        } else if (choice == 2) {
            makereport(f);
        } else if (choice == 3) {
            showcats(f);
        } else if (choice == 4) {
            detect_fraud(f);
        } else if (choice == 5) {
            savefile(f, "transactions.csv");
            savecats(f, "categories.txt");
            printf("bye\n");
            break;
        } else {
            printf("invalid choice\n");
        }
    }
}
