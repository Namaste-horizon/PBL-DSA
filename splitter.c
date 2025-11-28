#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "splitter.h"

static void clearscanline(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static int ensure_member_cap(splitter *s) {
    if (s->membercount < s->membercap) return 1;
    int newcap = s->membercap == 0 ? 4 : s->membercap * 2;
    splitmember *nm = (splitmember *)realloc(s->members, sizeof(splitmember) * newcap);
    if (!nm) return 0;
    s->members = nm;
    s->membercap = newcap;
    return 1;
}

static int ensure_expense_cap(splitter *s) {
    if (s->expensecount < s->expensecap) return 1;
    int newcap = s->expensecap == 0 ? 8 : s->expensecap * 2;
    expense_entry *ne = (expense_entry *)realloc(s->expenses, sizeof(expense_entry) * newcap);
    if (!ne) return 0;
    s->expenses = ne;
    s->expensecap = newcap;
    return 1;
}

static int ensure_settlement_cap(splitter *s) {
    if (s->settlementcount < s->settlementcap) return 1;
    int newcap = s->settlementcap == 0 ? 8 : s->settlementcap * 2;
    settlement_entry *ns = (settlement_entry *)realloc(s->settlements, sizeof(settlement_entry) * newcap);
    if (!ns) return 0;
    s->settlements = ns;
    s->settlementcap = newcap;
    return 1;
}

void initsplitter(splitter *s) {
    s->members = NULL;
    s->membercount = 0;
    s->membercap = 0;
    s->expenses = NULL;
    s->expensecount = 0;
    s->expensecap = 0;
    s->settlements = NULL;
    s->settlementcount = 0;
    s->settlementcap = 0;
}

static void free_expense_entry(expense_entry *e) {
    free(e->shares);
    e->shares = NULL;
}

void freesplitter(splitter *s) {
    for (int i = 0; i < s->expensecount; ++i) free_expense_entry(&s->expenses[i]);
    free(s->expenses);
    free(s->members);
    free(s->settlements);
    s->members = NULL;
    s->expenses = NULL;
    s->settlements = NULL;
    s->membercount = s->expensecount = s->settlementcount = 0;
    s->membercap = s->expensecap = s->settlementcap = 0;
}

static void list_members(splitter *s) {
    if (s->membercount == 0) {
        printf("no members yet\n");
        return;
    }
    printf("\nmembers\n");
    for (int i = 0; i < s->membercount; ++i) {
        printf("%d. %s (balance %.2f)\n", i + 1, s->members[i].name, s->members[i].balance);
    }
}

static int find_member(splitter *s, const char *name) {
    for (int i = 0; i < s->membercount; ++i) {
        if (strcmp(s->members[i].name, name) == 0) return i;
    }
    return -1;
}

static void add_member(splitter *s) {
    if (!ensure_member_cap(s)) {
        printf("memory error\n");
        return;
    }
    char name[50];
    printf("enter member name: ");
    scanf("%49s", name);
    if (find_member(s, name) != -1) {
        printf("member already exists\n");
        return;
    }
    strcpy(s->members[s->membercount].name, name);
    s->members[s->membercount].balance = 0.0;
    s->membercount++;
    printf("added %s\n", name);
}

static void remove_member(splitter *s) {
    if (s->membercount == 0) {
        printf("no members to remove\n");
        return;
    }
    list_members(s);
    int idx;
    printf("enter member number to remove: ");
    if (scanf("%d", &idx) != 1) {
        clearscanline();
        return;
    }
    if (idx < 1 || idx > s->membercount) {
        printf("invalid member\n");
        return;
    }
    idx -= 1;
    if (fabs(s->members[idx].balance) > 0.01) {
        printf("cannot remove member with unsettled balance\n");
        return;
    }
    printf("removed %s\n", s->members[idx].name);
    for (int i = idx; i < s->membercount - 1; ++i) s->members[i] = s->members[i + 1];
    s->membercount--;
}

static int select_member(splitter *s, const char *prompt) {
    if (s->membercount == 0) {
        printf("no members available\n");
        return -1;
    }
    list_members(s);
    int idx;
    printf("%s", prompt);
    if (scanf("%d", &idx) != 1) {
        clearscanline();
        return -1;
    }
    if (idx < 1 || idx > s->membercount) {
        printf("invalid choice\n");
        return -1;
    }
    return idx - 1;
}

static int collect_participants(splitter *s, int *indexes, int maxslots, int *count) {
    if (s->membercount == 0) {
        printf("add members first\n");
        return 0;
    }
    int num;
    printf("how many participants (1-%d): ", s->membercount);
    if (scanf("%d", &num) != 1) {
        clearscanline();
        return 0;
    }
    if (num < 1 || num > s->membercount || num > maxslots) {
        printf("invalid count\n");
        return 0;
    }
    for (int i = 0; i < num; ++i) {
        int idx = select_member(s, "select participant #: ");
        if (idx == -1) return 0;
        int duplicate = 0;
        for (int j = 0; j < i; ++j) {
            if (indexes[j] == idx) duplicate = 1;
        }
        if (duplicate) {
            printf("duplicate member\n");
            return 0;
        }
        indexes[i] = idx;
    }
    *count = num;
    return 1;
}

static void record_expense(splitter *s) {
    if (s->membercount == 0) {
        printf("add members before recording expenses\n");
        return;
    }
    int payer_idx = select_member(s, "select payer #: ");
    if (payer_idx == -1) return;
    double amount;
    printf("enter amount: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) {
        clearscanline();
        printf("invalid amount\n");
        return;
    }
    char date[11];
    printf("enter date (yyyy-mm-dd): ");
    scanf("%10s", date);
    clearscanline();
    char desc[80];
    printf("enter description: ");
    fgets(desc, sizeof(desc), stdin);
    int len = (int)strlen(desc);
    if (len > 0 && desc[len - 1] == '\n') desc[len - 1] = '\0';
    int mode;
    printf("split type (1 equal 2 custom percentages): ");
    if (scanf("%d", &mode) != 1) {
        clearscanline();
        return;
    }
    int *participants = (int *)malloc(sizeof(int) * s->membercount);
    if (!participants) return;
    int partcount = 0;
    if (!collect_participants(s, participants, s->membercount, &partcount)) {
        free(participants);
        return;
    }
    double *shares = (double *)calloc(partcount, sizeof(double));
    if (!shares) {
        free(participants);
        return;
    }
    if (mode == 1) {
        double share = amount / partcount;
        for (int i = 0; i < partcount; ++i) shares[i] = share;
    } else if (mode == 2) {
        double totalpct = 0.0;
        for (int i = 0; i < partcount; ++i) {
            double pct;
            printf("percent for %s: ", s->members[participants[i]].name);
            if (scanf("%lf", &pct) != 1 || pct < 0) {
                clearscanline();
                free(shares);
                return;
            }
            totalpct += pct;
            shares[i] = pct;
        }
        if (totalpct <= 0) {
            printf("invalid percentages\n");
            free(shares);
            return;
        }
        for (int i = 0; i < partcount; ++i) shares[i] = amount * (shares[i] / totalpct);
    } else {
        printf("unknown split type\n");
        free(shares);
        free(participants);
        return;
    }
    if (!ensure_expense_cap(s)) {
        free(shares);
        free(participants);
        printf("memory error\n");
        return;
    }
    expense_entry *e = &s->expenses[s->expensecount];
    strcpy(e->date, date);
    strncpy(e->desc, desc, sizeof(e->desc) - 1);
    e->desc[sizeof(e->desc) - 1] = '\0';
    e->amount = amount;
    strcpy(e->payer, s->members[payer_idx].name);
    e->sharecount = partcount;
    e->shares = (expense_share *)calloc(partcount, sizeof(expense_share));
    if (!e->shares) {
        free(shares);
        return;
    }
    for (int i = 0; i < partcount; ++i) {
        strcpy(e->shares[i].name, s->members[participants[i]].name);
        e->shares[i].amount = shares[i];
        s->members[participants[i]].balance -= shares[i];
    }
    s->members[payer_idx].balance += amount;
    s->expensecount++;
    free(shares);
    free(participants);
    printf("expense recorded\n");
}

static void show_balances(splitter *s) {
    if (s->membercount == 0) {
        printf("no members\n");
        return;
    }
    printf("\ncurrent balances\n");
    for (int i = 0; i < s->membercount; ++i) {
        printf("%-15s : %.2f\n", s->members[i].name, s->members[i].balance);
    }
}

static void history(splitter *s) {
    if (s->expensecount == 0) {
        printf("no expenses yet\n");
        return;
    }
    printf("\nexpense history\n");
    for (int i = 0; i < s->expensecount; ++i) {
        printf("%s %s paid %.2f for %s\n",
               s->expenses[i].date, s->expenses[i].payer,
               s->expenses[i].amount, s->expenses[i].desc);
        for (int j = 0; j < s->expenses[i].sharecount; ++j) {
            printf("  -> %s owes %.2f\n",
                   s->expenses[i].shares[j].name,
                   s->expenses[i].shares[j].amount);
        }
    }
}

static void record_settlement(splitter *s) {
    if (s->membercount < 2) {
        printf("need at least two members\n");
        return;
    }
    int payer = select_member(s, "payer #: ");
    if (payer == -1) return;
    int receiver = select_member(s, "receiver #: ");
    if (receiver == -1) return;
    if (payer == receiver) {
        printf("cannot settle with same member\n");
        return;
    }
    double amount;
    printf("amount: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) {
        clearscanline();
        return;
    }
    char date[11];
    printf("date (yyyy-mm-dd): ");
    scanf("%10s", date);
    s->members[payer].balance += amount;
    s->members[receiver].balance -= amount;
    if (!ensure_settlement_cap(s)) {
        printf("could not store settlement\n");
        return;
    }
    settlement_entry *se = &s->settlements[s->settlementcount++];
    strcpy(se->date, date);
    strcpy(se->from, s->members[payer].name);
    strcpy(se->to, s->members[receiver].name);
    se->amount = amount;
    printf("settlement recorded\n");
}

static void settlement_history(splitter *s) {
    if (s->settlementcount == 0) {
        printf("no settlements\n");
        return;
    }
    printf("\nsettlement history\n");
    for (int i = 0; i < s->settlementcount; ++i) {
        printf("%s %s paid %s %.2f\n",
               s->settlements[i].date,
               s->settlements[i].from,
               s->settlements[i].to,
               s->settlements[i].amount);
    }
}

static void suggestion(splitter *s) {
    if (s->membercount == 0) {
        printf("no members\n");
        return;
    }
    double *pos = (double *)calloc(s->membercount, sizeof(double));
    double *neg = (double *)calloc(s->membercount, sizeof(double));
    if (!pos || !neg) {
        free(pos); free(neg);
        return;
    }
    int poscount = 0, negcount = 0;
    int *posidx = (int *)calloc(s->membercount, sizeof(int));
    int *negidx = (int *)calloc(s->membercount, sizeof(int));
    if (!posidx || !negidx) {
        free(pos); free(neg); free(posidx); free(negidx);
        return;
    }
    for (int i = 0; i < s->membercount; ++i) {
        if (s->members[i].balance > 0.01) {
            pos[poscount] = s->members[i].balance;
            posidx[poscount++] = i;
        } else if (s->members[i].balance < -0.01) {
            neg[negcount] = -s->members[i].balance;
            negidx[negcount++] = i;
        }
    }
    if (poscount == 0 || negcount == 0) {
        printf("balances already settled\n");
        free(pos); free(neg); free(posidx); free(negidx);
        return;
    }
    printf("\nsuggested settlements\n");
    int ip = 0, in = 0;
    while (ip < poscount && in < negcount) {
        double pay = pos[ip] < neg[in] ? pos[ip] : neg[in];
        printf("%s should receive %.2f from %s\n",
               s->members[posidx[ip]].name,
               pay,
               s->members[negidx[in]].name);
        pos[ip] -= pay;
        neg[in] -= pay;
        if (pos[ip] <= 0.01) ip++;
        if (neg[in] <= 0.01) in++;
    }
    free(pos); free(neg); free(posidx); free(negidx);
}

void splittermenu(splitter *s) {
    while (1) {
        int choice;
        printf("\nexpense splitter menu\n");
        printf("1 add member\n");
        printf("2 remove member\n");
        printf("3 record expense\n");
        printf("4 view balances\n");
        printf("5 expense history\n");
        printf("6 settle debt\n");
        printf("7 settlement history\n");
        printf("8 suggestions\n");
        printf("9 back\n");
        printf("enter choice: ");
        if (scanf("%d", &choice) != 1) {
            clearscanline();
            continue;
        }
        if (choice == 1) {
            add_member(s);
        } else if (choice == 2) {
            remove_member(s);
        } else if (choice == 3) {
            record_expense(s);
        } else if (choice == 4) {
            show_balances(s);
        } else if (choice == 5) {
            history(s);
        } else if (choice == 6) {
            record_settlement(s);
        } else if (choice == 7) {
            settlement_history(s);
        } else if (choice == 8) {
            suggestion(s);
        } else if (choice == 9) {
            break;
        } else {
            printf("invalid choice\n");
        }
    }
}

