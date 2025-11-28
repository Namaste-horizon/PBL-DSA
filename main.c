#include "login.h"
#include "transaction.h"
#include <stdio.h>
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
    while (1) {
        int choice;
        printf("\nmain hub\n");
        printf("1 finance tracker\n");
        printf("2 expense splitter\n");
        printf("3 exit\n");
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
        } else {
            printf("invalid choice\n");
        }
    }
    freeauth(&a);
    return 0;
}