#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>

#include <ncurses.h>
#include <form.h>

#include <pedit.h>

#define ctrl(x) ((x) & 0x1f)

void redraw(WINDOW *curr_win, WINDOW *win_src, WINDOW *win_dest, WINDOW* win_form,
                int isrc, int idest, int *cksrc, int ckdest, int nsrc, int ndest, 
                FORM  *form, FIELD *field[2])
{
  mvprintw(0, 0, "curr_win: %s ", (curr_win == win_src) ? "win_src" : "win_dest");
  mvprintw(1, 0, "isrc=%d  ", isrc);
  mvprintw(2, 0, "idest=%d  ", idest);

  WINDOW *windows[2];
  windows[0] = win_src;
  windows[1] = win_dest;
  int n[2];
  n[0] = nsrc;
  n[1] = ndest;
  int i[2];
  i[0] = isrc;
  i[1] = idest;
  for (int w = 0; w < 2; w++)
    for (int l = 0; l < n[w]; l++)
    {
      if (windows[w] == win_src)
       if (cksrc[l])
          mvwprintw(win_src, l, 0, "   x");
        else
          mvwprintw(win_src, l, 0, "    ");

      if (windows[w] == win_dest)
        if (ckdest == l)
          mvwprintw(win_dest, l, 0, "   o");
        else
          mvwprintw(win_dest, l, 0, "    ");

      if (curr_win == windows[w] && l == i[w])
        mvwchgat(windows[w], l, 0, COLS, A_REVERSE, 1, NULL);
      else
        mvwchgat(windows[w], l, 0, COLS, 0, 1, NULL);
    }
  if (ckdest == ndest)
    set_field_buffer(field[0], 0, "   o NEW: ");
  else
    set_field_buffer(field[0], 0, "     NEW: ");

  refresh();
  wrefresh(win_src);
  wrefresh(win_dest);
//  wrefresh(win_form);
  refresh();
}

int field_loop(WINDOW *form_window, FORM *form, FIELD *field[])
{
  WINDOW *w = form_win(form);
  curs_set(1);
  set_current_field(form, field[1]);
  form_driver(form, 0); // move cursor to field
  int cq = 0;
  int ch;
  for (;;)
  {
    ch = wgetch(w);
    switch(ch)
    {
      case ctrl('q'):
        cq = !cq;
        break;
      case ctrl('e'):
      case KEY_UP:
      case '\t':
      case ' ':
      case KEY_ENTER:
      case '\r':
      case '\n':
        curs_set(0);
        return ch;
      case ctrl('s'):
      case KEY_LEFT:
        form_driver(form, REQ_PREV_CHAR);
        break;
      case ctrl('d'):
      case KEY_RIGHT:
        form_driver(form, REQ_NEXT_CHAR);
        break;
      case ctrl('g'):
        form_driver(form, REQ_DEL_CHAR);
        break;
      case KEY_BACKSPACE:
        form_driver(form, REQ_DEL_PREV);
        break;
      case 's':
        if (!cq)
          goto default_case;
        form_driver(form, REQ_BEG_FIELD);
        cq = 0;
        break;
      case 'd':
        if (!cq)
          goto default_case;
        form_driver(form, REQ_END_FIELD);
        cq = 0;
        break;
      case 'r':
        if (!cq)
          goto default_case;
        return 30;
default_case:
      default:
        form_driver(form, ch);
        refresh();
        wrefresh(w);
    }
  }
}

void endcurses(FORM *form, FIELD **field)
{
  unpost_form(form);
  free_form(form);
  free_field(field[0]);
  free_field(field[1]); 
  endwin();
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("args: files and folders to move to a separate folder\n");
    return 1;
  }

//  printf("input strings:\n");
  int nargs = argc - 1;
  uint32_t *strs32[nargs];
  for (int i = 0; i < nargs; i++)
  {
    char *fn = basename(argv[i + 1]);
    int len8 = utf8strlen(fn);
    int fnlen = strlen(fn);
//    printf("    \"%s\"\n", fn);

    strs32[i] = malloc(sizeof (uint32_t) * (len8 + 1));
    str_utf8_to_ucs32(fn, strs32[i]);
  }
  char *dir = dirname(argv[1]);
  int dirlen = strlen(dir);
//  printf("dir: \"%s\"\n", dir);

  struct gst *gst = gst_new();

  for (int i = 0; i < nargs; i++)
    gst_add_string(gst, strs32[i]);

  int ls_cnt;
  uint32_t **lss;
  gst_get_longest_strings(gst, &ls_cnt, &lss);

  if (ls_cnt == 0)
  {
    printf("no longest string found\n");
    gst_free(gst);
    return 0;
  }
  
  char lcs[0x1000];
  str_ucs32_to_utf8(lss[0], lcs);
