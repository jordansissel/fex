/*
 * Field EXtraction tool
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef NEED_SNPRINTF_2_2
#define NEED_ASPRINTF
#include <stdarg.h>
#include "snprintf_2.2/snprintf.h"
#endif

#include <sys/types.h>
#include <regex.h>

#include "fex.h"
#include "fex_version.h"

/* Options and Todos
 * What if fieldnum > fields?
 *   1) Error
 *   2) Empty (current behavior)
 *   3) Return last field
 *
 * Set output field separator?
 *
 * Support '3N' aka, return every i[3 * n] for (3*n < num_fields) ?
 */

static char *prog = NULL;

#define READBUFSIZE (64<<10)

void usage();
void apply_range_selector(const char *field, const strlist_t *tokens,
                          const char *buffer, strlist_t *results);
void apply_pattern_selector(const char *field, const strlist_t *tokens,
                            const char *buffer, strlist_t *results);

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

void strlist_append(strlist_t *list, const char *str) {
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

  if (argc == 1 && !strcmp(*argv, "-v")) {
    printf("%s\n", FEX_VERSION);
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
  printf("usage: %s [extract1 ... extractN]\n", prog);
  printf("Extract syntax is:\n"
         "  <separator><field number(s)>\n"
         "\n"
         "Fields start at 1, awk style. A field number of 0 means the whole\n"
         "string unchanged.\n"
         "\n"
         "The first separator is implied as space ' '\n"
         "You can specify multiple fields with curly braces and numbers split\n"
         "by commas. Also valid in curly braces {} are number ranges. Number\n"
         "ranges are similar to python array slices, split by colon.\n"
         "\n"
         "The first piece of your extraction code should be a number, since\n"
         "there is always an implied separator of space.\n"
         "Some examples:\n"
         "  1.1    First split by ' ', then first by '.'\n"
         "      'foo.bar baz' by '1.1' outputs 'foo'\n"
         "  0:{1,-1}    Output the first and last split by ':'\n"
         "      'foo:bar:baz:fizz' by '0:{1,-1}' outputs 'foo:fizz'\n"
         "  {1:3}     Output tokens 1 through 3\n"
         "      'foo bar baz fizz' by '{1:3}' outputs 'foo bar baz'\n"
         "  :/home/    First split by ':' and yield only fields matching the\n"
         "             regexp /home/'\n"
         "\n"
         " * Make sure you quote your extractions, or your shell may perform\n"
         "   some unintended expansion\n");
}

void process_line(char *buf, int len, int argc, char **argv) {
  int i = 0;
  char *field = NULL;
  int length = 0; /* track line length */
  for (i = 0; i < argc; i++) {
    field = extract(argv[i], buf);
    printf("%s", field);
    length += strlen(field);
    free(field);
    if (i < argc - 1) {
      length += 1;
      printf(" ");
    }
  }

  if (length > 0) {
    /* only print newlines if there's a field emitted to output */
    printf("\n");
  }
  fflush(stdout);
}

char *extract(char *format, char *buf) {
  char *sep = NULL;

  char *buffer = strdup(buf);
  int nbuffer = 0;
  int buffer_size = 1024;
  char *(*tokenizer)(char *, const char*, char **);

  /* strdup() because string literals aren't writable */
  sep = strdup(" ");

  /* If first char is not a number or '{', use it to split instead of the
   * default of space */
  if ((!isdigit(format[0]) && (format[0] != '{') && (format[0] != '-')) || (format[1] == '{' || format[1] == '/')) {
    sep[0] = format[0];
    format++;
  }

  //printf("extract: %s\n", format);

  while (format[0] != '\0') {
    strlist_t *tokens;
    strlist_t *fields;
    strlist_t *results;
    int use_regexp = 0;
    char *fieldstr;

    tokenizer = tokenizer_greedy;

    results = strlist_new();

    /* All of these cases will advance the format string position */
    /* This if case is a reallly lame hack */
    if (isdigit(format[0]) || (format[0] == '-' && isdigit(format[1]))) {
      /* the current format is a number (or negative number) */
      asprintf(&fieldstr, "%ld", strtol(format, &format, 10));

      /* fieldstr is the field selector(s). ie; "1" in "a1" */
      split(&fields, fieldstr, ",", tokenizer_greedy);
      free(fieldstr);
    } else if (format[0] == '{') {
      /* the current format marks the start of a group selector */
      int fieldlen;
      format++; /* Skip '{' */
      if (format[0] == '?') {
        //printf("Nongreedy\n");
        format++;
        tokenizer = tokenizer_nongreedy;
      }
      fieldlen = strcspn(format, "}") + 1;
      if (format[fieldlen - 1] != '}') {
        fprintf(stderr, "Could not find closing '}'. Bad format: %s\n",
               (format - 1));
        exit(1);
      }

      fieldstr = malloc(fieldlen * sizeof(char));
      memset(fieldstr, 0, fieldlen * sizeof(char));
      strncpy(fieldstr, format, fieldlen - 1);
      format += fieldlen;

      /* fieldstr is the field selector(s). ie; "1,3" in a{1,3} */
      split(&fields, fieldstr, ",", tokenizer_greedy);
      free(fieldstr);
    } else if (format[0] == '/') {
      /* the current format char marks the start of a pattern selector */
      int patternlen = 0;
      format++; /* Skip '/' */
      patternlen = strcspn(format, "/") + 1;
      if (format[patternlen - 1] != '/') {
        fprintf(stderr, "Could not find closing '/'. Bad format: %s\n",
                (format - 1));
        exit(1);
      }

      /* copy the pattern string (things between the '/' foo '/') */
      fieldstr = calloc(patternlen, sizeof(char));
      strncpy(fieldstr, format, patternlen - 1); /* copy up to the trailing '/' */

      /* store the pattern string for applying later */
      use_regexp = 1;
      fields = strlist_new();
      strlist_append(fields, fieldstr);

      format += patternlen;
    } else {
      /* Error, this format is invalid? */
      fprintf(stderr, "Invalid format... %s\n", format);
      exit(1);
    }

    /* Split the input by sep using tokenizer */
    split(&tokens, buffer, sep, tokenizer);

    int i = 0;
    //printf("\n");
    for (i = 0; i < fields->nitems; i++) {
      char *field = fields->items[i];
      //printf("field: %s\n", field);
      //for (int j = 0; j < tokens->nitems; j++) {
        //printf("  token[%d]: %s\n", j, tokens->items[j]);
      //}
      if (use_regexp) {
        apply_pattern_selector(field, tokens, buffer, results);
      } else {
        apply_range_selector(field, tokens, buffer, results);
      }
      
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

  free(sep);
  return buffer;
}

void split(strlist_t **tokens, const char *buf, const char *sep, 
           char *(*tokenizer)(char *, const char*, char **)) {

  char *strptr = NULL;
  char *tokctx;
  char *dupbuf = NULL;
  char *tok;

  dupbuf = strdup(buf);
  strptr = dupbuf;

  *tokens = strlist_new();

  //printf("Split: '%s' on '%s'\n", buf, sep);
  while ((tok = tokenizer(strptr, sep, &tokctx)) != NULL) {
    strptr = NULL;
    strlist_append(*tokens, tok);
  }
  free(dupbuf);
}

char *tokenizer_greedy(char *str, const char *sep, char **last) {
  return strtok_r(str, sep, last);
}

char *tokenizer_nongreedy(char *str, const char *sep, char **last) {
  int next_tok;

  if (str == NULL) {
    str = *last;
  } else {
    *last = str;
  }

  if (*str == '\0') {
    return NULL;
  }
  //printf("strtok '%s' '%s'\n", str, sep);
  next_tok = strcspn(str, sep);

  str[next_tok] = '\0';
  *last += next_tok + 1;
  //printf("str: %s\n", str);
  //printf("  Pos: %d\n", next_tok);
  //printf("  remain: %s\n", *last);
  return str;
}

void apply_range_selector(const char *field, const strlist_t *tokens,
                          const char *buffer, strlist_t *results) {
  long start, end;
  strlist_t *range;
  split(&range, field, ":", tokenizer_greedy);

  if (range->nitems == 1) {
    /* Support {N} and {N:} */
    start = end = strtol(range->items[0], NULL, 10);

    /* Support {:N} */
    if (field[strlen(field) - 1] == ':')
      end = (start > 0) ? tokens->nitems : 0;

    /* Support {N:} */
    if (field[0] == ':')
      start = 1;
  } else if (*field == ':') { 
    /* Support {:} as whole all fields */
    start = 0;
    end = 0;
  } else {
    start = strtol(range->items[0], NULL, 10);
    end = strtol(range->items[1], NULL, 10);
  }

  /* Add 1 here because field indexing starts at 1, not 0 */
  if (start < 0) {
    start += tokens->nitems + 1;
    /* For sanity, negative indexing shouldn't result in <= 0 values. */
    /* XXX: Throw an error for a bad index? */
    if (start < 0)
      start = 1;
  }

  if (end < 0) {
    end += tokens->nitems + 1;
    if (end < 0)
      end = start;
  }

  //printf("s/e= %ld %ld\n", start, end);

  /* If end is 0, and set end to start. End of 0 doesn't make sense */
  if (end == 0)
    end = start;

  //printf("%ld %ld\n", start, end);

  if (start > end) {
    fprintf(stderr, "start > end is invalid: %ld > %ld\n", start, end);
    exit(1);
  }

  if (((start == 0) && (end != 0))
      || ((start != 0) && (end == 0))) {
    fprintf(stderr, 
            "Start or end cannot be 0 when the other is not 0: %ld "
            "and %ld\n", start, end);
    exit(1);
  }

  /* We start indexing at 1, zeor means 'the entire buffer' */
  if (start == 0) {
    strlist_append(results, buffer);
  } else {
    start--;
    end--;

    int i = 0;
    for (i = start; i < tokens->nitems && i <= end; i++) {
      strlist_append(results, tokens->items[i]);
    }
  }
  strlist_free(range);
}

void apply_pattern_selector(const char *field, const strlist_t *tokens,
                            const char *buffer, strlist_t *results) {
  regex_t regex;
  int rc;
  rc = regcomp(&regex, field, REG_EXTENDED | REG_ICASE);
  if (rc != 0) {
    size_t errbuf_size = 1024;
    size_t len = 0;
    char *errbuf = NULL;
    errbuf = calloc(errbuf_size, 1);
    len = regerror(rc, &regex, errbuf, errbuf_size);
    fprintf(stderr, "Regex compile failed (code %d): %.*s\n", rc, (int)len, errbuf);
    exit(1); /* exiting from a nested call is really bad style, man... */
  }

  int i = 0;
  for (i = 0; i < tokens->nitems; i++) {
    rc = regexec(&regex, tokens->items[i], 0, NULL, 0);
    if (rc == 0) { /* match successful */
      strlist_append(results, tokens->items[i]);
    }
  }
  regfree(&regex);
}
