#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <pedit.h>

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("args: files and folders to move to a separate folder\n");
    return 1;
  }

  uint32_t *strs32[argc - 1];
  for (int i = 0; i < argc - 1; i++)
  {
//    printf("i=%d\n", i);
    uint8_t *utf8str = argv[i + 1];
//    printf("utf8str=%s\n", utf8str);
    int sz = strlen(utf8str);
    printf("sz=%d\n", sz);
    int len8 = utf8strlen(utf8str);
    printf("len8=%d\n", len8);
//    printf("sz=%d chars=", sz);
//    for (int j = 0; j < sz; j++)
//      printf("%02x ", utf8str[j]);
//    printf("\n");

    strs32[i] = malloc(sizeof (uint32_t) * len8);
    str_utf8_to_ucs32(utf8str, strs32[i]);

    int len32 = ucs32strlen(strs32[i]);
    printf("len32=%d chars=", len32);
    for (int j = 0; j < len32; j++)
      printf("%02x ", strs32[i][j]);
    printf("\n");
  }

  return 0;
}
