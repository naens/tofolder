#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>

#include <pedit.h>

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("args: files and folders to move to a separate folder\n");
    return 1;
  }

  printf("input strings:\n");
  int ndirs = argc - 1;
  uint32_t *strs32[ndirs];
  for (int i = 0; i < ndirs; i++)
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

  for (int i = 0; i < ndirs; i++)
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
  printf("found longest string: \"%s\"\n", lcs);
  for (int i = 0; i < ls_cnt; i++)
    free(lss[i]);
  gst_free(gst);

  /* list of files containing the lcs substring */
  DIR *d = opendir(dir);
  struct dirent *dirent;
  if (!d)
    return -1;

  while ((dirent = readdir(d)) != NULL)
  {
    if (strstr(dirent->d_name, lcs) != NULL)
      printf("%s\n", dirent->d_name);
  }
  closedir(d);

  return 0;
}