//  printf("found longest string: \"%s\"\n", lcs);
  for (int i = 0; i < ls_cnt; i++)
    free(lss[i]);
  gst_free(gst);

  /* list of files containing the lcs substring */
  DIR *d = opendir(dir);
  struct dirent *dirent;
  if (!d)
    return -1;

  /* count */
  int nfiles = 0;
  int ndirs = 0;
  while ((dirent = readdir(d)) != NULL)
  {
    if (strstr(dirent->d_name, lcs) == NULL)
      continue;
     struct stat statbuf;
     if (stat(dirent->d_name, &statbuf) != 0)
       return -1;
     if (S_ISREG(statbuf.st_mode))
       nfiles++;
     else if (S_ISDIR(statbuf.st_mode))
       ndirs++;

//     printf("%s\n", dirent->d_name);
  }

  /* get names */
  rewinddir(d);
  char *src_fns[nfiles + ndirs];
  char *dest_dirs[ndirs];
  int nsrc = 0;
  int ndest = 0;
  while ((dirent = readdir(d)) != NULL)
  {
    if (strstr(dirent->d_name, lcs) == NULL)
      continue;
     struct stat statbuf;
     if (stat(dirent->d_name, &statbuf) != 0)
       return -1;
     if (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode))
     {
       src_fns[nsrc] = malloc(strlen(dirent->d_name) + 1);
       strcpy(src_fns[nsrc], dirent->d_name);
       nsrc++;
     }
     if (S_ISDIR(statbuf.st_mode))
     {
       dest_dirs[ndest] = malloc(strlen(dirent->d_name) + 1);
       strcpy(dest_dirs[ndest], dirent->d_name);
       ndest++;
     }
  }

  closedir(d);

  /* TODO: special cases: no files to move, no potential destination dir*/

  /* initialize ncurses */
  initscr();
  cbreak();	
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  /* initialize values */
  int lines_src = nsrc;
  int lines_dest = ndest;
  int height_tot;
  int starty_src;
  int starty_dest;
  int height_src;
  int height_dest;

  if (lines_src + lines_dest + 4 < LINES)
  {
    height_tot = 1 + lines_src + 1 + 1 + lines_dest + 1;
    starty_src = LINES - height_tot + 1;
    height_src = lines_src;
    height_dest = lines_dest;
    starty_dest = (LINES - height_tot) + height_src + 2 + 1;
  }
  else 
  {
    height_tot = LINES;
    starty_src = 1;
    if (lines_src < (LINES / 2 - 4))
    {
      height_src = lines_src;
      height_dest = height_tot - height_src - 4;
    }
    else if (lines_dest < (LINES / 2 - 4))
    {
      height_dest = lines_dest;
      height_src = height_tot - height_dest - 4;
    }
    else
    {
      height_src = LINES / 2 - 2;
      height_dest = height_tot - height_src - 4;
    }
    starty_dest = height_tot - height_dest - 1;
  }


  /* draw windows */
  WINDOW *win_src;
  WINDOW *win_dest;
  WINDOW *win_form;
  FORM  *form;
  FIELD *field[3];
  int label_width = 10;
  int field_row = LINES - 1;
  int field_begin = label_width;
  int field_width = COLS - label_width;

  mvprintw(starty_src - 1, 0, "Files / directories to move");
  printw("  starty_src=%d starty_dest=%d LINES=%d",
                               starty_src, starty_dest, LINES);
  win_src = newwin(height_src, COLS, starty_src, 0);
  for (int i = 0; i < height_src; i++)
    mvwprintw(win_src, i, 0, "     %s", src_fns[i]);

  mvprintw(starty_dest - 1, 0, "Destination");
  win_dest = newwin(height_dest, COLS, starty_dest, 0);
  for (int i = 0; i < height_dest; i++)
    mvwprintw(win_dest, i, 0, "     %s", dest_dirs[i]);

  use_default_colors();



  win_form = newwin(1, COLS, field_row, 0);
  keypad(win_form, TRUE);
//  mvwprintw(win_form, 0, 0, "NNN  new: ");
//  wrefresh(win_form);
//  refresh();

  field[0] = new_field(1, label_width, field_row, 0, 0, 0);
  field_opts_off(field[0], O_ACTIVE);
  set_field_buffer(field[0], 0, "     NEW: ");

  field[1] = new_field(1, field_width, field_row, field_begin, 0, 0);
  field_opts_off(field[1], O_AUTOSKIP);
