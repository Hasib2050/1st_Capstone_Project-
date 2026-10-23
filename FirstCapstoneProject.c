/* =========================================================================
   UNIVERSITY STUDENT PORTAL  -  Capstone Project (C Language)
   -------------------------------------------------------------------------
   Features : Sign up / Login, Profile, Registered Courses + Teachers,
              Quiz / Mid / Final Results, Notice Board, Hall Information,
              Transport Facilities, Payment Ledger, Academic Result (CGPA)
   Concepts : structures (abstraction), functions, recursion instead of
              loops, switch-case, file handling (binary database)
   Build    : gcc student_portal.c -o portal        (Linux / macOS / MinGW)
   Run      : ./portal                              (portal.exe on Windows)
   ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define W               78            /* width of the terminal UI           */
#define ID_LEN          16
#define NAME_LEN        40
#define PASS_LEN        32
#define PASS_MIN        6
#define MAX_ATTEMPTS    3
#define MAX_COURSES     6
#define COURSES_PER_SEM 5
#define MAX_FEES        6
#define MAX_SEM         12
#define ROUTE_COUNT     6
#define NOTICE_COUNT    6
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

typedef struct {                 /* hall / residence information            */
    int   resident;              /* 1 = living in hall, 0 = not             */
    char  hall[32];
    char  room[10];
    int   seat;
    float rent;
    char  warden[28];
} HallInfo;

typedef struct {                 /* result of an earlier semester           */
    int   semester;
    float gpa;
    float credits;
} SemResult;

typedef struct {                 /* the whole student record (file record)  */
    char          id[ID_LEN];
    char          name[NAME_LEN];
    unsigned long passHash;      /* password is never stored as plain text  */
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

static const char *halls[3] = { "Residential Hall A", "Residential Hall B", "Residential Hall C" };

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
     "Mid-term marks are now visible in the portal. Report errors within 3 days."},
    {"2026-10-12", "Hall Seat Renewal",
     "Hall residents must renew their seat at the hall office before 1 Nov."},
    {"2026-10-08", "Programming Contest",
     "Inter-department programming contest on 5 Nov. Register at the CSE lab."},
    {"2026-10-01", "Winter Break Notice",
     "Campus will be closed from 24 Dec to 2 Jan for the winter break."}
};

static unsigned long seedState = 1;     /* deterministic demo-data generator */

/* ------------------------- forward declarations ------------------------ */
static void mainMenu(void);
static void dashboard(Student *s, int idx);

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

static void printError(const char *msg)   { printf("%s  [!] %s%s\n", C_RED,   msg, C_RESET); }
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
        case 16:         return "Outstanding";
        case 15:         return "Excellent";
        case 14:         return "Very Good";
        case 13: case 12:return "Good";
        case 11: case 10:return "Satisfactory";
        default:         return "Needs Improvement";
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

/* recursive sums over the course list / history / ledger                  */
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

/* ======================================================================= */
/*          DEMO DATA GENERATION (done once, when a student signs up)      */
/* ======================================================================= */
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

static void addFee(Student *s, const char *desc, float amount, int minPct, int maxPct)
{
    Fee *f = &s->fees[s->feeCount++];
    int pct = nextRand(minPct, maxPct);
    snprintf(f->desc, sizeof f->desc, "%s", desc);
    f->amount = amount;
    f->paid   = (float)((int)(amount * (float)pct / 100.0f) / 100 * 100);
}

