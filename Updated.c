/* =========================================================================
   UNIVERSITY STUDENT PORTAL  -  Capstone Project (C Language)  -  v2
   -------------------------------------------------------------------------
   Features : Sign up / Login / Forgot password, Profile + Update Profile,
              Registered Courses + Teachers, LIVE Results (Quiz / Mid / Final),
              Notice Board, Hall Information + Hall Payment, Transport,
              Payment Ledger + Receipts, Academic Result (CGPA)
   Concepts : structures (abstraction), functions, recursion instead of
              loops, switch-case, file handling:
                 students.dat      binary database (one record per student)
                 receipts.txt      text file - every payment receipt
                 security_log.txt  text file - logins, resets, password changes
   Build    : gcc student_portal.c -o portal        (Linux / macOS / MinGW)
   Run      : ./portal                              (portal.exe on Windows)
   ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #include <conio.h>
  #define CLEAR_CMD "cls"
#else
  #include <termios.h>
  #include <unistd.h>
  #define CLEAR_CMD "clear"
#endif

/* ------------------------------ constants ------------------------------ */
#define UNI_NAME        "UNIVERSITY STUDENT PORTAL"
#define DB_FILE         "students.dat"
#define DB_OLD          "students_old.bak"
#define REC_FILE        "receipts.txt"
#define LOG_FILE        "security_log.txt"
#define W               78            /* width of the terminal UI           */
#define ID_LEN          16
#define NAME_LEN        40
#define PHONE_LEN       16
#define EMAIL_LEN       40
#define PASS_LEN        32
#define PASS_MIN        6
#define MAX_ATTEMPTS    3
#define MAX_COURSES     6
#define COURSES_PER_SEM 5
#define MAX_FEES        6
#define MAX_SEM         12
#define ROUTE_COUNT     6
#define NOTICE_COUNT    6
#define SEC_Q_COUNT     5
#define HALL_MONTHS     4
#define HALL_RENT       3000.0f
#define HALL_MESS       2000.0f
#define TRANSPORT_FEE   7500.0f
#define HASH_SEED       5381UL

/* ------------------------------- colours ------------------------------- */
#define C_RESET  "\033[0m"
#define C_BOLD   "\033[1m"
#define C_RED    "\033[31m"
#define C_GREEN  "\033[32m"
#define C_YELLOW "\033[33m"
#define C_CYAN   "\033[36m"

/* ----------------------------- structures ------------------------------ */
typedef struct {                 /* one registered course + its marks       */
    char  code[8];
    char  title[40];
    char  teacher[28];
    float credit;
    int   quiz[3];               /* 3 quizzes x 10 marks                    */
    int   mid;                   /* out of 30                               */
    int   finalExam;             /* out of 40                               */
} Course;

typedef struct {                 /* one line of the payment ledger          */
    char  desc[32];
    float amount;
    float paid;
} Fee;

typedef struct {                 /* one month of hall dues                  */
    char  month[12];
    float rent;
    float mess;
    float paid;
} HallBill;

typedef struct {                 /* hall / residence information            */
    int      resident;           /* 1 = living in hall, 0 = not             */
    char     hall[32];
    char     room[10];
    int      seat;
    char     warden[28];
    HallBill bills[HALL_MONTHS];
} HallInfo;

typedef struct {                 /* result of an earlier semester           */
    int   semester;
    float gpa;
    float credits;
} SemResult;

typedef struct {                 /* the whole student record (file record)  */
    char          id[ID_LEN];
    char          name[NAME_LEN];
    char          phone[PHONE_LEN];
    char          email[EMAIL_LEN];
    unsigned long passHash;      /* password is never stored as plain text  */
    int           secQ;          /* security question number (1..5)         */
    unsigned long secHash;       /* hashed security answer                  */
    int           dept;          /* 1 CSE, 2 SWE, 3 EEE, 4 BBA              */
    int           semester;
    int           route;         /* 0 = no transport                        */
    HallInfo      hall;
    Course        courses[MAX_COURSES];
    int           courseCount;
    Fee           fees[MAX_FEES];
    int           feeCount;
    SemResult     history[MAX_SEM];
    int           historyCount;
} Student;

typedef struct { const char *code, *title; float credit; } CourseTemplate;
typedef struct { const char *date, *title, *body; } Notice;
typedef struct { const char *name, *bus, *pickup, *departs, *returns; } Route;

/* ------------------------------ static data ---------------------------- */
static const CourseTemplate pool[4][8] = {
    { {"CSE111","Programming in C",3},        {"CSE123","Data Structures",3},
      {"CSE221","Algorithms",3},              {"CSE233","Digital Logic Design",3},
      {"CSE313","Operating Systems",3},       {"CSE321","Database Systems",3},
      {"CSE331","Computer Networks",3},       {"CSE411","Artificial Intelligence",3} },
    { {"SWE111","Intro to Software Engineering",3}, {"SWE121","Object Oriented Programming",3},
      {"SWE221","Software Requirements",3},   {"SWE231","Software Design Patterns",3},
      {"SWE311","Software Testing",3},        {"SWE321","Web Engineering",3},
      {"SWE331","Software Project Mgmt",3},   {"SWE411","Software Architecture",3} },
    { {"EEE111","Basic Electrical Circuits",3},{"EEE121","Electronic Devices",3},
      {"EEE211","Digital Electronics",3},     {"EEE221","Signals and Systems",3},
      {"EEE311","Power Systems",3},           {"EEE321","Control Systems",3},
      {"EEE331","Communication Engineering",3},{"EEE411","VLSI Design",3} },
    { {"BUS111","Principles of Management",3},{"BUS121","Financial Accounting",3},
      {"BUS211","Marketing Management",3},   {"BUS221","Business Statistics",3},
      {"BUS311","Human Resource Mgmt",3},    {"BUS321","Corporate Finance",3},
      {"BUS331","Operations Management",3},  {"BUS411","Strategic Management",3} }
};

static const char *teachers[8] = {
    "Dr. Rahim Uddin",  "Ms. Nusrat Jahan",  "Mr. Tanvir Ahmed",  "Dr. Farhana Islam",
    "Mr. Kamal Hossain","Ms. Sadia Rahman",  "Dr. Mahmudul Hasan","Mr. Arif Chowdhury"
};

static const char *halls[3]  = { "Residential Hall A", "Residential Hall B", "Residential Hall C" };
static const char *monthNames[HALL_MONTHS] = { "September", "October", "November", "December" };

static const Route routes[ROUTE_COUNT] = {
    {"No transport",       "-",       "-",                "-",       "-"},
    {"Uttara - Campus",    "BUS-101", "Uttara Sector 10", "6:45 AM", "4:30 PM"},
    {"Mirpur - Campus",    "BUS-204", "Mirpur 10 Circle", "7:00 AM", "4:30 PM"},
    {"Dhanmondi - Campus", "BUS-307", "Dhanmondi 27",     "6:30 AM", "5:00 PM"},
    {"Savar - Campus",     "BUS-412", "Savar Bus Stand",  "7:30 AM", "4:00 PM"},
    {"Gabtoli - Campus",   "BUS-515", "Gabtoli Terminal", "7:15 AM", "4:30 PM"}
};

