
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

void usage();
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
  free(list->items);
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

  if (argc == 0 || !strcmp(*argv, "-h")) {
    usage();
    return 0;
  }

  while (NULL != fgets(buf, READBUFSIZE, stdin)) {
    int len;
    len = strlen(buf);
    len--; /* skip EOL */
    buf[len] = '\0'; /* Turn EOL into null */
    process_line(buf, len, argc, argv);
  }

  return 0;
}

void usage() {
  printf("usage: %s [extract1 ... extractN]\n"
         "\n"
         "Extract syntax is:\n"
         "  <separator><field number(s)>\n"
         "\n"
         "Fields start at 1, awk style. A field number of 0 means the whole\n"
         "string unchanged.\n"
         "The first separator is implied as space ' '\n"
         "You can specify multiple fields with curly braces and numbers split\n"
         "by commas. Also valid in curly braces {} are number ranges. Number\n"
         "ranges are similar to python array slices, split by colon.\n"
         "\n"
         "The first piece of your extraction code should be a number, since\n"
         "there is always an implied separator of space.\n"
         "Some examples:\n"
         "  1.1    First split by ' ', then first by '.'\n"
         "      'foo.bar baz' by '1.1' outputs 'bar'\n"
         "  0:{1,-1}    Output the first and last split by ';'\n"
         "      'foo:bar:baz:fizz' by '0:{1,-1}' outputs 'foo:fizz'\n"
         "  {1:3}     Output tokens 1 through 3\n"
         "      'foo bar baz fizz' by '{1:3}' outputs 'foo bar baz'\n"
         "\n"
         " * Make sure you quote your extractions, or your shell may perform\n"
         "some unintended expansion\n"
        , prog);
}

void process_line(char *buf, int len, int argc, char **argv) {
  int i;
  for (i = 0; i < argc; i++) {
    extract(argv[i], buf);
    printf(" ");
  }
  printf("\n");
}

void extract(char *format, char *buf) {
  char *sep = NULL;

  char *buffer = strdup(buf);
  int nbuffer = 0;
  int buffer_size = 1024;

  /* Because string literals aren't writable */
  sep = strdup(" ");

  //printf("extract: %s\n", format);

  while (format[0] != '\0') {
    strlist_t *tokens;
    strlist_t *fields;
    strlist_t *results;
    char *fieldstr;

    //printf("B: %s\n", buffer);
    results = strlist_new();
    tokenize(&tokens, buffer, sep);

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
      format += fieldlen;
    } else {
    }

    tokenize(&fields, fieldstr, ",");
    free(fieldstr);

    int i = 0;
    for (i = 0; i < fields->nitems; i++) {
      long start, end;
      strlist_t *range;
      char *field = fields->items[i];
      tokenize(&range, field, ":");

      if (*field == ':') {
        start = 0;
        end = 0;
      } else if (range->nitems == 1) {
        start = end =  strtol(range->items[0], NULL, 10);
        if (field[strlen(field) - 1] == ':')
          end = (start > 0) ? tokens->nitems : 0;
      } else {
        start = strtol(range->items[0], NULL, 10);
        end = strtol(range->items[1], NULL, 10);
      }

      int j;
      if (start > end) {
        fprintf(stderr, "start > end is invalid: %ld > %ld\n", start, end);
        exit(1);
      }

      if ((start < 0 && end > 0) 
          || (end < 0 && start > 0)) {
        fprintf(stderr, "start and end must be both positive or both negative: %ld and %ld\n", start, end);
        exit(1);
      }

      if (start == 0 && end) {
        fprintf(stderr, "Start of '0' is invalid when an endpoint is specified: %ld and %ld\n", start, end);
        exit(1);
      }

      /* We start indexing at 1. */
      if (start == 0) {
        strlist_append(results, buffer);
      } else {
        start--;
        end--;

        if (start < 0) {
          /* Add 1 because start of 1 means token index 0 in C */
          start += tokens->nitems + 1;
          end += tokens->nitems + 1;
        }

        if (start < 0)
          start = 0;
        if (end == 0)
          end = start;

        for (j = start; j < tokens->nitems && j <= end; j++) {
          strlist_append(results, tokens->items[j]);
        }
      }
      strlist_free(range);
    }

    /* Free buffer then allocate a new one for the new string slice */
    free(buffer);
    buffer_size = 1024;
    nbuffer = 0;
    buffer = malloc(buffer_size);
    memset(buffer, 0, buffer_size);
    for (i = 0; i < results->nitems; i++) {
      char *item = results->items[i];
      int len = strlen(item);
      if (len + nbuffer > buffer_size) {
        buffer_size = buffer_size * 2 + len + 1;
        buffer = realloc(buffer, buffer_size);
      }

      strncpy(buffer + nbuffer, item, len);
      nbuffer += len;

      if (i < results->nitems - 1) {
        buffer[nbuffer] = *sep;
        nbuffer++;
      }
    }

    if (format[0] != '\0') {
      sep[0] = format[0];
      format++;
    }
    strlist_free(fields);
    strlist_free(tokens);
    strlist_free(results);
  }

  printf("%s", buffer);
  free(buffer);
  free(sep);
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
