#include "../includes/client.h"
#include "../includes/client_net.h"
#include "../includes/net.h"
#include "../includes/util.h"
#include <memory.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

typedef struct client_shared_data {
    pthread_t display_thread;
    pthread_t receiver_thread;
    pthread_t sender_thread;
    pthread_cond_t display_cond;
    pthread_mutex_t history_mutex;
    pthread_mutex_t cursor_mutex;
    list_t *history_head;
    list_t *history_tail;
    int socket;
    int cursor;
    int down_pressed;
    int is_running; // 1-running, 0-ended
} client_shared_data_t;

#define MSG_BUFLEN (80 * 25 + 1)

typedef struct rendered_message {
    server_message_t msg;
    int height;
    char buffer[MSG_BUFLEN];
} rendered_message_t;

static int receiver_routine(client_shared_data_t *shared);
static void *receiver_routine_voidptr(void *ptr) {
    receiver_routine(ptr);
    return NULL;
}

int receiver_routine (client_shared_data_t *shared) {
    while (shared->is_running) {
        rendered_message_t *msg = malloc(sizeof(rendered_message_t));
        msg->height = -1;
        if (receive_server_message(shared->socket, &msg->msg) <  0) {
            fprintf(stderr, "Receive server msg error\n");
        }
        pthread_mutex_lock(&shared->history_mutex);

#define ID(iter) (((server_message_t *)((iter)->data))->id)
        if (!shared->history_head) {
            shared->history_head = shared->history_tail = new_list(msg);
        } else if (ID(shared->history_head) > msg->msg.id) {
            shared->history_head = list_insert_before(shared->history_head, msg);
        } else if (ID(shared->history_tail) < msg->msg.id) {
            shared->history_tail = list_insert_after(shared->history_tail, msg);
        } else {
            list_t *iter = shared->history_head;
            while (ID(iter) < msg->msg.id)
                iter = iter->next;
            list_insert_before(iter, msg);
        }
#undef ID

        pthread_mutex_unlock(&shared->history_mutex);
        pthread_cond_signal(&shared->display_cond);
    }
    return 0;
}

static int parse_line(client_shared_data_t *shared, char *buf, size_t len) {
    if (buf[0] == '/') {
        // при нажатии q - завершение программы
        if (buf[1] == 'q') {
            // атомарная запись 0 в поле is_running и завершение работы
            shared->is_running = 0;
            return 0;
        } else {
            // выделяем память для аргументов 1 и 2, где 1 - имя получателя, 2 - сообщение
            char *arg1 = malloc(len);
            char *arg2 = malloc(len);
            // считывание данных из массива buf, исключая перенос строки(\n), если значения были присвоены всем полям
            if (sscanf(buf, "/private %s %[^\n]", arg1, arg2)) {
                // заполняем информацию о принятом сообщении, получатель + текст
                client_text_message_t msg;
                msg.receiver_name_len = strlen(arg1);
                msg.receiver_name = arg1;
                msg.text_len = strlen(arg2);
                msg.text = arg2;
                // отправка сообщения клиенту
                if (send_client_text_message(shared->socket, &msg) <  0) {
                    fprintf(stderr, "Send client msg error\n");
                }
            }
            free(arg1);
            free(arg2);
        }
    } else {
        client_text_message_t msg;
        msg.receiver_name_len = 0;
        msg.text_len = len;
        msg.text = buf;
        if (send_client_text_message(shared->socket, &msg) < 0) {
            fprintf(stderr, "Send client msg error\n");
        }
    }
    return 1;
}


static int sender_routine(client_shared_data_t *shared);
static void *sender_routine_voidptr(void *ptr) {
    sender_routine(ptr);
    return NULL;
}

int sender_routine(client_shared_data_t *shared) {
    int height, width;
    getmaxyx(stdscr, height, width);
    WINDOW *window = newwin(1, width, height - 1, 0);
    keypad(window, TRUE);
    char *line = malloc(width - 1);
    size_t pos = 0;

    while (shared->is_running) {
        int c = wgetch(window);
        pthread_mutex_lock(&shared->cursor_mutex);
        switch (c) {
            case KEY_BACKSPACE:
            case KEY_DC:
            case 127:
                if (pos) {
                    line[--pos] = 0;
                    int y, x;
                    getyx(window, y, x);
                    mvwdelch(window, y, x - 1);
                }
                break;

            case '\n':
                if (pos)
                    parse_line(shared, line, pos);
                line[pos = 0] = 0;
                wclear(window);
                wrefresh(window);
                break;

            case KEY_PPAGE:
                shared->down_pressed -= getmaxy(stdscr) - 2;
                pthread_cond_signal(&shared->display_cond);
                break;

            case KEY_NPAGE:
                shared->down_pressed += getmaxy(stdscr) - 2;
                pthread_cond_signal(&shared->display_cond);
                break;

            case KEY_UP:
                shared->down_pressed--;
                pthread_cond_signal(&shared->display_cond);
                break;

            case KEY_DOWN:
                shared->down_pressed++;
                pthread_cond_signal(&shared->display_cond);
                break;

            case KEY_END:
                shared->cursor = -1;
                shared->down_pressed = 0;
                pthread_cond_signal(&shared->display_cond);
                break;

            case KEY_HOME:
                shared->cursor = 0;
                shared->down_pressed = 0;
                pthread_cond_signal(&shared->display_cond);
                break;

            default:
                if (0x20 <= c && c < 0x7f) {
                    if (pos < width - 1) {
                        waddch(window, c);
                        line[pos++] = c;
                        line[pos] = 0;
                    }
                }
        }
        pthread_mutex_unlock(&shared->cursor_mutex);
    }

    free(line);
    if (close_connection(shared->socket) <  0) {
        fprintf(stderr, "Close connection error\n");
    }
    return 0;
}

