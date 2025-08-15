#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>
#include <unistd.h>

void format_time_string(char *buffer, int total_seconds) {
    int h = total_seconds / 3600;
    int m = (total_seconds % 3600) / 60;
    int s = total_seconds % 60;
    sprintf(buffer, "%02d:%02d:%02d", h, m, s);
}

int main(int argc, char *argv[]) {
    int ch;
    int h, w;
    char timestr[64], datestr[64];
    char fmt[] = "%T";
    int ampm = 0;
    int border = 1;
    int color = 0;
    int utc = 0;
    int show_date = 0;
    int delay = 1;

    // Новые переменные для таймеров
    int countdown_mode = 0;
    int timer_mode = 0;
    int countdown_hours = 0;
    int countdown_minutes = 0;
    int countdown_seconds = 0;

    while ((ch = getopt(argc, argv, "C:brlctT:ahc:t")) != -1) {
        switch (ch) {
            case 'C':
                if (!strcmp(optarg, "red"))      color = 1;
                else if (!strcmp(optarg, "green"))  color = 2;
                else if (!strcmp(optarg, "blue"))   color = 3;
                else if (!strcmp(optarg, "cyan"))   color = 4;
                else if (!strcmp(optarg, "yellow")) color = 5;
                else if (!strcmp(optarg, "magenta"))color = 6;
                else if (!strcmp(optarg, "white"))  color = 7;
                else {
                    fprintf(stderr, "Invalid color: %s\n", optarg);
                    exit(1);
                }
                break;
            case 'b':
                border = 0;
                break;
            case 'r':
                ampm = 1;
                break;
            case 'l':
                utc = 1;
                break;
            case 'c':
                show_date = 1;
                break;
            case 't':
                timer_mode = 1;
                break;
            case 'T':
                delay = atoi(optarg);
                break;
            case 'a':
                strcpy(fmt, "%r");
                ampm = 1;
                break;
            case 'h':
                printf("Usage: %s [OPTIONS]\n"
                       "  -C COLOR    set color (red, green, blue, cyan, yellow, magenta, white)\n"
                       "  -b          disable border\n"
                       "  -r          12-hour format\n"
                       "  -l          use UTC time\n"
                       "  -c          show date\n"
                       "  -t          start stopwatch timer\n"
                       "  -c HH:MM:SS start countdown (e.g. -c 0:5:0)\n"
                       "  -T SEC      update every SEC seconds (default: 1)\n"
                       "  -a          show AM/PM\n"
                       "  -h          show this help\n"
                       "Press 'q' or ESC to quit.\n", argv[0]);
                exit(0);
                break;
            case 'c':  // Обрабатывается выше
                countdown_mode = 1;
                if (sscanf(optarg, "%d:%d:%d", &countdown_hours, &countdown_minutes, &countdown_seconds) == 3) {
                    // OK
                } else if (sscanf(optarg, "%d:%d", &countdown_minutes, &countdown_seconds) == 2) {
                    countdown_hours = 0;
                } else if (sscanf(optarg, "%d", &countdown_seconds) == 1) {
                    countdown_hours = 0;
                    countdown_minutes = 0;
                } else {
                    fprintf(stderr, "Error: invalid time format for -c. Use HH:MM:SS, MM:SS, or SS.\n");
                    exit(1);
                }
                break;
            case 't':
                timer_mode = 1;
                break;
            default:
                fprintf(stderr, "Unknown option\n");
                exit(1);
        }
    }

    if (color) {
        initscr();
        start_color();
        init_pair(1, COLOR_RED,    COLOR_BLACK);
        init_pair(2, COLOR_GREEN,  COLOR_BLACK);
        init_pair(3, COLOR_BLUE,   COLOR_BLACK);
        init_pair(4, COLOR_CYAN,   COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA,COLOR_BLACK);
        init_pair(7, COLOR_WHITE,  COLOR_BLACK);
    } else {
        initscr();
    }

    cbreak();
    noecho();
    curs_set(0);

    if (border) {
        getmaxyx(stdscr, h, w);
        WINDOW *win = newwin(h, w, 0, 0);
        box(win, 0, 0);
        wrefresh(win);
    } else {
        h = LINES;
        w = COLS;
        win = stdscr;
    }

    // === Режимы таймера и обратного отсчёта ===
    if (countdown_mode || timer_mode) {
        time_t start = time(NULL);
        int total_start_seconds = countdown_hours * 3600 + countdown_minutes * 60 + countdown_seconds;
        char time_str[32];
        int key;

        wtimeout(win, 100);  // Обновляем раз в 100 мс

        while (1) {
            time_t now = time(NULL);
            int elapsed = (int)(now - start);
            int remaining;

            if (timer_mode) {
                format_time_string(time_str, elapsed);
            } else if (countdown_mode) {
                remaining = total_start_seconds - elapsed;
                if (remaining <= 0) {
                    strcpy(time_str, "00:00:00");
                    werase(win);
                    if (border) box(win, 0, 0);
                    mvwprintw(win, h/2, (w - 11)/2, "TIME'S UP!");
                    if (color) wcolor_set(win, 5, NULL);  // yellow
                    wrefresh(win);
                    beep();
                    napms(1500);
                    break;
                }
                format_time_string(time_str, remaining);
            }

            // Отрисовка
            werase(win);
            if (border) box(win, 0, 0);
            if (color) wcolor_set(win, 7, NULL);  // white

            int text_x = (w - strlen(time_str)) / 2;
            int text_y = h / 2;
            mvwprintw(win, text_y, text_x, "%s", time_str);

            // Показ даты при -c (show_date)
            if (show_date) {
                char date_str[64];
                strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
                int dx = (w - strlen(date_str)) / 2;
                mvwprintw(win, text_y + 2, dx, "%s", date_str);
            }

            wrefresh(win);

            key = wgetch(win);
            if (key == 'q' || key == 27)  // 'q' или ESC
                break;
        }
    } else {
        // === Оригинальный режим: часы ===
        wtimeout(win, delay * 1000);

        struct tm tm;
        while (1) {
            get_time(&tm, ampm, utc);
            strftime(timestr, sizeof(timestr), fmt, &tm);

            if (show_date) {
                strftime(datestr, sizeof(datestr), "%Y-%m-%d %A", &tm);
            }

            werase(win);
            if (border) box(win, 0, 0);

            if (color) {
                if (ampm) {
                    int hr = tm.tm_hour % 12;
                    if (hr == 0) hr = 12;
                    if (hr < 6 || hr > 9) wcolor_set(win, 1, NULL);
                    else wcolor_set(win, 2, NULL);
                } else {
                    if (tm.tm_hour < 6 || tm.tm_hour > 19) wcolor_set(win, 1, NULL);
                    else wcolor_set(win, 2, NULL);
                }
            }

            mvwprintw(win, h/2, (w - strlen(timestr))/2, "%s", timestr);

            if (show_date) {
                if (color) wcolor_set(win, 0, NULL);
                mvwprintw(win, h/2 + 2, (w - strlen(datestr))/2, "%s", datestr);
            }

            wrefresh(win);

            int key = wgetch(win);
            if (key == 'q' || key == 27)
                break;
        }
    }

    endwin();
    return 0;
}