static const Notice notices[NOTICE_COUNT] = {
    {"2026-10-20", "Final Exam Schedule Published",
     "Final exams begin on 15 Dec. Check your department board for the routine."},
    {"2026-10-18", "Last Date of Tuition Payment",
     "Clear all dues by 30 Oct to avoid a late fee and exam permit hold."},
    {"2026-10-15", "Mid-term Result Published",
     "Mid-term marks are now visible in Live Results. Report errors within 3 days."},
    {"2026-10-12", "Hall Seat Renewal",
     "Hall residents must clear monthly hall dues before the 10th of each month."},
    {"2026-10-08", "Programming Contest",
     "Inter-department programming contest on 5 Nov. Register at the CSE lab."},
    {"2026-10-01", "Winter Break Notice",
     "Campus will be closed from 24 Dec to 2 Jan for the winter break."}
};

static unsigned long seedState = 1;     /* deterministic demo-data generator */
static int           receiptSeq = 0;

/* ------------------------- forward declarations ------------------------ */
static void mainMenu(void);
static void dashboard(Student *s, int idx);
static void loginAttempt(int left);
static int  forgotPassword(void);

/* ======================================================================= */
/*                           SMALL UTILITY FUNCTIONS                       */
/* ======================================================================= */

/* djb2 hash, written recursively (a loop over the string, without a loop) */
static unsigned long hashStr(const char *s, unsigned long h)
{
    if (*s == '\0') return h;
    return hashStr(s + 1, h * 33UL + (unsigned char)*s);
}

/* simple repeatable pseudo-random numbers (same ID -> same demo data)     */
static int nextRand(int lo, int hi)
{
    seedState = seedState * 1103515245UL + 12345UL;
    return lo + (int)((seedState >> 16) % (unsigned long)(hi - lo + 1));
}

static void initConsole(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004);       /* enable ANSI colours */
#endif
}

static void clearScreen(void) { if (system(CLEAR_CMD)) { /* ignore */ } }

/* prints character c, n times (recursion instead of a for-loop)           */
static void printLine(char c, int n)
{
    if (n <= 0) return;
    putchar(c);
    printLine(c, n - 1);
}

static void printRule(char c, const char *color)
{
    printf("%s", color);
    printLine(c, W);
    printf("%s\n", C_RESET);
}

static void printCentered(const char *text, const char *color)
{
    int pad = (W - (int)strlen(text)) / 2;
    if (pad < 0) pad = 0;
    printf("%*s%s%s%s\n", pad, "", color, text, C_RESET);
}

static void printHeader(const char *title)
{
    clearScreen();
    printRule('=', C_CYAN);
    printCentered(UNI_NAME, C_BOLD C_CYAN);
    printRule('-', C_CYAN);
    printCentered(title, C_BOLD C_YELLOW);
    printRule('=', C_CYAN);
    putchar('\n');
}

static void printError(const char *msg)   { printf("%s  [!] %s%s\n",  C_RED,   msg, C_RESET); }
static void printSuccess(const char *msg) { printf("%s  [OK] %s%s\n", C_GREEN, msg, C_RESET); }

static void drainLine(void)               /* throw away rest of input line */
{
    int ch = getchar();
    if (ch == '\n' || ch == EOF) return;
    drainLine();
}

static void pauseScreen(void)
{
    printf("\n%s  Press Enter to continue...%s", C_YELLOW, C_RESET);
    fflush(stdout);
    drainLine();
}

/* reads a line safely; exits cleanly if input ends                        */
static void readLine(const char *prompt, char *buf, int size)
{
    size_t n;
    printf("%s%s%s", C_YELLOW, prompt, C_RESET);
    fflush(stdout);
    if (!fgets(buf, size, stdin)) { printf("\n"); exit(0); }
    n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    else drainLine();                         /* line was longer than buffer */
    buf[strcspn(buf, "\r")] = '\0';
}

static void readNonEmpty(const char *prompt, char *buf, int size)
{
    readLine(prompt, buf, size);
    if (buf[0] == '\0') {
        printError("This field cannot be empty.");
        readNonEmpty(prompt, buf, size);
    }
}

/* reads an integer in [lo, hi]; asks again (recursively) if invalid       */
static int readInt(const char *prompt, int lo, int hi)
{
    char buf[32], msg[64], *end;
    long v;
    readLine(prompt, buf, sizeof buf);
    v = strtol(buf, &end, 10);
    if (end == buf || *end != '\0' || v < lo || v > hi) {
        snprintf(msg, sizeof msg, "Please enter a number between %d and %d.", lo, hi);
        printError(msg);
        return readInt(prompt, lo, hi);
    }
    return (int)v;
}

/* is the text exactly one letter (case-insensitive)? e.g. "L", "f"        */
static int isKey(const char *s, char key)
{
    return s[0] != '\0' && s[1] == '\0' && tolower((unsigned char)s[0]) == key;
}

/* lower-cases the text and removes spaces (for security answers)          */
static void normalize(const char *src, char *dst)
{
    if (*src == '\0') { *dst = '\0'; return; }
    if (*src == ' ')  { normalize(src + 1, dst); return; }
    *dst = (char)tolower((unsigned char)*src);
    normalize(src + 1, dst + 1);
}

/* ---------------------- hidden password input (****) -------------------- */
#ifndef _WIN32
static struct termios oldTerm;
static int rawOn = 0;
#endif

static void echoOff(void)
{
#ifndef _WIN32
    struct termios t;
    if (tcgetattr(STDIN_FILENO, &oldTerm) == 0) {
        t = oldTerm;
        t.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        rawOn = 1;
    }
#endif
}

static void echoOn(void)
{
#ifndef _WIN32
    if (rawOn) { tcsetattr(STDIN_FILENO, TCSANOW, &oldTerm); rawOn = 0; }
#endif
}

static int getKey(void)
{
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}

static void collectPassword(char *buf, int idx, int max)
{
    int ch = getKey();
    if (ch == '\r' || ch == '\n' || ch == EOF) { buf[idx] = '\0'; return; }
#ifdef _WIN32
    if (ch == 0 || ch == 224) { getKey(); collectPassword(buf, idx, max); return; }
#endif
    if (ch == 8 || ch == 127) {                       /* backspace */
        if (idx > 0) { printf("\b \b"); fflush(stdout); idx--; }
    } else if (ch >= 32 && ch < 127 && idx < max - 1) {
        buf[idx++] = (char)ch;
        putchar('*');
        fflush(stdout);
    }
    collectPassword(buf, idx, max);
}

static void readPassword(const char *prompt, char *buf, int size)
{
    printf("%s%s%s", C_YELLOW, prompt, C_RESET);
    fflush(stdout);
    echoOff();
    collectPassword(buf, 0, size);
    echoOn();
    putchar('\n');
}

/* ======================================================================= */
/*                     TEXT FILES: time stamp, log, receipts               */
/* ======================================================================= */
static void timeStamp(char *buf, size_t n)
{
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    if (tmv) strftime(buf, n, "%d %b %Y %H:%M", tmv);
    else     snprintf(buf, n, "unknown time");
}

/* appends one line to security_log.txt                                    */
static void logEvent(const char *id, const char *event)
{
    FILE *f = fopen(LOG_FILE, "a");
    char ts[32];
    if (!f) return;
    timeStamp(ts, sizeof ts);
    fprintf(f, "[%s] %-16s %s\n", ts, event, id);
    fclose(f);
}

/* appends one receipt line to receipts.txt, returns the receipt number    */
static void writeReceipt(const Student *s, const char *desc, int amount, char *rcptNo)
{
    FILE *f;
    char ts[32];
    timeStamp(ts, sizeof ts);
    snprintf(rcptNo, 16, "RC%05lu%d", (unsigned long)time(NULL) % 100000UL, receiptSeq++ % 10);
    f = fopen(REC_FILE, "a");
    if (f) {
        fprintf(f, "RCPT|%s|%s|%s|%d|%s\n", rcptNo, s->id, desc, amount, ts);
        fclose(f);
    }
}