static void prepare_message(rendered_message_t *msg, int window_width) {
    create_message(msg->buffer, MSG_BUFLEN, &msg->msg);
    int x = 0;
    msg->height = 0;
    for (char *c = msg->buffer; *c; ++c) {
        if (0x20 <= *c && *c <= 0x7f) {
            if (++x == window_width)
                msg->height++;
        } else if ('\n' == *c) {
            x = 0;
            msg->height++;
        }
    }
}

static int display_part(WINDOW *window, rendered_message_t *msg) {
    int maxy = getmaxy(window);
    char *c;
    for (c = msg->buffer; *c; ++c) {
        if (getcury(window) < maxy - 1) {
            waddch(window, *c);
        }
    }
    return getcury(window) < maxy - 1;
}

static int min(int a, int b) { return a < b ? a : b; }

static int max(int a, int b) { return a > b ? a : b; }

static int find_max_cursor_pos(list_t *head, int height) {
    int max_pos = 0;
    list_t *iter;
    for (iter = head; iter; iter = iter->next)
        max_pos += ((rendered_message_t *) iter->data)->height;
    return max(0, max_pos - height);
}

static int find_real_cursor_pos(int cursor, int max_pos) {
    return max(0, cursor < 0 ? max_pos : min(cursor, max_pos));
}

static int display_routine(client_shared_data_t *shared);
static void *display_routine_voidptr(void *ptr) {
    display_routine(ptr);
    return NULL;
}

int display_routine (client_shared_data_t *shared) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int winh = height - 2;
    int winw = width;
    WINDOW *window = newwin(winh + 1, winw, 0, 0);
    scrollok(window, TRUE);

    pthread_mutex_lock(&shared->history_mutex);
    while (shared->is_running) {
        pthread_cond_wait(&shared->display_cond, &shared->history_mutex);
        list_t *iter;
        for (iter = shared->history_head; iter; iter = iter->next) {
            rendered_message_t *msg = iter->data;
            if (msg->height == -1)
                prepare_message(msg, winw);
        }

        pthread_mutex_lock(&shared->cursor_mutex);
        int cursor = shared->cursor;
        int max_pos = find_max_cursor_pos(shared->history_head, winh);
        cursor = find_real_cursor_pos(cursor, max_pos);
        cursor = max(0, cursor + shared->down_pressed);
        cursor = find_real_cursor_pos(cursor, max_pos);

        if (shared->down_pressed) {
            shared->cursor = cursor;
            shared->down_pressed = 0;
        }
        pthread_mutex_unlock(&shared->cursor_mutex);

        iter = shared->history_head;
        int shift = 0;
        while (iter && shift < cursor) {
            shift += ((rendered_message_t *) iter->data)->height;
            iter = iter->next;
        }

        wclear(window);
        wmove(window, 0, 0);
        if (iter) {
            display_part(window, iter->data);
            wscrl(window, shift - cursor);
            iter = iter->next;
        }

        while (iter && display_part(window, iter->data))
            iter = iter->next;

        if (max_pos)
            wprintw(window, "%d lines above, %d below", shift, max_pos - shift);
        wrefresh(window);
    }
    pthread_mutex_unlock(&shared->history_mutex);

    delwin(window);
    return 0;
}

int run_client(int argc, char **argv) {
    if (argc < 5) {
        printf("%s","Invalid number of arguments for client mode\n");
        return -1;
    }

    client_shared_data_t shared;
    shared.socket = init_client_socket(argv[3], atoi(argv[4]));
    if (shared.socket !=  0) {
        fprintf(stderr, "Init client socket error\n");
    }
    if (send_msg(shared.socket, strlen(argv[2]), argv[2], 0) <  0) {
        fprintf(stderr, "Send msg error\n");
    }
    // инициализация conditional variable
    if (pthread_cond_init(&shared.display_cond, NULL) !=  0) {
        fprintf(stderr, "Init display_cond error\n");
    }
    // инициализация мьютексов для истории и курсора
    if (pthread_mutex_init(&shared.history_mutex, NULL) !=  0) {
        fprintf(stderr, "Init client history_mutex error\n");
    }
    if (pthread_mutex_init(&shared.cursor_mutex, NULL) !=  0) {
        fprintf(stderr, "Init client cursor_mutex error\n");
    }
    shared.history_head = NULL;
    shared.history_tail = NULL;
    shared.cursor = -1;
    shared.down_pressed = 0;
    shared.is_running = 1;
    // создание потоков
    if (pthread_create(&shared.receiver_thread, NULL,
                       receiver_routine_voidptr, &shared) !=  0) {
        fprintf(stderr, "Create receiver_thread error\n");
    }
    if (pthread_create(&shared.sender_thread, NULL,
                       sender_routine_voidptr, &shared) !=  0) {
        fprintf(stderr, "Create sender_thread error\n");
    }
    if (pthread_create(&shared.display_thread, NULL,
                       display_routine_voidptr, &shared) !=  0) {
        fprintf(stderr, "Create display_thread error\n");
    }

    pthread_join(shared.receiver_thread, NULL);
    pthread_join(shared.sender_thread, NULL);
    pthread_cond_signal(&shared.display_cond);
    pthread_join(shared.display_thread, NULL);
    pthread_mutex_destroy(&shared.history_mutex);
    pthread_mutex_destroy(&shared.cursor_mutex);
    return 0;
}