static void createStudentData(Student *s)
{
    seedState = hashStr(s->id, HASH_SEED);
    fillCourses(s, 0);
    fillHistory(s, 1);

    if (s->hall.resident) {
        snprintf(s->hall.hall,   sizeof s->hall.hall,   "%s", halls[nextRand(0, 2)]);
        snprintf(s->hall.room,   sizeof s->hall.room,   "R-%d", nextRand(101, 420));
        snprintf(s->hall.warden, sizeof s->hall.warden, "%s", teachers[nextRand(0, 7)]);
        s->hall.seat = nextRand(1, 4);
        s->hall.rent = 3000.0f;
    }

    addFee(s, "Tuition Fee",          45000.0f, 40, 100);
    addFee(s, "Registration Fee",      5000.0f, 100, 100);
    addFee(s, "Lab / Library Fee",     2500.0f, 0, 100);
    addFee(s, "Exam Fee",              1500.0f, 50, 100);
    if (s->hall.resident) addFee(s, "Hall Rent (4 months)", 12000.0f, 0, 100);
    if (s->route > 0)     addFee(s, "Transport Fee",         7500.0f, 0, 100);
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
    printHeader("QUIZ / MID / FINAL RESULTS");
    printf("%s  %-8s %3s %3s %3s %5s %6s %6s   %-4s %5s%s\n", C_BOLD,
           "CODE", "Q1", "Q2", "Q3", "MID", "FINAL", "TOTAL", "GRD", "GP", C_RESET);
    printRule('-', C_CYAN);
    printResultRows(s, 0);
    printRule('-', C_CYAN);
    printf("  Marks distribution : Quiz 3x10 | Mid 30 | Final 40 | Total 100\n");
    printf("  %sSemester GPA : %.2f%s\n", C_BOLD, semesterGpa(s), C_RESET);
    pauseScreen();
}

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

static void showHall(const Student *s)
{
    char buf[48];
    printHeader("HALL INFORMATION");
    if (!s->hall.resident) {
        printf("  You are registered as a %snon-resident%s student.\n\n", C_BOLD, C_RESET);
        printf("  To apply for a hall seat, visit the Hall Office (Admin Building,\n");
        printf("  Room 105) with your ID card and the seat application form.\n");
    } else {
        printField("Hall", s->hall.hall);
        printField("Room No.", s->hall.room);
        snprintf(buf, sizeof buf, "%d", s->hall.seat);
        printField("Seat No.", buf);
        snprintf(buf, sizeof buf, "%.0f Tk / month", s->hall.rent);
        printField("Monthly Rent", buf);
        printField("Hall Warden", s->hall.warden);
        printField("Gate Closing Time", "10:30 PM");
        printField("Mess / Dining", "Open 7:00 AM - 9:30 PM");
    }
    pauseScreen();
}

static void printRouteRows(int i)
{
    if (i >= ROUTE_COUNT) return;
    printf("  %-20s %-9s %-18s %-9s %-9s\n",
           routes[i].name, routes[i].bus, routes[i].pickup, routes[i].departs, routes[i].returns);
    printRouteRows(i + 1);
}

static void showTransport(const Student *s)
{
    printHeader("TRANSPORT FACILITIES");
    if (s->route == 0) {
        printf("  You have %snot subscribed%s to a university bus route.\n", C_BOLD, C_RESET);
    } else {
        printField("Your Route", routes[s->route].name);
        printField("Bus Number", routes[s->route].bus);
        printField("Pickup Point", routes[s->route].pickup);
        printField("Departure", routes[s->route].departs);
        printField("Return from Campus", routes[s->route].returns);
    }
    printf("\n%s  ALL AVAILABLE ROUTES%s\n", C_BOLD, C_RESET);
    printRule('-', C_CYAN);
    printf("%s  %-20s %-9s %-18s %-9s %-9s%s\n", C_BOLD, "ROUTE", "BUS", "PICKUP", "DEPART", "RETURN", C_RESET);
    printRouteRows(1);
    pauseScreen();
}

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

    printHeader("PAYMENT LEDGER");
    total = sumAmount(s, 0);
    paid  = sumPaid(s, 0);
    printf("%s  %-3s %-24s %9s %9s %9s   %s%s\n", C_BOLD, "No", "DESCRIPTION", "AMOUNT", "PAID", "DUE", "STATUS", C_RESET);
    printRule('-', C_CYAN);
    printFeeRows(s, 0);
    printRule('-', C_CYAN);
    printf("  %s%-28s %9.0f %9.0f %9.0f%s   (Tk)\n\n", C_BOLD, "TOTAL", total, paid, total - paid, C_RESET);
    printf("  1. Make a payment\n  2. Back to dashboard\n\n");

    choice = readInt("  Enter choice (1-2): ", 1, 2);
    switch (choice) {
        case 1:
            feeNo = readInt("  Fee number to pay: ", 1, s->feeCount);
            due   = (int)(s->fees[feeNo - 1].amount - s->fees[feeNo - 1].paid);
            if (due <= 0) {
                printError("This fee is already fully paid.");
            } else {
                amt = readInt("  Amount to pay (Tk): ", 1, due);
                s->fees[feeNo - 1].paid += (float)amt;
                if (saveStudentAt(idx, s)) printSuccess("Payment recorded (demo - no real money moves).");
                else                       printError("Could not save the payment.");
            }
            pauseScreen();
            paymentMenu(s, idx);                /* show updated ledger */
            return;
        case 2:
        default:
            return;
    }
}

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
/*                        SIGN UP / LOGIN / MENUS                          */
/* ======================================================================= */
static int validId(const char *id)           /* letters, digits and '-' only */
{
    if (*id == '\0') return 1;
    if (!((*id >= '0' && *id <= '9') || (*id >= 'a' && *id <= 'z') ||
          (*id >= 'A' && *id <= 'Z') || *id == '-')) return 0;
    return validId(id + 1);
}

