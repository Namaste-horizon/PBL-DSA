#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "fraud.h"

typedef struct daystat {
    char date[11];
    double sum;
    int count;
    int smallcount;
} daystat;

typedef struct daycatstat {
    char date[11];
    char category[30];
    double sum;
    int count;
    int near_threshold;
} daycatstat;

typedef struct catstat {
    char category[30];
    double weekday_sum;
    int weekday_count;
    double total_sum;
    int total_count;
} catstat;

typedef struct locstat {
    char merchant[50];
    char location[50];
    char date[11];
    int seen;
} locstat;

static void month_from_date(const char *date, char *out) {
    strncpy(out, date, 7);
    out[7] = '\0';
}

static int parsedate(const char *s, int *y, int *m, int *d) {
    if (sscanf(s, "%d-%d-%d", y, m, d) != 3) return 0;
    return 1;
}

static int day_number(int y, int m, int d) {
    int a = (14 - m) / 12;
    y += 4800 - a;
    m += 12 * a - 3;
    return d + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
}

static int daydiff(const char *a, const char *b) {
    int ya, ma, da, yb, mb, db;
    if (!parsedate(a, &ya, &ma, &da) || !parsedate(b, &yb, &mb, &db)) return 0;
    int oa = day_number(ya, ma, da);
    int ob = day_number(yb, mb, db);
    int diff = oa - ob;
    if (diff < 0) diff = -diff;
    return diff;
}

static int weekday_index(const char *date) {
    int y, m, d;
    if (!parsedate(date, &y, &m, &d)) return 1;
    if (m < 3) {
        m += 12;
        y -= 1;
    }
    int K = y % 100;
    int J = y / 100;
    int h = (d + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    int dow = ((h + 6) % 7); /* 0 = Sunday */
    return dow;
}

static int is_weekend(const char *date) {
    int dow = weekday_index(date);
    return dow == 0 || dow == 6;
}

static int extract_location(const char *text, char *out, int outsz) {
    const char *mark = strrchr(text, '@');
    if (!mark || *(mark + 1) == '\0') return 0;
    strncpy(out, mark + 1, outsz - 1);
    out[outsz - 1] = '\0';
    return 1;
}

static void merchant_base(const char *text, char *out, int outsz) {
    const char *mark = strrchr(text, '@');
    if (!mark) {
        strncpy(out, text, outsz - 1);
    } else {
        int len = (int)(mark - text);
        if (len >= outsz) len = outsz - 1;
        strncpy(out, text, len);
        out[len] = '\0';
    }
    out[outsz - 1] = '\0';
}

static int extract_hour(const char *text) {
    int len = (int)strlen(text);
    for (int i = 0; i < len - 4; ++i) {
        if (isdigit((unsigned char)text[i]) &&
            isdigit((unsigned char)text[i + 1]) &&
            text[i + 2] == ':' &&
            isdigit((unsigned char)text[i + 3]) &&
            isdigit((unsigned char)text[i + 4])) {
            int hour = (text[i] - '0') * 10 + (text[i + 1] - '0');
            if (hour >= 0 && hour <= 23) return hour;
        }
    }
    return -1;
}

static void ensure_header(int *flagged_any) {
    if (!*flagged_any) {
        printf("\nfraud warnings\n");
        *flagged_any = 1;
    }
}

static int find_day(daystat *days, int daycount, const char *date) {
    for (int i = 0; i < daycount; ++i) {
        if (strcmp(days[i].date, date) == 0) return i;
    }
    return -1;
}

static int find_daycat(daycatstat *dc, int count, const char *date, const char *cat) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(dc[i].date, date) == 0 && strcmp(dc[i].category, cat) == 0) return i;
    }
    return -1;
}

static int find_cat(catstat *cats, int count, const char *cat) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(cats[i].category, cat) == 0) return i;
    }
    return -1;
}

static int find_loc(locstat *locs, int count, const char *merchant) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(locs[i].merchant, merchant) == 0) return i;
    }
    return -1;
}

static int last_gap(finance *f, int idx) {
    const char *cur = f->list[idx].date;
    int best = -1;
    for (int j = 0; j < f->count; ++j) {
        if (j == idx) continue;
        if (strcmp(f->list[j].date, cur) < 0) {
            int diff = daydiff(cur, f->list[j].date);
            if (diff > 0 && (best == -1 || diff < best)) best = diff;
        }
    }
    return best;
}

