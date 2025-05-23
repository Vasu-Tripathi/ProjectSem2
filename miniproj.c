#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_EXPENSES 100
#define MAX_CATEGORIES 20

char categories[MAX_CATEGORIES][50];
COLORREF sliceColors[MAX_CATEGORIES];
int category_count = 0;

typedef struct {
    char category[50];
    float amount;
    struct tm date;
} Expense;

Expense expenses[MAX_EXPENSES];
int expense_count = 0;
float category_totals[MAX_CATEGORIES] = {0};
float monthly_income = 0;

float total_week = 0, total_month = 0, total_year = 0;

// ---------- Helpers ----------

int get_week_number(struct tm* date) {
    struct tm temp = *date;
    mktime(&temp);
    int wday = (temp.tm_wday == 0) ? 7 : temp.tm_wday;
    int week = (temp.tm_yday + 7 - wday + 3) / 7 + 1;
    return week;
}

int is_same_week(struct tm* a, struct tm* b) {
    return (a->tm_year == b->tm_year) && (get_week_number(a) == get_week_number(b));
}

int is_same_month(struct tm* a, struct tm* b) {
    return (a->tm_year == b->tm_year) && (a->tm_mon == b->tm_mon);
}

int is_same_year(struct tm* a, struct tm* b) {
    return (a->tm_year == b->tm_year);
}

int find_or_add_category(const char* name) {
    for (int i = 0; i < category_count; i++) {
        if (strcasecmp(categories[i], name) == 0) return i;
    }
    if (category_count < MAX_CATEGORIES) {
        strcpy(categories[category_count], name);
        sliceColors[category_count] = RGB(rand() % 256, rand() % 256, rand() % 256);
        return category_count++;
    }
    return -1;
}

// ---------- Add Expense ----------

void add_expense() {
    if (expense_count >= MAX_EXPENSES) {
        printf("\u274c Expense limit reached.\n");
        return;
    }

    Expense e;
    printf("Enter expense category: ");
    scanf(" %[^\n]", e.category);
    printf("Enter amount: ");
    scanf("%f", &e.amount);

    time_t now = time(NULL);
    e.date = *localtime(&now);
    expenses[expense_count++] = e;

    int idx = find_or_add_category(e.category);
    if (idx >= 0) {
        category_totals[idx] += e.amount;
    } else {
        printf("\u274c Category limit reached.\n");
    }

    printf("\u2705 Added: %s - \u20B9%.2f at %02d:%02d on %d-%02d-%02d\n", e.category, e.amount,
        e.date.tm_hour, e.date.tm_min, e.date.tm_year + 1900, e.date.tm_mon + 1, e.date.tm_mday);
}

// ---------- Summary ----------

void calculate_period_totals() {
    time_t now = time(NULL);
    struct tm* current = localtime(&now);
    total_week = total_month = total_year = 0;

    for (int i = 0; i < expense_count; i++) {
        Expense* e = &expenses[i];
        if (is_same_year(&e->date, current)) {
            total_year += e->amount;
            if (is_same_month(&e->date, current)) {
                total_month += e->amount;
                if (is_same_week(&e->date, current)) {
                    total_week += e->amount;
                }
            }
        }
    }
}

void show_balances() {
    calculate_period_totals();
    printf("\n--- Budget Summary ---\n");
    printf("Monthly Income: \u20B9%.2f\n", monthly_income);
    printf("This Week's Spending: ₹%.2f\n", total_week);
    printf("Remaining (Week): \u20B9%.2f\n", monthly_income - total_week);
    printf("This Month's Spending: ₹%.2f\n", total_month);
    printf("Remaining (Month): \u20B9%.2f\n", monthly_income - total_month);
    printf("This Year's Spending: ₹%.2f\n", total_year);
    printf("Remaining (Year): \u20B9%.2f\n", monthly_income - total_year);
}

// ---------- Drawing ----------

double calculateTotal() {
    double total = 0;
    for (int i = 0; i < category_count; i++) total += category_totals[i];
    return total;
}

void DrawPieChart(HDC hdc, int cx, int cy, int r) {
    double total = calculateTotal();
    if (total <= 0) return;

    double angle = 0.0;
    for (int i = 0; i < category_count; i++) {
        if (category_totals[i] <= 0) continue;
        double sweep = (category_totals[i] / total) * 360.0;
        double rad_start = angle * M_PI / 180.0;
        double rad_end = (angle + sweep) * M_PI / 180.0;

        HBRUSH brush = CreateSolidBrush(sliceColors[i]);
        SelectObject(hdc, brush);

        Pie(hdc, cx - r, cy - r, cx + r, cy + r,
            cx + (int)(r * cos(rad_start)), cy - (int)(r * sin(rad_start)),
            cx + (int)(r * cos(rad_end)), cy - (int)(r * sin(rad_end)));

        DeleteObject(brush);
        angle += sweep;
    }
}

void DrawLegend(HDC hdc, int x, int y) {
    int spacing = 25;
    for (int i = 0; i < category_count; i++) {
        if (category_totals[i] <= 0) continue;

        HBRUSH brush = CreateSolidBrush(sliceColors[i]);
        SelectObject(hdc, brush);
        Rectangle(hdc, x, y + i * spacing, x + 20, y + i * spacing + 20);
        DeleteObject(brush);

        char label[100];
        sprintf(label, "%s: %.2f Rs", categories[i], category_totals[i]);
        TextOut(hdc, x + 30, y + i * spacing, label, strlen(label));
    }
}

// ---------- GUI ----------

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HFONT font = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Segoe UI"));
        SelectObject(hdc, font);
        TextOut(hdc, 20, 10, "Monthly Budget Pie Chart", 26);
        DeleteObject(font);

        DrawPieChart(hdc, 300, 220, 150);
        DrawLegend(hdc, 500, 80);

        EndPaint(hWnd, &ps);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void launch_gui() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "BudgetChartClass";

    RegisterClass(&wc);

    HWND hWnd = CreateWindow("BudgetChartClass", "Budget Pie Chart",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 500, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
        DispatchMessage(&msg);
}

// ---------- Main ----------

int main() {
SetConsoleOutputCP(CP_UTF8);
    srand((unsigned int)time(NULL));
    printf("Enter your monthly income: \u20B9");
    scanf("%f", &monthly_income);

    int option;
    do {
        printf("\n--- Budget Menu ---\n");
        printf("1. Add Expense\n");
        printf("2. Show All Expenses\n");
        printf("3. Show Pie Chart\n");
        printf("4. Show Remaining Balances\n");
        printf("0. Exit\n");
        printf("Choice: ");
        scanf("%d", &option);

        switch (option) {
        case 1:
            add_expense();
            break;
        case 2:
            printf("\n=== Expenses ===\n");
            for (int i = 0; i < expense_count; i++) {
                Expense e = expenses[i];
                printf("%d-%02d-%02d %02d:%02d | %-15s \u20B9%.2f\n",
                    e.date.tm_year + 1900, e.date.tm_mon + 1, e.date.tm_mday,
                    e.date.tm_hour, e.date.tm_min, e.category, e.amount);
            }
            break;
        case 3:
            launch_gui();
            break;
        case 4:
            show_balances();
            break;
        case 0:
            printf("Goodbye!\n");
            break;
        default:
            printf("Invalid option.\n");
        }
    } while (option != 0);

    return 0;
}
