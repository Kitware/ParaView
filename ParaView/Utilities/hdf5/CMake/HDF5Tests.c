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
#endif

#ifdef HAVE_TM_ZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_zone);

#endif /* HAVE_TM_ZONE */

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

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
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


#define TO_STRING0(x) #x
#define TO_STRING(x) TO_STRING0(x)

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
  char *s = malloc(128);
  LL_TYPE x = (LL_TYPE)1048576 * (LL_TYPE)1048576;
  sprintf(s,"%" TO_STRING(PRINTF_LL_WIDTH) "d",x);
  exit(strcmp(s,"1099511627776"));
}

#endif /* PRINTF_LL_WIDTH */