static void showReceiptCard(const Student *s, const char *rcptNo, const char *desc, int amount)
{
    char ts[32], amt[24];
    timeStamp(ts, sizeof ts);
    snprintf(amt, sizeof amt, "%d Tk", amount);
    printf("\n  %s+--------------------------------------------+%s\n", C_GREEN, C_RESET);
    printf("  %s|%s %-42s %s|%s\n", C_GREEN, C_BOLD, "PAYMENT RECEIPT", C_GREEN, C_RESET);
    printf("  %s+--------------------------------------------+%s\n", C_GREEN, C_RESET);
    printf("  %s|%s Receipt No : %-30s%s|%s\n", C_GREEN, C_RESET, rcptNo, C_GREEN, C_RESET);
    printf("  %s|%s Student ID : %-30s%s|%s\n", C_GREEN, C_RESET, s->id,  C_GREEN, C_RESET);
    printf("  %s|%s For        : %-30s%s|%s\n", C_GREEN, C_RESET, desc,    C_GREEN, C_RESET);
    printf("  %s|%s Amount     : %-30s%s|%s\n", C_GREEN, C_RESET, amt,     C_GREEN, C_RESET);
    printf("  %s|%s Date       : %-30s%s|%s\n", C_GREEN, C_RESET, ts,      C_GREEN, C_RESET);
    printf("  %s+--------------------------------------------+%s\n", C_GREEN, C_RESET);
}

/* recursively reads receipts.txt, prints this student's lines, returns count */
static int printReceiptLines(FILE *f, const char *id)
{
    char line[200], no[16], rid[ID_LEN], desc[32], when[32];
    int amt;
    if (!fgets(line, sizeof line, f)) return 0;
    if (sscanf(line, "RCPT|%15[^|]|%15[^|]|%31[^|]|%d|%31[^\n]", no, rid, desc, &amt, when) == 5
        && strcmp(rid, id) == 0) {
        printf("  %-10s %-24s %8d   %s\n", no, desc, amt, when);
        return 1 + printReceiptLines(f, id);
    }
    return printReceiptLines(f, id);
}

static void showReceipts(const Student *s)
{
    FILE *f;
    int n = 0;
    printHeader("MY PAYMENT RECEIPTS");
    f = fopen(REC_FILE, "r");
    if (f) {
        printf("%s  %-10s %-24s %8s   %s%s\n", C_BOLD, "RECEIPT", "DESCRIPTION", "TAKA", "DATE", C_RESET);
        printRule('-', C_CYAN);
        n = printReceiptLines(f, s->id);
        fclose(f);
    }
    if (n == 0) printf("  No receipts found yet. Receipts appear here after you make a payment.\n");
    else        printf("\n  Total receipts: %d\n", n);
    pauseScreen();
}

/* ======================================================================= */
/*                      GRADING (switch-case based)                        */
/* ======================================================================= */
static int totalMarks(const Course *c)
{
    return c->quiz[0] + c->quiz[1] + c->quiz[2] + c->mid + c->finalExam;
}

static const char *gradeOf(int total)
{
    switch (total / 5) {
        case 20: case 19: case 18: case 17: case 16: return "A+";
        case 15: return "A";
        case 14: return "A-";
        case 13: return "B+";
        case 12: return "B";
        case 11: return "B-";
        case 10: return "C+";
        case 9:  return "C";
        case 8:  return "D";
        default: return "F";
    }
}

static float pointOf(int total)
{
    switch (total / 5) {
        case 20: case 19: case 18: case 17: case 16: return 4.00f;
        case 15: return 3.75f;
        case 14: return 3.50f;
        case 13: return 3.25f;
        case 12: return 3.00f;
        case 11: return 2.75f;
        case 10: return 2.50f;
        case 9:  return 2.25f;
        case 8:  return 2.00f;
        default: return 0.00f;
    }
}

static const char *standingOf(float cgpa)
{
    switch ((int)(cgpa * 100) / 25) {
        case 16:          return "Outstanding";
        case 15:          return "Excellent";
        case 14:          return "Very Good";
        case 13: case 12: return "Good";
        case 11: case 10: return "Satisfactory";
        default:          return "Needs Improvement";
    }
}

static const char *deptName(int d)
{
    switch (d) {
        case 1:  return "CSE - Computer Science & Engineering";
        case 2:  return "SWE - Software Engineering";
        case 3:  return "EEE - Electrical & Electronic Engineering";
        case 4:  return "BBA - Business Administration";
        default: return "Unknown";
    }
}

static const char *questionText(int q)
{
    switch (q) {
        case 1:  return "What was the name of your first school?";
        case 2:  return "What is your favourite teacher's name?";
        case 3:  return "What was your childhood nickname?";
        case 4:  return "In which city were you born?";
        case 5:  return "What is your best school friend's name?";
        default: return "Unknown question";
    }
}

/* recursive sums over the course list / history / ledger / hall bills     */
static float sumCredits(const Student *s, int i)
{
    if (i >= s->courseCount) return 0.0f;
    return s->courses[i].credit + sumCredits(s, i + 1);
}

static float sumPoints(const Student *s, int i)      /* grade point x credit */
{
    if (i >= s->courseCount) return 0.0f;
    return pointOf(totalMarks(&s->courses[i])) * s->courses[i].credit + sumPoints(s, i + 1);
}

static float histCredits(const Student *s, int i)
{
    if (i >= s->historyCount) return 0.0f;
    return s->history[i].credits + histCredits(s, i + 1);
}

static float histPoints(const Student *s, int i)
{
    if (i >= s->historyCount) return 0.0f;
    return s->history[i].gpa * s->history[i].credits + histPoints(s, i + 1);
}

static float sumAmount(const Student *s, int i)
{
    if (i >= s->feeCount) return 0.0f;
    return s->fees[i].amount + sumAmount(s, i + 1);
}

static float sumPaid(const Student *s, int i)
{
    if (i >= s->feeCount) return 0.0f;
    return s->fees[i].paid + sumPaid(s, i + 1);
}

static float hallTotal(const Student *s, int i)
{
    if (i >= HALL_MONTHS) return 0.0f;
    return s->hall.bills[i].rent + s->hall.bills[i].mess + hallTotal(s, i + 1);
}

static float hallPaid(const Student *s, int i)
{
    if (i >= HALL_MONTHS) return 0.0f;
    return s->hall.bills[i].paid + hallPaid(s, i + 1);
}

static float semesterGpa(const Student *s)
{
    float cr = sumCredits(s, 0);
    return cr > 0 ? sumPoints(s, 0) / cr : 0.0f;
}

/* ======================================================================= */
/*                         DATABASE (binary file)                          */
/* ======================================================================= */

/* recursive search through the file; returns record index or -1           */
static int findInFile(FILE *f, const char *id, Student *out, int idx)
{
    if (fread(out, sizeof(Student), 1, f) != 1) return -1;
    if (strcmp(out->id, id) == 0) return idx;
    return findInFile(f, id, out, idx + 1);
}

static int lookupStudent(const char *id, Student *out)
{
    int idx;
    FILE *f = fopen(DB_FILE, "rb");
    if (!f) return -1;
    idx = findInFile(f, id, out, 0);
    fclose(f);
    return idx;
}

static int appendStudent(const Student *s)
{
    FILE *f = fopen(DB_FILE, "ab");
    int ok;
    if (!f) return 0;
    ok = (fwrite(s, sizeof(Student), 1, f) == 1);
    fclose(f);
    return ok;
}

