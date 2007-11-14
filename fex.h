/*
 * Field EXtraction tool
 */

#define _GNU_SOURCE

typedef struct strlist {
  char **items;
  int nitems;
  int max_items;
} strlist_t;

void process_line(char *buf, int len, int argc, char **argv);
char *extract(char *format, char *buf);
void split(strlist_t **tokens, char *buf, char *sep, 
           char *(*tokenizer)(char *, const char*, char **));
char *tokenizer_greedy(char *str, const char *sep, char **last);
char *tokenizer_nongreedy(char *str, const char *sep, char **last);

strlist_t* strlist_new();
void strlist_free(strlist_t *list);
void strlist_append(strlist_t *list, char *str);
