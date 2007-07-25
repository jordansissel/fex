
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *prog = NULL;

#define READBUFSIZE (64<<10)

void process_line(char *buf, int len, int argc, char **argv);
void extract(char *format, char *buf);
void tokenize(char ***tokens, int *ntokens, char *buf, char *sep);
void freelist(char ***list);

int main(int argc, char **argv) {
  char buf[READBUFSIZE];
  char *sep = strdup(" ");
  char *line = strdup("Hello there how are you");
  char **tokens = NULL;
  int ntoks = 0;

  prog = *argv;
  argv++;
  argc--;

  memset(buf, 0, READBUFSIZE);

  //tokenize(&tokens, &ntoks, line, sep);
  //int i;
  //for (i = 0; i < ntoks; i++) {
    //printf("%d: %s\n", i, tokens[i]);
  //}
//
  //freelist(&tokens);

  while (NULL != fgets(buf, READBUFSIZE, stdin)) {
    int len;
    len = strlen(buf);
    len--; /* skip EOL */
    buf[len] = '\0'; /* Turn EOL into null */
    //printf("String: '%s'\n", buf);
    process_line(buf, len, argc, argv);
  }

  return 0;
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
  char *tmpbuf = NULL, *tmpstart;
  char *strptr = NULL;
  char *sep = NULL;
  int fieldnum;
  char **tokens = NULL;
  int ntokens = 0;

  /* Because string literals aren't writable */
  sep = strdup(" ");

  //printf("extract: %s\n", format);

  while (format[0] != '\0') {
    char *tok;

    /* if format[0] == '{', loop tokenizing on commas until '}' */
    /*
     * from '{'
     *   append field(s) to new string
     *   Fields are a number, or number:number, or -number
     *   :number and number: must be valid
     *   number:number is an inclusive range
     *
     */

    tokenize(&tokens, &ntokens, buf, sep);

    if (format[0] == '{') {
      char **fields = NULL;
      int nfields;
      int fieldlen;
      char *fieldstr;
      fieldlen = strcspn(format, "}") + 1;
      fieldstr = malloc(fieldlen * sizeof(char));
      memset(fieldstr, 0, fieldlen * sizeof(char));
      strncpy(fieldstr, format, fieldlen - 1);
      printf("Block: %s\n", fieldstr);
      free(fieldstr);
    }

    freelist(&tokens);

    tmpbuf = tok;
    if (format[0] != '\0') {
      //printf("Shifting ahead by %d\n", tok - tmpbuf);
      sep[0] = format[0];
      format++;
    }
  }

  printf("%s", tmpbuf);
  //fprintf(stderr, "Distance: %d vs %d\n", tmpbuf - tmpstart, strlen(buf));

  free(sep);
  free(tmpstart);
}

void tokenize(char ***tokens, int *ntokens, char *buf, char *sep) {
  int token_size = 10;
  char *strptr = NULL;
  char *tokctx;
  char *dupbuf = NULL;
  char *tok;

  *ntokens = 0;
  dupbuf = strdup(buf);
  strptr = dupbuf;

  *tokens = malloc(token_size * sizeof(char *));
  memset(*tokens, 0, token_size * sizeof(char *));

  while ((tok = strtok_r(strptr, sep, &tokctx)) != NULL) {
    strptr = NULL;
    (*tokens)[*ntokens] = strdup(tok);
    (*ntokens)++;

    /* Keep one spare token entry for a NULL entry */
    if (*ntokens == token_size - 1) {
      token_size *= 2;
      *tokens = realloc(tokens, token_size * sizeof(char *));
    }
  }

  /* Terminate the list with NULL */
  (*tokens)[*ntokens] = NULL;
  free(dupbuf);
}

void freelist(char ***list) {
  char **listp = *list;
  while (**list != NULL) {
    free(**list);
    (*list)++;
  }
  free(*listp);
}
