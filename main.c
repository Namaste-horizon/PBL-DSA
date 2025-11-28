#include <stdio.h>
#include <string.h>
#include "login.h"
#include "transaction.h"
#include "fraud.h"
#include "report.h"
#include "splitter.h"

static void run_finance(void) {
    finance f;
    initfinance(&f, 50);
    menufinance(&f);
    freefinance(&f);
}

static void run_splitter(void) {
    splitter s;
    initsplitter(&s);
    splittermenu(&s);
    freesplitter(&s);
}

int main() {
    auth a;
    a.start = NULL;

    int ok = loginmenu(&a);
    if (ok != 1) {
        freeauth(&a);
        return 0;
    }

    transaction *all = NULL;
    int n = 0;
    loadtransactions(&all, &n);

    char currentuser[100];
    printf("enter your username again to load your reports: ");
    scanf("%99s", currentuser);

    while (1) {
        int choice;
        printf("\nmain hub\n");
        printf("1 finance tracker\n");
        printf("2 expense splitter\n");
        printf("3 exit\n");
        printf("4 fraud check for your account\n");
        printf("5 generate report for your account\n");
        printf("enter choice: ");

        if (scanf("%d", &choice) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }

        if (choice == 1) {
            run_finance();
        } else if (choice == 2) {
            run_splitter();
        } else if (choice == 3) {
            break;
        } else if (choice == 4) {
            detectfraudforuser(currentuser, all, n);
            printf("fraud_%s.txt generated\n", currentuser);
        } else if (choice == 5) {
            makereportforuser(currentuser, all, n);
            printf("report_%s.csv generated\n", currentuser);
        } else {
            printf("invalid choice\n");
        }
    }

    freetransactions(all, n);
    freeauth(&a);
    return 0;
}
