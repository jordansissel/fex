
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *prog = NULL;

#define READBUFSIZE (64<<10)

typedef struct strlist {
  char **items;
  int nitems;
  int max_items;
} strlist_t;

void process_line(char *buf, int len, int argc, char **argv);
void extract(char *format, char *buf);
void tokenize(strlist_t **tokens, char *buf, char *sep);

strlist_t* strlist_new();
void strlist_free(strlist_t *list);
void strlist_append(strlist_t *list, char *str);

strlist_t* strlist_new() {
  strlist_t *list;
  list = malloc(sizeof(strlist_t));

  list->max_items = 10;
  list->nitems = 0;
  list->items = malloc(list->max_items * sizeof(char*));

  return list;
}

void strlist_free(strlist_t *list) {
  int i;
  for (i = 0; i < list->nitems; i++)
    free(list->items[i]);
  free(list);
}

void strlist_append(strlist_t *list, char *str) {
  list->items[list->nitems] = strdup(str);

  list->nitems++;
  if (list->nitems == list->max_items) {
    list->max_items *= 2;
    list->items = realloc(list->items, list->max_items * sizeof(char *));
  }
}

int main(int argc, char **argv) {
  char buf[READBUFSIZE];

  prog = *argv;
  argv++;
  argc--;

  memset(buf, 0, READBUFSIZE);


  if (0) { 
    strlist_t *tokens;
    tokenize(&tokens, "asdf asdf asdf one two three four", " ");
    int i;
    for (i = 0; i < tokens->nitems; i++) {
      printf("%d: %s\n", i, tokens->items[i]);
    }
    strlist_free(tokens);
    return 2;
  }

  while (NULL != fgets(buf, READBUFSIZE, stdin)) {
    int len;
    len = strlen(buf);
    len--; /* skip EOL */
    buf[len] = '\0'; /* Turn EOL into null */
    printf("String: '%s'\n", buf);
    process_line(buf, len, argc, argv);
  }

  return 0;
}

void process_line(char *buf, int len, int argc, char **argv) {
  int i;
  for (i = 0; i < argc; i++) {
    printf("%s\n", argv[i]);
    extract(argv[i], buf);
    printf(" ");
  }
  printf("\n");
}

void extract(char *format, char *buf) {
  char *sep = NULL;

  /* Because string literals aren't writable */
  sep = strdup(" ");

  printf("extract: %s\n", format);

  while (format[0] != '\0') {
    strlist_t *tokens;
    strlist_t *fields;
    char *fieldstr;

    tokenize(&tokens, buf, sep);

    /* All of these cases will advance the format string position */
    /* This if case is a reallly lame hack */
    if (isdigit(format[0]) || format[0] == '-') {
      asprintf(&fieldstr, "%ld", strtol(format, &format, 10));
    } else if (format[0] == '{') {
      format++; /* Skip '{' */
      int fieldlen;
      fieldlen = strcspn(format, "}") + 1;
      fieldstr = malloc(fieldlen * sizeof(char));
      memset(fieldstr, 0, fieldlen * sizeof(char));
      strncpy(fieldstr, format, fieldlen - 1);
    } else {
    }

    tokenize(&fields, fieldstr, ",");

    for (int i = 0; i < fields->nitems; i++) {
      int start, end;

    }
    free(fieldstr);

    //tmpbuf = tok;
    //if (format[0] != '\0') {
      ////printf("Shifting ahead by %d\n", tok - tmpbuf);
      //sep[0] = format[0];
      //format++;
    //}
    return;
    strlist_free(fields);
    strlist_free(tokens);
  }

  //printf("%s", tmpbuf);
  //fprintf(stderr, "Distance: %d vs %d\n", tmpbuf - tmpstart, strlen(buf));

  free(sep);
  //free(tmpstart);
}

void tokenize(strlist_t **tokens, char *buf, char *sep) {
  char *strptr = NULL;
  char *tokctx;
  char *dupbuf = NULL;
  char *tok;

  dupbuf = strdup(buf);
  strptr = dupbuf;

  *tokens = strlist_new();

  while ((tok = strtok_r(strptr, sep, &tokctx)) != NULL) {
    strptr = NULL;
    strlist_append(*tokens, tok);
  }
  free(dupbuf);
}