void detect_fraud(finance *f) {
    int n = f->count;
    if (n <= 0) {
        printf("no data\n");
        return;
    }

    double total = 0.0;
    for (int i = 0; i < n; ++i) total += f->list[i].amount;
    double avg_all = total > 0 ? total / n : 0.0;

    daystat *days = (daystat *)calloc(n, sizeof(daystat));
    daycatstat *daycats = (daycatstat *)calloc(n, sizeof(daycatstat));
    catstat *cats = (catstat *)calloc(n, sizeof(catstat));
    locstat *locs = (locstat *)calloc(n, sizeof(locstat));
    if (!days || !daycats || !cats || !locs) {
        free(days); free(daycats); free(cats); free(locs);
        printf("insufficient memory for analysis\n");
        return;
    }
    int daycount = 0, daycatcount = 0, catcount = 0, loccount = 0;

    for (int i = 0; i < n; ++i) {
        record *r = &f->list[i];
        int didx = find_day(days, daycount, r->date);
        if (didx == -1) {
            didx = daycount++;
            strcpy(days[didx].date, r->date);
            days[didx].sum = 0;
            days[didx].count = 0;
            days[didx].smallcount = 0;
        }
        days[didx].sum += r->amount;
        days[didx].count += 1;
        if (r->amount < 20.0) days[didx].smallcount += 1;

        int dcidx = find_daycat(daycats, daycatcount, r->date, r->category);
        if (dcidx == -1) {
            dcidx = daycatcount++;
            strcpy(daycats[dcidx].date, r->date);
            strcpy(daycats[dcidx].category, r->category);
            daycats[dcidx].sum = 0;
            daycats[dcidx].count = 0;
            daycats[dcidx].near_threshold = 0;
        }
        daycats[dcidx].sum += r->amount;
        daycats[dcidx].count += 1;
        if (r->amount >= 900.0 && r->amount < 1000.0) daycats[dcidx].near_threshold += 1;

        int cidx = find_cat(cats, catcount, r->category);
        if (cidx == -1) {
            cidx = catcount++;
            strcpy(cats[cidx].category, r->category);
            cats[cidx].weekday_sum = 0;
            cats[cidx].weekday_count = 0;
            cats[cidx].total_sum = 0;
            cats[cidx].total_count = 0;
        }
        cats[cidx].total_sum += r->amount;
        cats[cidx].total_count += 1;
        if (is_weekend(r->date)) {
            cats[cidx].weekday_sum += 0;
        } else {
            cats[cidx].weekday_sum += r->amount;
            cats[cidx].weekday_count += 1;
        }

        char merchant[50];
        merchant_base(r->text, merchant, sizeof(merchant));
        if (merchant[0] != '\0') {
            int lidx = find_loc(locs, loccount, merchant);
            if (lidx == -1) {
                lidx = loccount++;
                strcpy(locs[lidx].merchant, merchant);
                locs[lidx].location[0] = '\0';
                locs[lidx].date[0] = '\0';
                locs[lidx].seen = 0;
            }
        }
    }

    int flagged_any = 0;

    for (int i = 0; i < n; ++i) {
        record *r = &f->list[i];
        char m[8];
        month_from_date(r->date, m);
        double sum = 0.0;
        int cnt = 0;
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            char mj[8];
            month_from_date(f->list[j].date, mj);
            if (strncmp(m, mj, 8) == 0 &&
                strcmp(r->category, f->list[j].category) == 0) {
                sum += f->list[j].amount;
                cnt++;
            }
        }
        if (cnt > 0) {
            double avg = sum / cnt;
            if (avg > 0.0 && r->amount >= 3.0 * avg) {
                ensure_header(&flagged_any);
                printf("[high-amount] %s %s %s %.2f (>= 3x monthly avg %.2f)\n",
                       r->date, r->category, r->text, r->amount, avg);
            }
        }
        if (avg_all > 0 && r->amount >= 4.0 * avg_all && r->amount >= 500.0) {
            ensure_header(&flagged_any);
            printf("[global-spike] %s %s %s %.2f (>=4x overall avg %.2f)\n",
                   r->date, r->category, r->text, r->amount, avg_all);
        }
        int gap = last_gap(f, i);
        if (gap >= 45 && r->amount >= 2000.0) {
            ensure_header(&flagged_any);
            printf("[dormant-then-burst] %s %s %s %.2f (gap %d days)\n",
                   r->date, r->category, r->text, r->amount, gap);
        }
        int hour = extract_hour(r->text);
        if (hour != -1 && (hour < 5 || hour > 22) && r->amount >= 300.0) {
            ensure_header(&flagged_any);
            printf("[odd-hour] %s %s %s %.2f (time %02d:xx)\n",
                   r->date, r->category, r->text, r->amount, hour);
        }
        int cidx = find_cat(cats, catcount, r->category);
        if (cidx != -1 && is_weekend(r->date) && cats[cidx].weekday_count > 0) {
            double weekday_avg = cats[cidx].weekday_sum / cats[cidx].weekday_count;
            if (weekday_avg > 0 && r->amount >= weekday_avg * 2.0 && r->amount >= 1000.0) {
                ensure_header(&flagged_any);
                printf("[weekend-spike] %s %s %s %.2f (weekday avg %.2f)\n",
                       r->date, r->category, r->text, r->amount, weekday_avg);
            }
        }
        char merchant[50], location[50];
        merchant_base(r->text, merchant, sizeof(merchant));
        if (merchant[0] != '\0' && extract_location(r->text, location, sizeof(location))) {
            int lidx = find_loc(locs, loccount, merchant);
            if (lidx != -1) {
                if (locs[lidx].seen) {
                    if (strcmp(locs[lidx].location, location) != 0 &&
                        daydiff(r->date, locs[lidx].date) <= 2 &&
                        r->amount >= 500.0) {
                        ensure_header(&flagged_any);
                        printf("[geo-shift] %s %s %s %.2f (%s -> %s)\n",
                               r->date, r->category, r->text, r->amount,
                               locs[lidx].location, location);
                    }
                }
                strcpy(locs[lidx].location, location);
                strcpy(locs[lidx].date, r->date);
                locs[lidx].seen = 1;
            }
        }
    }

    for (int i = 0; i < daycount; ++i) {
        if (days[i].count >= 8 || days[i].sum >= avg_all * 5.0) {
            ensure_header(&flagged_any);
            printf("[velocity-day] %s count %d total %.2f\n",
                   days[i].date, days[i].count, days[i].sum);
        }
        if (days[i].smallcount >= 6) {
            ensure_header(&flagged_any);
            printf("[micro-velocity] %s %d sub-20 transactions\n",
                   days[i].date, days[i].smallcount);
        }
    }

    for (int i = 0; i < daycatcount; ++i) {
        if (daycats[i].count >= 4 && daycats[i].sum >= 2000.0) {
            ensure_header(&flagged_any);
            printf("[burst-category] %s %s count %d sum %.2f\n",
                   daycats[i].date, daycats[i].category,
                   daycats[i].count, daycats[i].sum);
        }
        if (daycats[i].near_threshold >= 3) {
            ensure_header(&flagged_any);
            printf("[threshold-split] %s %s %d payments near 1000\n",
                   daycats[i].date, daycats[i].category,
                   daycats[i].near_threshold);
        }
    }

    for (int i = 0; i < catcount; ++i) {
        if (cats[i].total_count == 1 && cats[i].total_sum >= 1500.0) {
            ensure_header(&flagged_any);
            printf("[new-category] %s first spend %.2f\n",
                   cats[i].category, cats[i].total_sum);
        }
    }

    for (int i = 0; i < n - 2; ++i) {
        record *a = &f->list[i];
        record *b = &f->list[i + 1];
        record *c = &f->list[i + 2];
        if (strcmp(a->category, b->category) == 0 &&
            strcmp(a->category, c->category) == 0 &&
            a->amount < b->amount && b->amount < c->amount &&
            daydiff(c->date, a->date) <= 3) {
            ensure_header(&flagged_any);
            printf("[escalating] %s/%s/%s %s amounts %.2f->%.2f->%.2f\n",
                   a->date, b->date, c->date, a->category,
                   a->amount, b->amount, c->amount);
        }
    }

    int *used = (int*)calloc(n, sizeof(int));
    if (used) {
        for (int i = 0; i < n; ++i) {
            if (used[i]) continue;
            char mi[8];
            month_from_date(f->list[i].date, mi);
            int count_same = 1;
            int idxs_cap = 8;
            int *idxs = (int*)malloc(sizeof(int) * idxs_cap);
            if (!idxs) { free(used); used = NULL; break; }
            idxs[0] = i;
            for (int j = i + 1; j < n; ++j) {
                if (used[j]) continue;
                char mj[8];
                month_from_date(f->list[j].date, mj);
                if (strncmp(mi, mj, 8) == 0 &&
                    strcmp(f->list[i].date, f->list[j].date) == 0 &&
                    strcmp(f->list[i].category, f->list[j].category) == 0 &&
                    strcmp(f->list[i].text, f->list[j].text) == 0 &&
                    fabs(f->list[i].amount - f->list[j].amount) < 0.01) {
                    if (count_same == idxs_cap) {
                        idxs_cap *= 2;
                        int *tmp = (int*)realloc(idxs, sizeof(int) * idxs_cap);
                        if (!tmp) { free(idxs); free(used); used = NULL; break; }
                        idxs = tmp;
                    }
                    idxs[count_same++] = j;
                }
            }
            if (!used) break;
            if (count_same >= 3) {
                ensure_header(&flagged_any);
                printf("[duplicate×%d] %s %s %s %.2f\n",
                       count_same,
                       f->list[i].date, f->list[i].category, f->list[i].text, f->list[i].amount);
                for (int k = 0; k < count_same; ++k) used[idxs[k]] = 1;
            }
            free(idxs);
        }
        free(used);
    }

    free(days);
    free(daycats);
    free(cats);
    free(locs);

    if (!flagged_any) {
        printf("\nno fraud patterns detected\n");
    }
}