static void readNewId(char *id)
{
    Student tmp;
    readNonEmpty("  Student ID (e.g. 221-15-1234) : ", id, ID_LEN);
    if (strlen(id) < 4 || !validId(id)) {
        printError("ID must be 4+ characters: letters, digits and '-' only.");
        readNewId(id);
    } else if (lookupStudent(id, &tmp) >= 0) {
        printError("This ID is already registered. Try logging in.");
        readNewId(id);
    }
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

static void signUp(void)
{
    Student s;
    char pw[PASS_LEN];
    memset(&s, 0, sizeof s);

    printHeader("NEW STUDENT REGISTRATION");
    readNonEmpty("  Full name                     : ", s.name, NAME_LEN);
    readNewId(s.id);

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
    createStudentData(&s);

    putchar('\n');
    if (appendStudent(&s)) printSuccess("Registration successful! You can now log in.");
    else                   printError("Could not write to the database file.");
    pauseScreen();
}

static void loginAttempt(int left)
{
    char id[ID_LEN], pw[PASS_LEN];
    Student s;
    int idx;

    printHeader("STUDENT LOGIN");
    if (left < MAX_ATTEMPTS) printf("%s  Attempts left: %d%s\n\n", C_RED, left, C_RESET);

    readLine("  Student ID : ", id, sizeof id);
    readPassword("  Password   : ", pw, sizeof pw);

    idx = lookupStudent(id, &s);
    if (idx >= 0 && s.passHash == hashStr(pw, HASH_SEED)) {
        dashboard(&s, idx);
        return;
    }
    printError("Invalid ID or password.");
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
    printf("   2. Registered Courses & Teachers\n");
    printf("   3. Quiz / Mid / Final Results\n");
    printf("   4. Notice Board\n");
    printf("   5. Hall Information\n");
    printf("   6. Transport Facilities\n");
    printf("   7. Payment Ledger\n");
    printf("   8. Academic Result (GPA / CGPA)\n");
    printf("   9. Logout\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-9): ", 1, 9);
    switch (choice) {
        case 1: showProfile(s);               break;
        case 2: showCourses(s);               break;
        case 3: showResults(s);               break;
        case 4: showNotices();                break;
        case 5: showHall(s);                  break;
        case 6: showTransport(s);             break;
        case 7: paymentMenu(s, idx);          break;
        case 8: showAcademicResult(s);        break;
        case 9:
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
    printf("   1. Sign Up  (new student)\n");
    printf("   2. Log In   (registered student)\n");
    printf("   3. Exit\n\n");
    printRule('-', C_CYAN);

    choice = readInt("  Enter your choice (1-3): ", 1, 3);
    switch (choice) {
        case 1: signUp();                  break;
        case 2: loginAttempt(MAX_ATTEMPTS);break;
        case 3:
            printf("\n  Thank you for using the portal. Goodbye!\n\n");
            return;
    }
    mainMenu();
}

int main(void)
{
    initConsole();
    mainMenu();
    return 0;
}
