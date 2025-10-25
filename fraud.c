#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fraud.h"

static void month_from_date(const char *date, char *out) {
    strncpy(out, date, 7);
    out[7] = '\0';
}

void detect_fraud(finance *f) {
    int n = f->count;
    if (n <= 0) {
        printf("no data\n");
        return;
    }

    int flagged_any = 0;

    for (int i = 0; i < n; ++i) {
        char m[8];
        month_from_date(f->list[i].date, m);
        double sum = 0.0;
        int cnt = 0;
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            char mj[8];
            month_from_date(f->list[j].date, mj);
            if (strncmp(m, mj, 8) == 0 &&
                strcmp(f->list[i].category, f->list[j].category) == 0) {
                sum += f->list[j].amount;
                cnt++;
            }
        }
        if (cnt > 0) {
            double avg = sum / cnt;
            if (avg > 0.0 && f->list[i].amount >= 3.0 * avg) {
                if (!flagged_any) {
                    printf("\nfraud warnings\n");
                    flagged_any = 1;
                }
                printf("[high-amount] %s %s %s %.2f (>= 3x monthly avg %.2f)\n",
                       f->list[i].date, f->list[i].category, f->list[i].text,
                       f->list[i].amount, avg);
            }
        }
    }

    int *used = (int*)calloc(n, sizeof(int));
    if (!used) return;

    for (int i = 0; i < n; ++i) {
        if (used[i]) continue;
        char mi[8];
        month_from_date(f->list[i].date, mi);
        int count_same = 1;
        int idxs_cap = 8;
        int *idxs = (int*)malloc(sizeof(int) * idxs_cap);
        if (!idxs) { free(used); return; }
        idxs[0] = i;
        for (int j = i + 1; j < n; ++j) {
            if (used[j]) continue;
            char mj[8];
            month_from_date(f->list[j].date, mj);
            if (strncmp(mi, mj, 8) == 0 &&
                strcmp(f->list[i].date, f->list[j].date) == 0 &&
                strcmp(f->list[i].category, f->list[j].category) == 0 &&
                strcmp(f->list[i].text, f->list[j].text) == 0 &&
                f->list[i].amount == f->list[j].amount) {
                if (count_same == idxs_cap) {
                    idxs_cap *= 2;
                    int *tmp = (int*)realloc(idxs, sizeof(int) * idxs_cap);
                    if (!tmp) { free(idxs); free(used); return; }
                    idxs = tmp;
                }
                idxs[count_same++] = j;
            }
        }
        if (count_same >= 3) {
            if (!flagged_any) {
                printf("\nfraud warnings\n");
                flagged_any = 1;
            }
            printf("[duplicate×%d] %s %s %s %.2f\n",
                   count_same,
                   f->list[i].date, f->list[i].category, f->list[i].text, f->list[i].amount);
            for (int k = 0; k < count_same; ++k) used[idxs[k]] = 1;
        }
        free(idxs);
    }

    free(used);

    if (!flagged_any) {
        printf("\nno fraud patterns detected\n");
    }
}