static int saveStudentAt(int idx, const Student *s)
{
    FILE *f = fopen(DB_FILE, "r+b");
    int ok;
    if (!f) return 0;
    fseek(f, (long)idx * (long)sizeof(Student), SEEK_SET);
    ok = (fwrite(s, sizeof(Student), 1, f) == 1);
    fclose(f);
    return ok;
}

/* An old-version students.dat has a different record size. Back it up.    */
static void checkDatabase(void)
{
    long size;
    FILE *f = fopen(DB_FILE, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
    if (size % (long)sizeof(Student) != 0) {
        remove(DB_OLD);
        if (rename(DB_FILE, DB_OLD) == 0) {
            printf("\n%s  An old-format database was found and backed up as %s.%s\n", C_YELLOW, DB_OLD, C_RESET);
            printf("  Please register again to create your account.\n");
            pauseScreen();
        }
    }
}

/* ======================================================================= */
/*          DEMO DATA GENERATION (done once, when a student signs up)      */
/* ======================================================================= */
static float randomPaid(float amount, int minPct, int maxPct)
{
    int pct = nextRand(minPct, maxPct);
    return (float)((int)(amount * (float)pct / 100.0f) / 100 * 100);
}

static void fillCourses(Student *s, int i)
{
    const CourseTemplate *t;
    Course *c;
    if (i >= COURSES_PER_SEM) return;
    t = &pool[s->dept - 1][((s->semester - 1) * 3 + i) % 8];
    c = &s->courses[i];
    snprintf(c->code,    sizeof c->code,    "%s", t->code);
    snprintf(c->title,   sizeof c->title,   "%s", t->title);
    snprintf(c->teacher, sizeof c->teacher, "%s", teachers[nextRand(0, 7)]);
    c->credit    = t->credit;
    c->quiz[0]   = nextRand(5, 10);
    c->quiz[1]   = nextRand(5, 10);
    c->quiz[2]   = nextRand(5, 10);
    c->mid       = nextRand(14, 30);
    c->finalExam = nextRand(18, 40);
    s->courseCount = i + 1;
    fillCourses(s, i + 1);
}

static void fillHistory(Student *s, int sem)
{
    if (sem >= s->semester || sem > MAX_SEM) return;
    s->history[sem - 1].semester = sem;
    s->history[sem - 1].gpa      = (float)nextRand(250, 400) / 100.0f;
    s->history[sem - 1].credits  = 15.0f;
    s->historyCount = sem;
    fillHistory(s, sem + 1);
}

static void addFee(Student *s, const char *desc, float amount, float paid)
{
    Fee *f;
    if (s->feeCount >= MAX_FEES) return;
    f = &s->fees[s->feeCount++];
    snprintf(f->desc, sizeof f->desc, "%s", desc);
    f->amount = amount;
    f->paid   = paid;
}

static int findFee(const Student *s, const char *desc, int i)
{
    if (i >= s->feeCount) return -1;
    if (strcmp(s->fees[i].desc, desc) == 0) return i;
    return findFee(s, desc, i + 1);
}

static void shiftFees(Student *s, int i)             /* close the gap at i */
{
    if (i >= s->feeCount - 1) return;
    s->fees[i] = s->fees[i + 1];
    shiftFees(s, i + 1);
}

static void fillHallBills(Student *s, int i, int demo)
{
    HallBill *b;
    float total = HALL_RENT + HALL_MESS;
    if (i >= HALL_MONTHS) return;
    b = &s->hall.bills[i];
    snprintf(b->month, sizeof b->month, "%s", monthNames[i]);
    b->rent = HALL_RENT;
    b->mess = HALL_MESS;
    if (!demo)        b->paid = 0.0f;
    else if (i == 0)  b->paid = total;
    else              b->paid = randomPaid(total, 0, 100);
    fillHallBills(s, i + 1, demo);
}

/* gives the student a hall seat; demo=1 pre-fills some paid amounts       */
static void assignHall(Student *s, int demo)
{
    seedState = hashStr(s->id, HASH_SEED) + 77UL;
    s->hall.resident = 1;
    snprintf(s->hall.hall,   sizeof s->hall.hall,   "%s", halls[nextRand(0, 2)]);
    snprintf(s->hall.room,   sizeof s->hall.room,   "R-%d", nextRand(101, 420));
    snprintf(s->hall.warden, sizeof s->hall.warden, "%s", teachers[nextRand(0, 7)]);
    s->hall.seat = nextRand(1, 4);
    fillHallBills(s, 0, demo);
}

static void createStudentData(Student *s)
{
    seedState = hashStr(s->id, HASH_SEED);
    fillCourses(s, 0);
    fillHistory(s, 1);

    addFee(s, "Tuition Fee",      45000.0f, randomPaid(45000.0f, 40, 100));
    addFee(s, "Registration Fee",  5000.0f, 5000.0f);
    addFee(s, "Lab / Library Fee", 2500.0f, randomPaid(2500.0f, 0, 100));
    addFee(s, "Exam Fee",          1500.0f, randomPaid(1500.0f, 50, 100));
    if (s->route > 0) addFee(s, "Transport Fee", TRANSPORT_FEE, randomPaid(TRANSPORT_FEE, 0, 100));

    if (s->hall.resident) assignHall(s, 1);
}

/* ======================================================================= */
/*                          INPUT VALIDATION HELPERS                       */
/* ======================================================================= */
static int validId(const char *id)           /* letters, digits and '-' only */
{
    if (*id == '\0') return 1;
    if (!((*id >= '0' && *id <= '9') || (*id >= 'a' && *id <= 'z') ||
          (*id >= 'A' && *id <= 'Z') || *id == '-')) return 0;
    return validId(id + 1);
}

static int digitsOnly(const char *p)         /* digits, '+' allowed         */
{
    if (*p == '\0') return 1;
    if (!isdigit((unsigned char)*p) && *p != '+') return 0;
    return digitsOnly(p + 1);
}

static void readPhone(const char *prompt, char *out)
{
    readNonEmpty(prompt, out, PHONE_LEN);
    if (strlen(out) < 8 || !digitsOnly(out)) {
        printError("Enter a valid phone number (8-15 digits).");
        readPhone(prompt, out);
    }
}

static void readEmail(const char *prompt, char *out)
{
    char *at;
    readNonEmpty(prompt, out, EMAIL_LEN);
    at = strchr(out, '@');
    if (!at || at == out || !strchr(at, '.') || strchr(out, ' ')) {
        printError("Enter a valid email address (e.g. name@mail.com).");
        readEmail(prompt, out);
    }
}

static void printQuestionList(int q)
{
    if (q > SEC_Q_COUNT) return;
    printf("  %d. %s\n", q, questionText(q));
    printQuestionList(q + 1);
}

static void setSecurity(Student *s)
{
    char ans[64], norm[64];
    printf("\n  Choose a security question (used to recover your password):\n\n");
    printQuestionList(1);
    putchar('\n');
    s->secQ = readInt("  Question number (1-5)         : ", 1, SEC_Q_COUNT);
    readNonEmpty("  Your answer                   : ", ans, sizeof ans);
    normalize(ans, norm);
    if (norm[0] == '\0') {
        printError("Answer cannot be blank.");
        setSecurity(s);
        return;
    }
    s->secHash = hashStr(norm, HASH_SEED);
}

static void choosePassword(char *out)
{
    char a[PASS_LEN], b[PASS_LEN];
    readPassword("  Create password (min 6 chars) : ", a, sizeof a);
    if (strlen(a) < PASS_MIN) {
        printError("Password is too short.");
        choosePassword(out);
        return;
    }
    readPassword("  Confirm password              : ", b, sizeof b);
    if (strcmp(a, b) != 0) {
        printError("Passwords do not match. Try again.");
        choosePassword(out);
        return;
    }
    snprintf(out, PASS_LEN, "%s", a);
}

/* ======================================================================= */
/*                          PORTAL SCREENS (features)                      */
/* ======================================================================= */
static void printField(const char *label, const char *value)
{
    printf("  %s%-18s%s : %s\n", C_CYAN, label, C_RESET, value);
}

static void showProfile(const Student *s)
{
    char buf[32];
    printHeader("MY PROFILE");
    printField("Name", s->name);
    printField("Student ID", s->id);
    printField("Phone", s->phone);
    printField("Email", s->email);
    printField("Department", deptName(s->dept));
    snprintf(buf, sizeof buf, "%d", s->semester);
    printField("Current Semester", buf);
    printField("Residence", s->hall.resident ? s->hall.hall : "Non-resident");
    printField("Transport", routes[s->route].name);
    pauseScreen();
}

static void printCourseRows(const Student *s, int i)
{
    const Course *c;
    if (i >= s->courseCount) return;
    c = &s->courses[i];
    printf("  %-8s %-31s %-7.1f %-22s\n", c->code, c->title, c->credit, c->teacher);
    printCourseRows(s, i + 1);
}

static void showCourses(const Student *s)
{
    printHeader("REGISTERED COURSES & TEACHERS");
    printf("%s  %-8s %-31s %-7s %-22s%s\n", C_BOLD, "CODE", "COURSE TITLE", "CREDIT", "COURSE TEACHER", C_RESET);
    printRule('-', C_CYAN);
    printCourseRows(s, 0);
    printRule('-', C_CYAN);
    printf("  Total registered credits : %.1f\n", sumCredits(s, 0));
    pauseScreen();
}

/* ---------------------------- LIVE RESULTS ------------------------------ */
static void printResultRows(const Student *s, int i)
{
    const Course *c;
    int total;
    if (i >= s->courseCount) return;
    c = &s->courses[i];
    total = totalMarks(c);
    printf("  %-8s %3d %3d %3d %5d %6d %6d   %s%-4s%s %5.2f\n",
           c->code, c->quiz[0], c->quiz[1], c->quiz[2], c->mid, c->finalExam, total,
           total < 40 ? C_RED : C_GREEN, gradeOf(total), C_RESET, pointOf(total));
    printResultRows(s, i + 1);
}

static void showResults(const Student *s)
{
    printHeader("FULL RESULT SHEET");
    printf("%s  %-8s %3s %3s %3s %5s %6s %6s   %-4s %5s%s\n", C_BOLD,
           "CODE", "Q1", "Q2", "Q3", "MID", "FINAL", "TOTAL", "GRD", "GP", C_RESET);
    printRule('-', C_CYAN);
    printResultRows(s, 0);
    printRule('-', C_CYAN);
    printf("  Marks distribution : Quiz 3x10 | Mid 30 | Final 40 | Total 100\n");
    printf("  %sSemester GPA : %.2f%s\n", C_BOLD, semesterGpa(s), C_RESET);
    pauseScreen();
}

/* progress bar:  ##########..........  (colour depends on percentage)     */
static void printBar(int score, int max)
{
    int filled = score * 20 / max;
    int pct = score * 100 / max;
    printf("%s", pct < 40 ? C_RED : (pct < 70 ? C_YELLOW : C_GREEN));
    printLine('#', filled);
    printf("%s", C_RESET);
    printLine('.', 20 - filled);
}

/* kind: 1 = quiz, 2 = mid, 3 = final                                      */
static int scoreOf(const Course *c, int kind)
{
    switch (kind) {
        case 1:  return c->quiz[0] + c->quiz[1] + c->quiz[2];
        case 2:  return c->mid;
        default: return c->finalExam;
    }
}

static int maxOf(int kind)
{
    switch (kind) {
        case 1:  return 30;
        case 2:  return 30;
        default: return 40;
    }
}

static int sumScores(const Student *s, int i, int kind)
{
    if (i >= s->courseCount) return 0;
    return scoreOf(&s->courses[i], kind) + sumScores(s, i + 1, kind);
}

static void printLiveRows(const Student *s, int i, int kind)
{
    const Course *c;
    int score, max;
    if (i >= s->courseCount) return;
    c = &s->courses[i];
    score = scoreOf(c, kind);
    max = maxOf(kind);
    switch (kind) {
        case 1:
            printf("  %-8s %-24.24s %3d %3d %3d  %3d/%-3d ", c->code, c->title,
                   c->quiz[0], c->quiz[1], c->quiz[2], score, max);
            break;
        default:
            printf("  %-8s %-24.24s %-11s %3d/%-3d ", c->code, c->title, "", score, max);
            break;
    }
    printBar(score, max);
    putchar('\n');
    printLiveRows(s, i + 1, kind);
}

static void showLive(const Student *s, int kind)
{
    char ts[32], line[64];
    int max = maxOf(kind), sum = sumScores(s, 0, kind);
    printHeader(kind == 1 ? "LIVE RESULTS - QUIZ" : (kind == 2 ? "LIVE RESULTS - MID-TERM" : "LIVE RESULTS - FINAL EXAM"));
    timeStamp(ts, sizeof ts);
    printf("  %s[LIVE]%s Updated: %s\n\n", C_GREEN, C_RESET, ts);
    if (kind == 1)
        printf("%s  %-8s %-24s %3s %3s %3s  %-7s %s%s\n", C_BOLD, "CODE", "COURSE", "Q1", "Q2", "Q3", "TOTAL", "PROGRESS", C_RESET);
    else
        printf("%s  %-8s %-24s %-11s %-7s %s%s\n", C_BOLD, "CODE", "COURSE", "", "SCORE", "PROGRESS", C_RESET);
    printRule('-', C_CYAN);
    printLiveRows(s, 0, kind);
    printRule('-', C_CYAN);
    snprintf(line, sizeof line, "Average : %.1f / %d  (%.0f%%)",
             (float)sum / (float)s->courseCount, max, 100.0f * (float)sum / (float)(max * s->courseCount));
    printf("  %s%s%s\n", C_BOLD, line, C_RESET);
    pauseScreen();
}

static void liveResultsMenu(const Student *s)
{
    int choice;
    char ts[32];
    printHeader("LIVE RESULTS");
    timeStamp(ts, sizeof ts);
    printf("  %s[LIVE]%s Results are shown as of %s\n\n", C_GREEN, C_RESET, ts);
    printf("   1. Quiz Results\n");
    printf("   2. Mid-term Results\n");
    printf("   3. Final Exam Results\n");
    printf("   4. Full Result Sheet (with grades)\n");
    printf("   5. Back to dashboard\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-5): ", 1, 5);
    switch (choice) {
        case 1: case 2: case 3: showLive(s, choice); break;
        case 4:                 showResults(s);      break;
        case 5:                 return;
    }
    liveResultsMenu(s);
}

/* ------------------------------ NOTICES --------------------------------- */
static void printNoticeRows(int i)
{
    if (i >= NOTICE_COUNT) return;
    printf("  %s[%s]%s %s%s%s\n", C_CYAN, notices[i].date, C_RESET, C_BOLD, notices[i].title, C_RESET);
    printf("     %s\n\n", notices[i].body);
    printNoticeRows(i + 1);
}

static void showNotices(void)
{
    printHeader("NOTICE BOARD");
    printNoticeRows(0);
    pauseScreen();
}

/* -------------------------- HALL INFO + PAYMENT ------------------------- */
static void printHallBillRows(const Student *s, int i)
{
    const HallBill *b;
    float tot, due;
    if (i >= HALL_MONTHS) return;
    b = &s->hall.bills[i];
    tot = b->rent + b->mess;
    due = tot - b->paid;
    printf("  %-3d %-10s %7.0f %7.0f %8.0f %8.0f %8.0f   %s%s%s\n", i + 1, b->month, b->rent, b->mess,
           tot, b->paid, due, due <= 0 ? C_GREEN : C_RED, due <= 0 ? "PAID" : "DUE", C_RESET);
    printHallBillRows(s, i + 1);
}

static void payAllHall(Student *s, int i)
{
    if (i >= HALL_MONTHS) return;
    s->hall.bills[i].paid = s->hall.bills[i].rent + s->hall.bills[i].mess;
    payAllHall(s, i + 1);
}

static void hallPaymentMenu(Student *s, int idx)
{
    int choice, month, due, amt, totalDue;
    char rcpt[16], desc[32];
    float total, paid;

    printHeader("HALL PAYMENT");
    total = hallTotal(s, 0);
    paid  = hallPaid(s, 0);
    printf("%s  %-3s %-10s %7s %7s %8s %8s %8s   %s%s\n", C_BOLD, "No", "MONTH", "RENT", "DINING",
           "TOTAL", "PAID", "DUE", "STATUS", C_RESET);
    printRule('-', C_CYAN);
    printHallBillRows(s, 0);
    printRule('-', C_CYAN);
    printf("  %s%-14s %7s %7s %8.0f %8.0f %8.0f%s   (Tk)\n\n", C_BOLD, "TOTAL", "", "", total, paid, total - paid, C_RESET);
    printf("  1. Pay a month's bill\n  2. Pay all hall dues\n  3. View my receipts\n  4. Back\n\n");

    choice = readInt("  Enter choice (1-4): ", 1, 4);
    switch (choice) {
        case 1:
            month = readInt("  Month number to pay: ", 1, HALL_MONTHS);
            due = (int)(s->hall.bills[month - 1].rent + s->hall.bills[month - 1].mess - s->hall.bills[month - 1].paid);
            if (due <= 0) {
                printError("This month is already fully paid.");
            } else {
                amt = readInt("  Amount to pay (Tk): ", 1, due);
                s->hall.bills[month - 1].paid += (float)amt;
                snprintf(desc, sizeof desc, "Hall - %s", s->hall.bills[month - 1].month);
                if (saveStudentAt(idx, s)) {
                    writeReceipt(s, desc, amt, rcpt);
                    printSuccess("Hall payment recorded (demo - no real money moves).");
                    showReceiptCard(s, rcpt, desc, amt);
                } else printError("Could not save the payment.");
            }
            pauseScreen();
            break;
        case 2:
            totalDue = (int)(total - paid);
            if (totalDue <= 0) {
                printError("You have no hall dues.");
            } else {
                printf("\n  Total hall dues: %d Tk\n", totalDue);
                if (readInt("  Pay everything now? (1 = Yes, 2 = No): ", 1, 2) == 1) {
                    payAllHall(s, 0);
                    if (saveStudentAt(idx, s)) {
                        writeReceipt(s, "Hall - all dues", totalDue, rcpt);
                        printSuccess("All hall dues cleared (demo - no real money moves).");
                        showReceiptCard(s, rcpt, "Hall - all dues", totalDue);
                    } else printError("Could not save the payment.");
                }
            }
            pauseScreen();
            break;
        case 3:
            showReceipts(s);
            break;
        case 4:
            return;
    }
    hallPaymentMenu(s, idx);
}

static void applyHall(Student *s, int idx)
{
    printf("\n  Hall seat fee: %.0f Tk rent + %.0f Tk dining per month.\n", HALL_RENT, HALL_MESS);
    switch (readInt("  Apply for a hall seat now? (1 = Yes, 2 = No): ", 1, 2)) {
        case 1:
            assignHall(s, 0);
            if (saveStudentAt(idx, s)) {
                logEvent(s->id, "HALL_APPLIED");
                printSuccess("Hall seat allotted! Your monthly bills are now available.");
            } else {
                s->hall.resident = 0;
                printError("Could not save your application.");
            }
            pauseScreen();
            break;
        default:
            break;
    }
}

static void hallMenu(Student *s, int idx)
{
    printHeader("HALL INFORMATION & PAYMENT");
    if (!s->hall.resident) {
        printf("  You are registered as a %snon-resident%s student.\n\n", C_BOLD, C_RESET);
        printf("   1. Apply for a hall seat\n   2. Back to dashboard\n\n");
        printRule('-', C_CYAN);
        switch (readInt("  Enter choice (1-2): ", 1, 2)) {
            case 1:  applyHall(s, idx); hallMenu(s, idx); return;
            default: return;
        }
    } else {
        char buf[48];
        printField("Hall", s->hall.hall);
        printField("Room No.", s->hall.room);
        snprintf(buf, sizeof buf, "%d", s->hall.seat);
        printField("Seat No.", buf);
        snprintf(buf, sizeof buf, "%.0f Tk + %.0f Tk dining / month", HALL_RENT, HALL_MESS);
        printField("Monthly Charges", buf);
        printField("Hall Warden", s->hall.warden);
        printField("Gate Closing Time", "10:30 PM");
        snprintf(buf, sizeof buf, "%.0f Tk", hallTotal(s, 0) - hallPaid(s, 0));
        printField("Hall Dues", buf);
        printf("\n   1. Hall payment\n   2. Back to dashboard\n\n");
        printRule('-', C_CYAN);
        switch (readInt("  Enter choice (1-2): ", 1, 2)) {
            case 1:  hallPaymentMenu(s, idx); hallMenu(s, idx); return;
            default: return;
        }
    }
}

/* ------------------------------- TRANSPORT ------------------------------ */
static void printRouteRows(int i)
{
    if (i >= ROUTE_COUNT) return;
    printf("  %-3d %-20s %-9s %-18s %-9s %-9s\n",
           i, routes[i].name, routes[i].bus, routes[i].pickup, routes[i].departs, routes[i].returns);
    printRouteRows(i + 1);
}

static void printRouteTable(void)
{
    printf("%s  ALL AVAILABLE ROUTES%s\n", C_BOLD, C_RESET);
    printRule('-', C_CYAN);
    printf("%s  %-3s %-20s %-9s %-18s %-9s %-9s%s\n", C_BOLD, "No", "ROUTE", "BUS", "PICKUP", "DEPART", "RETURN", C_RESET);
    printRouteRows(1);
}

static void showTransport(const Student *s)
{
    printHeader("TRANSPORT FACILITIES");
    if (s->route == 0) {
        printf("  You have %snot subscribed%s to a university bus route.\n", C_BOLD, C_RESET);
        printf("  (You can subscribe from Update Profile -> Change bus route.)\n");
    } else {
        printField("Your Route", routes[s->route].name);
        printField("Bus Number", routes[s->route].bus);
        printField("Pickup Point", routes[s->route].pickup);
        printField("Departure", routes[s->route].departs);
        printField("Return from Campus", routes[s->route].returns);
    }
    putchar('\n');
    printRouteTable();
    pauseScreen();
}

/* ----------------------------- PAYMENT LEDGER --------------------------- */
static void printFeeRows(const Student *s, int i)
{
    const Fee *f;
    float due;
    if (i >= s->feeCount) return;
    f = &s->fees[i];
    due = f->amount - f->paid;
    printf("  %-3d %-24s %9.0f %9.0f %9.0f   %s%s%s\n", i + 1, f->desc, f->amount, f->paid, due,
           due <= 0 ? C_GREEN : C_RED, due <= 0 ? "PAID" : "DUE", C_RESET);
    printFeeRows(s, i + 1);
}

static void paymentMenu(Student *s, int idx)
{
    float total, paid;
    int choice, feeNo, amt, due;
    char rcpt[16];

    printHeader("PAYMENT LEDGER");
    total = sumAmount(s, 0);
    paid  = sumPaid(s, 0);
    printf("%s  %-3s %-24s %9s %9s %9s   %s%s\n", C_BOLD, "No", "DESCRIPTION", "AMOUNT", "PAID", "DUE", "STATUS", C_RESET);
    printRule('-', C_CYAN);
    printFeeRows(s, 0);
    printRule('-', C_CYAN);
    printf("  %s%-28s %9.0f %9.0f %9.0f%s   (Tk)\n\n", C_BOLD, "TOTAL", total, paid, total - paid, C_RESET);
    printf("  (Hall dues are paid from the Hall menu.)\n\n");
    printf("  1. Make a payment\n  2. View my receipts\n  3. Back to dashboard\n\n");

    choice = readInt("  Enter choice (1-3): ", 1, 3);
    switch (choice) {
        case 1:
            feeNo = readInt("  Fee number to pay: ", 1, s->feeCount);
            due   = (int)(s->fees[feeNo - 1].amount - s->fees[feeNo - 1].paid);
            if (due <= 0) {
                printError("This fee is already fully paid.");
            } else {
                amt = readInt("  Amount to pay (Tk): ", 1, due);
                s->fees[feeNo - 1].paid += (float)amt;
                if (saveStudentAt(idx, s)) {
                    writeReceipt(s, s->fees[feeNo - 1].desc, amt, rcpt);
                    printSuccess("Payment recorded (demo - no real money moves).");
                    showReceiptCard(s, rcpt, s->fees[feeNo - 1].desc, amt);
                } else printError("Could not save the payment.");
            }
            pauseScreen();
            break;
        case 2:
            showReceipts(s);
            break;
        case 3:
            return;
    }
    paymentMenu(s, idx);
}

/* ---------------------------- ACADEMIC RESULT --------------------------- */
static void printHistoryRows(const Student *s, int i)
{
    if (i >= s->historyCount) return;
    printf("  %-10d %-10.1f %-8.2f\n", s->history[i].semester, s->history[i].credits, s->history[i].gpa);
    printHistoryRows(s, i + 1);
}

static void showAcademicResult(const Student *s)
{
    float curCr = sumCredits(s, 0);
    float curGpa = semesterGpa(s);
    float totalCr = histCredits(s, 0) + curCr;
    float cgpa = totalCr > 0 ? (histPoints(s, 0) + sumPoints(s, 0)) / totalCr : 0.0f;

    printHeader("ACADEMIC RESULT");
    printf("%s  %-10s %-10s %-8s%s\n", C_BOLD, "SEMESTER", "CREDITS", "GPA", C_RESET);
    printRule('-', C_CYAN);
    printHistoryRows(s, 0);
    printf("  %-10d %-10.1f %-8.2f %s(current)%s\n", s->semester, curCr, curGpa, C_YELLOW, C_RESET);
    printRule('-', C_CYAN);
    printf("  Total credits completed : %.1f\n", totalCr);
    printf("  %sCGPA                    : %.2f%s\n", C_BOLD C_GREEN, cgpa, C_RESET);
    printf("  Academic standing       : %s\n", standingOf(cgpa));
    pauseScreen();
}

/* ======================================================================= */
/*                             UPDATE PROFILE                              */
/* ======================================================================= */
static void commit(int idx, const Student *s)
{
    if (saveStudentAt(idx, s)) printSuccess("Profile updated successfully.");
    else                       printError("Could not save your changes.");
}

static int confirmCurrentPassword(const Student *s, int left)
{
    char pw[PASS_LEN];
    if (left <= 0) return 0;
    readPassword("  Current password              : ", pw, sizeof pw);
    if (s->passHash == hashStr(pw, HASH_SEED)) return 1;
    printError("Incorrect password.");
    return confirmCurrentPassword(s, left - 1);
}

static void changePassword(Student *s, int idx)
{
    char pw[PASS_LEN];
    if (!confirmCurrentPassword(s, MAX_ATTEMPTS)) {
        printError("Too many wrong attempts. Password not changed.");
        logEvent(s->id, "PWD_CHANGE_FAIL");
        return;
    }
    choosePassword(pw);
    s->passHash = hashStr(pw, HASH_SEED);
    commit(idx, s);
    logEvent(s->id, "PWD_CHANGED");
}

static void changeRoute(Student *s, int idx)
{
    int newRoute, f;
    printf("\n  Current route: %s\n\n", routes[s->route].name);
    printRouteTable();
    putchar('\n');
    newRoute = readInt("  New route (0 = no transport, 1-5): ", 0, ROUTE_COUNT - 1);
    if (newRoute == s->route) { printError("That is already your route."); return; }

    f = findFee(s, "Transport Fee", 0);
    if (newRoute == 0) {
        if (f >= 0 && s->fees[f].paid > 0) {
            printError("Transport fee is already (partly) paid. Contact the accounts office.");
            return;
        }
        if (f >= 0) { shiftFees(s, f); s->feeCount--; }
    } else if (f < 0) {
        if (s->feeCount >= MAX_FEES) { printError("Cannot add the transport fee right now."); return; }
        addFee(s, "Transport Fee", TRANSPORT_FEE, 0.0f);
    }
    s->route = newRoute;
    commit(idx, s);
}

static void updateProfileMenu(Student *s, int idx)
{
    int choice;
    printHeader("UPDATE PROFILE");
    printField("Name", s->name);
    printField("Phone", s->phone);
    printField("Email", s->email);
    printField("Bus route", routes[s->route].name);
    printf("\n   1. Change name\n");
    printf("   2. Change phone number\n");
    printf("   3. Change email\n");
    printf("   4. Change password\n");
    printf("   5. Change bus route\n");
    printf("   6. Change security question\n");
    printf("   7. Back to dashboard\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-7): ", 1, 7);
    switch (choice) {
        case 1:
            readNonEmpty("  New full name : ", s->name, NAME_LEN);
            commit(idx, s);
            break;
        case 2:
            readPhone("  New phone     : ", s->phone);
            commit(idx, s);
            break;
        case 3:
            readEmail("  New email     : ", s->email);
            commit(idx, s);
            break;
        case 4:
            changePassword(s, idx);
            break;
        case 5:
            changeRoute(s, idx);
            break;
        case 6:
            if (confirmCurrentPassword(s, MAX_ATTEMPTS)) { setSecurity(s); commit(idx, s); }
            else printError("Incorrect password. Security question not changed.");
            break;
        case 7:
            return;
    }
    pauseScreen();
    updateProfileMenu(s, idx);
}

/* ======================================================================= */
/*                  SIGN UP / LOGIN / FORGOT PASSWORD / MENUS              */
/* ======================================================================= */

/* returns 1 = got a fresh ID, 0 = user wants to go to the Login screen    */
static int readNewId(char *id)
{
    Student tmp;
    readNonEmpty("  Student ID (L = go to Login)  : ", id, ID_LEN);
    if (isKey(id, 'l')) return 0;
    if (strlen(id) < 4 || !validId(id)) {
        printError("ID must be 4+ characters: letters, digits and '-' only.");
        return readNewId(id);
    }
    if (lookupStudent(id, &tmp) >= 0) {
        printError("This ID is already registered.");
        printf("\n  1. Go to Login\n  2. Enter a different ID\n\n");
        switch (readInt("  Your choice (1-2): ", 1, 2)) {
            case 1:  return 0;
            default: return readNewId(id);
        }
    }
    return 1;
}

static void signUp(void)
{
    Student s;
    char pw[PASS_LEN];
    memset(&s, 0, sizeof s);

    printHeader("NEW STUDENT REGISTRATION");
    printf("  Already registered? Type L at the Student ID prompt to go to Login.\n\n");
    readNonEmpty("  Full name                     : ", s.name, NAME_LEN);
    if (!readNewId(s.id)) { loginAttempt(MAX_ATTEMPTS); return; }
    readPhone("  Phone number                  : ", s.phone);
    readEmail("  Email address                 : ", s.email);

    printf("\n  1. CSE   2. SWE   3. EEE   4. BBA\n");
    s.dept     = readInt("  Department (1-4)              : ", 1, 4);
    s.semester = readInt("  Current semester (1-12)       : ", 1, MAX_SEM);

    printf("\n  1. Hall resident   2. Non-resident\n");
    s.hall.resident = (readInt("  Residence (1-2)               : ", 1, 2) == 1);

    printf("\n  0. No transport   1. Uttara   2. Mirpur   3. Dhanmondi   4. Savar   5. Gabtoli\n");
    s.route = readInt("  Bus route (0-5)               : ", 0, ROUTE_COUNT - 1);

    putchar('\n');
    choosePassword(pw);
    s.passHash = hashStr(pw, HASH_SEED);
    setSecurity(&s);
    createStudentData(&s);

    putchar('\n');
    if (appendStudent(&s)) {
        logEvent(s.id, "SIGNUP");
        printSuccess("Registration successful!");
        printf("\n  1. Log in now\n  2. Back to main menu\n\n");
        switch (readInt("  Your choice (1-2): ", 1, 2)) {
            case 1:  loginAttempt(MAX_ATTEMPTS); break;
            default: break;
        }
    } else {
        printError("Could not write to the database file.");
        pauseScreen();
    }
}

static int verifyAnswer(const Student *s, int left)
{
    char ans[64], norm[64], msg[48];
    if (left <= 0) return 0;
    readNonEmpty("  Your answer : ", ans, sizeof ans);
    normalize(ans, norm);
    if (hashStr(norm, HASH_SEED) == s->secHash) return 1;
    snprintf(msg, sizeof msg, "Incorrect answer. Attempts left: %d", left - 1);
    printError(msg);
    return verifyAnswer(s, left - 1);
}

/* returns 1 if the password was reset successfully                        */
static int forgotPassword(void)
{
    char id[ID_LEN], pw[PASS_LEN];
    Student s;
    int idx;

    printHeader("FORGOT PASSWORD");
    readLine("  Enter your Student ID : ", id, sizeof id);
    idx = lookupStudent(id, &s);
    if (idx < 0) {
        printError("No account found with this ID.");
        pauseScreen();
        return 0;
    }
    printf("\n  Security question:\n  %s%s%s\n\n", C_BOLD, questionText(s.secQ), C_RESET);
    if (!verifyAnswer(&s, MAX_ATTEMPTS)) {
        printError("Verification failed. Password was not changed.");
        logEvent(s.id, "RESET_FAILED");
        pauseScreen();
        return 0;
    }
    printSuccess("Identity verified. Set a new password.");
    putchar('\n');
    choosePassword(pw);
    s.passHash = hashStr(pw, HASH_SEED);
    if (!saveStudentAt(idx, &s)) {
        printError("Could not save the new password.");
        pauseScreen();
        return 0;
    }
    logEvent(s.id, "PWD_RESET");
    printSuccess("Password reset successful! Please log in.");
    pauseScreen();
    return 1;
}

static void loginAttempt(int left)
{
    char id[ID_LEN], pw[PASS_LEN];
    Student s;
    int idx;

    printHeader("STUDENT LOGIN");
    printf("  Tip: type F as the ID if you forgot your password, or B to go back.\n\n");
    if (left < MAX_ATTEMPTS) printf("%s  Attempts left: %d%s\n\n", C_RED, left, C_RESET);

    readLine("  Student ID : ", id, sizeof id);
    if (isKey(id, 'b')) return;
    if (isKey(id, 'f')) {
        if (forgotPassword()) loginAttempt(MAX_ATTEMPTS);
        return;
    }
    readPassword("  Password   : ", pw, sizeof pw);

    idx = lookupStudent(id, &s);
    if (idx >= 0 && s.passHash == hashStr(pw, HASH_SEED)) {
        logEvent(s.id, "LOGIN_OK");
        dashboard(&s, idx);
        return;
    }
    logEvent(id, "LOGIN_FAILED");
    printError("Invalid ID or password.  (Forgot it? Type F at the ID prompt.)");
    if (left <= 1) {
        printError("Too many failed attempts. Returning to main menu.");
        pauseScreen();
        return;
    }
    pauseScreen();
    loginAttempt(left - 1);
}

static void dashboard(Student *s, int idx)
{
    int choice;
    char line[96];

    printHeader("STUDENT DASHBOARD");
    snprintf(line, sizeof line, "Welcome, %s   |   ID: %s   |   Semester %d", s->name, s->id, s->semester);
    printCentered(line, C_GREEN);
    printf("\n");
    printf("   1. My Profile\n");
    printf("   2. Update Profile\n");
    printf("   3. Registered Courses & Teachers\n");
    printf("   4. Live Results  (Quiz / Mid / Final)\n");
    printf("   5. Notice Board\n");
    printf("   6. Hall Information & Payment\n");
    printf("   7. Transport Facilities\n");
    printf("   8. Payment Ledger\n");
    printf("   9. Academic Result (GPA / CGPA)\n");
    printf("  10. Logout\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-10): ", 1, 10);
    switch (choice) {
        case 1:  showProfile(s);               break;
        case 2:  updateProfileMenu(s, idx);    break;
        case 3:  showCourses(s);               break;
        case 4:  liveResultsMenu(s);           break;
        case 5:  showNotices();                break;
        case 6:  hallMenu(s, idx);             break;
        case 7:  showTransport(s);             break;
        case 8:  paymentMenu(s, idx);          break;
        case 9:  showAcademicResult(s);        break;
        case 10:
            logEvent(s->id, "LOGOUT");
            printSuccess("Logged out successfully. Goodbye!");
            pauseScreen();
            return;
    }
    dashboard(s, idx);
}

static void mainMenu(void)
{
    int choice;
    printHeader("WELCOME");
    printf("   1. Sign Up          (new student)\n");
    printf("   2. Log In           (registered student)\n");
    printf("   3. Forgot Password\n");
    printf("   4. Exit\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-4): ", 1, 4);
    switch (choice) {
        case 1: signUp();                          break;
        case 2: loginAttempt(MAX_ATTEMPTS);        break;
        case 3: if (forgotPassword()) loginAttempt(MAX_ATTEMPTS); break;
        case 4:
            printf("\n  Thank you for using the portal. Goodbye!\n\n");
            return;
    }
    mainMenu();
}

int main(void)
{
    initConsole();
    checkDatabase();
    mainMenu();
    return 0;
}
