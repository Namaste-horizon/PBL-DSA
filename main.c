#include "login.h"
#include "transaction.h"
#include<stdio.h>
int main() {
    auth a;
    a.start = NULL;
    int ok = loginmenu(&a);
    if (ok != 1) {
        freeauth(&a);
        return 0;
    }
    finance f;
    initfinance(&f, 50);
    menufinance(&f);
    freefinance(&f);
    freeauth(&a);
    return 0;
}