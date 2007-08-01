#define SIMPLE_TEST(x) int main(){ x; return 0; }

#ifdef TIME_WITH_SYS_TIME
/* Time with sys/time test */
 
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

int
main ()
{
if ((struct tm *) 0)
return 0;
  ;
  return 0;
}

#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
int main() { return 0; }
#endif /* STDC_HEADERS */

#ifdef HAVE_TM_ZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_zone);

#endif /* HAVE_TM_ZONE */

#ifdef HAVE_STRUCT_TM_TM_ZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_zone);

#endif /* HAVE_STRUCT_TM_TM_ZONE */

#ifdef HAVE_ATTRIBUTE

SIMPLE_TEST(int __attribute__((unused)) x);

#endif /* HAVE_ATTRIBUTE */

#ifdef HAVE_FUNCTION

SIMPLE_TEST((void)__FUNCTION__);

#endif /* HAVE_FUNCTION */

#ifdef HAVE_TM_GMTOFF

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_gmtoff=0);

#endif /* HAVE_TM_GMTOFF */

#ifdef HAVE_TIMEZONE

#include <sys/time.h>
#include <time.h>
SIMPLE_TEST(timezone=0);

#endif /* HAVE_TIMEZONE */

#ifdef HAVE_STRUCT_TIMEZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct timezone tz; tz.tz_minuteswest=0);

#endif /* HAVE_STRUCT_TIMEZONE */

#ifdef HAVE_STAT_ST_BLOCKS

#include <sys/stat.h>
SIMPLE_TEST(struct stat sb; sb.st_blocks=0);

#endif /* HAVE_STAT_ST_BLOCKS */

#ifdef PRINTF_LL_WIDTH

#ifdef HAVE_LONG_LONG
#  define LL_TYPE long long
#else /* HAVE_LONG_LONG */
#  define LL_TYPE __int64
#endif /* HAVE_LONG_LONG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
  char *llwidthArgs[] = { "l64", "l", "L", "q", "ll", NULL };
  char *s = malloc(128);
  char **currentArg = NULL;
  LL_TYPE x = (LL_TYPE)1048576 * (LL_TYPE)1048576;
  for (currentArg = llwidthArgs; *currentArg != NULL; currentArg++)
    {
    char formatString[64];
    sprintf(formatString, "%%%sd", *currentArg);
    sprintf(s, formatString, x);
    if (strcmp(s, "1099511627776") == 0)
      {
      printf("PRINTF_LL_WIDTH=[%s]\n", *currentArg);
      exit(0);
      }
    }
  exit(1);
}

#endif /* PRINTF_LL_WIDTH */

#ifdef SYSTEM_SCOPE_THREADS
#include <stdlib.h>
#include <pthread.h>

int main(void)
{
    pthread_attr_t attribute;
    int ret;

    pthread_attr_init(&attribute);
    ret=pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM);
    exit(ret==0 ? 0 : 1);
}

#endif /* SYSTEM_SCOPE_THREADS */

#ifdef HAVE_SOCKLEN_T

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

SIMPLE_TEST(socklen_t foo);

#endif /* HAVE_SOCKLEN_T */

#ifdef DEV_T_IS_SCALAR

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

int main ()
{
  dev_t d1, d2; 
  if(d1==d2)
    return 0;
  return 1;
}

#endif /* DEV_T_IS_SCALAR */

#if defined( INLINE_TEST_inline ) || defined( INLINE_TEST___inline__ ) || defined( INLINE_TEST___inline )
#ifndef __cplusplus
typedef int foo_t;
static INLINE_TEST_INLINE foo_t static_foo () { return 0; }
INLINE_TEST_INLINE foo_t foo () {return 0; }
int main() { return 0; }
#endif

#endif /* INLINE_TEST */

#ifdef HAVE_OFF64_T
#include <sys/types.h>
int main()
{
  off64_t n = 0;
  return (int)n;
}
#endif
