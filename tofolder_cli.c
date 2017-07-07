#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>

#include <ncurses.h>

#include <pedit.h>

void redraw(WINDOW *curr_win, WINDOW *win_src, WINDOW *win_dest, 
                int isrc, int idest, int nsrc, int ndest)
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
      if (curr_win == windows[w] && l == i[w])
        mvwchgat(windows[w], l, 0, COLS, A_REVERSE, 1, NULL);
      else
        mvwchgat(windows[w], l, 0, COLS, 0, 1, NULL);

  if (curr_win == win_dest && idest == ndest)
  {
    mvprintw(3, 0, "in field");
    wmove(stdscr, LINES - 1, 0);
    curs_set(1);
//    mvprintw(LINES - 1, 0, "========");
  }
  else
  {
    curs_set(0);
    mvprintw(3, 0, "        ");
  }


  refresh();
  wrefresh(win_src);
  wrefresh(win_dest);
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("args: files and folders to move to a separate folder\n");
    return 1;
  }

  printf("input strings:\n");
  int nargs = argc - 1;
  uint32_t *strs32[nargs];
  for (int i = 0; i < nargs; i++)
  {
    char *fn = basename(argv[i + 1]);
    int len8 = utf8strlen(fn);
    int fnlen = strlen(fn);
    printf("    \"%s\"\n", fn);

    strs32[i] = malloc(sizeof (uint32_t) * (len8 + 1));
    str_utf8_to_ucs32(fn, strs32[i]);
  }
  char *dir = dirname(argv[1]);
  int dirlen = strlen(dir);
  printf("dir: \"%s\"\n", dir);

  struct gst *gst = gst_new();

  for (int i = 0; i < nargs; i++)
  {
    printf("i=%d\n", i);
    gst_add_string(gst, strs32[i]);
  }

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
  printf("found longest string: \"%s\"\n", lcs);
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

     printf("%s\n", dirent->d_name);
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



  /* initialize ncurses */
  initscr();
  cbreak();	
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

  mvprintw(starty_src - 1, 0, "Files / directories to move");
  printw("  starty_src=%d starty_dest=%d LINES=%d",
                               starty_src, starty_dest, LINES);
  win_src = newwin(height_src, COLS, starty_src, 0);
  for (int i = 0; i < height_src; i++)
    mvwprintw(win_src, i, 0, "    x %s", src_fns[i]);

  mvprintw(starty_dest - 1, 0, "Destination");
  win_dest = newwin(height_dest, COLS, starty_dest, 0);
  for (int i = 0; i < height_dest; i++)
    mvwprintw(win_dest, i, 0, "    o %s", dest_dirs[i]);

  use_default_colors();

  int isrc = 0;
  int idest = 0;
  WINDOW *curr_win = win_src;

  redraw(curr_win, win_src, win_dest, isrc, idest, nsrc, ndest);
  int ch;
  while (ch = getch())
  {
    switch(ch)
    {
      case '\t':
        curr_win = (curr_win == win_src) ? win_dest : win_src;
        break;
      case KEY_UP:
        if (curr_win == win_src && isrc >= 1)
          isrc--;
        if (curr_win == win_dest && idest >= 1)
          idest--;
        break;
      case KEY_DOWN:
        if (curr_win == win_src && isrc < nsrc - 1)
          isrc++;
        if (curr_win == win_dest && idest < ndest)
          idest++;
        /* idest == ndest => cursor in new dir field (outside scroll) */
        break;
      case 'q':
        goto end;
    }
    redraw(curr_win, win_src, win_dest, isrc, idest, nsrc, ndest);
  }

end:
  endwin();

  return 0;
}
