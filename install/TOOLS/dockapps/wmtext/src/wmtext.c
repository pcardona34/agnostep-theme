#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <libdockapp/dockapp.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>
#include <locale.h>


const unsigned int update_interval = 100; /* in ms */
unsigned int cmd_update_interval = 1; /* in s */

const unsigned int cmd_size = 1000;
char *cmd;
const unsigned int cmd_output_size = 1000;
char *cmd_output;

const int size = 58;
char *title = "wmtext";
char *display_name = NULL;
Pixmap dock_pixmap;
int line_height; /* vertical spacing between lines */
XFontSet font_set;

typedef struct re_colour_list_elem {
  regex_t re;
  long unsigned bg_colour;
  long unsigned fg_colour;
  struct re_colour_list_elem *next;
} re_colour;
re_colour *re_colours = NULL;
long unsigned default_bg_colour;
long unsigned default_fg_colour;

time_t next_update_time;


void
perror_if(int cond, const char *msg) {
  if (cond) {
    perror(msg);
    exit(EXIT_FAILURE);
  }
}


void
die_if(int cond, const char *msg) {
  if (cond) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
  }
}


void
warn_if(int cond, const char *msg) {
  if (cond) fprintf(stderr, "%s\n", msg);
}


time_t
checked_time(time_t *t) {
  time_t res = time(t);
  perror_if(res == ((time_t) - 1), "time failed");
  return res;
}


void
update_cmd_output() {
  pid_t pid;
  int status;
  int pipefd[2];
  int res;

  res = pipe(pipefd);
  perror_if(res == -1, "update_cmd_output: pipe failed");
  pid = fork();
  perror_if(pid == -1, "update_cmd_output: fork failed");
  if (pid == 0) {
    res = dup2(pipefd[1], STDOUT_FILENO);
    perror_if(res == -1, "update_cmd_output: dup2 stdout failed");
    close(pipefd[0]);
    system(cmd);
    close(pipefd[1]);
    exit(0);
  } else {
    char *buf = cmd_output;
    close(pipefd[1]);
    while (buf < cmd_output + cmd_output_size - 1
           && read(pipefd[0], buf, 1) > 0)
      buf++;
    *buf = '\0';
    close(pipefd[0]);
    waitpid(pid, &status, 0);
  }
}


void
set_colours(unsigned long bg, unsigned long fg) {
  XGCValues gcv;

  gcv.foreground = bg;
  XChangeGC(DADisplay, DAClearGC, GCForeground, &gcv);
  gcv.background = bg;
  gcv.foreground = fg;
  XChangeGC(DADisplay, DAGC, GCBackground | GCForeground, &gcv);
}


void
update_colours() {
  re_colour *cursor = re_colours;
  while (cursor != NULL) {
    if (regexec(&(cursor->re), cmd_output, 0, NULL, 0) == 0) {
      set_colours(cursor->bg_colour, cursor->fg_colour);
      break;
    }
    cursor = cursor->next;
  }
  if (cursor == NULL) set_colours(default_bg_colour, default_fg_colour);
}


void
update() {
  int i, len;
  time_t t;
  char *bol, *eol;

  checked_time(&t);
  if (t >= next_update_time) {
    update_cmd_output();
    update_colours();
    next_update_time +=
      ((unsigned int)((t - next_update_time) / cmd_update_interval) + 1)
      * cmd_update_interval;
  }

  XFillRectangle(DADisplay, dock_pixmap, DAClearGC, 0, 0, size, size);

  i = 0;
  bol = cmd_output;
  while (1) {
    eol = strchr(bol, '\n');
    len = eol == NULL?  strlen(bol):  eol - bol;
    XmbDrawString(
      DADisplay, dock_pixmap, font_set, DAGC, 1, line_height * (i + 1),
      bol, len);
    if (bol[len] == '\0') break;
    bol += len + 1;
    i++;
  }

  DASetPixmap(dock_pixmap);
}


void
button_press(int button, int _state, int x, int y) {
  switch (button) {
  case 1:
    update_cmd_output();
    break;
  default:
    break;
  }
}


