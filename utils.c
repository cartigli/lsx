// #include 

#include "utils.h"

int sf_strcat(char *a, char *o, int bufflen) {
     int lim = bufflen - 1;
     int c = 0;
     /* skip past the used values of a */
     // while (*(a + c) && c < lim) { c++; }
     while (*(a + c)) { c++; }
     if (c >= lim) { return 1; }
     // while (*o && c < lim) {
     while (1) {
          if (!*o) { break; }
          if ( c >= lim) { return 1; } /* bad strcat */
          /* copy o to a's free values one byte at a time */
          *(a + c) = *o;
          c++;
          o++;
     }

     /* add a terminator (room from bufflen - 1) */
     *(a + c) = '\0';
     return 0;
}
