#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct usernode {
    char name[100];
    char pass[100];
    int qindex;
    char answer[100];
    struct usernode *next;
} usernode;

typedef struct auth {
    usernode *start;
} auth;

static void encrypttext(char *p) {
    for (int i = 0; p[i] != '\0'; i++) p[i] ^= 128;
}

static void decrypttext(char *p) {
    encrypttext(p);
}

static void adduser(auth *a, char *n, char *p, int q, char *ans) {
    usernode *t = (usernode *)malloc(sizeof(usernode));
    if (!t) return;
    strcpy(t->name, n);
    strcpy(t->pass, p);
    t->qindex = q;
    strcpy(t->answer, ans);
    t->next = a->start;
    a->start = t;
}

static void loadusers(auth *a, const char *fname) {
    FILE *f = fopen(fname, "r");
    if (!f) return;
    char n[100], p[100], ans[100];
    int q;
    while (fscanf(f, "%99s %99s %d %99s", n, p, &q, ans) != EOF) {
        decrypttext(p);
        adduser(a, n, p, q, ans);
    }
    fclose(f);
}

static void saveusers(auth *a, const char *fname) {
    FILE *f = fopen(fname, "w");
    if (!f) return;
    usernode *t = a->start;
    while (t) {
        char temp[100];
        strcpy(temp, t->pass);
        encrypttext(temp);
        fprintf(f, "%s %s %d %s\n", t->name, temp, t->qindex, t->answer);
        t = t->next;
    }
    fclose(f);
}

static void inputpass(char *p) {
    scanf("%99s", p);
}

static void createuser(auth *a) {
    char n[100], p[100], ans[100];
    int q;
    printf("enter new username: ");
    scanf("%99s", n);
    usernode *t = a->start;
    while (t) {
        if (strcmp(t->name, n) == 0) {
            printf("username already exists\n");
            return;
        }
        t = t->next;
    }
    printf("enter new password: ");
    inputpass(p);
    printf("choose a security question:\n");
    printf("1. What is the name of your first pet?\n");
    printf("2. What is the first dish you learned to cook?\n");
    printf("3. What is your favorite book?\n");
    printf("4. What is the first word you said (except mother and father)?\n");
    printf("5. What city were you born in?\n");
    scanf("%d", &q);
    printf("enter answer: ");
    scanf("%99s", ans);
    adduser(a, n, p, q, ans);
    saveusers(a, "data.txt");
    printf("account created successfully\n");
}

static int signin(auth *a) {
    char n[100], p[100];
    printf("enter username: ");
    scanf("%99s", n);
    printf("enter password: ");
    inputpass(p);
    usernode *t = a->start;
    while (t) {
        if (strcmp(t->name, n) == 0 && strcmp(t->pass, p) == 0) {
            printf("login successful\n");
            return 1;
        }
        t = t->next;
    }
    printf("invalid username or password\n");
    return 0;
}

static void resetpass(auth *a) {
    char n[100], p[100], ans[100];
    printf("enter username to reset password: ");
    scanf("%99s", n);
    usernode *t = a->start;
    while (t) {
        if (strcmp(t->name, n) == 0) {
            printf("answer your security question:\n");
            if (t->qindex == 1) printf("What is the name of your first pet? ");
            if (t->qindex == 2) printf("What is the first dish you learned to cook? ");
            if (t->qindex == 3) printf("What is your favorite book? ");
            if (t->qindex == 4) printf("What is the first word you said (except mother and father)? ");
            if (t->qindex == 5) printf("What city were you born in? ");
            scanf("%99s", ans);
            if (strcmp(ans, t->answer) == 0) {
                printf("enter new password: ");
                inputpass(p);
                strcpy(t->pass, p);
                saveusers(a, "data.txt");
                printf("password reset successfully\n");
            } else {
                printf("verification failed\n");
            }
            return;
        }
        t = t->next;
    }
    printf("user not found\n");
}

void freeauth(auth *a) {
    usernode *t = a->start;
    while (t) {
        usernode *next = t->next;
        free(t);
        t = next;
    }
    a->start = NULL;
}

int loginmenu(auth *a) {
    loadusers(a, "data.txt");
    while (1) {
        int ch;
        printf("\n1. create new account\n");
        printf("2. login\n");
        printf("3. forget password\n");
        printf("4. exit\n");
        printf("enter choice: ");
        if (scanf("%d", &ch) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        if (ch == 1) {
            createuser(a);
        } else if (ch == 2) {
            if (signin(a)) return 1;
        } else if (ch == 3) {
            resetpass(a);
        } else if (ch == 4) {
            saveusers(a, "data.txt");
            return 0;
        } else {
            printf("invalid choice\n");
        }
    }
}