void
init_dock_app(int argc, char **argv) {
  DACallbacks callbacks = {
    NULL, /* destroy */
    &button_press, /* buttonPress */
    NULL, /* buttonRelease */
    NULL, /* motion (mouse) */
    NULL, /* mouse enters window */
    NULL, /* mouse leaves window */
    &update /* timeout */
  };

  /* with libdockapp 0.6.2, not calling DAParseArguments causes
   * _daContext not to be allocated and a crash further down the line */
  char *dummy_argv[1] = { "wmtext" };
  DAParseArguments(1, dummy_argv, NULL, 0, "", "");
  DASetExpectedVersion(20030126);
  DAInitialize(display_name, title, size, size, argc, argv);
  DAShow();
  DASetCallbacks(&callbacks);
  DASetTimeout(update_interval);
}


void
init_cmd() {
  cmd_output = malloc(cmd_output_size);
  perror_if(cmd_output == NULL, "malloc (cmd_output) failed");
  update_cmd_output();
  checked_time(&next_update_time);
}


const char *usage_msg =
  "Usage:\n"
  "  wmtext [-i cmd_update_interval_in_s]\n"
  "    [-b default_background_colour] [-f default_foreground_colour]\n"
  "    [-font font_name]\n"
  "    [-a regular_expression background_colour foreground_colour]...\n"
  "    'command args...'";

int
main(int argc, char **argv) {
  int i = 1, ret, cmd_seen = 0;
  char *bg_s = "black", *fg_s = "white";
  re_colour **cursor = &re_colours;
  XFontSetExtents *extents;
  char *locale;
  char *font_name = "fixed";
  char **missing_charset_list;
  int missing_charset_count;
  char *def_string;

  init_dock_app(argc, argv);

  locale = getenv("LC_ALL");
  if (locale == NULL) locale = getenv("LANG");
  setlocale(LC_ALL, locale);

  while (i < argc) {
    if (strcmp(argv[i], "-i") == 0) {
      die_if(i + 1 >= argc, usage_msg);
      cmd_update_interval = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-b") == 0) {
      die_if(i + 1 >= argc, usage_msg);
      bg_s = argv[++i];
    } else if (strcmp(argv[i], "-f") == 0) {
      die_if(i + 1 >= argc, usage_msg);
      fg_s = argv[++i];
    } else if (strcmp(argv[i], "-font") == 0) {
      die_if(i + 1 >= argc, usage_msg);
      font_name = argv[++i];
    } else if (strcmp(argv[i], "-a") == 0) {
      die_if(i + 2 >= argc, usage_msg);
      *cursor = malloc(sizeof(re_colour));
      perror_if(*cursor == NULL, "malloc (re_colour) failed");
      ret = regcomp(&((*cursor)->re), argv[++i], REG_EXTENDED);
      die_if(ret != 0, "invalid regular expression (regcomp failed)");
      (*cursor)->bg_colour = DAGetColor(argv[++i]);
      (*cursor)->fg_colour = DAGetColor(argv[++i]);
      (*cursor)->next = NULL;
      cursor = &((*cursor)->next);
    } else {
      die_if(i + 1 != argc , usage_msg); /* there are extra words */
      cmd = argv[i];
      cmd_seen = 1;
    }
    i++;
  }
  die_if(cmd_seen == 0, usage_msg);

  font_set = XCreateFontSet(
    DADisplay, font_name,
    &missing_charset_list, &missing_charset_count, &def_string);
  die_if(font_set == NULL, "XCreateFontSet failed");
  extents = XExtentsOfFontSet(font_set);
  line_height = extents->max_logical_extent.height;
  default_bg_colour = DAGetColor(bg_s);
  default_fg_colour = DAGetColor(fg_s);
  set_colours(default_bg_colour, default_fg_colour);
  dock_pixmap = DAMakePixmap();
  init_cmd();
  DAEventLoop();
  XFreePixmap(DADisplay, dock_pixmap);

  return 0;
}
