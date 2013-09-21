#include "common.h"

struct timeval last_recv_time[2] = {0};
pthread_mutex_t recv_time_lock;

int main(int argc, char *argv[])
{   
    initscr();          /* Start curses mode        */
    if (has_colors() == FALSE)
    {   
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }

    curs_set(0); // hide cursor
    noecho();

    start_color();          /* Start color          */
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);
    init_pair(4, COLOR_RED, COLOR_WHITE);

    attron(COLOR_PAIR(1));
    print_in_middle(stdscr, 1, 0, 0, "master and slave bars background is colored if receiving");
    print_in_middle(stdscr, 2, 0, 0, "Press 'q' to quit");
    attroff(COLOR_PAIR(1));

    pthread_mutex_init(&recv_time_lock, NULL);

    update_background(MASTER, false);
    update_background(SLAVE, false);

    pthread_t tid_updater, tid_timer;
    pthread_create(&tid_updater, NULL, (void * (*)(void *))&init_updater, NULL);
    pthread_create(&tid_timer,   NULL, (void * (*)(void *))&init_timer,   NULL);

    while (getch() != 'q');

    endwin();
}

void update_background(int line, bool onoff)
{
    int color = onoff ? line+2 : 1;
    attron(COLOR_PAIR(color));
    char* text = line ? "   slave    " : "   master   ";
    print_in_middle(stdscr, LINES/2 + 2*line, 0, 0, text);
    attroff(COLOR_PAIR(color));
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string)
{
    int length, x, y;
    float temp;

    if(win == NULL)
        win = stdscr;
    getyx(win, y, x);
    if(startx != 0)
        x = startx;
    if(starty != 0)
        y = starty;
    if(width == 0)
        width = 80;

    length = strlen(string);
    temp = (width - length)/ 2;
    x = startx + (int)temp;
    mvwprintw(win, y, x, "%s", string);
    refresh();
}
