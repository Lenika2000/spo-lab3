#ifndef _UTIL_H_
#define _UTIL_H_

#include "net.h"
#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

typedef struct list {
  struct list *next;
  struct list *prev;
  void *data;
} list_t;

list_t *new_list(void *data);
list_t *list_insert_after(list_t *node, void *data);
list_t *list_insert_before(list_t *node, void *data);
void list_remove(list_t *node);
void create_message(char *buf, size_t buflen, server_message_t *msg);
void show_message(WINDOW *window, server_message_t *msg);

#endif
