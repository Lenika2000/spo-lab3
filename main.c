#include "includes/client.h"
#include "includes/server.h"
#include <ncurses.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("%s","Invalid number of arguments\n");
        return -1;
    }

    initscr();
    cbreak();
    noecho();
    int curs_state = curs_set(0);
    int return_code;
    if (argv[1][0] == 'c') {
        return_code =  run_client(argc, argv);
    } else {
        return_code = run_server(argc, argv);
    }
    curs_set(curs_state);
    endwin();
    return return_code;
}