//  field[1] = new_field(1, COLS - field_begin - 1, field_row, field_begin, 0, 0);
  set_field_buffer(field[1], 0, lcs);
  field_opts_off(field[1], O_BLANK); 

  field[2] = NULL;

  form = new_form(field);
  post_form(form);
  set_form_win(form, win_form);
  set_form_sub(form, win_form);

  refresh();

  int isrc = 0;
  int idest = 0;
  WINDOW *curr_win = win_src;

  int cksrc[nsrc];
  for (int i = 0; i < nsrc; i++)
    cksrc[i] = 1;
  int ckdest = 0;

  redraw(curr_win, win_src, win_dest, win_form,
             isrc, idest, cksrc, ckdest, nsrc, ndest, form, field);

  int cq = 0;
  int ch = getch();
  for (;;)
  {
    switch(ch)
    {
      case ctrl('q'):
        cq = !cq;
        break;
      case '\t':
        curr_win = (curr_win == win_src) ? win_dest : win_src;
        break;
      case ' ':
        if (curr_win == win_src)
        /* TODO: uncheck dest (check new?) if src selected same as dest */
          cksrc[isrc] = !cksrc[isrc];
        else
        /* TODO: uncheck src if dest selected same as src */
          ckdest = idest;
        break;
      case ctrl('e'):
      case KEY_UP:
        if (curr_win == win_src && isrc >= 1)
          isrc--;
        if (curr_win == win_dest && idest >= 1)
          idest--;
        break;
      case ctrl('x'):
      case KEY_DOWN:
        if (curr_win == win_src && isrc < nsrc - 1)
          isrc++;
        if (curr_win == win_dest && idest < ndest)
          idest++;
        /* idest == ndest => cursor in new dir field (outside scroll) */
        break;
      case 'r':
        if (!cq)
          goto default_case;
      case 30:
        cq = 0;
        if (curr_win == win_src)
          isrc = 0;
        if (curr_win == win_dest)
          idest = 0;
        break;
      case 'c':
        if (!cq)
          goto default_case;
        cq = 0;
        if (curr_win == win_src)
          isrc = nsrc - 1;
        if (curr_win == win_dest)
          idest = ndest;
        break;
      case KEY_ENTER:
      case '\r':
      case '\n':
        goto work;
      case ctrl('u'):
      case 'q':
        endcurses(form, field);
        goto end;
default_case:
      default:
        break;
    }
    if (curr_win == win_dest && idest == ndest)
      ckdest = ndest;
    redraw(curr_win, win_src, win_dest, win_form,
               isrc, idest, cksrc, ckdest, nsrc, ndest, form, field);
    if (curr_win == win_dest && idest == ndest)
    {
      mvprintw(6, 0, "form enter");
      refresh();
      ch = field_loop(win_form, form, field);
      mvprintw(6, 0, "form exit ");
      refresh();
    }
    else
    ch = getch();
  }

work:
;
  char *dest_string;
  if (idest == ndest)
  {
    /* get field value */
    form_driver(form, REQ_VALIDATION);
    char *tmp = field_buffer(field[1], 0);
    int last_chr = field_width - 1;
    while (tmp[last_chr] == ' ')
      last_chr--;
    int len = last_chr + 1;
    tmp[len] = 0;
    char *field_string = malloc(len + 1);
    strcpy(field_string, tmp);
    dest_string = field_string;
  }
  else
  {
    dest_string = dest_dirs[idest];
  }
  endcurses(form, field);
  /* TODO: special cases: no right to move, no files to move */
  printf("move files:\n");
  for (int i = 0; i < nsrc; i++)
    if (cksrc[i])
      printf("    %s/%s\n", dir, src_fns[i]);
  printf("to %s/\n", dest_string);
  printf("OK? (y/n) ");
  scanf(" %c",&ch);
  if (ch == 'y' || ch == 'Y')
  {
    printf("#### moving ####\n");
    struct stat st;
    if (stat(dest_string, &st) != 0)
//      mkdir(dest_string, 0777);
      printf("making directory %s\n", dest_string);
    else if (st.st_mode & S_IFDIR == 0)
    {
      printf("%s exists and is not a directory\n", dest_string);
      goto end;
    }
    int dest_len = strlen(dest_string);
    for (int i = 0; i < nsrc; i++)
      if (cksrc[i])
      {
        /* from path */
        int flen = strlen(src_fns[i]);
        char *from_path = malloc(dirlen + 1 + flen + 1);
        sprintf(from_path, "%s/%s", dir, src_fns[i]);

        /* to path */
        char *to_path;
        /* if dest starts with '/' then absolute path otherwise relative */
        if (dest_string[0] == '/')
        {
          to_path = malloc(dest_len + 1 + flen + 1);
          sprintf(to_path, "%s/%s", dest_string, src_fns[i]);
        }
        else
        {
          to_path = malloc(dirlen + 1 + dest_len + 1 + flen + 1);
          sprintf(to_path, "%s/%s/%s", dir, dest_string, src_fns[i]);
        }
        printf("moving %s to %s\n", from_path, to_path);
//        rename(from_path, to_path);
        free(from_path);
        free(to_path);
      }
  }
  else
    printf("do not move\n");

  if (idest == ndest)
    free(dest_string);

end:
  return 0;
}
