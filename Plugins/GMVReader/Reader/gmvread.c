#include <stdlib.h>
#include <stdio.h>
#include <string.h>
    
#define RDATA_INIT
#include "gmvread.h"
#include "gmvrayread.h"

#include <sys/types.h>
/* #include <malloc.h> */
#include <math.h>

#define FREE(a) { if (a) free(a); a = NULL; }

#define CHAR 0
#define SHORT 1
#define INT 2
#define FLOAT 3
#define WORD 4
#define DOUBLE 5
#define LONGLONG 6

#define CHARSIZE 1
#define SHORTSIZE 2
#define INTSIZE 4
#define WORDSIZE 4
#define FLOATSIZE 4
#define LONGSIZE 4
#define DOUBLESIZE 8
#define LONGLONGSIZE 8

#define IEEE 0
#define ASCII 1
#define IEEEI4R4 0
#define IEEEI4R8 2
#define IEEEI8R4 3
#define IEEEI8R8 4
#define IECXI4R4 5
#define IECXI4R8 6
#define IECXI8R4 7
#define IECXI8R8 8

#define MAXVERTS 10000
#define MAXFACES 10000
#define GMV_MIN(a1,a2)   ( ((a1) < (a2)) ? (a1):(a2) )

static int charsize = CHARSIZE, /*shortsize = SHORTSIZE,*/ intsize = INTSIZE, 
           /*wordsize = WORDSIZE,*/ floatsize = FLOATSIZE,
           /*longsize = LONGSIZE,*/ doublesize = DOUBLESIZE,
           longlongsize = LONGLONGSIZE, charsize_in;

static long numnodes, numcells, lncells, numcellsin, numfaces, lnfaces,
            numfacesin, ncells_struct;

static int numsurf, lnsurf, numsurfin, numtracers, numunits;

static short amrflag_in, structflag_in, fromfileflag, fromfileskip = 0;
static short nodes_read = 0, cells_read = 0, faces_read = 0, 
             surface_read = 0, iend = 0, swapbytes_on = 0, skipflag = 0, 
             reading_fromfile = 0, vfaceflag = 0, node_inp_type, 
             printon = 0;

static int curr_keyword, ftypeGlobal, ftype_sav, readkeyword, ff_keyword = -1;

static unsigned wordbuf;
static char sav_keyword[MAXKEYWORDLENGTH+64], input_dir[MAXFILENAMELENGTH];

void swapbytes(void *from, int size, int nitems),
     readnodes(FILE *gmvin, int ftype),
     readcells(FILE *gmvin, int ftype),
     readfaces(FILE *gmvin, int ftype),
     readvfaces(FILE *gmvin, int ftype),
     readxfaces(FILE *gmvin, int ftype),
     readmats(FILE *gmvin, int ftype),
     readvels(FILE *gmvin, int ftype),
     readvars(FILE *gmvin, int ftype),
     readflags(FILE *gmvin, int ftype),
     readpolygons(FILE *gmvin, int ftype),
     readtracers(FILE *gmvin, int ftype),
     readnodeids(FILE *gmvin, int ftype),
     readcellids(FILE *gmvin, int ftype),
     readfaceids(FILE *gmvin, int ftype),
     readtracerids(FILE *gmvin, int ftype),
     readunits(FILE *gmvin, int ftype),
     readsurface(FILE *gmvin, int ftype),
     readsurfvel(FILE *gmvin, int ftype),
     readsurfmats(FILE *gmvin, int ftype),
     readsurfvars(FILE *gmvin, int ftype),
     readsurfflag(FILE *gmvin, int ftype),
     readsurfids(FILE *gmvin, int ftype),
     readvinfo(FILE *gmvin, int ftype),
     readcomments(FILE *gmvin, int ftype),
     readgroups(FILE *gmvin, int ftype),
     readcellpes(FILE *gmvin, int ftype),
     readsubvars(FILE *gmvin, int ftype),
     readghosts(FILE *gmvin, int ftype),
     readvects(FILE *gmvin, int ftype),
     gmvrdmemerr(), ioerrtst(FILE *gmvin), endfromfile();
static FILE *gmvinGlobal = NULL, *gmvin_sav = NULL;

static char *file_path = NULL;
static int errormsgvarlen = 0;

int binread(void* ptr, int size, int type, long nitems, FILE* stream);
int word2int(unsigned wordin);



int chk_gmvend(FILE *gmvchk);

/* Mark C. Miller, Wed Aug 22, 2012: fix leak of filnam */
int gmvread_checkfile(char *filnam)
{
   /*                           */
   /*  Check a GMV input file.  */
   /*                           */
   //int chkend;
   char magic[MAXKEYWORDLENGTH+64], filetype[MAXKEYWORDLENGTH+64];
   FILE *gmvchk;
   char* slash;
   int alloc_filnam = 0;

   /* check for the path - if include open and save if not append */
#ifdef _WIN32
   slash = strrchr( filnam,  '\\' );
#else
   slash = strrchr( filnam,  '/' );
#endif
   if( file_path != NULL && slash == NULL )
      {
       /* append the path and check again*/
       size_t len = strlen(file_path) + strlen(filnam) + 1;
       char *temp = (char*)malloc(len *sizeof(char));
       strcpy(temp, file_path);
       strcat(temp, filnam);
       free( filnam );
       filnam = (char*)malloc(len *sizeof(char));
       alloc_filnam = 1;
       strcpy(filnam, temp);
       free(temp);       
      }
   else if( file_path == NULL && slash != NULL)
      {
       size_t pos = slash - filnam + 1;
       file_path = (char*) malloc ( (pos+1) *sizeof(char));
       strncpy( file_path, filnam, pos );
       file_path[pos] = 0;
      }
   else if( file_path == NULL && slash == NULL )
      {
          fprintf(stderr,"Error with the path");
          gmv_data.errormsg = (char *)malloc(20 * sizeof(char));
          snprintf(gmv_data.errormsg,20,"Error with the path");
          return 1;
      }
   gmvchk = fopen(filnam, "r");
   if(gmvchk == NULL)
      {
       fprintf(stderr,"GMV cannot open file %s\n",filnam);
       errormsgvarlen = (int)strlen(filnam);
       gmv_data.errormsg = (char *)malloc((22 + errormsgvarlen) * sizeof(char));
       snprintf(gmv_data.errormsg,22 + errormsgvarlen,"GMV cannot open file %s",filnam);
       if (alloc_filnam) free(filnam);
       return 1;
      }
   if (alloc_filnam) free(filnam);
    
   /*  Read header. */
   binread(magic,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvchk);
   if (strncmp(magic,"gmvinput",8) != 0)
     {
      fprintf(stderr,"This is not a GMV input file.\n");
      gmv_data.errormsg = (char *)malloc(30 * sizeof(char));
      snprintf(gmv_data.errormsg,30,"This is not a GMV input file.");
      fclose(gmvchk);
      return 2;
     }

   /*  Check that gmv input file has "endgmv".  */
#ifdef BEFORE_TERRY_JORDAN_WINDOWS_CHANGES
   if (strncmp(magic,"gmvinput",8) == 0)
     {
      chkend = chk_gmvend(gmvchk);
      if (!chkend)
        {
         fprintf(stderr,"Error - endgmv not found.\n");
         gmv_data.errormsg = (char *)malloc(26 * sizeof(char));
         snprintf(gmv_data.errormsg,26,"Error - endgmv not found.");
         fclose(gmvchk);
         return 3;
        }
     }
#endif

   /*  Read file type and set ftype: 0-ieee binary, 1-ascii text,  */
   /*  0-ieeei4r4 binary, 2-ieeei4r8 binary, 3-ieeei8r4 binary,    */
   /*  4-ieeei8r8 binary, 5-iecxi4r4 binary, 6-iecxi4r8 binary,    */
   /*  7-iecxi8r4 binary or 8-iecxi8r8 binary.                     */

   binread(filetype,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvchk);

   /*  Rewind the file and re-read header for file type.  */
   ftypeGlobal = -1;
   if (strncmp(filetype,"ascii",5) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype," ascii",6) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"  ascii",7) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"   ascii",8) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"ieee",4) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieee",5) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r4",8) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieeei4r4",9) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r8",8) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype," ieeei4r8",9) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype,"ieeei8r4",8) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype," ieeei8r4",9) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype,"ieeei8r8",8) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype," ieeei8r8",9) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype,"iecxi4r4",8) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype," iecxi4r4",9) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype,"iecxi4r8",8) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype," iecxi4r8",9) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype,"iecxi8r4",8) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype," iecxi8r4",9) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype,"iecxi8r8",8) == 0) ftypeGlobal = IECXI8R8;
   if (strncmp(filetype," iecxi8r8",9) == 0) ftypeGlobal = IECXI8R8;

   /*  Check for valid file type.  */
   if (ftypeGlobal == -1)
     {
      fprintf(stderr,"Invalid GMV input file type.  Type must be:\n");
      fprintf(stderr,
       "  ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8,\n");
      fprintf(stderr,
       "  iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8,\n");
      gmv_data.errormsg = (char *)malloc(137 * sizeof(char));
      snprintf(gmv_data.errormsg,137,"Invalid GMV input file type.  Type must be: %s%s",
              "ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8, ",
              "iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8.");
      fclose(gmvchk);
      return 4;
     }

   /*  Check that machine can read I8 types.  */
   if (ftypeGlobal == IEEEI8R4 || ftypeGlobal == IEEEI8R8 || ftypeGlobal == IECXI8R4 ||
       ftypeGlobal == IECXI8R8)
     {
      if (sizeof(long) < 8)
        {
         fprintf(stderr,"Cannot read 64bit I* types on this machine.\n");
         gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
         snprintf(gmv_data.errormsg,44,"Cannot read 64bit I* types on this machine.");
         fclose(gmvchk);
         return 5;
        }
     }

   fclose(gmvchk);
   return 0;
}


/* Mark C. Miller, Wed Aug 22, 2012: fix leak of filnam */
int gmvread_open(char *filnam)
{
   /*                                    */
   /*  Open and check a GMV input file.  */
   /*                                    */
   int /*chkend,*/ ilast, i, isize;
   char magic[MAXKEYWORDLENGTH+64], filetype[MAXKEYWORDLENGTH+64];
   char* slash;
   int alloc_filnam = 0;

   /* check for the path - if include open and save if not append */
#ifdef _WIN32
   slash = strrchr( filnam,  '\\' );
#else
   slash = strrchr( filnam,  '/' );
#endif
   if( file_path != NULL && slash == NULL )
      {
       /* append the path and check again*/
       size_t len = strlen(file_path) + strlen(filnam) + 1;
       char *temp = (char*)malloc(len *sizeof(char));
       strcpy(temp, file_path);
       strcat(temp, filnam);
       free( filnam );
       filnam = (char*)malloc(len *sizeof(char));
       alloc_filnam = 1;
       strcpy(filnam, temp);
       free(temp);
      }
   else if( file_path == NULL && slash != NULL)
      {
       size_t pos = slash - filnam + 1;
       file_path = (char*) malloc ( (pos+1) *sizeof(char));
       strncpy( file_path, filnam, pos );
       file_path[pos] = 0;
      }
   else if( file_path == NULL && slash == NULL )
      {
          fprintf(stderr,"Error with the path");
          gmv_data.errormsg = (char *)malloc(20 * sizeof(char));
          snprintf(gmv_data.errormsg,20,"Error with the path");
          return 1;
      }
   gmvinGlobal = fopen(filnam, "r");
   if(gmvinGlobal == NULL)
      {
       fprintf(stderr,"GMV cannot open file %s\n",filnam);
       errormsgvarlen = (int)strlen(filnam);
       gmv_data.errormsg = (char *)malloc((22 + errormsgvarlen) * sizeof(char));
       snprintf(gmv_data.errormsg,22 + errormsgvarlen,"GMV cannot open file %s",filnam);
       if (alloc_filnam) free(filnam);
       return 1;
      }
    
   /*  Read header. */
   binread(magic,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvinGlobal);
   if (strncmp(magic,"gmvinput",8) != 0)
      {
       fprintf(stderr,"This is not a GMV input file.\n");
       gmv_data.errormsg = (char *)malloc(30 * sizeof(char));
       snprintf(gmv_data.errormsg,30,"This is not a GMV input file.");
       if (alloc_filnam) free(filnam);
       return 2;
      }

   /*  Check that gmv input file has "endgmv".  */
#ifdef BEFORE_TERRY_JORDAN_WINDOWS_CHANGES
   if (strncmp(magic,"gmvinput",8) == 0)
      {
       chkend = chk_gmvend(gmvinGlobal);
       if (!chkend)
         {
          fprintf(stderr,"Error - endgmv not found.\n");
          gmv_data.errormsg = (char *)malloc(26 * sizeof(char));
          snprintf(gmv_data.errormsg,26,"Error - endgmv not found.");
          if (alloc_filnam) free(filnam);
          return 3;
         }
      }
#endif

   /*  Read file type and set ftype: 0-ieee binary, 1-ascii text,  */
   /*  0-ieeei4r4 binary, 2-ieeei4r8 binary, 3-ieeei8r4 binary,    */
   /*  or 4-ieeei8r8 binary.                                       */

   binread(filetype,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvinGlobal);

   /*  Rewind the file and re-read header for file type.  */
   ftypeGlobal = -1;
   if (strncmp(filetype,"ascii",5) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype," ascii",6) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"  ascii",7) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"   ascii",8) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"ieee",4) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieee",5) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r4",8) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieeei4r4",9) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r8",8) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype," ieeei4r8",9) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype,"ieeei8r4",8) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype," ieeei8r4",9) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype,"ieeei8r8",8) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype," ieeei8r8",9) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype,"iecxi4r4",8) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype," iecxi4r4",9) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype,"iecxi4r8",8) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype," iecxi4r8",9) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype,"iecxi8r4",8) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype," iecxi8r4",9) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype,"iecxi8r8",8) == 0) ftypeGlobal = IECXI8R8;
   if (strncmp(filetype," iecxi8r8",9) == 0) ftypeGlobal = IECXI8R8;

   /*  Determine character input size.  */
   charsize_in = 8;
   if (ftypeGlobal == ASCII || ftypeGlobal > IEEEI8R8) charsize_in = 32;

   /*  Reset IECX types back to regular types.  */
   if (ftypeGlobal == IECXI4R4) ftypeGlobal = IEEEI4R4;
   if (ftypeGlobal == IECXI4R8) ftypeGlobal = IEEEI4R8;
   if (ftypeGlobal == IECXI8R4) ftypeGlobal = IEEEI8R4;
   if (ftypeGlobal == IECXI8R8) ftypeGlobal = IEEEI8R8;

   /*  Check for valid file type.  */
   if (ftypeGlobal == -1)
     {
      fprintf(stderr,"Invalid GMV input file type.  Type must be:\n");
      fprintf(stderr,
       "  ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8,\n");
      fprintf(stderr,
       "  iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8.\n");
      gmv_data.errormsg = (char *)malloc(137 * sizeof(char));
      snprintf(gmv_data.errormsg,137,"Invalid GMV input file type.  Type must be: %s%s",
              "ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8, ",
              "iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8.");
      if (alloc_filnam) free(filnam);
      return 4;
     }

   /*  Check that machine can read I8 types.  */
   if (ftypeGlobal == IEEEI8R4 || ftypeGlobal == IEEEI8R8)
     {
      if (sizeof(long) < 8)
        {
         fprintf(stderr,"Cannot read 64bit I* types on this machine.\n");
         gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
         snprintf(gmv_data.errormsg,44,"Cannot read 64bit I* types on this machine.");
         if (alloc_filnam) free(filnam);
         return 4;
        }
     }

#ifdef BEFORE_TERRY_JORDAN_WINDOWS_CHANGES
   rewind(gmvinGlobal);
#endif

   /*re-open the file with the proper translation mode*/
   fclose(gmvinGlobal);
   if( ftypeGlobal != ASCII)
   {
    gmvinGlobal = fopen(filnam,"rb");
   }
   else
   {
    /* To be able to read on Windows ASCII files with both */
    /* CR and CRLF endings open file in binary mode.       */
    /* On Unix, the "b" option is ignored (at least since  */
    /* C90).                                               */
    gmvinGlobal = fopen(filnam,"rb");
   }

   if (ftypeGlobal != ASCII)
     {
      binread(magic,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvinGlobal);
      binread(filetype,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvinGlobal);
     }
   if (ftypeGlobal == ASCII) { int res = fscanf(gmvinGlobal,"%s%s",magic,filetype); (void) res; }

   readkeyword = 1;

   /*  Get directory of GMV input file from the file name  */
   /*  for use in case any fromfiles found in the file.    */
   /*  First find the last /.                              */
   if (fromfileskip == 0)
     {
      isize = (int)strlen(filnam);
      ilast = -1;
      for (i = 0; i < isize - 1; i++)
        {
         if (strncmp((filnam+i),"/",1) == 0 ||
             strncmp((filnam+i),"\\",1) == 0)
           ilast = i;
        }
      if (ilast > -1)
        {
         strncpy(input_dir, filnam, GMV_MIN(ilast+1, MAXFILENAMELENGTH - 1));
         /* When repeatedly calling this function and loading a file from    */
         /* a directory higher up in the hierarchy in a later call, prevent  */
         /* that the previous directory is used again by properly ending the */
         /* string input_dir, i.e. avoid cases where in a fromfile statement */
         /* /path/to/directory/subdirectory is used first and                */
         /* /path/to/directory referenced next. Without properly null        */
         /* terminating the string, /path/to/directory/subdirectory would    */
         /* still get used in the second case.                               */
         *(input_dir + GMV_MIN(ilast+1, MAXFILENAMELENGTH-1)) = (char)0;
        }
     }
   
   if (alloc_filnam) free(filnam);
   return 0;
}


int gmvread_open_fromfileskip(char *filnam)
{
  /*                                    */
  /*  Open and check a GMV input file.  */
  /*  Skip read of fromfiles.           */
  /*                                    */
  int ierr;

   fromfileskip = 1;
   
   ierr = gmvread_open(filnam);
   return ierr;
}


void gmvread_close()
{
   if (gmvinGlobal)
     {
       fclose(gmvinGlobal);
       gmvinGlobal = NULL;
     }
   fromfileskip = 0;
   nodes_read = 0;  cells_read = 0;  faces_read = 0; 
   surface_read = 0;  iend = 0;  swapbytes_on = 0;  skipflag = 0; 
   reading_fromfile = 0;  vfaceflag = 0;
}


void gmvread_printon()
{
   printon = 1;
}


void gmvread_printoff()
{
   printon = 0;
}


int fromfilecheck(int keyword);

void gmvread_data()
{
  char keyword[MAXKEYWORDLENGTH+64], tmpchar[20];
  double ptime=0.;
  float tmptime;
  int cycleno=0, before_nodes_ok;

   /*  Zero gmv_data and free structure arrays.  */
   gmv_data.keyword = 0;
   gmv_data.datatype = 0;
   strcpy(gmv_data.name1,"                   ");
   gmv_data.num = 0;
   gmv_data.num2 = 0;
   FREE(gmv_data.errormsg);
   gmv_data.ndoubledata1 = 0;
   gmv_data.ndoubledata2 = 0;
   gmv_data.ndoubledata3 = 0;
   FREE(gmv_data.doubledata1);  
   FREE(gmv_data.doubledata2);
   FREE(gmv_data.doubledata3);
   gmv_data.nlongdata1 = 0;
   gmv_data.nlongdata2 = 0;
   FREE(gmv_data.longdata1);
   FREE(gmv_data.longdata2);
   gmv_data.nchardata1 = 0;
   gmv_data.nchardata2 = 0;
   FREE(gmv_data.chardata1);
   FREE(gmv_data.chardata2);

   /*  Check current keyword type for continued reading.  */
   if (readkeyword == 0)
     {
      switch (curr_keyword)
        {
         case(CELLS):
            readcells(gmvinGlobal,ftypeGlobal);
            break;   
         case(FACES):
            readfaces(gmvinGlobal,ftypeGlobal);
            break;
         case(VFACES):
            readvfaces(gmvinGlobal,ftypeGlobal);
            break;
         case(XFACES):
            readxfaces(gmvinGlobal,ftypeGlobal);
            break;
         case(VARIABLE):
            readvars(gmvinGlobal,ftypeGlobal);
            break;
         case(FLAGS):
            readflags(gmvinGlobal,ftypeGlobal);
            break;
         case(POLYGONS):
            readpolygons(gmvinGlobal,ftypeGlobal);
            break;
         case(TRACERS):
            readtracers(gmvinGlobal,ftypeGlobal);
            break;
         case(SURFACE):
            readsurface(gmvinGlobal,ftypeGlobal);
            break;
         case(SURFVARS):
            readsurfvars(gmvinGlobal,ftypeGlobal);
            break;
         case(SURFFLAG):
            readsurfflag(gmvinGlobal,ftypeGlobal);
            break;
         case(UNITS):
            readunits(gmvinGlobal,ftypeGlobal);
            break;
         case(VINFO):
            readvinfo(gmvinGlobal,ftypeGlobal);
            break;
         case(GROUPS):
            readgroups(gmvinGlobal,ftypeGlobal);
            break;
         case(SUBVARS):
            readsubvars(gmvinGlobal,ftypeGlobal);
            break;
         case(VECTORS):
            readvects(gmvinGlobal,ftypeGlobal);
            break;
        }
     }

   /*  Read and process keyword based data until endgmv found.  */
   if (readkeyword == 1)
     {
      if (ftypeGlobal != ASCII)
        {
         binread(keyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvinGlobal);
         *(keyword+MAXKEYWORDLENGTH)=(char)0;
        }
      if (ftypeGlobal == ASCII) { int res = fscanf(gmvinGlobal,"%s",keyword); (void) res; }

      if ((feof(gmvinGlobal) != 0) | (ferror(gmvinGlobal) != 0)) iend = 1;

      /*  If comments keyword, read through comments,  */
      /*  then read and process next keyword.           */
      if (strncmp(keyword,"comments",8) == 0)
        {
         readcomments(gmvinGlobal,ftypeGlobal);
         if (ftypeGlobal != ASCII)
           {
            binread(keyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvinGlobal);
            *(keyword+MAXKEYWORDLENGTH)=(char)0;
           }
         if (ftypeGlobal == ASCII) { int res = fscanf(gmvinGlobal,"%s",keyword); (void) res; }
         if ((feof(gmvinGlobal) != 0) | (ferror(gmvinGlobal) != 0)) iend = 1;
        }

      if (strncmp(keyword,"endgmv",6) == 0)
        {
         curr_keyword = GMVEND;
         iend = 1;
        }
      else if (strncmp(keyword,"nodes",5) == 0)
        {
         curr_keyword = NODES;
         node_inp_type = 0;
        }
      else if (strncmp(keyword,"nodev",5) == 0)
        {
         curr_keyword = NODES;
         node_inp_type = 1;
        }
      else if (strncmp(keyword,"cells",5) == 0) curr_keyword = CELLS;
      else if (strncmp(keyword,"faces",5) == 0) curr_keyword = FACES;
      else if (strncmp(keyword,"vfaces",6) == 0) curr_keyword = VFACES;
      else if (strncmp(keyword,"xfaces",6) == 0) curr_keyword = XFACES;
      else if (strncmp(keyword,"material",8) == 0) curr_keyword = MATERIAL;
      else if (strncmp(keyword,"velocity",8) == 0) curr_keyword = VELOCITY;
      else if (strncmp(keyword,"variable",8) == 0) curr_keyword = VARIABLE;
      else if (strncmp(keyword,"flags",5) == 0) curr_keyword = FLAGS;
      else if (strncmp(keyword,"polygons",8) == 0) curr_keyword = POLYGONS;
      else if (strncmp(keyword,"tracers",7) == 0) curr_keyword = TRACERS;
      else if (strncmp(keyword,"probtime",8) == 0) curr_keyword = PROBTIME;
      else if (strncmp(keyword,"cycleno",7) == 0) curr_keyword = CYCLENO;
      else if (strncmp(keyword,"nodeids",7) == 0) curr_keyword = NODEIDS;
      else if (strncmp(keyword,"cellids",7) == 0) curr_keyword = CELLIDS;
      else if (strncmp(keyword,"surface",7) == 0) curr_keyword = SURFACE;
      else if (strncmp(keyword,"surfmats",8) == 0) curr_keyword = SURFMATS;
      else if (strncmp(keyword,"surfvel",7) == 0) curr_keyword = SURFVEL;
      else if (strncmp(keyword,"surfvars",8) == 0) curr_keyword = SURFVARS;
      else if (strncmp(keyword,"surfflag",8) == 0) curr_keyword = SURFFLAG;
      else if (strncmp(keyword,"surfids",7) == 0) curr_keyword = SURFIDS;
      else if (strncmp(keyword,"units",5) == 0) curr_keyword = UNITS;
      else if (strncmp(keyword,"vinfo",5) == 0) curr_keyword = VINFO;
      else if (strncmp(keyword,"traceids",7) == 0) curr_keyword = TRACEIDS;
      else if (strncmp(keyword,"groups",6) == 0) curr_keyword = GROUPS;
      else if (strncmp(keyword,"codename",8) == 0) curr_keyword = CODENAME;
      else if (strncmp(keyword,"codever",7) == 0) curr_keyword = CODEVER;
      else if (strncmp(keyword,"simdate",7) == 0) curr_keyword = SIMDATE;
      else if (strncmp(keyword,"cellpes",7) == 0) curr_keyword = CELLPES;
      else if (strncmp(keyword,"subvars",7) == 0) curr_keyword = SUBVARS;
      else if (strncmp(keyword,"ghosts",6) == 0) curr_keyword = GHOSTS;
      else if (strncmp(keyword,"vectors",7) == 0) curr_keyword = VECTORS;
      else curr_keyword = INVALIDKEYWORD;

      /*  If invalid keyword, send error.  */
      if (curr_keyword == INVALIDKEYWORD)
        {
         gmv_data.keyword = GMVERROR;
         fprintf(stderr,"Error, %s is an invalid keyword.\n",keyword);
         errormsgvarlen = (int)strlen(keyword);
         gmv_data.errormsg = (char *)malloc((31 + errormsgvarlen) * sizeof(char));
         snprintf(gmv_data.errormsg,31 + errormsgvarlen,"Error, %s is an invalid keyword.",keyword);
        }

      strncpy(sav_keyword,keyword,MAXKEYWORDLENGTH);

      /*  Set skipflag if reading from a fromfile.  */
      if (reading_fromfile)
        {
         if (curr_keyword == ff_keyword) skipflag = 0;
         else skipflag = 1;
        }

      /*  Check for keywords that are allowed before nodes, cells, etc.  */
      before_nodes_ok = 0;
      if (curr_keyword == CODENAME || curr_keyword == CODEVER ||
          curr_keyword == SIMDATE) before_nodes_ok = 1;

      /*  Check that nodes have been input.  */
      if (curr_keyword > NODES && nodes_read == 0 && before_nodes_ok == 0)
        {
         fprintf(stderr,"Error, 'nodes' keyword missing.\n");
         gmv_data.errormsg = (char *)malloc(32 * sizeof(char));
         snprintf(gmv_data.errormsg,32,"Error, 'nodes' keyword missing.");
         gmv_data.keyword = GMVERROR;
        }

      /*  Check that cells, faces, vfaces or xfaces have been input.  */
      if (curr_keyword > XFACES && cells_read == 0 && faces_read == 0 &&
          before_nodes_ok == 0)
        {
         fprintf(stderr,"Error, 'cells, faces or xfaces' keyword missing.\n");
         gmv_data.errormsg = (char *)malloc(49 * sizeof(char));
         snprintf(gmv_data.errormsg,49,"Error, 'cells, faces or xfaces' keyword missing.");
         gmv_data.keyword = GMVERROR;
        }

      /*  Read current keyword data.  */
      switch (curr_keyword)
        {
         case(NODES):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(NODES) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readnodes(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;   
         case(CELLS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(CELLS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readcells(gmvinGlobal,ftypeGlobal);
            if (amrflag_in == 0 && structflag_in == 0)
               readkeyword = 0;
            break;   
         case(FACES):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(FACES) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readfaces(gmvinGlobal,ftypeGlobal);
            readkeyword = 0;
            break;
         case(VFACES):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(VFACES) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readvfaces(gmvinGlobal,ftypeGlobal);
            readkeyword = 0;
            break;
         case(XFACES):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(XFACES) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readxfaces(gmvinGlobal,ftypeGlobal);
            readkeyword = 0;
            break;
         case(MATERIAL):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(MATERIAL) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readmats(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(VELOCITY):
            readvels(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(VARIABLE):
            readvars(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(FLAGS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(FLAGS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readflags(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(POLYGONS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(POLYGONS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readpolygons(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(TRACERS):
            readtracers(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(PROBTIME):
            if (ftypeGlobal != ASCII)
              {
               if (ftypeGlobal == IEEEI4R8 || ftypeGlobal == IEEEI8R8)
                  binread(&ptime,doublesize,DOUBLE,(long)1,gmvinGlobal);
               else
                 {
                  binread(&tmptime,floatsize,FLOAT,(long)1,gmvinGlobal);
                  ptime = tmptime;
                 }
              }
            if (ftypeGlobal == ASCII) { int res = fscanf(gmvinGlobal,"%lf",&ptime); (void) res; }
            gmv_data.keyword = PROBTIME;
            gmv_data.datatype = 0;
            gmv_data.ndoubledata1 = 1;
            gmv_data.doubledata1 = (double *)malloc(1*sizeof(double));
            gmv_data.doubledata1[0] = ptime;
            readkeyword = 1;
            break;
         case(CYCLENO):
            if (ftypeGlobal != ASCII) binread(&cycleno,intsize,INT,
                                        (long)1,gmvinGlobal);
            if (ftypeGlobal == ASCII) { int res = fscanf(gmvinGlobal,"%d",&cycleno); (void) res; }
            gmv_data.keyword = CYCLENO;
            gmv_data.num = cycleno;
            readkeyword = 1;
            break;
         case(NODEIDS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(NODEIDS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readnodeids(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(CELLIDS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(CELLIDS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            readcellids(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(FACEIDS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(FACEIDS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            readfaceids(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(TRACEIDS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(TRACEIDS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            readtracerids(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(SURFACE):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(SURFACE) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readsurface(gmvinGlobal,ftypeGlobal);
            readkeyword = 0;
            break;
         case(SURFMATS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(SURFMATS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readsurfmats(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(SURFVEL):
            readsurfvel(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(SURFVARS):
            readsurfvars(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(SURFFLAG):
            readsurfflag(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(SURFIDS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(SURFIDS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            readsurfids(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(UNITS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(UNITS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readunits(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(VINFO):
            readvinfo(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(GROUPS):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(GROUPS) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            if (fromfileflag == 1) break;
            readgroups(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(CODENAME):  case(CODEVER):  case(SIMDATE):
            /* at most 8 characters for values allowed */
            if (ftypeGlobal != ASCII)
              {
               binread(tmpchar,charsize,CHAR,(long)8,gmvinGlobal);
               *(tmpchar+8)=(char)0;
              }
            if (ftypeGlobal == ASCII)
              {
               int res = fscanf(gmvinGlobal,"%s",tmpchar); (void) res;
               *(tmpchar+8)=(char)0;
              }
            ioerrtst(gmvinGlobal);
            gmv_data.keyword = curr_keyword;
            gmv_data.datatype = 0;
            strncpy(gmv_data.name1, tmpchar, 8);
           *(gmv_data.name1 + GMV_MIN(strlen(tmpchar), 8)) = (char)0;
            readkeyword = 1;
            break;
         case(CELLPES):
            /* SB: Modification due to disabled exit statement on fromfile error */
            if (fromfilecheck(CELLPES) < 0)
              {
                gmv_data.keyword = GMVERROR;
                break;
              }
            readcellpes(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(SUBVARS):
            readsubvars(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
         case(GHOSTS):
            readghosts(gmvinGlobal,ftypeGlobal);
            readkeyword = 1;
            break;
         case(VECTORS):
            readvects(gmvinGlobal,ftypeGlobal);
            if (readkeyword == 1) readkeyword = 0;
            break;
        }
     }

   if (iend)
     {
      gmv_data.keyword = GMVEND;
      /* fclose(gmvinGlobal); */
     }

   if (gmv_data.keyword == GMVERROR)
     if (gmvinGlobal)
       {
         fclose(gmvinGlobal);
         gmvinGlobal = NULL;
       }

   if (readkeyword == 1 && reading_fromfile && curr_keyword == ff_keyword) 
      endfromfile();

   if (readkeyword == 2)
     {
      if (reading_fromfile && curr_keyword == ff_keyword) endfromfile();
      readkeyword = 1;
     }

   if (fromfileflag == 1 && fromfileskip == 1) fromfileflag = 0;
   if (fromfileflag == 1 && reading_fromfile == 0) fromfileflag = 0;
}


void rdints(int iarray[], int nvals, FILE* gmvin)
{
  /*                                                  */
  /*  Read an integer array from an ASCII text file.  */
  /*                                                  */
  int i, j, ret_stat;

  for (i = 0; i < nvals; i++)
    {
      ret_stat = fscanf(gmvin,"%d",&iarray[i]);

      /* File ends abruptly or could not be read from anymore? */
      if (feof(gmvin) != 0)
        {
          fprintf(stderr,"%d integer values expected, but gmv input file end reached after %d.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%d integer values expected, but gmv input file end reached after %d.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          return;
        }
      if (ferror(gmvin) != 0)
        {
          fprintf(stderr,"I/O error while reading gmv input file.\n");
          gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
          snprintf(gmv_data.errormsg,40,"I/O error while reading gmv input file.");
          gmv_data.keyword = GMVERROR;
          return;
        }

      /* File end may have not yet been reached, but format requirements could have been violated */
      if (ret_stat == 0)
        {
          /* Array shorter than it is supposed to be */
          fprintf(stderr,"%d integer values expected, only %d found while reading gmv input file.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%d integer values expected, only %d found while reading gmv input file.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          /* Initialise remainder of array */
          for (j = i; j < nvals; j++)
            {
              iarray[j] = 0;
            }
          return;
        }
    }
}


void rdlongs(long iarray[], long nvals, FILE* gmvin)
{
  /*                                                  */
  /*  Read an integer array from an ASCII text file.  */
  /*                                                  */
  long i, j, ret_stat;

  for (i = 0; i < nvals; i++)
    {
      ret_stat = fscanf(gmvin,"%ld",&iarray[i]);

      /* File ends abruptly or could not be read from anymore? */
      if (feof(gmvin) != 0)
        {
          fprintf(stderr,"%ld long values expected, but gmv input file end reached after %ld.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%ld long values expected, but gmv input file end reached after %ld.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          return;
        }
      if (ferror(gmvin) != 0)
        {
          fprintf(stderr,"I/O error while reading gmv input file.\n");
          gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
          snprintf(gmv_data.errormsg,40,"I/O error while reading gmv input file.");
          gmv_data.keyword = GMVERROR;
          return;
        }

      /* File end may have not yet been reached, but format requirements could have been violated */
      if (ret_stat == 0)
        {
          /* Array shorter than it is supposed to be */
          fprintf(stderr,"%ld long values expected, only %ld found while reading gmv input file.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%ld long values expected, only %ld found while reading gmv input file.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          /* Initialise remainder of array */
          for (j = i; j < nvals; j++)
            {
              iarray[j] = (long)0;
            }
          return;
        }
    }
}


void rdfloats(double farray[], long nvals, FILE* gmvin)
{
  /*                                                  */
  /*  Read an integer array from an ASCII text file.  */
  /*                                                  */
  int i, j, ret_stat;

  for (i = 0; i < nvals; i++)
    {
      ret_stat = fscanf(gmvin,"%lf",&farray[i]);

      /* File ends abruptly or could not be read from anymore? */
      if (feof(gmvin) != 0)
        {
          fprintf(stderr,"%ld double values expected, but gmv input file end reached after %d.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%ld double values expected, but gmv input file end reached after %d.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          return;
        }
      if (ferror(gmvin) != 0)
        {
          fprintf(stderr,"I/O error while reading gmv input file.\n");
          gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
          snprintf(gmv_data.errormsg,40,"I/O error while reading gmv input file.");
          gmv_data.keyword = GMVERROR;
          return;
        }

      /* File end may have not yet been reached, but format requirements could have been violated */
      if (ret_stat == 0)
        {
          /* Array shorter than it is supposed to be */
          fprintf(stderr,"%ld double values expected, only %d found while reading gmv input file.\n", nvals, i);
          gmv_data.errormsg = (char *)malloc(90 * sizeof(char));
          snprintf(gmv_data.errormsg, 90,
                   "%ld double values expected, only %d found while reading gmv input file.\n", nvals, i);
          gmv_data.keyword = GMVERROR;
          /* Initialise remainder of array */
          for (j = i; j < nvals; j++)
            {
              farray[j] = 0.0;
            }
          return;
        }
    }
}


int rdcellkeyword(FILE* gmvin, int ftype, char* keystring)
{
  char ckeyword[MAXKEYWORDLENGTH+64];

   if (ftype != ASCII)
     {
      binread(ckeyword, charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvin);

      if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
        return (-1);

      *(ckeyword+MAXKEYWORDLENGTH)=(char)0;
     }
   if (ftype == ASCII) { int res = fscanf(gmvin,"%s",ckeyword); (void) res; }
 
   return
     strncmp(ckeyword,keystring,strlen(keystring));
}


int checkfromfile();

int fromfilecheck(int fkeyword)
{
  long pos_after_keyword;
  int base_ftype;
  FILE *basefile;  

   basefile = gmvinGlobal;
   base_ftype = ftypeGlobal;
   pos_after_keyword = ftell(gmvinGlobal);
   if (checkfromfile() < 0) return -1;

   if (gmvinGlobal == basefile)
     {
      if (fromfileflag == 0)
         fseek(gmvinGlobal, pos_after_keyword, SEEK_SET);
      return 0;
     }
   else
     {

      /*  Save current data.  */
      gmvin_sav = basefile;
      ftype_sav = base_ftype;

      /*  Skip to fkeyword.  */
      ff_keyword = fkeyword;
      reading_fromfile = 1;
      while (9)
        {
         gmvread_data();
         if (gmv_data.keyword == fkeyword) fromfileflag = 1;
         if (gmv_data.keyword == fkeyword) break;
        }

     }
 
   return 0;
}


void endfromfile()
{
   /*  Reset current data.  */
   ftypeGlobal = ftype_sav;
   fromfileskip = 0;
   if (gmvinGlobal)
     fclose(gmvinGlobal);
   gmvinGlobal = gmvin_sav;
   fromfileflag = 0;
   reading_fromfile = 0;
   ff_keyword = -1;
   skipflag = 0;
}


int checkfromfile()
{
  char c, charptr[MAXFILENAMELENGTH], *charptr2=NULL, tmpbuf[200], stringbuf[100];
  int i, ierr, fkeyword;

   /*  Check for "from".  */
   if (ftypeGlobal != ASCII)
      binread(stringbuf, charsize, CHAR, (long)4, gmvinGlobal);
   else
      { int res = fscanf(gmvinGlobal,"%s",stringbuf); (void) res; }

   if (strncmp("from",stringbuf, 4) != 0) return 0;

   if (ftypeGlobal != ASCII)
     {
      /* Gobble the "file" part */
      binread(&wordbuf, intsize, WORD, (long)1, gmvinGlobal);

      /* Get the doublequote */
      binread(&c, charsize, CHAR, (long)1, gmvinGlobal);
      for (i = 0; 1; i++)
        {
         binread(&c, charsize, CHAR, (long)1, gmvinGlobal);
         if (c == '"') break;
         tmpbuf[i] = c;
         tmpbuf[i + 1] = '\0';
        }
      charptr2 = tmpbuf;
     }

   if (ftypeGlobal == ASCII) 
     {
      int res = fscanf(gmvinGlobal,"%s",tmpbuf); (void) res;
      charptr2 = strpbrk(&tmpbuf[1],"\"");
      *charptr2 = '\0';
      charptr2 = &tmpbuf[1];
     }

   if (charptr2 == 0) return 0;
#ifdef _WIN32
#  if !defined(__CYGWIN__)
   /*  If the first character of the fromfile filename is    */
   /*  not \\ (network path) or <letter>:\, then add the     */
   /*  input directory to the filename.  */
   if (strncmp(&charptr2[0],"\\\\",2) != 0 &&
       strncmp(&charptr2[1],":\\",2) != 0)
#  else
   /*  If the first character of the fromfile filename is    */
   /*  not /, then add the input directory to the filename.  */
   if (strncmp(charptr2,"/",1) != 0)
#  endif
#else
   /*  If the first character of the fromfile filename is    */
   /*  not /, then add the input directory to the filename.  */
   if (strncmp(charptr2,"/",1) != 0)
#endif
     {
      strncpy(charptr,input_dir,MAXFILENAMELENGTH-1);
      strncat(charptr,charptr2,MAXFILENAMELENGTH-1 - strlen(input_dir));
      *(charptr + GMV_MIN(strlen(input_dir) + strlen(charptr2), MAXFILENAMELENGTH-1)) = (char)0;
     }
   else
     {
      strncpy(charptr,charptr2,MAXFILENAMELENGTH-1);
      *(charptr + GMV_MIN(strlen(charptr2), MAXFILENAMELENGTH-1)) = (char)0;
     }

   /*  Only returning fromfile filename.  */
   if (fromfileskip == 1 && fromfileflag == 0)
     {

      /*  Check nodes, cells, faces and surface    */
      /*  and read numbers for the other keywords.  */
      if ((curr_keyword == NODES && nodes_read == 0) || 
          (curr_keyword == CELLS && cells_read == 0) ||
          (curr_keyword == FACES && faces_read == 0) || 
          (curr_keyword == VFACES && faces_read == 0) || 
          (curr_keyword == XFACES && faces_read == 0) || 
          (curr_keyword == SURFACE && surface_read == 0))
        {
         gmvin_sav = gmvinGlobal;
         ftype_sav = ftypeGlobal;

         ierr = gmvread_open_fromfileskip(charptr);
         if (ierr > 0)
            {
             fprintf(stderr,"GMV cannot read fromfile %s\n",charptr);
             errormsgvarlen = (int)strlen(charptr);
             gmv_data.errormsg = (char *)malloc((26 + errormsgvarlen) * sizeof(char));
             snprintf(gmv_data.errormsg,26 + errormsgvarlen,"GMV cannot read fromfile %s",charptr);
             return -1;
            }

         /*  Skip to fkeyword.  */
         ff_keyword = curr_keyword;
         fkeyword = curr_keyword;
         reading_fromfile = 1;
         while (9)
           {
            gmvread_data();
            if (gmv_data.keyword == fkeyword) break;
           }

         if (ff_keyword > -1)
           {
            fclose(gmvinGlobal);
            gmvinGlobal = gmvin_sav;
            ftypeGlobal = ftype_sav;
           }
         skipflag = 0;
         reading_fromfile = 0;
         ff_keyword = -1;
         readkeyword = 1;
         fromfileskip = 1;
        }

      fromfileflag = 1;
      gmv_data.keyword = curr_keyword;
      gmv_data.datatype = FROMFILE;
      i = (int)strlen(charptr);
      gmv_data.nchardata1 = i;
      gmv_data.chardata1 = (char *)malloc(i*sizeof(char));
      /*  No need for strncpy here because charptr is by design not too long.  */
      strcpy(gmv_data.chardata1,charptr);
      return 0;
     }

   if (fromfileskip == 1 && fromfileflag == 1) return 0;

   /*  Open fromfile, but do not allow more fromfiles.  */
   ierr = gmvread_open_fromfileskip(charptr);
   if (ierr > 0)
      {
       fprintf(stderr,"GMV cannot read fromfile %s\n",charptr);
       errormsgvarlen = (int)strlen(charptr);
       gmv_data.errormsg = (char *)malloc((26 + errormsgvarlen) * sizeof(char));
       snprintf(gmv_data.errormsg,26 + errormsgvarlen,"GMV cannot read fromfile %s",charptr);
       return -1;
      }
   printf("GMV reading %s from fromfile %s\n",sav_keyword,charptr);

   return 0;
}


void readnodes(FILE* gmvin, int ftype)
{
  int i, k, iswap=-1, lnxv=-1, lnyv=-1, lnzv=-1, lstructuredflag;
  long lnodes, tmplnodes;
  double *lxic = NULL, *lyic = NULL, *lzic = NULL, *tmpdouble; /* TODO: check fix for uninitialized pointer */
  long pos_after_lnodes, exp_cell_pos;
  float *tmpfloat;
  char ckkeyword[MAXKEYWORDLENGTH+64];

   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%ld",&lnodes); (void) res;
      ioerrtst(gmvin);
     }
   else
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {         
         binread(&lnodes,longlongsize,LONGLONG,(long)1,gmvin);
        }
      else
        {         
         binread(&i,intsize,INT,(long)1,gmvin);
         iswap = i;
         swapbytes(&iswap, intsize, 1);
         lnodes = i;
        }
     }

   /*  Check for byte swapping.  */
   if (lnodes < -10 && ftype != ASCII)
     {
      swapbytes_on = 1;
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
         swapbytes(&lnodes, longlongsize, 1);
      else
         lnodes = iswap;
     }
   /* Is more than the bottom half of the word full? */
   else if (ftype != ASCII && lnodes > 0xffff) 
     {
      tmplnodes = lnodes;
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
         swapbytes(&tmplnodes, longlongsize, 1);
      else 
         tmplnodes = iswap;
      if (tmplnodes == -2)
        {
         /* Byte swapped, structured grid */
         lnodes = tmplnodes;
         swapbytes_on = 1;
        }
      else 
        {
   
         /*  Unstructured grid, seek to "cells" keyword and see     */
         /*  if it is where we expect it. If so, not byte swapped.  */
         
         pos_after_lnodes = ftell(gmvin);
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
            exp_cell_pos = 3 * lnodes * doublesize;
         else
            exp_cell_pos = 3 * lnodes * floatsize;
         fseek(gmvin, exp_cell_pos, SEEK_CUR);
         if (ftype != ASCII)
           {
            binread(ckkeyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
            *(ckkeyword+MAXKEYWORDLENGTH)=(char)0;
           }
         if (ftype == ASCII) { int res = fscanf(gmvin,"%s",ckkeyword); (void) res; }
         if (strncmp(ckkeyword,"cells",5) != 0 &&
             strncmp(ckkeyword,"faces",5) != 0 &&
             strncmp(ckkeyword,"xfaces",6) != 0 &&
             strncmp(ckkeyword,"endgmv",6) != 0)
           {
            /* We are not where expected, must be byte swapped */

            swapbytes_on = 1;
            if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
               swapbytes(&lnodes, longlongsize, 1);
            else 
               lnodes = iswap;
           }

         /* Go back to the previous position */
         fseek(gmvin, pos_after_lnodes, SEEK_SET);
        }
     }
   else if (ftype != ASCII && lnodes == -1)
     {
      /* Special case, -1 is still -1 if byte swapped */
         
      pos_after_lnodes = ftell(gmvin);

      binread(&lnxv,intsize,INT,(long)1,gmvin);
      binread(&lnyv,intsize,INT,(long)1,gmvin);
      binread(&lnzv,intsize,INT,(long)1,gmvin);

      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
         exp_cell_pos = (lnxv + lnyv + lnzv) * doublesize;
      else
         exp_cell_pos = (lnxv + lnyv + lnzv) * floatsize;
      fseek(gmvin, exp_cell_pos, SEEK_CUR);
      if (ftype != ASCII)
        {
         binread(ckkeyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
         *(ckkeyword+MAXKEYWORDLENGTH)=(char)0;
        }
      if (ftype == ASCII) { int res = fscanf(gmvin,"%s",ckkeyword); (void) res; }
      if (strncmp(ckkeyword,"cells",5) != 0 &&
          strncmp(ckkeyword,"faces",5) != 0 &&
          strncmp(ckkeyword,"vfaces",6) != 0)
        {
         /* We are not where expected, must be byte swapped */

         swapbytes_on = 1;
        }
 
      /* Go back to the previous position */
 
      fseek(gmvin, pos_after_lnodes, SEEK_SET);
     }

   /*  If lnodes is -1, then block structured grid.      */
   /*  If lnodes is -2, then logically structured grid.  */
   /*  Read lnxv, lnyv, lnzv. for structured grids.      */
   lstructuredflag = 0;
   structflag_in = 0;
   if (lnodes == -1 || lnodes == -2)
     {
      if (lnodes == -1) lstructuredflag = 1;
      if (lnodes == -2) lstructuredflag = 2;
      if (ftype != ASCII)
        {
         binread(&lnxv,intsize,INT,(long)1,gmvin);
         binread(&lnyv,intsize,INT,(long)1,gmvin);
         binread(&lnzv,intsize,INT,(long)1,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) 
        {
         int res = fscanf(gmvin,"%d%d%d",&lnxv,&lnyv,&lnzv); (void) res;
         ioerrtst(gmvin);
        }

      lnodes = lnxv * lnyv * lnzv;
      structflag_in = 1;
     }

   /*  If lnodes is -3, then amr grid.  Read      */
   /*  lnxv, lnyv, lnzv, start xyz and dx,dy,dz.  */
   amrflag_in = 0;
   if (lnodes == -3)
     {
      amrflag_in = 1;
      if (ftype != ASCII)
        {
         binread(&lnxv,intsize,INT,(long)1,gmvin);
         binread(&lnyv,intsize,INT,(long)1,gmvin);
         binread(&lnzv,intsize,INT,(long)1,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) 
        {
         int res = fscanf(gmvin,"%d%d%d",&lnxv,&lnyv,&lnzv); (void) res;
         ioerrtst(gmvin);
        }
   
      lxic = (double *)malloc((3)*sizeof(double));
      lyic = (double *)malloc((3)*sizeof(double));
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            binread(lxic,doublesize,DOUBLE,(long)3,gmvin);
            ioerrtst(gmvin);
            binread(lyic,doublesize,DOUBLE,(long)3,gmvin);
            ioerrtst(gmvin);
           }
         else
           {
            tmpfloat = (float *)malloc((3)*sizeof(float));
            binread(tmpfloat,floatsize,FLOAT,(long)3,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < 3; i++) lxic[i] = tmpfloat[i];
            binread(tmpfloat,floatsize,FLOAT,(long)3,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < 3; i++) lyic[i] = tmpfloat[i];
            free(tmpfloat);
           }
        }
      if (ftype == ASCII)
        {
         rdfloats(lxic,(long)3,gmvin);
         rdfloats(lyic,(long)3,gmvin);
        }

      gmv_data.keyword = NODES;
      gmv_data.datatype = AMR;
      gmv_data.num2 = lnxv;
      gmv_data.nlongdata1 = lnyv;
      gmv_data.nlongdata2 = lnzv;
      gmv_data.ndoubledata1 = 3;
      gmv_data.doubledata1 = lxic;
      gmv_data.ndoubledata2 = 3;
      gmv_data.doubledata2 = lyic;
      numnodes = lnxv * lnyv * lnzv;
      nodes_read = 1;
      return;
     }

   if (printon)
      printf("Reading %ld nodes.\n",lnodes);

   /*  Allocate and read node x,y,z arrays.  */

   if (lstructuredflag == 0 || lstructuredflag == 2)
     {
      lxic = (double *)malloc((lnodes)*sizeof(double));
      lyic = (double *)malloc((lnodes)*sizeof(double));
      lzic = (double *)malloc((lnodes)*sizeof(double));
      if (lxic == NULL || lyic == NULL || lzic == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            tmpdouble = (double *)malloc((3*lnodes)*sizeof(double));
            if (tmpdouble == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpdouble,doublesize,DOUBLE,3*lnodes,gmvin);
            ioerrtst(gmvin);
            if (node_inp_type == 0)  /*  nodes type  */
              {
               for (i = 0; i < lnodes; i++)
                 {
                  lxic[i] = tmpdouble[i];
                  lyic[i] = tmpdouble[lnodes+i];
                  lzic[i] = tmpdouble[2*lnodes+i];
                 }
              }
            if (node_inp_type == 1)  /*  nodev type  */
              {
               for (i = 0; i < lnodes; i++)
                 {
                  lxic[i] = tmpdouble[3*i];
                  lyic[i] = tmpdouble[3*i+1];
                  lzic[i] = tmpdouble[3*i+2];
                 }
              }
            FREE(tmpdouble);
           }
         else
           {
            tmpfloat = (float *)malloc((3*lnodes)*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpfloat,floatsize,FLOAT,3*lnodes,gmvin);
            ioerrtst(gmvin);
            if (node_inp_type == 0)  /*  nodes type  */
              {
               for (i = 0; i < lnodes; i++)
                 {
                  lxic[i] = tmpfloat[i];
                  lyic[i] = tmpfloat[lnodes+i];
                  lzic[i] = tmpfloat[2*lnodes+i];
                 }
              }
            if (node_inp_type == 1)  /*  nodev type  */
              {
               for (i = 0; i < lnodes; i++)
                 {
                  lxic[i] = tmpfloat[3*i];
                  lyic[i] = tmpfloat[3*i+1];
                  lzic[i] = tmpfloat[3*i+2];
                 }
              }
            FREE(tmpfloat);
           }
        }
      if (ftype == ASCII)
        {
         tmpdouble = (double *)malloc((3*lnodes)*sizeof(double));
         if (tmpdouble == NULL)
              {
               gmvrdmemerr();
               return;
              }
         rdfloats(tmpdouble,3*lnodes,gmvin);
         if (node_inp_type == 0)  /*  nodes type  */
           {
            for (i = 0; i < lnodes; i++)
              {
               lxic[i] = tmpdouble[i];
               lyic[i] = tmpdouble[lnodes+i];
               lzic[i] = tmpdouble[2*lnodes+i];
              }
           }
         if (node_inp_type == 1)  /*  nodev type  */
           {
            for (i = 0; i < lnodes; i++)
              {
               lxic[i] = tmpdouble[3*i];
               lyic[i] = tmpdouble[3*i+1];
               lzic[i] = tmpdouble[3*i+2];
              }
           }
         FREE(tmpdouble);
        }
     }

   if (lstructuredflag == 1)
     {
      lxic = (double *)malloc((lnxv)*sizeof(double));
      lyic = (double *)malloc((lnyv)*sizeof(double));
      lzic = (double *)malloc((lnzv)*sizeof(double));
      if (lxic == NULL || lyic == NULL || lzic == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            binread(lxic,doublesize,DOUBLE,(long)lnxv,gmvin);
            ioerrtst(gmvin);
            binread(lyic,doublesize,DOUBLE,(long)lnyv,gmvin);
            ioerrtst(gmvin);
            binread(lzic,doublesize,DOUBLE,(long)lnzv,gmvin);
            ioerrtst(gmvin);
           }
         else
           {
            k = (lnxv > lnyv) ? lnxv : lnyv;
            k = (k > lnzv) ? k : lnzv;
            tmpfloat = (float *)malloc((k)*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpfloat,floatsize,FLOAT,(long)lnxv,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < lnxv; i++) lxic[i] = tmpfloat[i];
            binread(tmpfloat,floatsize,FLOAT,(long)lnyv,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < lnyv; i++) lyic[i] = tmpfloat[i];
            binread(tmpfloat,floatsize,FLOAT,(long)lnzv,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < lnzv; i++) lzic[i] = tmpfloat[i];
            free(tmpfloat);
           }
        }
      if (ftype == ASCII)
        {
         rdfloats(lxic,(long)lnxv,gmvin);
         rdfloats(lyic,(long)lnyv,gmvin);
         rdfloats(lzic,(long)lnzv,gmvin);
        }
     }

   if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading nodes.\n");
      gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
      snprintf(gmv_data.errormsg,31,"I/O error while reading nodes.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   if (!skipflag)
     {
      numnodes = lnodes;
      nodes_read = 1;
     }

   if (amrflag_in == 0) 
     {
      gmv_data.keyword = NODES;
      if (lstructuredflag == 0) gmv_data.datatype = UNSTRUCT;
      if (lstructuredflag == 1) gmv_data.datatype = STRUCT;
      if (lstructuredflag == 2) gmv_data.datatype = LOGICALLY_STRUCT;
      gmv_data.num = lnodes;
      gmv_data.num2 = NODES;
      if (node_inp_type == 1) gmv_data.num2 = NODE_V;
      if (lstructuredflag == 1 || lstructuredflag == 2)
        {
         gmv_data.ndoubledata1 = lnxv;
         gmv_data.doubledata1 = lxic;
         gmv_data.ndoubledata2 = lnyv;
         gmv_data.doubledata2 = lyic;
         gmv_data.ndoubledata3 = lnzv;
         gmv_data.doubledata3 = lzic;
         ncells_struct = (lnxv-1) * (lnyv-1) * (lnzv-1);
         if (lnzv == 1) ncells_struct = (lnxv-1) * (lnyv-1);
        }
      else
        {
         gmv_data.ndoubledata1 = lnodes;
         gmv_data.doubledata1 = lxic;
         gmv_data.ndoubledata2 = lnodes;
         gmv_data.doubledata2 = lyic;
         gmv_data.ndoubledata3 = lnodes;
         gmv_data.doubledata3 = lzic;
        }
     }
}


void readcells(FILE* gmvin, int ftype)
{
  int i, ndat=-1, nverts[MAXVERTS], totverts, *verts,
      *cellnodenos, nfaces;
  long *lfaces, numtop, *daughters;
  long *lcellnodenos;
  char keyword[MAXKEYWORDLENGTH+64];

  for (i = 0; i < MAXVERTS; ++i)
  {
     nverts[i] = -1;
  }

   if (readkeyword == 1)
     {
      numcellsin = 0;
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%ld",&lncells); (void) res;
         ioerrtst(gmvin);
        }
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {         
            binread(&lncells,longlongsize,LONGLONG,(long)1,gmvin);
           }
         else
           {         
            binread(&i,intsize,INT,(long)1,gmvin);
            lncells = i;
           }
        }

      if (printon)
         printf("Reading %ld cells.\n",lncells);

      if (!skipflag)
        {
         numcells = lncells;
         cells_read = 1;
        }
     }

   /*  If amr, read numtop and daughter list.  */
   if (amrflag_in)
     {
      if (ftype == ASCII)
        {
         int res =fscanf(gmvin,"%ld",&numtop); (void) res;
         ioerrtst(gmvin);
        }
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {         
            binread(&numtop,longlongsize,LONGLONG,(long)1,gmvin);
           }
         else
           {         
            binread(&i,intsize,INT,(long)1,gmvin);
            numtop = i;
           }
        }

      daughters = (long *)malloc(lncells*sizeof(long));
      if (daughters == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R4 || ftype == IEEEI4R8)
           {
            cellnodenos = (int *)malloc(lncells*sizeof(int));
            if (cellnodenos == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(cellnodenos,intsize,INT,lncells,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < lncells; i++)
              daughters[i] = cellnodenos[i];
            FREE(cellnodenos);
           }
         else
           {
            binread(daughters,longlongsize,LONGLONG,lncells,gmvin);
            ioerrtst(gmvin);
           }
        }
      if (ftype == ASCII) rdlongs(daughters,lncells,gmvin);

      if (gmv_data.keyword == GMVERROR) return;

      gmv_data.keyword = CELLS;
      gmv_data.datatype = AMR;
      gmv_data.num = lncells;
      gmv_data.num2 = numtop;
      gmv_data.nlongdata1 = lncells;
      gmv_data.longdata1 = daughters;
      numcells = numtop;
      readkeyword = 1;
      return;      
     }

   /*  See if all cells read.  */
   numcellsin++;
   if (numcellsin > lncells)
     {
      readkeyword = 2;
      if (numcells == 0) readkeyword = 1;

      /*  If structured mesh, reset numcells.  */
      if (structflag_in == 1) numcells = ncells_struct;
      gmv_data.keyword = CELLS;
      gmv_data.datatype = ENDKEYWORD;
      gmv_data.num = numcells;
      return;
     }

   /*  Read a cell at at time.  */
   if (ftype != ASCII)
     {
      binread(keyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(keyword+MAXKEYWORDLENGTH)=(char)0;
      binread(&ndat,intsize,INT,(long)1,gmvin);
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) { int res = fscanf(gmvin,"%s%d",keyword,&ndat); (void) res; }

   /*  Check cell type.  */
   if (strncmp(keyword,"general",7) != 0 &&
       strncmp(keyword,"line",4) != 0 &&
       strncmp(keyword,"tri",3) != 0 &&
       strncmp(keyword,"quad",4) != 0 &&
       strncmp(keyword,"tet",3) != 0 &&
       strncmp(keyword,"hex",3) != 0 &&
       strncmp(keyword,"prism",5) != 0 &&
       strncmp(keyword,"pyramid",7) != 0 &&
       strncmp(keyword,"vface2d",7) != 0 &&
       strncmp(keyword,"vface3d",7) != 0 &&
       strncmp(keyword,"phex8",5) != 0 &&
       strncmp(keyword,"phex20",6) != 0 &&
       strncmp(keyword,"phex27",6) != 0 &&
       strncmp(keyword,"ppyrmd5",7) != 0 &&
       strncmp(keyword,"ppyrmd13",8) != 0 &&
       strncmp(keyword,"pprism6",7) != 0 &&
       strncmp(keyword,"pprism15",8) != 0 &&
       strncmp(keyword,"ptet4",5) != 0 &&
       strncmp(keyword,"ptet10",6) != 0 &&
       strncmp(keyword,"6tri",4) != 0 &&
       strncmp(keyword,"8quad",5) != 0 &&
       strncmp(keyword,"3line",5) != 0)
     {
      fprintf(stderr,
              "Error, %s is an invalid cell type.\n",keyword);
      errormsgvarlen = (int)strlen(keyword);
      gmv_data.errormsg = (char *)malloc((33 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,33 + errormsgvarlen,"Error, %s is an invalid cell type.",keyword);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if ((strncmp(keyword,"line",4) == 0 && ndat != 2) ||
       (strncmp(keyword,"tri",3) == 0 && ndat != 3) ||
       (strncmp(keyword,"quad",4) == 0 && ndat != 4) ||
       (strncmp(keyword,"tet",3) == 0 && ndat != 4) ||
       (strncmp(keyword,"hex",3) == 0 && ndat != 8) ||
       (strncmp(keyword,"prism",5) == 0 && ndat != 6) ||
       (strncmp(keyword,"pyramid",7) == 0 && ndat != 5) ||
       (strncmp(keyword,"phex8",5) == 0 && ndat != 8) ||
       (strncmp(keyword,"phex20",6) == 0 && ndat != 20) ||
       (strncmp(keyword,"phex27",6) == 0 && ndat != 27) ||
       (strncmp(keyword,"ppyrmd5",7) == 0 && ndat != 5) ||
       (strncmp(keyword,"ppyrmd13",8) == 0 && ndat != 13) ||
       (strncmp(keyword,"pprism6",7) == 0 && ndat != 6) ||
       (strncmp(keyword,"pprism15",8) == 0 && ndat != 15) ||
       (strncmp(keyword,"ptet4",5) == 0 && ndat != 4) ||
       (strncmp(keyword,"ptet10",6) == 0 && ndat != 10) ||
       (strncmp(keyword,"6tri",4) == 0 && ndat != 6) ||
       (strncmp(keyword,"8quad",5) == 0 && ndat != 8) ||
       (strncmp(keyword,"3line",5) == 0 && ndat != 3))
     {
      fprintf(stderr,
              "Error, %d nodes is invalid for a %s.\n",ndat,keyword);
      errormsgvarlen = (int)strlen(keyword) + 20; /* 20 = ceil(log10(2**sizeof(int64))) for 'ndat' */
      gmv_data.errormsg = (char *)malloc((32 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,32 + errormsgvarlen,"Error, %d nodes is invalid for a %s.",ndat,keyword);
      gmv_data.keyword = GMVERROR;
      return;
     }

   if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading cells.\n");
      gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
      snprintf(gmv_data.errormsg,31,"I/O error while reading cells.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  At first cell, set vfaceflag.  */
   if (readkeyword == 1)
     {
      vfaceflag = 0;
      if (strncmp(keyword,"vface2d",7) == 0) vfaceflag = 2;
      if (strncmp(keyword,"vface3d",7) == 0) vfaceflag = 3;
     }

   /*  After first cell, check for vface consistency.  */
   if (readkeyword == 0)
     {
      if ((vfaceflag == 0 && strncmp(keyword,"vface",5) == 0) ||
          (vfaceflag > 0 && strncmp(keyword,"vface",5) != 0))
        {
         fprintf(stderr,
           "Error, cannot mix vface2d or vface3d with other cell types.\n");
         gmv_data.errormsg = (char *)malloc(60 * sizeof(char));
         snprintf(gmv_data.errormsg,60,"Error, cannot mix vface2d or vface3d with other cell types.");
         gmv_data.keyword = GMVERROR;
         return;
        }
      if ((vfaceflag == 2 && strncmp(keyword,"vface3d",7) == 0) ||
          (vfaceflag == 3 && strncmp(keyword,"vface2d",7) == 0))
        {
         fprintf(stderr,
           "Error, cannot mix vface2d and vface3d cell types.\n");
         gmv_data.errormsg = (char *)malloc(50 * sizeof(char));
         snprintf(gmv_data.errormsg,50,"Error, cannot mix vface2d and vface3d cell types.");
         gmv_data.keyword = GMVERROR;
         return;
        }
     }

   /*  Read general cell data.  */
   if (strncmp(keyword,"general",7) == 0)
     {

      /*  Read no. of vertices per face.  */
      nfaces = ndat;
      if (nfaces > MAXFACES)
        {
         fprintf(stderr,
                  "Error, Read %d faces - %d faces per cell allowed.\n",
                   nfaces, MAXFACES);
         errormsgvarlen = 40; /* 20 = ceil(log10(2**sizeof(int64))) for 'nfaces', 'MAXFACES' */
         gmv_data.errormsg = (char *)malloc((46 + errormsgvarlen) * sizeof(char));
         snprintf(gmv_data.errormsg,46 + errormsgvarlen,"Error, Read %d faces - %d faces per cell allowed.",nfaces, MAXFACES);
         gmv_data.keyword = GMVERROR;
         return;
        }
      if (ftype != ASCII) binread(nverts,intsize,INT,(long)nfaces,gmvin);
      if (ftype == ASCII) rdints(nverts,nfaces,gmvin);
      ioerrtst(gmvin);
      if (gmv_data.keyword == GMVERROR) return;

      /*  Read all face vertices, reallocate if needed.  */
      totverts = 0;
      for (i = 0; i < nfaces; i++) totverts += nverts[i];
      lcellnodenos = (long *)malloc(totverts*sizeof(long));
      if (lcellnodenos == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R4 || ftype == IEEEI4R8)
           {
            cellnodenos = (int *)malloc(totverts*sizeof(int));
            if (cellnodenos == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(cellnodenos,intsize,INT,(long)totverts,gmvin);
            for (i = 0; i < totverts; i++)
              lcellnodenos[i] = cellnodenos[i];
            FREE(cellnodenos);
           }
         else
            binread(lcellnodenos,longlongsize,LONGLONG, 
                    (long)totverts,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) rdlongs(lcellnodenos,(long)totverts,gmvin);
      if (gmv_data.keyword == GMVERROR) return;
      lfaces = (long *)malloc(nfaces*sizeof(long));
      if (lfaces == NULL)
        {
         gmvrdmemerr();
         return;
        }
      for (i = 0; i < nfaces; i++)
        lfaces[i] = nverts[i];

      gmv_data.keyword = CELLS;
      gmv_data.datatype = GENERAL;
      strcpy(gmv_data.name1,keyword);
      gmv_data.num = lncells;
      gmv_data.num2 = nfaces;
      gmv_data.nlongdata1 = nfaces;
      gmv_data.longdata1 = lfaces;
      gmv_data.nlongdata2 = totverts;
      gmv_data.longdata2 = lcellnodenos;
     }

   /*  Else read vface2d or vface3d data.  */
   else if (strncmp(keyword,"vface2d",7) == 0 ||
            strncmp(keyword,"vface3d",7) == 0)
     {

      /*  Check no. of  faces.  */
      nfaces = ndat;
      if (nfaces > MAXFACES)
        {
         fprintf(stderr,
                  "Error, Read %d faces - %d faces per cell allowed.\n",
                   nfaces, MAXFACES);
         errormsgvarlen = 40; /* 20 = ceil(log10(2**sizeof(int64))) for 'nfaces', 'MAXFACES' */
         gmv_data.errormsg = (char *)malloc((46 + errormsgvarlen) * sizeof(char));
         snprintf(gmv_data.errormsg,46 + errormsgvarlen,"Error, Read %d faces - %d faces per cell allowed.",nfaces, MAXFACES);
         gmv_data.keyword = GMVERROR;
         return;
        }

      /*  Read all face numbers.  */
      lfaces = (long *)malloc(nfaces*sizeof(long));
      if (lfaces == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R4 || ftype == IEEEI4R8)
           {
            cellnodenos = (int *)malloc(nfaces*sizeof(int));
            if (cellnodenos == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(cellnodenos,intsize,INT,(long)nfaces,gmvin);
            for (i = 0; i < nfaces; i++)
              lfaces[i] = cellnodenos[i];
            FREE(cellnodenos);
           }
         else
            binread(lfaces,longlongsize,LONGLONG, 
                    (long)nfaces,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) rdlongs(lfaces,(long)nfaces,gmvin);
/*
      for (i = 0; i < nfaces; i++)
         lfaces[i]--;
*/

      if (gmv_data.keyword == GMVERROR) return;
      
      gmv_data.keyword = CELLS;
      if (strncmp(keyword,"vface2d",7) == 0)
         gmv_data.datatype = VFACE2D;
      else gmv_data.datatype = VFACE3D;
      strncpy(gmv_data.name1, keyword, MAXKEYWORDLENGTH-1);
      *(gmv_data.name1 + GMV_MIN(strlen(keyword), MAXKEYWORDLENGTH-1)) = (char)0;
      gmv_data.num = lncells;
      gmv_data.num2 = nfaces;
      gmv_data.nlongdata1 = nfaces;
      gmv_data.longdata1 = lfaces;
     }

   /*  Else read regular cells.  */
   else 
     {
      lcellnodenos = (long *)malloc(ndat*sizeof(long));
      if (lcellnodenos == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R4 || ftype == IEEEI4R8)
           {
            verts = (int *)malloc(ndat*sizeof(int));
            if (verts == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(verts,intsize,INT,(long)ndat,gmvin);
            for (i = 0; i < ndat; i++)
              lcellnodenos[i] = verts[i];
            free(verts);
           }
         else
            binread(lcellnodenos,longlongsize,LONGLONG,(long)ndat,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) rdlongs(lcellnodenos,(long)ndat,gmvin);

      if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
        {
         fprintf(stderr,"I/O error while reading cells.\n");
         gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
         snprintf(gmv_data.errormsg,31,"I/O error while reading cells.");
         gmv_data.keyword = GMVERROR;
         return;
        }
      if (gmv_data.keyword == GMVERROR) return;

      gmv_data.keyword = CELLS;
      gmv_data.datatype = REGULAR;
      strcpy(gmv_data.name1,keyword);
      strncpy(gmv_data.name1, keyword, MAXCUSTOMNAMELENGTH-1);
      *(gmv_data.name1 + GMV_MIN(strlen(keyword), MAXCUSTOMNAMELENGTH-1)) = (char)0;
      gmv_data.num = lncells;
      gmv_data.num2 = ndat;
      gmv_data.nlongdata1 = ndat;
      gmv_data.longdata1 = lcellnodenos;
     }
}


void readfaces(FILE* gmvin, int ftype)
{
  int i, nverts=0, *tmpvertsin;
  long *vertsin;

   if (readkeyword == 1)
     {

      /*  Read no. of faces to read and cells to generate.  */
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%ld",&lnfaces); (void) res;
         res = fscanf(gmvin,"%ld",&lncells);
        }
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(&lnfaces,longlongsize,LONGLONG,(long)1,gmvin);
            binread(&lncells,longlongsize,LONGLONG,(long)1,gmvin);
           }
         else
           {
            binread(&i,intsize,INT,(long)1,gmvin);
            lnfaces = i;
            binread(&i,intsize,INT,(long)1,gmvin);
            lncells = i;
           }
        }
      ioerrtst(gmvin);
      numfacesin = 0;

      if (printon)
         printf("Reading %ld faces.\n",lnfaces);

      if (!skipflag)
        {
         numfaces = lnfaces;
         numcells = lncells;
         faces_read = 1;
        }
     }

   /*  Check no. of faces read.  */
   numfacesin++;
   if (numfacesin > lnfaces)
     {
      readkeyword = 2;
      gmv_data.keyword = FACES;
      gmv_data.num = lnfaces;
      gmv_data.num2 = lncells;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read face vertices and cell info for each face.  */
   if (ftype != ASCII) binread(&nverts,intsize,INT,(long)1,gmvin);
   if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&nverts); (void) res; }
   ioerrtst(gmvin);

   /*  Read a face vertices and cells.  */
   vertsin = (long *)malloc((nverts+2)*sizeof(long));
   if (vertsin == NULL)
     {
      gmvrdmemerr();
      return;
     }
   if (ftype == ASCII) rdlongs(vertsin,(long)nverts+2,gmvin);
   else
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(vertsin,longlongsize,LONGLONG,(long)nverts+2,gmvin);
        }
      else 
        {
         tmpvertsin = (int *)malloc((nverts+2)*sizeof(int));
         if (tmpvertsin == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpvertsin,intsize,INT,(long)nverts+2,gmvin);
         for (i = 0; i < nverts+2; i++)
            vertsin[i] = tmpvertsin[i];
         free(tmpvertsin);
        }
      ioerrtst(gmvin);
     }

   if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading faces.\n");
      gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
      snprintf(gmv_data.errormsg,31,"I/O error while reading faces.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = FACES;
   gmv_data.datatype = REGULAR;
   gmv_data.num = lnfaces;
   gmv_data.num2 = lncells;
   gmv_data.nlongdata1 = nverts + 2;
   gmv_data.longdata1 = vertsin;
}


void readvfaces(FILE* gmvin, int ftype)
{
  int i, nverts=0, *tmpvertsin, facepe=-1, oppfacepe=-1;
  long *vertsin, oppface=-1, cellid=-1;

   if (readkeyword == 1)
     {

      /*  Read no. of vfaces to read.  */
      if (ftype == ASCII) {
        int res = fscanf(gmvin,"%ld",&lnfaces); (void) res;
      }
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(&lnfaces,longlongsize,LONGLONG,(long)1,gmvin);
           }
         else
           {
            binread(&i,intsize,INT,(long)1,gmvin);
            lnfaces = i;
           }
        }
      ioerrtst(gmvin);
      numfacesin = 0;

      if (printon)
         printf("Reading %ld vfaces.\n",lnfaces);

      if (!skipflag)
        {
         numfaces = lnfaces;
        }
     }

   /*  Check no. of vfaces read.  */
   numfacesin++;
   if (numfacesin > lnfaces)
     {
      readkeyword = 2;
      gmv_data.keyword = VFACES;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read vface vertices, PE number, opposite  */
   /*  face info and cell info for each face.    */
   if (ftype != ASCII)
     {
      binread(&nverts,intsize,INT,(long)1,gmvin);
      binread(&facepe,intsize,INT,(long)1,gmvin);
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(&oppface,longlongsize,LONGLONG,(long)1,gmvin);
         binread(&oppfacepe,intsize,INT,(long)1,gmvin);
         binread(&cellid,longlongsize,LONGLONG,(long)1,gmvin);
        }
      else 
        {
         binread(&i,intsize,INT,(long)1,gmvin);
         oppface = i;
         binread(&oppfacepe,intsize,INT,(long)1,gmvin);
         binread(&i,intsize,INT,(long)1,gmvin);
         cellid = i;
        }
     }
   if (ftype == ASCII)
      {
       int res = 0; (void) res;
       res = fscanf(gmvin,"%d%d",&nverts,&facepe);
       res = fscanf(gmvin,"%ld",&oppface);
       res = fscanf(gmvin,"%d",&oppfacepe);
       res = fscanf(gmvin,"%ld",&cellid);
      }
   ioerrtst(gmvin);

   /*  Read vface vertices.  */
   vertsin = (long *)malloc((nverts)*sizeof(long));
   if (vertsin == NULL)
     {
      gmvrdmemerr();
      return;
     }
   if (ftype == ASCII) rdlongs(vertsin,(long)nverts,gmvin);
   else
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(vertsin,longlongsize,LONGLONG,(long)nverts,gmvin);
        }
      else 
        {
         tmpvertsin = (int *)malloc((nverts)*sizeof(int));
         if (tmpvertsin == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpvertsin,intsize,INT,(long)nverts,gmvin);
         for (i = 0; i < nverts; i++)
            vertsin[i] = tmpvertsin[i];
         free(tmpvertsin);
        }
      ioerrtst(gmvin);
     }

   if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading faces.\n");
      gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
      snprintf(gmv_data.errormsg,31,"I/O error while reading faces.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = VFACES;
   gmv_data.datatype = REGULAR;
   gmv_data.num = lnfaces;
   gmv_data.nlongdata1 = nverts;
   gmv_data.longdata1 = vertsin;
   gmv_data.nlongdata2 = 4;
   gmv_data.longdata2 = (long *)malloc(4*sizeof(long));
   gmv_data.longdata2[0] = facepe;
   gmv_data.longdata2[1] = oppface;
   /* gmv_data.longdata2[1] = oppface - 1; */
   gmv_data.longdata2[2] = oppfacepe;
   gmv_data.longdata2[3] = cellid;
}


void readxfaces(FILE* gmvin, int ftype)
{
  int i, j, *tmpftvin;
  long *nxvertsin, *vertsin, totverts;
  static int xfaceloc;


   if (readkeyword == 1)
     {

      /*  Read no. of xfaces to read.  */
      if (ftype == ASCII) { int res = fscanf(gmvin,"%ld",&lnfaces); (void) res; }
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(&lnfaces,longlongsize,LONGLONG,(long)1,gmvin);
           }
         else
           {
            binread(&i,intsize,INT,(long)1,gmvin);
            lnfaces = i;
           }
        }
      ioerrtst(gmvin);
      xfaceloc = 0;

      if (printon)
         printf("Reading %ld xfaces.\n",lnfaces);

      if (!skipflag)
        {
         numfaces = lnfaces;
         faces_read = 1;
        }
     }

   /*  Check location of xfaces to determine what to read.  */
   if (xfaceloc == 0)
     {

      /*  Allocate and read the face to nodes array.  */
      nxvertsin = (long *)malloc(lnfaces*sizeof(long));
      if (nxvertsin == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype == ASCII) rdlongs(nxvertsin,(long)lnfaces,gmvin);
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(nxvertsin,longlongsize,LONGLONG,(long)lnfaces,gmvin);
           }
         else 
           {
            tmpftvin = (int *)malloc(lnfaces*sizeof(int));
            if (tmpftvin == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpftvin,intsize,INT,(long)lnfaces,gmvin);
            for (i = 0; i < lnfaces; i++)
              {
               nxvertsin[i] = tmpftvin[i];
              }
            free(tmpftvin);
           }
        }
      ioerrtst(gmvin);
      if (gmv_data.keyword == GMVERROR) return;

      /*  Count the total number of vertices to read.  */
      totverts = 0;
      for (i = 0; i < lnfaces; i++)
        {
         totverts += nxvertsin[i];
        }

      /*  Allocate and read the vertices.  */
      vertsin = (long *)malloc(totverts*sizeof(long));
      if (vertsin == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype == ASCII) rdlongs(vertsin,(long)totverts,gmvin);
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(vertsin,longlongsize,LONGLONG,(long)totverts,gmvin);
           }
         else 
           {
            tmpftvin = (int *)malloc(totverts*sizeof(int));
            if (tmpftvin == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpftvin,intsize,INT,(long)totverts,gmvin);
            for (i = 0; i < totverts; i++)
              {
               vertsin[i] = tmpftvin[i];
              }
            free(tmpftvin);
           }
        }
      ioerrtst(gmvin);
      if (gmv_data.keyword == GMVERROR) return;

      gmv_data.nlongdata1 = lnfaces;
      gmv_data.longdata1 = nxvertsin;
      gmv_data.nlongdata2 = totverts;
      gmv_data.longdata2 = vertsin;
     }

   if (xfaceloc > 0 && xfaceloc < 5)
     {

      /*  Allocate and read face sized data.  */
      /*  If xfaceloc == 1, cell per face.    */
      /*  If xfaceloc == 2, opposite face.    */
      /*  If xfaceloc == 3, face pe.          */
      /*  If xfaceloc == 4, opposite face pe. */

      vertsin = (long *)malloc(lnfaces*sizeof(long));
      if (vertsin == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype == ASCII) rdlongs(vertsin,(long)lnfaces,gmvin);
      else
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(vertsin,longlongsize,LONGLONG,(long)lnfaces,gmvin);
           }
         else 
           {
            tmpftvin = (int *)malloc(lnfaces*sizeof(int));
            if (tmpftvin == NULL)
              {
               gmvrdmemerr();
               return;
             }
            binread(tmpftvin,intsize,INT,(long)lnfaces,gmvin);
            for (i = 0; i < lnfaces; i++)
              {
               vertsin[i] = tmpftvin[i];
              }
            free(tmpftvin);
           }
        }
      ioerrtst(gmvin);
      if (gmv_data.keyword == GMVERROR) return;

      /*  Get the number of cells from the largest cell  */
      /*  when reading the cells per face data.          */
      if (xfaceloc == 1)
        {
         j = 0;
         for (i = 0; i < lnfaces; i++)
           {
            if (j < vertsin[i]) j = vertsin[i];
           }
         numcells = lncells = j;
        }

      gmv_data.nlongdata1 = lnfaces;
      gmv_data.longdata1 = vertsin;
     }

   gmv_data.keyword = XFACES;
   gmv_data.datatype = REGULAR;
   gmv_data.num = lnfaces;
   gmv_data.num2 = xfaceloc;
   if (xfaceloc > 4)
     {
      readkeyword = 2;
      gmv_data.num = lnfaces;
      gmv_data.num2 = lncells;
      gmv_data.datatype = ENDKEYWORD;
     }

   xfaceloc++;
}


void readmats(FILE* gmvin, int ftype)
{
  /*                               */
  /*  Read and set material data.  */
  /*                               */
  int i=-1, data_type=0, *matin, lnmatin=-1, lmmats;
  char mname[MAXCUSTOMNAMELENGTH], *matnames;

   /*  Read no. of materials and data type (cells or nodes).  */
   if (ftype != ASCII)
      binread(&lmmats,intsize,INT,(long)1,gmvin);
   else { int res = fscanf(gmvin,"%d",&lmmats); (void) res; }
   ioerrtst(gmvin);

   if (ftype != ASCII)
      binread(&i,intsize,INT,(long)1,gmvin);
   if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&i); (void) res; }
   ioerrtst(gmvin);
   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell materials.\n");
      gmv_data.errormsg = (char *)malloc(42 * sizeof(char));
      snprintf(gmv_data.errormsg,42,"Error, no cells exist for cell materials.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node materials.\n");
      gmv_data.errormsg = (char *)malloc(42 * sizeof(char));
      snprintf(gmv_data.errormsg,42,"Error, no nodes exist for node materials.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Allocate and read 8 or 32 character material names.  */
   matnames = (char *)malloc(lmmats*MAXCUSTOMNAMELENGTH*sizeof(char));
   if (matnames == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < lmmats; i++)
      {
      if (ftype != ASCII)
        {
         binread(mname, charsize_in*charsize, CHAR, (long)1, gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%s",mname); (void) res;
         ioerrtst(gmvin);
        }
      strncpy(&matnames[i*MAXCUSTOMNAMELENGTH],mname,MAXCUSTOMNAMELENGTH-1);
      *(matnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
     }

   /*  Allocate and read material nos.  */
   if (data_type == CELL) lnmatin = numcells;
   if (data_type == NODE) lnmatin = numnodes;
   matin=(int *)malloc(lnmatin*sizeof(int));
   if (matin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(matin,intsize,INT,(long)lnmatin,gmvin);
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdints(matin,lnmatin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = MATERIAL;
   gmv_data.datatype = data_type;
   gmv_data.num = lmmats;
   gmv_data.nchardata1 = lmmats;
   gmv_data.chardata1 = matnames;
   gmv_data.nlongdata1 = lnmatin;
   gmv_data.longdata1 = (long *)malloc(lnmatin*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < lnmatin; i++)
      gmv_data.longdata1[i] = matin[i];
   free(matin);
}


void readvels(FILE* gmvin, int ftype)
{
  /*                               */
  /*  Read and set velocity data.  */
  /*                               */
  int i=-1, data_type=-1, nvelin=-1;
  double *uin, *vin, *win;
  float *tmpfloat;

   /*  Read data type (cells, nodes or faces).  */
   if (ftype != ASCII) binread(&i,intsize,INT,(long)1,gmvin);
   if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&i); (void) res; }
   ioerrtst(gmvin);
   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;
   if (i == 2) data_type = FACE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell velocities.\n");
      gmv_data.errormsg = (char *)malloc(43 * sizeof(char));
      snprintf(gmv_data.errormsg,43,"Error, no cells exist for cell velocities.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node velocities.\n");
      gmv_data.errormsg = (char *)malloc(43 * sizeof(char));
      snprintf(gmv_data.errormsg,43,"Error, no nodes exist for node velocities.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == FACE && numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for face velocities.\n");
      gmv_data.errormsg = (char *)malloc(43 * sizeof(char));
      snprintf(gmv_data.errormsg,43,"Error, no faces exist for face velocities.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Allocate and read velocity data.  */
   if (data_type == CELL) nvelin = numcells;
   if (data_type == NODE) nvelin = numnodes;
   if (data_type == FACE) nvelin = numfaces;

   uin = (double *)malloc(nvelin*sizeof(double));
   vin = (double *)malloc(nvelin*sizeof(double));
   win = (double *)malloc(nvelin*sizeof(double));
   if (uin == NULL || vin == NULL || win == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(uin,doublesize,DOUBLE,(long)nvelin,gmvin);
         ioerrtst(gmvin);
         binread(vin,doublesize,DOUBLE,(long)nvelin,gmvin);
         ioerrtst(gmvin);
         binread(win,doublesize,DOUBLE,(long)nvelin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(nvelin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)nvelin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nvelin; i++) uin[i] = tmpfloat[i];
         binread(tmpfloat,floatsize,FLOAT,(long)nvelin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nvelin; i++) vin[i] = tmpfloat[i];
         binread(tmpfloat,floatsize,FLOAT,(long)nvelin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nvelin; i++) win[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII)
     {
      rdfloats(uin,(long)nvelin,gmvin);
      rdfloats(vin,(long)nvelin,gmvin);
      rdfloats(win,(long)nvelin,gmvin);
     }

   gmv_data.keyword = VELOCITY;
   gmv_data.datatype = data_type;
   gmv_data.num = nvelin;
   gmv_data.ndoubledata1 = nvelin;
   gmv_data.doubledata1 = uin;
   gmv_data.ndoubledata2 = nvelin;
   gmv_data.doubledata2 = vin;
   gmv_data.ndoubledata3 = nvelin;
   gmv_data.doubledata3 = win;
}


void readvars(FILE* gmvin, int ftype)
{
  /*                                     */
  /*  Read and set variable field data.  */
  /*                                     */
  int i=0, data_type=0, nvarin=0;
  double *varin;
  float *tmpfloat;
  char varname[MAXCUSTOMNAMELENGTH];

   /*  Read a variable name and data type (cells or nodes). */
   if (ftype != ASCII)
     {
      binread(varname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(varname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(varname,"endvars",7) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(varname,charsize,CHAR,(long)charsize_in,gmvin);
         *(varname+charsize_in)=(char)0;
        }
      if (strncmp(varname,"endvars",7) != 0)
         binread(&i,intsize,INT,(long)1,gmvin);
     }
   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%s",varname); (void) res;
      if (strncmp(varname,"endvars",7) != 0)
        res = fscanf(gmvin,"%d",&i);
     }
   ioerrtst(gmvin);

   /*  Check for endvars.  */
   if (strncmp(varname,"endvars",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = VARIABLE;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;
   if (i == 2) data_type = FACE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell variable %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((42 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,42 + errormsgvarlen,"Error, no cells exist for cell variable %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node variable %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((42 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,42 + errormsgvarlen,"Error, no nodes exist for node variable %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == FACE && numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for face variable %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((42 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,42 + errormsgvarlen,"Error, no faces exist for face variable %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read one set of variable data.  */
   if (data_type == CELL) nvarin = numcells;
   if (data_type == NODE) nvarin = numnodes;
   if (data_type == FACE) nvarin = numfaces;
   varin = (double *)malloc(nvarin*sizeof(double));
   if (varin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(varin,doublesize,DOUBLE,(long)nvarin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(nvarin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)nvarin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nvarin; i++) varin[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII) rdfloats(varin,(long)nvarin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = VARIABLE;
   gmv_data.datatype = data_type;
   gmv_data.num = nvarin;
   strncpy(gmv_data.name1, varname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(varname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.ndoubledata1 = nvarin;
   gmv_data.doubledata1 = varin;
}


void readflags(FILE* gmvin, int ftype)
{
  /*                                     */
  /*  Read and set selection flag data.  */
  /*                                     */
  int i=-1, data_type=-1, ntypes=-1, nflagin=-1;
  int *flagin=NULL;
  char flgname[MAXCUSTOMNAMELENGTH], fname[MAXCUSTOMNAMELENGTH], *fnames;

   /*  Read flag name, no. flag types,  */
   /*  and data type (cells or nodes).  */
   if (ftype != ASCII)
     {
      binread(flgname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(flgname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(flgname,"endflag",7) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(flgname,charsize,CHAR,(long)charsize_in,gmvin);
         *(flgname+charsize_in)=(char)0;
        }
      if (strncmp(flgname,"endflag",7) != 0)
        {
         binread(&ntypes,intsize,INT,(long)1,gmvin);
         binread(&i,intsize,INT,(long)1,gmvin);
        }
     }
   if (ftype == ASCII) 
     {
      int res = fscanf(gmvin,"%s",flgname); (void) res;
      if (strncmp(flgname,"endflag",7) != 0)
         res = fscanf(gmvin,"%d%d",&ntypes,&i);
     }
   ioerrtst(gmvin);

   /*  Check for endflag.  */
   if (strncmp(flgname,"endflag",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = FLAGS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell flags %s.\n",flgname);
      errormsgvarlen = (int)strlen(flgname);
      gmv_data.errormsg = (char *)malloc((39 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,39 + errormsgvarlen,"Error, no cells exist for cell flags %s.",flgname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node flags %s.\n",flgname);
      errormsgvarlen = (int)strlen(flgname);
      gmv_data.errormsg = (char *)malloc((39 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,39 + errormsgvarlen,"Error, no nodes exist for node flags %s.",flgname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read one set of flag data.  */
   fnames = (char *)malloc(ntypes*MAXCUSTOMNAMELENGTH*sizeof(char));
   if (fnames == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read 8 or 32 character flag names.  */
   for (i = 0; i < ntypes; i++)
     {
      if (ftype != ASCII)
        {
         binread(fname, charsize_in*charsize, CHAR, (long)1, gmvin);
         ioerrtst(gmvin);
         *(fname+charsize_in) = (char) 0;
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%s",fname); (void) res;
         ioerrtst(gmvin);
         *(fname+charsize_in) = (char) 0;
        }
      strncpy(&fnames[i*MAXCUSTOMNAMELENGTH],fname,charsize_in);
      *(fnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
     }

   /*  Allocate and read flag data.  */
   if (data_type == CELL) nflagin = numcells;
   if (data_type == NODE) nflagin = numnodes;
   flagin = (int *)malloc(nflagin*sizeof(int));
   if (flagin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(flagin,intsize,INT,(long)nflagin,gmvin);
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdints(flagin,nflagin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = FLAGS;
   gmv_data.datatype = data_type;
   strncpy(gmv_data.name1, flgname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(flgname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.num = nflagin;
   gmv_data.num2 = ntypes;
   gmv_data.nlongdata1 = nflagin;
   gmv_data.longdata1 = (long *)malloc(nflagin*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nflagin; i++)
      gmv_data.longdata1[i] =  flagin[i];
   free(flagin);
   gmv_data.nchardata1 = ntypes;
   gmv_data.chardata1 = fnames;
}


void readpolygons(FILE* gmvin, int ftype)
{
  /*                                             */
  /*  Read and set interface/boundary polygons.  */
  /*                                             */
  int i, limatno=-1, nvertsin=-1, junk;
  double *vertsin=NULL;
  float *tmpfloat=NULL;
  char varname[MAXKEYWORDLENGTH+1], *tmpchar=NULL;

   /*  Read material no.  */
   if (ftype != ASCII)
     {
      binread(&wordbuf,intsize,WORD,(long)1,gmvin);
      tmpchar = (char *) &wordbuf;
      for (i = 0; i < 4; i++) varname[i] = tmpchar[i];
      varname[4] = '\0';
      limatno = word2int(wordbuf);
     }
   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%s",varname); (void) res;
      sscanf(varname,"%d", &limatno);
     }
   ioerrtst(gmvin);

   /*  Check for endpoly.  */
   if (strncmp(varname,"end",3) == 0)
     {

      /*  Read second part of endpoly for bin data.  */
      if (ftype != ASCII) binread(&junk,intsize,INT,(long)1,gmvin);
      ioerrtst(gmvin);

      readkeyword = 2;
      gmv_data.keyword = POLYGONS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read no. vertices. */
   if (ftype != ASCII) binread(&nvertsin,intsize,INT,(long)1,gmvin);
   if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&nvertsin); (void) res; }
   ioerrtst(gmvin);

   /*  Read vertices and set data.  */
   vertsin = (double *)malloc(3*nvertsin*sizeof(double));
   if (vertsin == NULL)
     {
      gmvrdmemerr();
      return;
     }
   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(vertsin,doublesize,DOUBLE,(long)3*nvertsin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(3*nvertsin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)3*nvertsin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < 3*nvertsin; i++) vertsin[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII) rdfloats(vertsin,(long)3*nvertsin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = POLYGONS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = limatno;
   gmv_data.ndoubledata1 = nvertsin;
   gmv_data.doubledata1 = (double *)malloc(nvertsin*sizeof(double));
   if (gmv_data.doubledata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nvertsin; i++)
      gmv_data.doubledata1[i] = vertsin[i];
   gmv_data.ndoubledata2 = nvertsin;
   gmv_data.doubledata2 = (double *)malloc(nvertsin*sizeof(double));
   if (gmv_data.doubledata2 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nvertsin; i++)
      gmv_data.doubledata2[i] = vertsin[nvertsin+i];
   gmv_data.ndoubledata3 = nvertsin;
   gmv_data.doubledata3 = (double *)malloc(nvertsin*sizeof(double));
   if (gmv_data.doubledata3 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nvertsin; i++)
      gmv_data.doubledata3[i] = vertsin[2*nvertsin+i];
   free(vertsin);
}


void readtracers(FILE* gmvin, int ftype)
{
  /*                             */
  /*  Read and set tracer data.  */
  /*                             */
  char varname[MAXCUSTOMNAMELENGTH];
  int i;
  double *lxtr, *lytr, *lztr, *lfieldtr;
  float *tmpfloat;

   if (readkeyword == 1)
     {

      /*  Read no of tracers and x,y,z data.  */
      if (ftype != ASCII) binread(&numtracers,intsize,INT,(long)1,gmvin);
      if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&numtracers); (void) res; }
      ioerrtst(gmvin);

      lxtr = NULL;  lytr = NULL;  lztr = NULL;
      if (numtracers > 0)
        {

         /*  Allocate and read tracer x,y,z's.  */
         lxtr = (double *)malloc(numtracers*sizeof(double));
         lytr = (double *)malloc(numtracers*sizeof(double));
         lztr = (double *)malloc(numtracers*sizeof(double));
         if (lxtr == NULL || lytr == NULL || lztr == NULL)
           {
            gmvrdmemerr();
            return;
           }

         if (ftype != ASCII)
           {
            if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
              {
               binread(lxtr,doublesize,DOUBLE,(long)numtracers,gmvin);
               ioerrtst(gmvin);
               binread(lytr,doublesize,DOUBLE,(long)numtracers,gmvin);
               ioerrtst(gmvin);
               binread(lztr,doublesize,DOUBLE,(long)numtracers,gmvin);
               ioerrtst(gmvin);
              }
            else
              {
               tmpfloat = (float *)malloc(numtracers*sizeof(float));
               if (tmpfloat == NULL)
                 {
                  gmvrdmemerr();
                  return;
                 }
               binread(tmpfloat,floatsize,FLOAT,(long)numtracers,gmvin);
               ioerrtst(gmvin);
               for (i = 0; i < numtracers; i++) lxtr[i] = tmpfloat[i];
               binread(tmpfloat,floatsize,FLOAT,(long)numtracers,gmvin);
               ioerrtst(gmvin);
               for (i = 0; i < numtracers; i++) lytr[i] = tmpfloat[i];
               binread(tmpfloat,floatsize,FLOAT,(long)numtracers,gmvin);
               ioerrtst(gmvin);
               for (i = 0; i < numtracers; i++) lztr[i] = tmpfloat[i];
               free(tmpfloat);
              }
           }
         if (ftype == ASCII)
           {
            rdfloats(lxtr,(long)numtracers,gmvin);
            rdfloats(lytr,(long)numtracers,gmvin);
            rdfloats(lztr,(long)numtracers,gmvin);
           }
        }

      gmv_data.keyword = TRACERS;
      gmv_data.datatype = XYZ;
      gmv_data.num = numtracers;
      gmv_data.ndoubledata1 = numtracers;
      gmv_data.doubledata1 = lxtr;
      gmv_data.ndoubledata2 = numtracers;
      gmv_data.doubledata2 = lytr;
      gmv_data.ndoubledata3 = numtracers;
      gmv_data.doubledata3 = lztr;

      readkeyword = 0;
      return;
     }

   /*  Read tracer field name and data.  */
   if (ftype != ASCII)
     {
      binread(varname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(varname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(varname,"endtrace",8) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(varname,charsize,CHAR,(long)charsize_in,gmvin);
         *(varname+charsize_in)=(char)0;
        }
     }
   if (ftype == ASCII) { int res = fscanf(gmvin,"%s",varname); (void) res; }
   ioerrtst(gmvin);

   /*  Check for endtrace.  */
   if (strncmp(varname,"endtrace",8) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = TRACERS;
      gmv_data.datatype = ENDKEYWORD;
      gmv_data.num = numtracers;
      return;
     }

   lfieldtr = NULL;
   if (numtracers > 0)
     {
      lfieldtr=(double *)malloc(numtracers*sizeof(double));
      if (lfieldtr == NULL)
        {
         gmvrdmemerr();
         return;
        }

      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            binread(lfieldtr,doublesize,DOUBLE,(long)numtracers,gmvin);
            ioerrtst(gmvin);
           }
         else
           {
            tmpfloat = (float *)malloc(numtracers*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpfloat,floatsize,FLOAT,(long)numtracers,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < numtracers; i++) lfieldtr[i] = tmpfloat[i];
            free(tmpfloat);
           }
        }
      if (ftype == ASCII) rdfloats(lfieldtr,(long)numtracers,gmvin);
     }

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = TRACERS;
   gmv_data.datatype = TRACERDATA;
   strncpy(gmv_data.name1, varname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(varname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.num = numtracers;
   gmv_data.ndoubledata1 = numtracers;
   gmv_data.doubledata1 = lfieldtr;
}


void readnodeids(FILE* gmvin, int ftype)
{
  /*                                              */
  /*  Read and set alternate node numbers (ids).  */
  /*                                              */
  long *lnodeids = NULL;
  int *tmpids, i;

   /*  Allocate node ids.  */
   lnodeids=(long *)malloc(numnodes*sizeof(long));
   if (lnodeids == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read node ids.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(lnodeids,longlongsize,LONGLONG,numnodes,gmvin);
        }
      else
        {
         tmpids=(int *)malloc(numtracers*sizeof(int));
         if (tmpids == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpids,intsize,INT,numnodes,gmvin);
         for (i = 0; i < numnodes; i++)
            lnodeids[i] = tmpids[i];
         free(tmpids);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(lnodeids,numnodes,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = NODEIDS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numnodes;
   gmv_data.nlongdata1 = numnodes;
   gmv_data.longdata1 = lnodeids;
}


void readcellids(FILE* gmvin, int ftype)
{
  /*                                              */
  /*  Read and set alternate cell numbers (ids).  */
  /*                                              */
  long *lcellids = NULL;
  int *tmpids, i;

   /*  Allocate cell ids.  */
   lcellids=(long *)malloc(numcells*sizeof(long));
   if (lcellids == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read cell ids.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(lcellids,longlongsize,LONGLONG,numcells,gmvin);
        }
      else
        {
         tmpids=(int *)malloc(numcells*sizeof(int));
         if (tmpids == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpids,intsize,INT,numcells,gmvin);
         for (i = 0; i < numcells; i++)
            lcellids[i] = tmpids[i];
         free(tmpids);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(lcellids,numcells,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = CELLIDS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numcells;
   gmv_data.nlongdata1 = numcells;
   gmv_data.longdata1 = lcellids;
}


void readfaceids(FILE* gmvin, int ftype)
{
  /*                                              */
  /*  Read and set alternate face numbers (ids).  */
  /*                                              */
  long *lfaceids = NULL;
  int *tmpids, i;

   /*  Check that faces have been read.  */
   if (numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for faceids.\n");
      gmv_data.errormsg = (char *)malloc(35 * sizeof(char));
      snprintf(gmv_data.errormsg,35,"Error, no faces exist for faceids.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Allocate face ids.  */
   lfaceids=(long *)malloc(numfaces*sizeof(long));
   if (lfaceids == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read face ids.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(lfaceids,longlongsize,LONGLONG,numcells,gmvin);
        }
      else
        {
         tmpids=(int *)malloc(numfaces*sizeof(int));
         if (tmpids == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpids,intsize,INT,numfaces,gmvin);
         for (i = 0; i < numfaces; i++)
            lfaceids[i] = tmpids[i];
         free(tmpids);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(lfaceids,numfaces,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = FACEIDS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numcells;
   gmv_data.nlongdata1 = numfaces;
   gmv_data.longdata1 = lfaceids;
}


void readtracerids(FILE* gmvin, int ftype)
{
  /*                                                */
  /*  Read and set alternate tracer numbers (ids).  */
  /*                                                */
  long *ltracerids = NULL;
  int *tmpids, i;

   /*  Allocate tracer ids.  */
   if (numtracers > 0)
     {
      ltracerids=(long *)malloc(numtracers*sizeof(long));
      if (ltracerids == NULL)
        {
         gmvrdmemerr();
         return;
        }

      /*  Read tracer ids.  */
      if (ftype != ASCII)
        {
         if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
           {
            binread(ltracerids,longlongsize,LONGLONG,numtracers,gmvin);
           }
         else
           {
            tmpids=(int *)malloc(numtracers*sizeof(int));
            if (tmpids == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpids,intsize,INT,numtracers,gmvin);
            for (i = 0; i < numtracers; i++)
               ltracerids[i] = tmpids[i];
            free(tmpids);
           }
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) rdlongs(ltracerids,(long)numtracers,gmvin);
     }

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = TRACEIDS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numtracers;
   gmv_data.nlongdata1 = numtracers;
   gmv_data.longdata1 = ltracerids;
}


void readunits(FILE* gmvin, int ftype)
{
  /*                                   */
  /*  Read and set units information.  */
  /*                                   */
  int i;
  char unittype[MAXKEYWORDLENGTH+64], fldname[MAXKEYWORDLENGTH+64], unitname[17];
  char *fldstr, *unitstr;

   /*  Read unit type (xyz, velocity, nodes, cells).  */
   if (ftype != ASCII)
     {
      binread(unittype,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(unittype+MAXKEYWORDLENGTH)=(char)0;
     }
   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%s",unittype); (void) res;
     }
   ioerrtst(gmvin);

   /*  Check for endunit.  */
   if (strncmp(unittype,"endunit",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = UNITS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   numunits = 0;
   gmv_data.keyword = UNITS;

   /*  If xyz or velocity type, read units.  */
   if (strncmp(unittype,"xyz",3) == 0 || 
       strncmp(unittype,"velocity",8) == 0)
     {
      if (ftype != ASCII)
        {
         binread(unitname, 16*charsize, CHAR, (long)1, gmvin);
         ioerrtst(gmvin);
         *(unitname+16) = (char) 0;
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%s",unitname); (void) res;
         ioerrtst(gmvin);
         *(unitname+16) = (char) 0;
        }
      if (strncmp(unittype,"xyz",3) == 0)
         gmv_data.datatype = XYZ;
      else gmv_data.datatype = VEL;
      gmv_data.nchardata1 = 1;
      gmv_data.chardata1 = (char *)malloc(20*sizeof(char));
      if (gmv_data.chardata1 == NULL)
        {
         gmvrdmemerr();
         return;
        }
      strncpy(gmv_data.chardata1, unittype, GMV_MIN(strlen(unittype), 20-1));
      *(gmv_data.chardata1 + GMV_MIN(strlen(unittype), 20-1)) = (char)0;
      gmv_data.nchardata2 = 1;
      gmv_data.chardata2 = (char *)malloc(20*sizeof(char));
      if (gmv_data.chardata2 == NULL)
        {
         gmvrdmemerr();
         return;
        }
      strncpy(gmv_data.chardata2, unitname, GMV_MIN(strlen(unitname), 20-1));
      *(gmv_data.chardata2 + GMV_MIN(strlen(unitname), 20-1)) = (char)0;
      return;
     }

   /*  If nodes or cells type, read field units.  */
   if (strncmp(unittype,"cells",5) == 0 ||
       strncmp(unittype,"nodes",5) == 0 ||
       strncmp(unittype,"faces",5) == 0)
     {

      /*  Read no. of field units to read.  */
      if (ftype != ASCII)
        {
     binread((void*)(&numunits),intsize,INT,(long)1,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII)
        {
         int res =fscanf(gmvin,"%d",&numunits); (void) res;
         ioerrtst(gmvin);
        }

      /*  Allocate two character strings.  */
      fldstr = (char *)malloc(numunits*MAXCUSTOMNAMELENGTH*sizeof(char));
      unitstr = (char *)malloc(numunits*MAXCUSTOMNAMELENGTH*sizeof(char));
      if (fldstr == NULL || unitstr == NULL)
        {
         gmvrdmemerr();
         return;
        }

      for (i = 0; i < numunits; i++)
        {
         if (ftype != ASCII)
           {
            binread(fldname, MAXKEYWORDLENGTH*charsize, CHAR, (long)1, gmvin);
            ioerrtst(gmvin);
            *(fldname+MAXKEYWORDLENGTH) = (char) 0;
            binread(unitname, 16*charsize, CHAR, (long)1, gmvin);
            ioerrtst(gmvin);
            *(unitname+16) = (char) 0;
           }
         if (ftype == ASCII)
           {
            int res = 0; (void) res;
            res = fscanf(gmvin,"%s",fldname);
            ioerrtst(gmvin);
            *(fldname+MAXKEYWORDLENGTH) = (char) 0;
            res = fscanf(gmvin,"%s",unitname);
            ioerrtst(gmvin);
            *(unitname+16) = (char) 0;
           }
         strncpy(&fldstr[i*MAXCUSTOMNAMELENGTH], fldname, GMV_MIN(strlen(fldname), MAXCUSTOMNAMELENGTH-1));
         fldstr[i*MAXCUSTOMNAMELENGTH + GMV_MIN(strlen(fldname), MAXCUSTOMNAMELENGTH-1)] = '\0';
         strncpy(&unitstr[i*MAXCUSTOMNAMELENGTH], unitname, GMV_MIN(strlen(unitname), MAXCUSTOMNAMELENGTH-1));
         unitstr[i*MAXCUSTOMNAMELENGTH + GMV_MIN(strlen(unitname), MAXCUSTOMNAMELENGTH-1)] = '\0';
        }

      if (strncmp(unittype,"nodes",5) == 0)
         gmv_data.datatype = NODE;
      if (strncmp(unittype,"nodes",5) == 0)
         gmv_data.datatype = CELL;
      if (strncmp(unittype,"faces",5) == 0)
         gmv_data.datatype = FACE;
      gmv_data.num = numunits;
      gmv_data.nchardata1 = numunits;
      gmv_data.chardata1 = fldstr;
      gmv_data.nchardata2 = numunits;
      gmv_data.chardata2 = unitstr;
     }
}


void readsurface(FILE* gmvin, int ftype)
{
  int i, nverts=0, *tmpverts;
  long *vertsin;

   if (readkeyword == 1)
     {

      /*  Read no. of surface facets to read.  */
      if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&lnsurf); (void) res; }
      else
         binread(&lnsurf,intsize,INT,(long)1,gmvin);
      ioerrtst(gmvin);
      numsurfin = 0;

      if (!skipflag)
        {
         numsurf = lnsurf;
         surface_read = 1;
        }
     }

   /*  Check no. of surfaces read.  */
   numsurfin++;
   if (numsurfin > lnsurf)
     {
      readkeyword = 2;
      gmv_data.keyword = SURFACE;
      gmv_data.datatype = ENDKEYWORD;
      gmv_data.num = numsurf;
      if (numsurf == 0) readkeyword = 1;
      return;
     }

   /*  Read surface facet vertices.  */
   if (ftype != ASCII)
     {
      binread(&nverts,intsize,INT,(long)1,gmvin);
     }
   if (ftype == ASCII) { int res = fscanf(gmvin,"%d",&nverts); (void) res; }
   ioerrtst(gmvin);

   /*  Read all face vertices.  */
   vertsin = (long *)malloc(nverts*sizeof(long));
   if (vertsin == NULL)
     {
      gmvrdmemerr();
      return;
     }
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(vertsin,longlongsize,LONGLONG,(long)nverts,gmvin);
        }
      else
        {
         tmpverts = (int *)malloc(nverts*sizeof(int));
         if (tmpverts == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpverts,intsize,INT,(long)nverts,gmvin);
         for (i = 0; i < nverts; i++)
            vertsin[i] = tmpverts[i];
         free(tmpverts);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(vertsin,(long)nverts,gmvin);

   if ((feof(gmvin) != 0) | (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading surfaces.\n");
      gmv_data.errormsg = (char *)malloc(34 * sizeof(char));
      snprintf(gmv_data.errormsg,34,"I/O error while reading surfaces.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = SURFACE;
   gmv_data.datatype = REGULAR;
   gmv_data.num = lnsurf;
   gmv_data.nlongdata1 = nverts;
   gmv_data.longdata1 = vertsin;
}


void readsurfmats(FILE* gmvin, int ftype)
{
  /*                                       */
  /*  Read and set surface material data.  */
  /*                                       */
  int i, *matin;

   /*  Check that surfaces have been input.  */
   if (surface_read == 0)
     {
      fprintf(stderr,
              "Error, surface must be read before surfmats.\n");
      gmv_data.errormsg = (char *)malloc(45 * sizeof(char));
      snprintf(gmv_data.errormsg,45,"Error, surface must be read before surfmats.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Check for numsurf == 0.  */
   if (numsurf == 0) return;

   /*  Allocate and read surface material nos.  */
   matin=(int *)malloc((numsurf)*sizeof(int));
   if (matin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(matin,intsize,INT,(long)numsurf,gmvin);
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdints(matin,numsurf,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = SURFMATS;
   gmv_data.num = numsurf;
   gmv_data.nlongdata1 = numsurf;
   gmv_data.longdata1 = (long *)malloc(numsurf*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < numsurf; i++)
      gmv_data.longdata1[i] = matin[i];
   free(matin);
}


void readsurfvel(FILE* gmvin, int ftype)
{
  /*                                       */
  /*  Read and set surface velocity data.  */
  /*                                       */
  int i;
  double *uin, *vin, *win;
  float *tmpfloat;

   /*  Check that surfaces have been input.  */
   if (surface_read == 0)
     {
      fprintf(stderr,
              "Error, surface must be read before surfvel.\n");
      gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
      snprintf(gmv_data.errormsg,44,"Error, surface must be read before surfvel.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Check for numsurf == 0.  */
   if (numsurf == 0)
     {
      gmv_data.keyword = SURFVEL;
      return;
     }

   /*  Allocate and read velocity data.  */
   uin = (double *)malloc(numsurf*sizeof(double));
   vin = (double *)malloc(numsurf*sizeof(double));
   win = (double *)malloc(numsurf*sizeof(double));
   if (uin == NULL || vin == NULL || win == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(uin,doublesize,DOUBLE,(long)numsurf,gmvin);
         ioerrtst(gmvin);
         binread(vin,doublesize,DOUBLE,(long)numsurf,gmvin);
         ioerrtst(gmvin);
         binread(win,doublesize,DOUBLE,(long)numsurf,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(numsurf*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)numsurf,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < numsurf; i++) uin[i] = tmpfloat[i];
         binread(tmpfloat,floatsize,FLOAT,(long)numsurf,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < numsurf; i++) vin[i] = tmpfloat[i];
         binread(tmpfloat,floatsize,FLOAT,(long)numsurf,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < numsurf; i++) win[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII)
     {
      rdfloats(uin,(long)numsurf,gmvin);
      rdfloats(vin,(long)numsurf,gmvin);
      rdfloats(win,(long)numsurf,gmvin);
     }

   gmv_data.keyword = SURFVEL;
   gmv_data.num = numsurf;
   gmv_data.ndoubledata1 = numsurf;
   gmv_data.doubledata1 = uin;
   gmv_data.ndoubledata2 = numsurf;
   gmv_data.doubledata2 = vin;
   gmv_data.ndoubledata3 = numsurf;
   gmv_data.doubledata3 = win;
}


void readsurfvars(FILE* gmvin, int ftype)
{
  /*                                             */
  /*  Read and set surface variable field data.  */
  /*                                             */
  int i;
  double *varin;
  float *tmpfloat;
  char varname[MAXCUSTOMNAMELENGTH];

   /*  Check that surfaces have been input.  */
   if (surface_read == 0)
     {
      fprintf(stderr,
              "Error, surface must be read before surfvars.\n");
      gmv_data.errormsg = (char *)malloc(45 * sizeof(char));
      snprintf(gmv_data.errormsg,45,"Error, surface must be read before surfvars.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read variable name. */
   if (ftype != ASCII)
     {
      binread(varname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(varname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(varname,"endsvar",7) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(varname,charsize,CHAR,(long)charsize_in,gmvin);
         *(varname+charsize_in)=(char)0;
        }
     }
   if (ftype == ASCII) { int res = fscanf(gmvin,"%s",varname); (void) res; }
   ioerrtst(gmvin);

   /*  Check for endsvar.  */
   if (strncmp(varname,"endsvar",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = SURFVARS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read variable data.  */
   varin = NULL;
   if (numsurf > 0)
     {
      varin = (double *)malloc(numsurf*sizeof(double));
      if (varin == NULL)
        {
         gmvrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            binread(varin,doublesize,DOUBLE,(long)numsurf,gmvin);
            ioerrtst(gmvin);
           }
         else
           {
            tmpfloat = (float *)malloc(numsurf*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrdmemerr();
               return;
              }
            binread(tmpfloat,floatsize,FLOAT,(long)numsurf,gmvin);
            ioerrtst(gmvin);
            for (i = 0; i < numsurf; i++) varin[i] = tmpfloat[i];
             free(tmpfloat);
           }
        }
      if (ftype == ASCII) rdfloats(varin,(long)numsurf,gmvin);
     }

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = SURFVARS;
   gmv_data.datatype = REGULAR;
   strncpy(gmv_data.name1, varname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(varname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.num = numsurf;
   gmv_data.ndoubledata1 = numsurf;
   gmv_data.doubledata1 = varin;
}


void readsurfflag(FILE* gmvin, int ftype)
{
  /*                                     */
  /*  Read and set selection flag data.  */
  /*                                     */
  int i, ntypes=0;
  int *flagin;
  char flgname[MAXCUSTOMNAMELENGTH], fname[MAXCUSTOMNAMELENGTH], *fnames;

   /*  Check that surfaces have been input.  */
   if (surface_read == 0)
     {
      fprintf(stderr,
              "Error, surface must be read before surfflag.\n");
      gmv_data.errormsg = (char *)malloc(45 * sizeof(char));
      snprintf(gmv_data.errormsg,45,"Error, surface must be read before surfflag.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read flag name.  */
   if (ftype != ASCII)
     {
      binread(&flgname,charsize,CHAR,(long)charsize_in,gmvin);
      *(flgname+charsize_in)='\0';
     }
   if (ftype == ASCII) 
     {
      int res = fscanf(gmvin,"%s",flgname); (void) res;
     }
   ioerrtst(gmvin);

   /*  Check for endsflag.  */
   if (strncmp(flgname,"endsflag",8) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = SURFFLAG;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read no. of flag types.  */
   if (ftype != ASCII)
     {
      binread(&ntypes,intsize,INT,(long)1,gmvin);
     }
   if (ftype == ASCII) 
     {
      int res = fscanf(gmvin,"%d",&ntypes); (void) res;
     }
   ioerrtst(gmvin);

   /*  Read one set of surface flag data.  */
   fnames = (char *)malloc(ntypes*MAXCUSTOMNAMELENGTH*sizeof(char));
   if (fnames == NULL)
     {
      gmvrdmemerr();
      return;
     }
   flagin = NULL;
   if (numsurf > 0)
     {
      flagin = (int *)malloc(numsurf*sizeof(int));
      if (flagin == NULL)
        {
         gmvrdmemerr();
         return;
        }
     }

   /*  Read 8 character flag names.  */
   for (i = 0; i < ntypes; i++)
     {
      if (ftype != ASCII)
        {
         binread(fname, charsize_in*charsize, CHAR, (long)1, gmvin);
         ioerrtst(gmvin);
         *(fname+charsize_in) = (char) 0;
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvin,"%s",fname); (void) res;
         ioerrtst(gmvin);
         *(fname+charsize_in) = (char) 0;
        }
      strncpy(&fnames[i*MAXCUSTOMNAMELENGTH],fname,charsize_in);
      *(fnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
     }

   /*  Allocate and read flag data.  */
   if (numsurf > 0)
     { 
      if (ftype != ASCII)
        {
         binread(flagin,intsize,INT,(long)numsurf,gmvin);
         ioerrtst(gmvin);
        }
      if (ftype == ASCII) rdints(flagin,numsurf,gmvin);
     }

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = SURFFLAG;
   gmv_data.datatype = REGULAR;
   strncpy(gmv_data.name1, flgname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(flgname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.num = numsurf;
   gmv_data.num2 = ntypes;
   gmv_data.nlongdata1 = numsurf;
   if (numsurf > 0)
     {
      gmv_data.longdata1 = (long *)malloc(numsurf*sizeof(long));
      if (gmv_data.longdata1 == NULL)
        {
         gmvrdmemerr();
         return;
        }
      for (i = 0; i < numsurf; i++)
         gmv_data.longdata1[i] = flagin[i];
      free(flagin);
     }
   gmv_data.nchardata1 = ntypes;
   gmv_data.chardata1 = fnames;
}


void readsurfids(FILE* gmvin, int ftype)
{
  /*                                                 */
  /*  Read and set alternate surface numbers (ids).  */
  /*                                                 */
  long *lsurfids = NULL;
  int *tmpids, i;

   /*  Check that surfaces have been input.  */
   if (surface_read == 0)
     {
      fprintf(stderr,
              "Error, surface must be read before surfids.\n");
      gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
      snprintf(gmv_data.errormsg,44,"Error, surface must be read before surfids.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Check for numsurf == 0.  */
   if (numsurf == 0) return;

   /*  Allocate surf ids.  */
   lsurfids=(long *)malloc(numsurf*sizeof(long));
   if (lsurfids == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read surf ids.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(lsurfids,longlongsize,LONGLONG,numsurf,gmvin);
        }
      else
        {
         tmpids=(int *)malloc(numsurf*sizeof(int));
         if (tmpids == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpids,intsize,INT,numsurf,gmvin);
         for (i = 0; i < numsurf; i++)
            lsurfids[i] = tmpids[i];
         free(tmpids);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(lsurfids,numsurf,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = SURFIDS;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numsurf;
   gmv_data.nlongdata1 = numsurf;
   gmv_data.longdata1 = lsurfids;
}


void readvinfo(FILE* gmvin, int ftype)
{
  /*                               */
  /*  Read one set of vinfo data.  */
  /*                               */
  int i, nelem_line=-1, nlines=0, nvarin;
  double *varin=NULL;
  float *tmpfloat  = NULL; /* TODO: check fix for uninitialized pointer */
  char varname[33];

   /*  Read a variable name, no. of elements per line, and no. of lines. */
   if (ftype != ASCII)
     {
      binread(varname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(varname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(varname,"endvinfo",8) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(varname,charsize,CHAR,(long)charsize_in,gmvin);
         *(varname+charsize_in)=(char)0;
        }
      if (strncmp(varname,"endvinfo",8) != 0)
        {
         binread(&nelem_line,intsize,INT,(long)1,gmvin);
         binread(&nlines,intsize,INT,(long)1,gmvin);
        }
     }
   if (ftype == ASCII)
     {
      int res = 0; (void) res;
      res = fscanf(gmvin,"%s",varname);
      if (strncmp(varname,"endvinfo",8) != 0)
        res = fscanf(gmvin,"%d%d",&nelem_line,&nlines);
     }
   ioerrtst(gmvin);

   /*  Check for endvinfo.  */
   if (strncmp(varname,"endvinfo",8) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = VINFO;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Read one set of vinfo data.  */
   nvarin = nelem_line * nlines;
   varin = (double *)malloc(nvarin*sizeof(double));
   if (varin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(varin,doublesize,DOUBLE,(long)nvarin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(nvarin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)nvarin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nvarin; i++) varin[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII) rdfloats(varin,(long)nvarin,gmvin);

   if (ftype == IEEEI4R4 || ftype == IEEEI8R4)
      free(tmpfloat);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = VINFO;
   gmv_data.datatype = REGULAR;
   gmv_data.num = nelem_line;
   gmv_data.num2 = nlines;
   strncpy(gmv_data.name1, varname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(varname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.ndoubledata1 = nvarin;
   gmv_data.doubledata1 = varin;
}


void readcomments(FILE* gmvin, int ftype)
{
  /*                                   */
  /*  Read through (ignore) comments.  */
  /*                                   */
  int rdcomms;
#define MAXCOMMENTLINELENGTH 100
  char varname[MAXCOMMENTLINELENGTH];
  int firstNonWS;

   /*  Read comments until endcomm found.  */
   rdcomms = 1;
   while (rdcomms)
     {
      /* int res = fscanf(gmvin,"%s",varname); (void) res; */
      char* res = fgets(varname, MAXCOMMENTLINELENGTH, gmvin); (void) res;
      /* Ignore leading whitespace characters as would have done fscanf() */
      firstNonWS = 0;
      while (firstNonWS < MAXCOMMENTLINELENGTH)
       if (*(varname+firstNonWS) == 0x09 || /* horizontal tab */
           *(varname+firstNonWS) == 0x0a || /* linefeed */
           *(varname+firstNonWS) == 0x0b || /* vertical tab */
           *(varname+firstNonWS) == 0x0c || /* form feed */
           *(varname+firstNonWS) == 0x0d || /* carriage return */
           *(varname+firstNonWS) == 0x20)   /* space */
         firstNonWS++;
       else
         break;
      ioerrtst(gmvin);
      if (strncmp(&varname[firstNonWS],"endcomm",7) == 0)
         rdcomms = 0;
      else
      {
         /* Only support "endcomm" with leading blanks, if any.
            Read reminder of comment line, in chunks. */
         while (varname[strlen(varname)-1] != '\n')
         {
            fgets(varname, MAXCOMMENTLINELENGTH, gmvin);
            ioerrtst(gmvin);
         }
      }
     }

   /*  If binary file, read space after endcomm.  */
   if (ftype != ASCII) binread(varname,charsize,CHAR,(long)1,gmvin);
}


void readgroups(FILE* gmvin, int ftype)
{
  /*                            */
  /*  Read and set group data.  */
  /*                            */
  int i=0, data_type=0, ngroupin=0;
  int *groupin;
  char grpname[40];

   /*  Read group name, data type (cells, nodes, faces, or  */
   /*  surfaces) and the number of elements in the group.   */
   if (ftype != ASCII)
     {
      binread(grpname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(grpname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(grpname,"endgrp",6) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(grpname,charsize,CHAR,(long)charsize_in,gmvin);
         *(grpname+charsize_in)=(char)0;
        }
      if (strncmp(grpname,"endgrp",6) != 0)
        {
         binread(&i,intsize,INT,(long)1,gmvin);
         binread(&ngroupin,intsize,INT,(long)1,gmvin);
        }
     }
   if (ftype == ASCII) 
     {
      int res = fscanf(gmvin,"%s",grpname); (void) res;
      if (strncmp(grpname,"endgrp",6) != 0)
         res = fscanf(gmvin,"%d%d",&i,&ngroupin);
     }
   ioerrtst(gmvin);

   /*  Check for endflag.  */
   if (strncmp(grpname,"endgrp",6) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = GROUPS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;
   if (i == 2) data_type = FACE;
   if (i == 3) data_type = SURF;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell group %s.\n",grpname);
      errormsgvarlen = (int)strlen(grpname);
      gmv_data.errormsg = (char *)malloc((39 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,39 + errormsgvarlen,"Error, no cells exist for cell group %s.",grpname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node group %s.\n",grpname);
      errormsgvarlen = (int)strlen(grpname);
      gmv_data.errormsg = (char *)malloc((39 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,39 + errormsgvarlen,"Error, no nodes exist for node group %s.",grpname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == FACE && numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for face group %s.\n",grpname);
      errormsgvarlen = (int)strlen(grpname);
      gmv_data.errormsg = (char *)malloc((39 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,39 + errormsgvarlen,"Error, no faces exist for face group %s.",grpname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read the group data.  */
   groupin = (int *)malloc(ngroupin*sizeof(int));
   if (groupin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(groupin,intsize,INT,(long)ngroupin,gmvin);
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdints(groupin,ngroupin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = GROUPS;
   gmv_data.datatype = data_type;
   strncpy(gmv_data.name1, grpname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(grpname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.num = ngroupin;
   gmv_data.nlongdata1 = ngroupin;
   gmv_data.longdata1 = (long *)malloc(ngroupin*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < ngroupin; i++)
      gmv_data.longdata1[i] =  groupin[i];
   free(groupin);
}


void readcellpes(FILE* gmvin, int ftype)
{
  /*                                                */
  /*  Read and set cell processor (pe) identifier.  */
  /*                                                */
  long *lcellpes = NULL;
  int *tmpids, i;

   /*  Allocate cell pes.  */
   lcellpes=(long *)malloc(numcells*sizeof(long));
   if (lcellpes == NULL)
     {
      gmvrdmemerr();
      return;
     }

   /*  Read cell pes.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         binread(lcellpes,longlongsize,LONGLONG,numcells,gmvin);
        }
      else
        {
         tmpids=(int *)malloc(numcells*sizeof(int));
         if (tmpids == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpids,intsize,INT,numcells,gmvin);
         for (i = 0; i < numcells; i++)
            lcellpes[i] = tmpids[i];
         free(tmpids);
        }
      ioerrtst(gmvin);
     }
   if (ftype == ASCII) rdlongs(lcellpes,numcells,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = CELLPES;
   gmv_data.datatype = REGULAR;
   gmv_data.num = numcells;
   gmv_data.nlongdata1 = numcells;
   gmv_data.longdata1 = lcellpes;
}


void readsubvars(FILE* gmvin, int ftype)
{
  /*                                     */
  /*  Read and set subvars field data.     */
  /*                                     */
  int i=0, data_type=0, nsubvarin=0, *subvarid;
  double *subvarin;
  float *tmpfloat;
  char varname[MAXCUSTOMNAMELENGTH];

   /*  Read a subvars name, data type (cells, nodes,  */
   /*  etc),and the number of elements in the set.    */
   if (ftype != ASCII)
     {
      binread(varname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(varname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(varname,"endsubv",7) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(varname,charsize,CHAR,(long)charsize_in,gmvin);
         *(varname+charsize_in)=(char)0;
        }
      if (strncmp(varname,"endsubv",7) != 0)
        {
         binread(&i,intsize,INT,(long)1,gmvin);
         binread(&nsubvarin,intsize,INT,(long)1,gmvin);
        }
     }
   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%s",varname); (void) res;
      if (strncmp(varname,"endsubv",7) != 0)
         res = fscanf(gmvin,"%d%d",&i,&nsubvarin);
     }
   ioerrtst(gmvin);

   /*  Check for endsubv.  */
   if (strncmp(varname,"endsubv",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = SUBVARS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   /*  Check that there is data to read.  */
   if (nsubvarin == 0)
     {
      fprintf(stderr,"Error, no data to read for subvars %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((37 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,37 + errormsgvarlen,"Error, no data to read for subvars %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;
   if (i == 2) data_type = FACE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell subvars %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((41 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,41 + errormsgvarlen,"Error, no cells exist for cell subvars %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node subvars %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((41 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,41 + errormsgvarlen,"Error, no nodes exist for node subvars %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == FACE && numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for face subvars %s.\n",varname);
      errormsgvarlen = (int)strlen(varname);
      gmv_data.errormsg = (char *)malloc((41 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,41 + errormsgvarlen,"Error, no faces exist for face subvars %s.",varname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read one set of subvars data.  */
   subvarid = (int *)malloc(nsubvarin*sizeof(int));
   subvarin = (double *)malloc(nsubvarin*sizeof(double));
   if (subvarid == NULL || subvarin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(subvarid,intsize,INT,(long)nsubvarin,gmvin);
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(subvarin,doublesize,DOUBLE,(long)nsubvarin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(nsubvarin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)nsubvarin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nsubvarin; i++) subvarin[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII)
     {
      rdints(subvarid,(long)nsubvarin,gmvin);
      rdfloats(subvarin,(long)nsubvarin,gmvin);
     }

   gmv_data.keyword = SUBVARS;
   gmv_data.datatype = data_type;
   gmv_data.num = nsubvarin;
   strncpy(gmv_data.name1, varname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(varname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.nlongdata1 = nsubvarin;
   gmv_data.longdata1 = (long *)malloc(nsubvarin*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nsubvarin; i++)
      gmv_data.longdata1[i] =  subvarid[i];
   gmv_data.ndoubledata1 = nsubvarin;
   gmv_data.doubledata1 = subvarin;
   free(subvarid);
}


void readghosts(FILE* gmvin, int ftype)
{
  /*                                     */
  /*  Read and set subvars field data.     */
  /*                                     */
  int i=0, data_type=0, nghostin=0, *ghostid;

   /*  Read the data type (cells or nodes),    */
   /*  and the number of elements in the set.  */
   if (ftype != ASCII)
     {
      binread(&i,intsize,INT,(long)1,gmvin);
      binread(&nghostin,intsize,INT,(long)1,gmvin);
     }
   if (ftype == ASCII)
     {
      int res = fscanf(gmvin,"%d%d",&i,&nghostin); (void) res;
     }
   ioerrtst(gmvin);

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for ghost cells.\n");
      gmv_data.errormsg = (char *)malloc(39 * sizeof(char));
      snprintf(gmv_data.errormsg,39,"Error, no cells exist for ghost cells.");
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for ghosts nodes.\n");
      gmv_data.errormsg = (char *)malloc(39 * sizeof(char));
      snprintf(gmv_data.errormsg,39,"Error, no nodes exist for ghost nodes.");
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read one set of ghost data.  */
   ghostid = (int *)malloc(nghostin*sizeof(int));
   if (ghostid == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      binread(ghostid,intsize,INT,(long)nghostin,gmvin);
     }
   if (ftype == ASCII)
     {
      rdints(ghostid,(long)nghostin,gmvin);
     }

   gmv_data.keyword = GHOSTS;
   gmv_data.datatype = data_type;
   gmv_data.num = nghostin;
   gmv_data.nlongdata1 = nghostin;
   gmv_data.longdata1 = (long *)malloc(nghostin*sizeof(long));
   if (gmv_data.longdata1 == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < nghostin; i++)
      gmv_data.longdata1[i] =  ghostid[i];
   free(ghostid);
}


void readvects(FILE* gmvin, int ftype)
{
  /*                                   */
  /*  Read and set vector field data.  */
  /*                                   */
  int i=0, data_type=0, nvectin = 0, ncomps=0, nreadin, cnamein=0; /*TODO: check fix for uninitialized pointer */
  double *vectin;
  float *tmpfloat;
  char vectname[MAXCUSTOMNAMELENGTH], cvname[MAXCUSTOMNAMELENGTH], *cvnames;

   /*  Read a vector name, data type (cells, nodes, faces), */
   /*  the number of components in the vector and the       */
   /*  component name option.                               */
   if (ftype != ASCII)
     {
      binread(vectname,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvin);
      *(vectname+MAXKEYWORDLENGTH)=(char)0;
      if (strncmp(vectname,"endvect",7) != 0 && charsize_in == 32)
        {
         fseek(gmvin,(long)(-MAXKEYWORDLENGTH),SEEK_CUR);
         binread(vectname,charsize,CHAR,(long)charsize_in,gmvin);
         *(vectname+charsize_in)=(char)0;
        }
      if (strncmp(vectname,"endvect",7) != 0)
        {
         binread(&i,intsize,INT,(long)1,gmvin);
         binread(&ncomps,intsize,INT,(long)1,gmvin);
         binread(&cnamein,intsize,INT,(long)1,gmvin);
        }
     }
   if (ftype == ASCII)
     {
      int res = 0; (void) res;
      res = fscanf(gmvin,"%s",vectname);
      if (strncmp(vectname,"endvect",7) != 0)
        {
         res = fscanf(gmvin,"%d",&i);
         res = fscanf(gmvin,"%d",&ncomps);
         res = fscanf(gmvin,"%d",&cnamein);
        }
     }
   ioerrtst(gmvin);

   /*  Check for endvect.  */
   if (strncmp(vectname,"endvect",7) == 0)
     {
      readkeyword = 2;
      gmv_data.keyword = VECTORS;
      gmv_data.datatype = ENDKEYWORD;
      return;
     }

   if (i == 0) data_type = CELL;
   if (i == 1) data_type = NODE;
   if (i == 2) data_type = FACE;

   /*  Check for existence of data_type.  */
   if (data_type == CELL && numcells == 0)
     {
      fprintf(stderr,"Error, no cells exist for cell vector %s.\n",vectname);
      errormsgvarlen = (int)strlen(vectname);
      gmv_data.errormsg = (char *)malloc((40 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,40 + errormsgvarlen,"Error, no cells exist for cell vector %s.",vectname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == NODE && numnodes == 0)
     {
      fprintf(stderr,"Error, no nodes exist for node vector %s.\n",vectname);
      errormsgvarlen = (int)strlen(vectname);
      gmv_data.errormsg = (char *)malloc((40 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,40 + errormsgvarlen,"Error, no nodes exist for node vector %s.",vectname);
      gmv_data.keyword = GMVERROR;
      return;
     }
   if (data_type == FACE && numfaces == 0)
     {
      fprintf(stderr,"Error, no faces exist for face vector %s.\n",vectname);
      errormsgvarlen = (int)strlen(vectname);
      gmv_data.errormsg = (char *)malloc((40 + errormsgvarlen) * sizeof(char));
      snprintf(gmv_data.errormsg,40 + errormsgvarlen,"Error, no faces exist for face vector %s.",vectname);
      gmv_data.keyword = GMVERROR;
      return;
     }

   /*  Read component names, if they exist.  */
   /*  Otherwise generate a name.            */
   cvnames = (char *)malloc(ncomps*MAXCUSTOMNAMELENGTH*sizeof(char));
   if (cvnames == NULL)
     {
      gmvrdmemerr();
      return;
     }
   if (cnamein)
     {
      for (i = 0; i < ncomps; i++)
        {
         if (ftype != ASCII)
           {
            binread(cvname, charsize_in*charsize, CHAR, (long)1, gmvin);
            ioerrtst(gmvin);
           }
         if (ftype == ASCII)
           {
            int res = fscanf(gmvin,"%s",cvname); (void) res;
            ioerrtst(gmvin);
           }
         strncpy(&cvnames[i*MAXCUSTOMNAMELENGTH],cvname,MAXCUSTOMNAMELENGTH-1);
         *(cvnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
        }
     }
   else
     {
      for (i = 0; i < ncomps; i++)
        {
         char cvname2[2*MAXCUSTOMNAMELENGTH];
         sprintf(cvname2,"%d-%s",i+1,vectname);
         strncpy(&cvnames[i*MAXCUSTOMNAMELENGTH],cvname,MAXCUSTOMNAMELENGTH-1);
         *(cvnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
        }
     }

   /*  Read one set of vector data.  */
   if (data_type == CELL) nvectin = numcells;
   if (data_type == NODE) nvectin = numnodes;
   if (data_type == FACE) nvectin = numfaces;
   nreadin = nvectin * ncomps;
   vectin = (double *)malloc(nreadin*sizeof(double));
   if (vectin == NULL)
     {
      gmvrdmemerr();
      return;
     }

   if (ftype != ASCII)
     {
      if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
        {
         binread(vectin,doublesize,DOUBLE,(long)nreadin,gmvin);
         ioerrtst(gmvin);
        }
      else
        {
         tmpfloat = (float *)malloc(nreadin*sizeof(float));
         if (tmpfloat == NULL)
           {
            gmvrdmemerr();
            return;
           }
         binread(tmpfloat,floatsize,FLOAT,(long)nreadin,gmvin);
         ioerrtst(gmvin);
         for (i = 0; i < nreadin; i++) vectin[i] = tmpfloat[i];
         free(tmpfloat);
        }
     }
   if (ftype == ASCII) rdfloats(vectin,(long)nreadin,gmvin);

   if (gmv_data.keyword == GMVERROR) return;

   gmv_data.keyword = VECTORS;
   gmv_data.datatype = data_type;
   gmv_data.num = nvectin;
   gmv_data.num2 = ncomps;
   strncpy(gmv_data.name1, vectname, MAXCUSTOMNAMELENGTH-1);
   *(gmv_data.name1 + GMV_MIN(strlen(vectname), MAXCUSTOMNAMELENGTH-1)) = (char)0;
   gmv_data.nchardata1 = ncomps;
   gmv_data.chardata1 = cvnames;
   gmv_data.ndoubledata1 = nreadin;
   gmv_data.doubledata1 = vectin;
}


void gmvrdmemerr()
{
  /*                 */
  /*  Memory error.  */
  /*                 */

   fprintf(stderr,"Not enough memory to read gmv data.\n");
   gmv_data.errormsg = (char *)malloc(36 * sizeof(char));
   snprintf(gmv_data.errormsg,36,"Not enough memory to read gmv data.");
   gmv_data.keyword = GMVERROR;
   gmv_meshdata.intype = GMVERROR;
}


void gmvrdmemerr2()
{
  /*                 */
  /*  Memory error.  */
  /*                 */
   fprintf(stderr,"Not enough memory to fill gmv mesh data.\n");
   gmv_data.errormsg = (char *)malloc(41 * sizeof(char));
   snprintf(gmv_data.errormsg,41,"Not enough memory to fill gmv mesh data.");
   gmvread_close();
/*LLNL*/
/*   exit(0);*/
  }


void ioerrtst(FILE *gmvin)
{
   /*                                      */
   /*  Test input file for eof and error.  */
   /*                                      */

   if ((feof(gmvin) != 0) || (ferror(gmvin) != 0))
     {
      fprintf(stderr,"I/O error while reading gmv input file.\n");
      gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
      snprintf(gmv_data.errormsg,40,"I/O error while reading gmv input file.");
      gmv_data.keyword = GMVERROR;
/*LLNL*/
/*
      exit(0);
*/
     }
}



int binread(void* ptr, int size, int type, long nitems, FILE* stream)
{
  int ret_stat;

#ifdef CRAY

  float *floatptr, *floatbuf; 
  double *doubleptr, *doublebuf; 
  int tierr, ttype, tbitoff;
  char *charptr;
  int  *intptr, *intbuf;
  short *shortptr, *shortbuf;
 
   tbitoff = 0;  tierr = 0;
   ret_stat = 0;
 
   switch(type)
     {
 
      case CHAR:
        charptr = (char *)ptr;
        ret_stat = fread(charptr, size, nitems, stream);
        break;
 
      case SHORT:
        ttype = 7;
        shortbuf = (short *)malloc(size*nitems);
        shortptr = (short *)ptr;
 
        ret_stat = fread(shortbuf, size, nitems, stream);
        tierr = IEG2CRAY(&ttype, &nitems, shortbuf, &tbitoff, shortptr);
        free(shortbuf);
        break;
 
      case INT:
        ttype = 1;
        intptr = (int *)ptr;
        intbuf = (int *)malloc(size*nitems);
 
        ret_stat = fread(intbuf, size, nitems, stream);
        tierr = IEG2CRAY(&ttype, &nitems, intbuf, &tbitoff, intptr);
        free(intbuf);
        break;
 
      case FLOAT:
        ttype = 2;
        floatptr = (float *)ptr;
        floatbuf = (float *)malloc(size*nitems);
 
        ret_stat = fread(floatbuf, size, nitems, stream);
        tierr = IEG2CRAY(&ttype, &nitems, floatbuf, &tbitoff, floatptr);
        free(floatbuf);
        break;
 
      case DOUBLE:
        ttype = 3;
        doubleptr = (double *)ptr;
        doublebuf = (double *)malloc(size*nitems);
 
        ret_stat = fread(doublebuf, size, nitems, stream);
        tierr = IEG2CRAY(&ttype, &nitems, doublebuf, &tbitoff, doubleptr);
        free(doublebuf);
        break;
 
      case WORD:
        intptr = (int *)ptr;
        ret_stat = fread(intptr, size, nitems, stream);
        break;
 
      default:
        fprintf(stderr,"Error: Cannot match input datatype.\n");
        gmv_data.errormsg = (char *)malloc(36 * sizeof(char));
        snprintf(gmv_data.errormsg,36,"Error: Cannot match input datatype.");
        gmv_data.keyword = GMVERROR;
        return;
     }
 
     if(tierr != 0)
       {
        fprintf(stderr,"Error: Cannot convert IEEE data to CRAY\n");
        gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
        snprintf(gmv_data.errormsg,40,"Error: Cannot convert IEEE data to CRAY");
        gmv_data.keyword = GMVERROR;
        return;
       }
 
     return ret_stat;

#else

   ret_stat = (int)fread(ptr, size, nitems, stream);

   if (ret_stat < nitems)
     {
     /* Initialise remainder of array */
     memset((char*)ptr + size*ret_stat, '\0', size*(nitems - ret_stat));
     }

   if (swapbytes_on && type != CHAR && type != WORD)
      swapbytes(ptr, size, nitems);

   return ret_stat;

#endif

}


int word2int(unsigned wordin)
{

  int intout;
 
#ifdef CRAY

  int ttype, tnum, tbitoff;
 
     tnum = 1; tbitoff = 0; ttype = 1; 

     if(IEG2CRAY(&ttype, &tnum, &wordin, &tbitoff, &intout) != 0)
       {
        fprintf(stderr,"Error: Cannot convert IEEE data to CRAY\n");
        gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
        snprintf(gmv_data.errormsg,40,"Error: Cannot convert IEEE data to CRAY");
        gmv_data.keyword = GMVERROR;
        return;
       }

     return intout;

#else

     intout = (int)wordin;
     if (swapbytes_on)
        swapbytes(&intout, intsize, 1);
     return intout;

#endif

}


void swapbytes(void *from, int size, int nitems)
{
  int i, j;
  char *tmp1, tmp2[8];

  tmp1 = (char*)(from);

   if (size == 8)
     {
      for (i = 0; i < nitems; i++)
        {
         j = i * size;
         tmp2[0] = tmp1[j+7];
         tmp2[1] = tmp1[j+6];
         tmp2[2] = tmp1[j+5];
         tmp2[3] = tmp1[j+4];
         tmp2[4] = tmp1[j+3];
         tmp2[5] = tmp1[j+2];
         tmp2[6] = tmp1[j+1];
         tmp2[7] = tmp1[j];
         tmp1[j]   = tmp2[0];
         tmp1[j+1] = tmp2[1];
         tmp1[j+2] = tmp2[2];
         tmp1[j+3] = tmp2[3];
         tmp1[j+4] = tmp2[4];
         tmp1[j+5] = tmp2[5];
         tmp1[j+6] = tmp2[6];
         tmp1[j+7] = tmp2[7];
        }
     }
   if (size == 4)
     {
      for (i = 0; i < nitems; i++)
        {
         j = i * size;
         tmp2[0] = tmp1[j+3];
         tmp2[1] = tmp1[j+2];
         tmp2[2] = tmp1[j+1];
         tmp2[3] = tmp1[j];
         tmp1[j]   = tmp2[0];
         tmp1[j+1] = tmp2[1];
         tmp1[j+2] = tmp2[2];
         tmp1[j+3] = tmp2[3];
        }
     }
   if (size == 2)
     {
      for (i = 0; i < nitems; i++)
        {
         j = i * size;
         tmp2[0] = tmp1[j+1];
         tmp2[1] = tmp1[j];
         tmp1[j]   = tmp2[0];
         tmp1[j+1] = tmp2[1];
        }
     }
}


int chk_gmvend(FILE *fin)
{
  /*                          */
  /*  Read a GMV input file.  */
  /*                          */
  int i, chkend;
  long int currpos;
  char rdend[21];
  size_t res;

   /*  Get the current file position.  */
   /* currpos = ftell(fin); */
   currpos = 8;

   /*  Read the last 20 characters of the file.  */
   fseek(fin, -20L, 2);
   res = fread(rdend,sizeof(char), 20, fin); (void) res;

   /*  Check the 20 characters for endgmv.  */
   chkend = 0;
   for (i = 0; i < 15; i++)
      if (strncmp((rdend+i),"endgmv",6) == 0) chkend = 1;

   /*  Reset file position.  */
   fseek(fin, currpos, 0);

   return chkend;
}


long *celltoface, *cell_faces, cellfaces_alloc, totfaces,
     *facetoverts, facetoverts_alloc, nfacesin,
     *faceverts, faceverts_alloc, nvertsin,
     *cellnnode, *cellnodes, cellnodes_alloc, totcellnodes;
static short vfacetype;


void rdcells(int nodetype_in);
void rdfaces(), rdxfaces();

void gmvread_mesh()
{
  int nxv, nyv, nzv, nodetype_in=0, j, k;
  long nn, i, ip;
  double *xin = NULL, *yin = NULL, *zin = NULL, x0, y0, z0, dx, dy, dz; /* TODO: check fix for uninitialized pointers */

   /*  Read and return mesh info. from a gmv input file.  */
   gmv_meshdata.celltoface = NULL;
   gmv_meshdata.cellfaces = NULL;
   gmv_meshdata.facetoverts = NULL;
   gmv_meshdata.faceverts = NULL;
   gmv_meshdata.facecell1 = NULL;
   gmv_meshdata.facecell2 = NULL;
   gmv_meshdata.vfacepe = NULL;
   gmv_meshdata.vfaceoppface = NULL;
   gmv_meshdata.vfaceoppfacepe = NULL;
   gmv_meshdata.cellnnode = NULL;
   gmv_meshdata.cellnodes = NULL;
   xin = NULL; yin = NULL; zin = NULL;

   if (printon)
      printf("Reading mesh data.\n");

   /*  Read and save node x,y,zs.  */
   /* gmvread_data(); */

   /*  Check for GMVERROR.  */
   /* SB: Modification due to disabled exit statement on fromfile error */
   if (gmv_data.keyword == GMVERROR)
     {
      gmv_meshdata.intype = GMVERROR;
      return;
     }

   if (gmv_data.keyword != NODES)
     {
      fprintf(stderr,"Error - nodes keyword missing.\n");
      gmv_data.errormsg = (char *)malloc(31 * sizeof(char));
      snprintf(gmv_data.errormsg,31,"Error - nodes keyword missing.");
      gmvread_close();
      gmv_meshdata.intype = GMVERROR;
      return;
     }
   gmv_meshdata.nxv = 0;
   gmv_meshdata.nyv = 0;
   gmv_meshdata.nzv = 0;
   if (gmv_data.keyword == NODES)
     {
      gmv_meshdata.nnodes = gmv_data.num;
      nn = gmv_data.num;
      gmv_meshdata.intype = gmv_data.datatype;
      nodetype_in = gmv_data.datatype;

      if (gmv_data.datatype != AMR)
        {
         gmv_meshdata.x = (double *)malloc(nn*sizeof(double));
         gmv_meshdata.y = (double *)malloc(nn*sizeof(double));
         gmv_meshdata.z = (double *)malloc(nn*sizeof(double));
         if (gmv_meshdata.x == NULL || gmv_meshdata.y == NULL || 
             gmv_meshdata.z == NULL)
           {
            gmvrdmemerr2();
            return;
           }
        }

      if (gmv_data.datatype == UNSTRUCT ||
          gmv_data.datatype == LOGICALLY_STRUCT)
        {
         for (i = 0; i < nn; i++)
           {
            gmv_meshdata.x[i] = gmv_data.doubledata1[i];
            gmv_meshdata.y[i] = gmv_data.doubledata2[i];
            gmv_meshdata.z[i] = gmv_data.doubledata3[i];
           }
        }

      if (gmv_data.datatype == STRUCT ||
          gmv_data.datatype == LOGICALLY_STRUCT)
        {
         gmv_meshdata.nxv = gmv_data.ndoubledata1;
         gmv_meshdata.nyv = gmv_data.ndoubledata2;
         gmv_meshdata.nzv = gmv_data.ndoubledata3;
         nxv = gmv_meshdata.nxv;
         nyv = gmv_meshdata.nyv;
         nzv = gmv_meshdata.nzv;

         if (gmv_data.datatype == STRUCT)
           {
            xin = (double *)malloc(nxv*sizeof(double));
            yin = (double *)malloc(nyv*sizeof(double));
            zin = (double *)malloc(nzv*sizeof(double));
            if (xin == NULL || yin == NULL || zin == NULL)
              {
               gmvrdmemerr2();
               return;
              }
            for (i = 0; i < nxv; i++)
               xin[i] = gmv_data.doubledata1[i];
            for (i = 0; i < nyv; i++)
               yin[i] = gmv_data.doubledata2[i];
            for (i = 0; i < nzv; i++)
               zin[i] = gmv_data.doubledata3[i];
            
            /*  Generate nodes from direction vectors.  */
            ip = 0;
            for (k = 0; k < nzv; k++)
              {
               for (j = 0; j < nyv; j++)
                 {
                  for (i = 0; i < nxv; i++)
                    {
                     gmv_meshdata.x[ip] = xin[i];
                     gmv_meshdata.y[ip] = yin[j];
                     gmv_meshdata.z[ip] = zin[k];
                     ip++;
                    }
                 }
              }
           }
        }

      if (gmv_data.datatype == AMR)
        {
         nxv = gmv_data.num2;
         nyv = gmv_data.nlongdata1;
         nzv = gmv_data.nlongdata2;
         x0 = gmv_data.doubledata1[0];
         y0 = gmv_data.doubledata1[1];
         z0 = gmv_data.doubledata1[2];
         dx = gmv_data.doubledata2[0];
         dy = gmv_data.doubledata2[1];
         dz = gmv_data.doubledata2[2];
         gmv_meshdata.nxv = nxv;
         gmv_meshdata.nyv = nyv;
         gmv_meshdata.nzv = nzv;
         gmv_meshdata.x = (double *)malloc(2*sizeof(double));
         gmv_meshdata.y = (double *)malloc(2*sizeof(double));
         gmv_meshdata.z = (double *)malloc(2*sizeof(double));
         gmv_meshdata.x[0] = x0;  gmv_meshdata.x[1] = dx;
         gmv_meshdata.y[0] = y0;  gmv_meshdata.y[1] = dy;
         gmv_meshdata.z[0] = z0;  gmv_meshdata.z[1] = dz;
        }
     }

   /*  Read cell or face input.  */
   gmvread_data();

   /*  Check for GMVERROR.  */
   if (gmv_data.keyword == GMVERROR)
     {
      gmv_meshdata.intype = GMVERROR;
      return;
     }

   if (gmv_data.keyword == CELLS) rdcells(nodetype_in);
   if (gmv_data.keyword == FACES) rdfaces();
   if (gmv_data.keyword == XFACES) rdxfaces();

   if (xin) free(xin);
   if (yin) free(yin);
   if (zin) free(zin);
}


void gencell(long icell, long nc);
void regcell(long icell, long nc);
void vfacecell(long icell, long nc), rdvfaces(long nc);
void fillmeshdata(long nc);
void structmesh();

void rdcells(int nodetype_in)
{
  static long icell;
  int i, nfa, nna;
  long nc;

   /*  Get first cell info.  */
   gmv_meshdata.ncells = gmv_data.num;
   nc = gmv_data.num;

   /*  If amr, save the daughter list and return.  */
   if (gmv_data.datatype == AMR)
     {
      gmv_meshdata.ncells = gmv_data.num2;
      gmv_meshdata.nfaces = gmv_data.num;
      nc = gmv_data.num;
      gmv_meshdata.celltoface = (long *)malloc((nc+1)*sizeof(long));
      if (gmv_meshdata.celltoface == NULL)
        {
         gmvrdmemerr2();
         return;
        }
      for (i = 0; i < nc; i++)
         gmv_meshdata.celltoface[i] = gmv_data.longdata1[i];
      return;
     }

   /*  If structured, return.  */
   if (nodetype_in == STRUCT ||
       nodetype_in == LOGICALLY_STRUCT)
      return;

   /*  Check for vfaces cells.  */
   gmv_meshdata.intype = CELLS;
   vfacetype = 0;
   if (gmv_data.datatype == VFACE2D || gmv_data.datatype == VFACE3D)
     {
      if (gmv_data.datatype == VFACE2D)
        {
         gmv_meshdata.intype = VFACES2D;
         vfacetype = 2;
        }
      else
        {
         gmv_meshdata.intype = VFACES3D;
         vfacetype = 3;
        }
     }

   nfa = 6;
   nna = 24;
   if (nc < 100)
     {
      nfa = 48;
      nna = 144;
     }
   celltoface = (long *)malloc((nc+1)*sizeof(long));
   cell_faces = (long *)malloc(nc*nfa*sizeof(long));
   if (nc > 0 && (celltoface == NULL || cell_faces == NULL))
     {
      gmvrdmemerr2();
      return;
     }
   cellfaces_alloc = nc*nfa;
   if (vfacetype == 0)
     {
      facetoverts = (long *)malloc(nc*nfa*sizeof(long));
      facetoverts_alloc = nc*nfa;
      faceverts = (long *)malloc(nc*nna*sizeof(long));
      faceverts_alloc = nc*nna;
      if (nc > 0 && (facetoverts == NULL || faceverts == NULL))
        {
         gmvrdmemerr2();
         return;
        }
     }

   if (gmv_meshdata.intype == CELLS)
     {
      cellnodes_alloc = 1;
      totcellnodes = 0;
      cellnnode = (long *)malloc(nc*sizeof(long));
      cellnodes = (long *)malloc(1*sizeof(long));
      for (i = 0; i < nc; i++) cellnnode[i] = 0;
     }

   /*  Loop through cells.  */
   icell = 0;  nfacesin = 0;  nvertsin = 0;
   while (gmv_data.datatype != ENDKEYWORD)
     {
      if (gmv_data.datatype == GENERAL) gencell(icell,nc);
      if (gmv_data.datatype == REGULAR) regcell(icell,nc);
      if (gmv_data.datatype == VFACE2D || gmv_data.datatype == VFACE3D) 
         vfacecell(icell,nc);
      icell++;
      gmvread_data();
      if (gmv_data.datatype == ENDKEYWORD)
        {
         if (vfacetype > 0)
           {
            gmvread_data();
            if (gmv_data.keyword != VFACES)
              {
               fprintf(stderr,"Error, vfaces keyword not found.\n");
	       gmv_data.errormsg = (char *)malloc(33 * sizeof(char));
	       snprintf(gmv_data.errormsg,33,"Error, vfaces keyword not found.");
               gmv_meshdata.intype = GMVERROR;
               return;
              }
            rdvfaces(nc);
           }
         else
           {
            totfaces = nfacesin; 
            fillmeshdata(nc);
            if (totcellnodes > 0)
              {
               cellnodes = (long *)realloc(cellnodes,totcellnodes*sizeof(long));
               gmv_meshdata.cellnnode = cellnnode;
               gmv_meshdata.cellnodes = cellnodes;
              }
            else
              {
               free(cellnnode);  free(cellnodes);
              }
           }
         return;
        }

      /*  Check for GMVERROR.  */
      if (gmv_data.keyword == GMVERROR)
        {
         gmv_meshdata.intype = GMVERROR;
         return;
        }
     }
}


void gencell(long icell, long nc)
{
  /*                                */
  /*  Set data for a general cell.  */
  /*                                */
  int totverts, nfaces; 
  long i, j, k, nverts[MAXVERTS];
  static long sumverts = 0, gcellcount = 0;

   /*  Save first face location for cell to faces pointer.  */
   celltoface[icell] = nfacesin;

   /*  Fill cell_faces array.  */
   nfaces = gmv_data.nlongdata1;
   if (nfacesin+nfaces > cellfaces_alloc)
     {
      j = (nfacesin+1) / (icell+1);
      k = cellfaces_alloc + (nc-icell)*j;
      if (k < nfacesin+nfaces) k = nfacesin+nfaces + nc*j;
      cell_faces = (long *)realloc(cell_faces,k*sizeof(long));
      if (cell_faces == NULL)
        {
         gmvrdmemerr2();
         return;
        }
      cellfaces_alloc = k;
     }
   for (i = 0; i < nfaces; i++)
      cell_faces[nfacesin+i] = nfacesin+i;      

   /*  Save all face vertices, reallocate if needed.  */
   totverts = gmv_data.nlongdata2;
   sumverts += totverts;
   gcellcount ++;
   if (nvertsin+totverts > faceverts_alloc)
     {
      j = sumverts / gcellcount;
      k = faceverts_alloc + (nc-icell)*j;
      if (k < nvertsin+totverts) k = nvertsin+totverts + (nc-icell)*j;
      faceverts = (long *)realloc(faceverts,k*sizeof(long));
      if (faceverts == NULL) gmvrdmemerr2();
      faceverts_alloc = k;
     }
   for (i = 0; i < totverts; i++)
      faceverts[nvertsin+i] = gmv_data.longdata2[i];

   /*  Set facetoverts pointer array.  */
   if (nfacesin+nfaces > facetoverts_alloc)
     {
      j = (nfacesin+1) / (icell+1);
      k = facetoverts_alloc + nc*j;
      if (k < nfacesin+nfaces) k = nfacesin+nfaces + nc*j;
      facetoverts = (long  *)realloc(facetoverts,k*sizeof(long));
      if (facetoverts == NULL) gmvrdmemerr2();
      facetoverts_alloc = k;
     }
   for (i = 0; i < nfaces; i++)
      nverts[i] = gmv_data.longdata1[i];
   j = 0;
   for (i = 0; i < nfaces; i++)
     {
      facetoverts[nfacesin+i] = nvertsin + j;
      j += nverts[i];
     }

   /*  Reset counters.  */
   nfacesin += nfaces;
   nvertsin += totverts;
}


void regcell(long icell, long nc)
{
  /*                                                       */
  /*  Read and set data for a regular cell.                */
  /*  icelltype indicates the cell type,                   */
  /*    1-tri (triangle), 2-quad,  3-tet, 4-hex,           */
  /*    5-prism, 6-pyramid, 7-line, 8-phex8, 9-phex20,     */
  /*    10-pprymd5, 11-pprymd13, 12-pprism6, 13-pprism15,  */
  /*    14-ptet4, 15-ptet10, 16-6tri, 17-8quad,            */
  /*    18-3line, 19-phex27                                */
  /*                                                       */
  long i, j, k, cnodes[30], fverts[145], l1, l2; 
  int nfaces = 0, nverts[144], totverts = 0,  dupflag, ncnodes, /* TODO: check fix for uninitialized type */
      dupverts[145], dupnverts[145], ndup, nf;
  int icelltype  = 0; /* TODO: check fix for uninitialized type */
  char ckeyword[MAXKEYWORDLENGTH+64];
  short trinverts[1] = {3};
  short trifverts[3] = {1,2,3};
  short quadnverts[1] = {4};
  short quadfverts[4] = {1,2,3,4};
  short tetnverts[4] = {3,3,3,3};
  short tetfverts[12] = {1,2,3, 1,3,4, 1,4,2, 4,3,2};
  short pyrnverts[5] = {3,3,3,3,4};
  short pyrfverts[16] = {1,2,3, 1,3,4, 1,4,5, 1,5,2, 5,4,3,2};
  short prsnverts[5] = {3,4,4,4,3};
  short prsfverts[18] = {1,2,3, 1,4,5,2 ,2,5,6,3, 1,3,6,4, 6,5,4};
  short hexnverts[6] = {4,4,4,4,4,4};
  short hexfverts[24] = {1,2,3,4, 1,5,6,2, 2,6,7,3, 3,7,8,4,
                         4,8,5,1, 8,7,6,5};
  short linenverts[1] = {2};
  short linefverts[2] = {1,2};
  short phex8nverts[6] = {4,4,4,4,4,4};
  short phex8fverts[24] = {1,4,3,2, 1,2,6,5, 2,3,7,6, 3,4,8,7,
                           4,1,5,8, 5,6,7,8};
  short phex20nverts[6] = {8,8,8,8,8,8};
  short phex20fverts[48] = {12,4,11,3,10,2,9,1, 9,2,18,6,13,5,17,1,
                            10,3,19,7,14,6,18,2, 11,4,20,8,15,7,19,3,
                            12,1,17,5,16,8,20,4, 13,6,14,7,15,8,16,5};
  short pyr5nverts[5] = {4,3,3,3,3};
  short pyr5fverts[16] = {4,3,2,1, 1,2,5, 2,3,5, 3,4,5, 4,1,5};
  short pyr13nverts[5] = {8,6,6,6,6};
  short pyr13fverts[32] = {4,8,3,7,2,6,1,9, 1,6,2,11,5,10, 2,7,3,12,5,11, 
                           3,8,4,13,5,12, 4,9,1,10,5,13};
  short prs6nverts[5] = {3,4,4,4,3};
  short prs6fverts[18] = {1,3,2, 1,2,5,4 ,2,3,6,5, 1,4,6,3, 5,6,4};
  short prs15nverts[5] = {6,8,8,8,6};
  short prs15fverts[36] = {1,9,3,8,2,7, 1,7,2,14,5,10,4,13, 2,8,3,15,6,11,5,14,
                           3,9,1,13,4,12,6,15, 5,11,6,12,4,10};
  short tet4nverts[4] = {3,3,3,3};
  short tet4fverts[12] = {1,3,2, 1,2,4, 1,3,4, 2,3,4};
  short tet10nverts[4] = {6,6,6,6};
  short tet10fverts[24] = {1,7,3,6,2,5, 1,5,2,9,4,8, 1,7,3,10,4,8, 
                            2,6,3,10,4,9};
  short quad8nverts[1] = {8};
  short quad8fverts[8] = {1,5,2,6,3,7,4,8};
  short tri6nverts[1] = {6};
  short tri6fverts[6] = {1,4,2,5,3,6};
  short line3nverts[2] = {2,2};
  short line3fverts[4] = {1,2, 2,3};
  short phex27nverts[48] = {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
                            3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3};
  short phex27fverts[144] = {1,9,21,  9,2,21,  2,18,21, 18,6,21, 
                             6,13,21, 13,5,21, 5,17,21, 17,1,21,
                             2,10,22, 10,3,22, 3,19,22, 19,7,22,
                             7,14,22, 14,6,22, 6,18,22, 18,2,22,
                             3,11,23, 11,4,23, 4,20,23, 20,8,23,
                             8,15,23, 15,7,23, 7,19,23, 19,3,23,
                             4,12,24, 12,1,24, 1,17,24, 17,5,24,
                             5,16,24, 16,8,24, 8,20,24, 20,4,24,
                             1,12,25, 12,4,25, 4,11,25, 11,3,25,
                             3,10,25, 10,2,25, 2,9,25,  9,1,25,
                             5,13,26, 13,6,26, 6,14,26, 14,7,26,
                             7,15,26, 15,8,26, 8,16,26, 16,5,26};
  short *nv = NULL, *fv = NULL; /* TODO: check fix for uninitialized pointers */

   /*  Get cell nodes.  */
   ncnodes = gmv_data.nlongdata1;
   for (i = 0; i < ncnodes; i++)
      cnodes[i] = gmv_data.longdata1[i];

   /*  Save cell nodes for regular cells.  */
   cellnnode[icell] = ncnodes;
   if (totcellnodes + ncnodes > cellnodes_alloc)
     {
      i = (nc - icell + 1) * ncnodes;
      cellnodes_alloc += i;
      cellnodes = (long *)realloc(cellnodes,cellnodes_alloc*sizeof(long));
     }
   for (i = 0; i < ncnodes; i++)
      cellnodes[totcellnodes+i] = cnodes[i];
   totcellnodes += ncnodes;

   /*  Check for duplicate nodes.  */
   dupflag = 0;
   for (i = 0; i < ncnodes-1; i++)
     {
      for (j = i+1; j < ncnodes; j++)
        {
         if (cnodes[i] == cnodes[j])
            dupflag = 1;
         }
     }

   /*  Determine cell type.  */
   strncpy(ckeyword,gmv_data.name1,MAXKEYWORDLENGTH);
   *(ckeyword + GMV_MIN(strlen(gmv_data.name1), MAXKEYWORDLENGTH)) = (char)0;
   if (strncmp(ckeyword,"tri",3) == 0) icelltype = 1;
   else if (strncmp(ckeyword,"quad",4) == 0) icelltype = 2;
   else if (strncmp(ckeyword,"tet",3) == 0) icelltype = 3;
   else if (strncmp(ckeyword,"hex",3) == 0) icelltype = 4;
   else if (strncmp(ckeyword,"prism",5) == 0) icelltype = 5;
   else if (strncmp(ckeyword,"pyramid",7) == 0) icelltype = 6;
   else if (strncmp(ckeyword,"line",4) == 0) icelltype = 7;
   else if (strncmp(ckeyword,"phex8",5) == 0) icelltype = 8;
   else if (strncmp(ckeyword,"phex20",6) == 0) icelltype = 9;
   else if (strncmp(ckeyword,"ppyrmd5",7) == 0) icelltype = 10;
   else if (strncmp(ckeyword,"ppyrmd13",8) == 0) icelltype = 11;
   else if (strncmp(ckeyword,"pprism6",7) == 0) icelltype = 12;
   else if (strncmp(ckeyword,"pprism15",8) == 0) icelltype = 13;
   else if (strncmp(ckeyword,"ptet4",5) == 0) icelltype = 14;
   else if (strncmp(ckeyword,"ptet10",6) == 0) icelltype = 15;
   else if (strncmp(ckeyword,"6tri",4) == 0) icelltype = 16;
   else if (strncmp(ckeyword,"8quad",5) == 0) icelltype = 17;
   else if (strncmp(ckeyword,"3line",5) == 0) icelltype = 18;
   else if (strncmp(ckeyword,"phex27",6) == 0) icelltype = 19;

   /*  Set face information according to cell type.  */
   switch (icelltype)
     {
      case 1: nfaces = 1;    /* tri */
              totverts = 3;
              nv = trinverts;
              fv = trifverts;
              break;
      case 2: nfaces = 1;    /* quad */
              totverts = 4;
              nv = quadnverts;
              fv = quadfverts;
              break;
      case 3: nfaces = 4;    /* tet */
              totverts = 12;
              nv = tetnverts;
              fv = tetfverts;
              break;
      case 4: nfaces = 6;    /* hex */
              totverts = 24;
              nv = hexnverts;
              fv = hexfverts;
              break;
      case 5: nfaces = 5;    /* prism */
              totverts = 18;
              nv = prsnverts;
              fv = prsfverts;
              break;
      case 6: nfaces = 5;    /* pyramid */
              totverts = 16;
              nv = pyrnverts;
              fv = pyrfverts;
              break;
      case 7: nfaces = 1;    /* line */
              totverts = 2;
              nv = linenverts;
              fv = linefverts;
              break;
      case 8: nfaces = 6;    /* phex8 */
              totverts = 24;
              nv = phex8nverts;
              fv = phex8fverts;
              break;
      case 9: nfaces = 6;    /* phex20 */
              totverts = 48;
              nv = phex20nverts;
              fv = phex20fverts;
              break;
      case 10: nfaces = 5;    /* ppyrmd5, patran 5 point pyramid */
               totverts = 16;
               nv = pyr5nverts;
               fv = pyr5fverts;
               break;
      case 11: nfaces = 5;    /* ppyrmd13, patran 13 point pyramid */
               totverts = 32;
               nv = pyr13nverts;
               fv = pyr13fverts;
               break;
      case 12: nfaces = 5;    /* pprism6, patran 6 point prism */
               totverts = 18;
               nv = prs6nverts;
               fv = prs6fverts;
               break;
      case 13: nfaces = 5;    /* pprism15, patran 15 point prism */
               totverts = 36;
               nv = prs15nverts;
               fv = prs15fverts;
               break;
      case 14: nfaces = 4;    /* ptet4, patran tet */
               totverts = 12;
               nv = tet4nverts;
               fv = tet4fverts;
               break;
      case 15: nfaces = 4;    /* ptet10, patran tet */
               totverts = 24;
               nv = tet10nverts;
               fv = tet10fverts;
               break;
      case 16: nfaces = 1;    /* 6tri */
               totverts = 6;
               nv = tri6nverts;
               fv = tri6fverts;
               break;
      case 17: nfaces = 1;    /* 8quad */
               totverts = 8;
               nv = quad8nverts;
               fv = quad8fverts;
               break;
      case 18: nfaces = 2;    /* 3line, 3 point line */
               totverts = 4;
               nv = line3nverts;
               fv = line3fverts;
               break;
      case 19: nfaces = 48;    /* phex27, 27 point hex */
               totverts = 144;
               nv = phex27nverts;
               fv = phex27fverts;
               break;
      default: break;
     }

   /* Avoid warning about possible use of uninitialized array element */
   /* fverts[0] used below. */
   fverts[0] = 0;

   /*  Build face information.  */
   for (i = 0; i < nfaces; i++)
      nverts[i] = nv[i];

   /*  Build face vertices.  */
   for (i = 0; i < totverts; i++)
     {
      j = fv[i] - 1;
      fverts[i] = cnodes[j];
     }

   /*  If duplicate nodes exist, check   */ 
   /*  and adjust for degenerate faces.  */
   if (dupflag)
     {
      ndup = 0;
      k = 0;
      nf = 0;
      for (i = 0; i < nfaces; i++)
        {
         if (i > 0) k += nverts[i-1];
         dupnverts[nf] = nverts[i];
         for (j = 0; j < nverts[i]; j++)
           {
            l1 = k + j;
            l2 = l1 + 1;
            if (j == nverts[i] - 1) l2 = k;
            if (fverts[l1] != fverts[l2])
              {
               dupverts[ndup] = fverts[l1];
               ndup++;
              }
            else
              {
               dupnverts[nf]--;
              }
           }
         if (dupnverts[nf] > 2) nf++;
         else ndup -= dupnverts[nf];
        }

      /*  If cell consists of all duplicate nodes, set to first node.  */
      if (ndup <= 0)
        {
         dupverts[0] = fverts[0];
         dupnverts[0] = 1;
        }

      nfaces = nf;
      if (nfaces == 0) nfaces = 1;
      totverts = 0;
      for (i = 0; i < nfaces; i++)
       {
        nverts[i] = dupnverts[i];
        totverts += dupnverts[i];
       }
      for (i = 0; i < totverts; i++)
        fverts[i] = dupverts[i];
     }

   /*  Save first face location for cell to faces pointer.  */
   celltoface[icell] = nfacesin;

   /*  Save no. of vertices per face, reallocate array if needed.  */
   if (nfacesin+nfaces > cellfaces_alloc)
     {
      k = cellfaces_alloc + (nc-icell)*nfaces;
      cell_faces = (long *)realloc(cell_faces,k*sizeof(long));
      if (cell_faces == NULL)
        {
         gmvrdmemerr2();
         return;
        }
      cellfaces_alloc = k;
     }
   for (i = 0; i < nfaces; i++)
      cell_faces[nfacesin+i] = nfacesin+i;      

   /*  Save all face vertices, reallocate if needed.  */
   if (nvertsin+totverts > faceverts_alloc)
     {
      j = totverts;
      k = faceverts_alloc + (nc-icell)*j;
      if (k < nvertsin+totverts) k = nvertsin+totverts + (nc-icell)*j;
      faceverts = (long  *)realloc(faceverts,k*sizeof(long));
      if (faceverts == NULL) gmvrdmemerr2();
      faceverts_alloc = k;
     }
   for (i = 0; i < totverts; i++)
      faceverts[nvertsin+i] = fverts[i];

   /*  Set facetoverts pointer array.  */
   if (nfacesin+nfaces > facetoverts_alloc)
     {
      k = facetoverts_alloc + (nc-icell)*nfaces;
      facetoverts = (long *)realloc(facetoverts,k*sizeof(long));
      if (facetoverts == NULL) gmvrdmemerr2();
      facetoverts_alloc = k;
     }
   j = 0;
   for (i = 0; i < nfaces; i++)
     {
      facetoverts[nfacesin+i] = nvertsin + j;
      j += nverts[i];
     }

   /*  Reset counters.  */
   nfacesin += nfaces;
   nvertsin += totverts;
}


void vfacecell(long icell, long nc)
{
  /*                                */
  /*  Set data for a vface cell.  */
  /*                                */
  int nfaces; 
  long i, j, k;

   /*  Save first face location for cell to faces pointer.  */
   celltoface[icell] = nfacesin;

   /*  Read and fill cell_faces array, reallocate if needed.  */
   nfaces = gmv_data.nlongdata1;
   if (nfacesin+nfaces > cellfaces_alloc)
     {
      j = (nfacesin+1) / (icell+1);
      k = cellfaces_alloc + (nc-icell)*j;
      if (k < nfacesin+nfaces) k = nfacesin+nfaces + nc*j;
      cell_faces = (long *)realloc(cell_faces,k*sizeof(long));
      if (cell_faces == NULL) gmvrdmemerr2();
      cellfaces_alloc = k;
     }
   for (i = 0; i < nfaces; i++)
      cell_faces[nfacesin+i] = gmv_data.longdata1[i] - 1;      

   /*  Reset counters.  */
   nfacesin += nfaces;
}


void fillmeshdata(long nc)
{
  /*                                               */
  /*  Fill gmv_meshdata structure with cell data.  */
  /*                                               */

   gmv_meshdata.ncells = nc;
   gmv_meshdata.nfaces = nfacesin;
   gmv_meshdata.totfaces = totfaces;
   gmv_meshdata.totverts = nvertsin;

   if (nc == 0) return;

   gmv_meshdata.celltoface = celltoface;
   gmv_meshdata.celltoface[nc] = totfaces;

   cell_faces = (long *)realloc(cell_faces,(totfaces+1)*sizeof(long));
   if (cell_faces == NULL) gmvrdmemerr2();
   gmv_meshdata.cellfaces = cell_faces;
   gmv_meshdata.cellfaces[totfaces] = nfacesin;

   facetoverts = (long *)realloc(facetoverts,(nfacesin+1)*sizeof(long));
   if (facetoverts == NULL) gmvrdmemerr2();
   gmv_meshdata.facetoverts = facetoverts;
   gmv_meshdata.facetoverts[nfacesin] = nvertsin;

   faceverts = (long *)realloc(faceverts,nvertsin*sizeof(long));
   if (faceverts == NULL) gmvrdmemerr2();
   gmv_meshdata.faceverts = faceverts;
}


void fillmeshdata(long nc);
void fillcellinfo(long nc, long *facecell1, long *facecell2);

void rdfaces()
{
  static long iface, *facecell1, *facecell2;
  long nc, i, k;
  int nverts;

   /*  Get first face info.  */
   gmv_meshdata.nfaces = gmv_data.num;
   gmv_meshdata.ncells = gmv_data.num2;
   nc = gmv_data.num2;
   nfacesin = gmv_data.num;

   gmv_meshdata.intype = FACES;

   celltoface = (long *)malloc((nc+1)*sizeof(long));
   facetoverts = (long *)malloc((nfacesin+1)*sizeof(long));
   faceverts = (long *)malloc(nfacesin*8*sizeof(long));
   faceverts_alloc = nfacesin*8;
   facecell1 = (long *)malloc(nfacesin*sizeof(long));
   facecell2 = (long *)malloc(nfacesin*sizeof(long));
   if (celltoface == NULL || faceverts == NULL || facecell1 == NULL ||
       facecell2 == NULL)
      gmvrdmemerr2();

   /*  Loop through faces.  */
   iface = 0;  nvertsin = 0;
   while (gmv_data.datatype != ENDKEYWORD)
     {
      nverts = gmv_data.nlongdata1 - 2;

      /*  Fill faceverts, allocate if needed.  */
      if (nvertsin+nverts > faceverts_alloc)
        {
         k = faceverts_alloc + nc*8;
         faceverts = (long *)realloc(faceverts,k*sizeof(long));
         if (faceverts == NULL) gmvrdmemerr2();
         faceverts_alloc = k;
        }
      for (i = 0; i < nverts; i++)
         faceverts[nvertsin+i] = gmv_data.longdata1[i];

      /*  Fill face cell nos.  */
      facecell1[iface] = gmv_data.longdata1[nverts];
      facecell2[iface] = gmv_data.longdata1[nverts+1];

      /*  Set facetoverts pointer array.  */
      facetoverts[iface] = nvertsin;

      nvertsin += nverts;
      iface++;

      gmvread_data();
      if (gmv_data.datatype == ENDKEYWORD)
        {
         fillcellinfo(nc, facecell1, facecell2);
         fillmeshdata(nc);
         return;
        }

      /*  Check for GMVERROR.  */
      if (gmv_data.keyword == GMVERROR)
        {
         gmv_meshdata.intype = GMVERROR;
         return;
        }
     }
}


void rdvfaces(long nc)
{
  static long iface, *facecell1, *facecell2;
  static long *facepe, *oppface, *oppfacepe;
  long i, k;
  int nverts;

   /*  Get first vface info.  */
   gmv_meshdata.nfaces = gmv_data.num;
   gmv_meshdata.ncells = nc;
   if (gmv_data.num != nfacesin)
     {
      fprintf(stderr,"I/O error while reading vfaces.\n");
      gmv_data.errormsg = (char *)malloc(32 * sizeof(char));
      snprintf(gmv_data.errormsg,32,"I/O error while reading vfaces.");
      gmv_meshdata.intype = GMVERROR;
      return;
     }
   nfacesin = gmv_data.num;
   totfaces = nfacesin;

   facetoverts = (long *)malloc((nfacesin+1)*sizeof(long));
   faceverts = (long *)malloc(nfacesin*8*sizeof(long));
   faceverts_alloc = nfacesin*8;
   facecell1 = (long *)malloc(nfacesin*sizeof(long));
   facecell2 = (long *)malloc(nfacesin*sizeof(long));
   facepe = (long *)malloc(nfacesin*sizeof(long));
   oppface = (long *)malloc(nfacesin*sizeof(long));
   oppfacepe = (long *)malloc(nfacesin*sizeof(long));
   if (facetoverts == NULL || faceverts == NULL || facecell1 == NULL ||
       facecell2 == NULL || facepe == NULL || oppface == NULL || 
       oppfacepe == NULL)
      gmvrdmemerr2();

   /*  Loop through faces.  */
   iface = 0;  nvertsin = 0;
   while (gmv_data.datatype != ENDKEYWORD)
     {
      nverts = gmv_data.nlongdata1;

      /*  Fill faceverts, allocate if needed.  */
      if (nvertsin+nverts > faceverts_alloc)
        {
         k = faceverts_alloc + nc*8;
         faceverts = (long *)realloc(faceverts,k*sizeof(long));
         if (faceverts == NULL) gmvrdmemerr2();
         faceverts_alloc = k;
        }
      for (i = 0; i < nverts; i++)
         faceverts[nvertsin+i] = gmv_data.longdata1[i];

      /*  Fill face pe, oppface, oppfacepe and cell no.  */
      facepe[iface] = gmv_data.longdata2[0];
      oppface[iface] = gmv_data.longdata2[1] - 1;
      if (oppface[iface] >= nfacesin) oppface[iface] = 0;
      oppfacepe[iface] = gmv_data.longdata2[2];
      facecell1[iface] = gmv_data.longdata2[3];
      facecell2[iface] = 0;

      /*  Set facetoverts pointer array.  */
      facetoverts[iface] = nvertsin;

      nvertsin += nverts;
      iface++;

      gmvread_data();
      if (gmv_data.datatype == ENDKEYWORD)
        {

         /*  Fill second cellid for face.  */
         for (i = 0; i < nfacesin; i++)
           {
            
            /*  If face and opposite faces have same  */
            /*  pe, get cell.  Otherwise use ncells.  */
            if (oppface[i] >= 0)
              {
               if (facepe[i] == oppfacepe[i])
                 {
                  k = oppface[i];
                  facecell2[i] = facecell1[k];
                 }
              }
           }

         /*  Check for GMVERROR.  */
         if (gmv_data.keyword == GMVERROR)
           {
            gmv_meshdata.intype = GMVERROR;
            return;
           }
         fillmeshdata(nc);

         /*  Fill vface extra data.  */
         gmv_meshdata.facecell1 = facecell1;
         gmv_meshdata.facecell2 = facecell2;
         gmv_meshdata.vfacepe = facepe;
         gmv_meshdata.vfaceoppface = oppface;
         gmv_meshdata.vfaceoppfacepe = oppfacepe;
         return;
        }
     }
}



void rdxfaces()
{
  static long *facecell1, *facecell2;
  static long *facepe, *oppface, *oppfacepe;
  long nc, i, k, totverts;
  int maxnvert;

   /*  Get info for first xfaces read.  */
   gmv_meshdata.nfaces = gmv_data.num;
   nfacesin = gmv_data.num;
   totfaces = nfacesin;
   totverts = gmv_data.nlongdata2;
   nvertsin = totverts;

   facetoverts = (long *)malloc((nfacesin+1)*sizeof(long));
   faceverts = (long *)malloc(totverts*sizeof(long));
   facecell1 = (long *)malloc(nfacesin*sizeof(long));
   facecell2 = (long *)malloc(nfacesin*sizeof(long));
   facepe = (long *)malloc(nfacesin*sizeof(long));
   oppface = (long *)malloc(nfacesin*sizeof(long));
   oppfacepe = (long *)malloc(nfacesin*sizeof(long));
   if (facetoverts == NULL || faceverts == NULL || facecell1 == NULL ||
       facecell2 == NULL || facepe == NULL || oppface == NULL || 
       oppfacepe == NULL)
      gmvrdmemerr2();
   for (i = 0; i < nfacesin; i++) facecell2[i] = 0;

   /*  Fill the face to verts and the faceverts arrays.  */
   k = 0;
   facetoverts[0] = 0;
   for (i = 0; i < nfacesin; i++)
     {
      k +=  gmv_data.longdata1[i];
      facetoverts[i+1] = k;
     }
   for (i = 0; i < totverts; i++)
      faceverts[i] = gmv_data.longdata2[i];

   /*  Determine if the mesh is 2D (max. nverts is 2).  */
   maxnvert = 0;
   for (i = 0; i < nfacesin; i++)
     {
      if (gmv_data.longdata1[i] > maxnvert) 
         maxnvert = gmv_data.longdata1[i];
     }

   /*  Save the mesh type as VFACES2D or VFACES3D.  */
   if (maxnvert <= 2)
      gmv_meshdata.intype = VFACES2D;
   else
      gmv_meshdata.intype = VFACES3D;

   /*  Loop through xfaces data.  */
   while (gmv_data.datatype != ENDKEYWORD)
     {
      gmvread_data();

      if (gmv_data.datatype == ENDKEYWORD)
        {

         gmv_meshdata.ncells = gmv_data.num2;
         nc = gmv_data.num2;
         celltoface = (long *)malloc((nc+1)*sizeof(long));
         if (celltoface == NULL) gmvrdmemerr2();
         fillcellinfo(nc, facecell1, facecell2);
         fillmeshdata(nc);

         /*  Fill second cellid for face.  */
         for (i = 0; i < nfacesin; i++)
           {
            
            /*  If face and opposite faces have same  */
            /*  pe, get cell.  Otherwise use ncells.  */
            facecell2[i] = 0;
            if (oppface[i] >= 0)
              {
               if (facepe[i] == oppfacepe[i])
                 {
                  k = oppface[i];
                  facecell2[i] = facecell1[k];
                 }
              }
           }

         /*  Fill vface extra data.  */
         gmv_meshdata.facecell1 = facecell1;
         gmv_meshdata.facecell2 = facecell2;
         gmv_meshdata.vfacepe = facepe;
         gmv_meshdata.vfaceoppface = oppface;
         gmv_meshdata.vfaceoppfacepe = oppfacepe;
         return;
        }

      /*  Check for GMVERROR.  */
      if (gmv_data.keyword == GMVERROR)
        {
         gmv_meshdata.intype = GMVERROR;
         return;
        }

      /*  Save cell1 info.  */
      if (gmv_data.num2 == 1)
        {
         for (i = 0; i < nfacesin; i++)
            facecell1[i] = gmv_data.longdata1[i];
        }

      /*  Save opposite face info.  */
      if (gmv_data.num2 == 2)
        {
         for (i = 0; i < nfacesin; i++)
            oppface[i] = gmv_data.longdata1[i] - 1;
        }

      /*  Save face pe info.  */
      if (gmv_data.num2 == 3)
        {
         for (i = 0; i < nfacesin; i++)
            facepe[i] = gmv_data.longdata1[i];
        }

      /*  Save opposite face pe info.  */
      if (gmv_data.num2 == 4)
        {
         for (i = 0; i < nfacesin; i++)
            oppfacepe[i] = gmv_data.longdata1[i];
        }
     }
}


void fillcellinfo(long nc, long *facecell1, long *facecell2)
{
  long i, j, k, sumcount;
  int *fcount;

   /*  Count the number of faces per cell.  */
   fcount = (int *)malloc(nc*sizeof(int));
   if (fcount == NULL) gmvrdmemerr2();
   for (i = 0; i < nc; i++) fcount[i] = 0;
   for (i = 0; i < nfacesin; i++)
     {
      j = facecell1[i];
      if (j > 0) fcount[j-1]++;
      j = facecell2[i];
      if (j > 0) fcount[j-1]++;
     }

   /*  Get the total number of faces per cell    */
   /*  and fill the cellto face array by count.  */
   totfaces = 0;
   sumcount = 0;
   for (i = 0; i < nc; i++)
     {
      celltoface[i] = sumcount;
      sumcount += fcount[i];
      totfaces += fcount[i];
     }

   /*  Allocate and fill cell_faces pointer array.  */
   cell_faces = (long *)malloc((totfaces+1)*sizeof(long));
   if (cell_faces == NULL) gmvrdmemerr2();
   for (i = 0; i < nc; i++) fcount[i] = 0;
   for (i = 0; i < nfacesin; i++)
     {
      j = facecell1[i];
      if (j > 0)
        {
         j--;
         k = celltoface[j] + fcount[j];
         cell_faces[k] = i;
         fcount[j]++;
        }
      j = facecell2[i];
      if (j > 0)
        {
         j --;
         k = celltoface[j] + fcount[j];
         cell_faces[k] = i;
         fcount[j]++;
        }
     }
   free(fcount);

   /*  Fill facecell1 and facecell2 in gmv_meshdata.  */
   facecell1 = (long *)realloc(facecell1,nfacesin*sizeof(long));
   facecell2 = (long *)realloc(facecell2,nfacesin*sizeof(long));
   if (facecell1 == NULL || facecell2 == NULL)
      gmvrdmemerr2();
   gmv_meshdata.facecell1 = facecell1;
   gmv_meshdata.facecell2 = facecell2;
}



void struct2vface()
{
  int nxv, nyv, nzv, nxyv, nxc, nyc, nxyc, itemp[8], izc, ixy, iyc, ixc,
      nc, nf, nv, struct2dflag, nfaces, nfv, kf, jf, icell, icell2, ii, jj,
      kk, kv, j;
  short hexfverts[24] = {1,4,3,2, 1,2,6,5, 2,3,7,6, 3,4,8,7,
                         4,1,5,8, 5,6,7,8};
  short quadfverts[8] = {1,2, 2,3, 3,4, 4,1};

   /*  Generate a set of vface data from a structured grid.  */
   nxv = gmv_meshdata.nxv;
   nyv = gmv_meshdata.nyv;
   nzv = gmv_meshdata.nzv;

   /*  Check for 2D structured (nzv = 1). */
   gmv_meshdata.intype = VFACES3D;
   struct2dflag = 0;
   if (nzv == 1)
     {
      gmv_meshdata.intype = VFACES2D;
      struct2dflag = 1;
     }

   /*  Determine the number of cells, faces and vertices.  */
   nc = (nxv-1) * (nyv-1) * (nzv-1);
   nf = nc * 6;
   nfaces = 6;
   nv = nc * 24;
   nfv = 4;
   if (struct2dflag == 1)
     {
      nc = (nxv-1) * (nyv-1);
      nf = nc * 4;
      nfaces = 4;
      nv = nc * 8;
      nfv = 2;
     }
   gmv_meshdata.ncells = nc;
   gmv_meshdata.nfaces = nf;
   gmv_meshdata.totfaces = nf;
   gmv_meshdata.totverts = nv;

   /*  Set boundary info.  */
   nxyv = nxv * nyv;
   nxc = nxv - 1;
   nyc = nyv - 1;
   nxyc = nxc * nyc;

   /*  Allocate memory needed to fill mesh data.  */
   gmv_meshdata.celltoface = (long *)malloc((nc+1)*sizeof(long));
   if (gmv_meshdata.celltoface == NULL) gmvrdmemerr2();
   gmv_meshdata.cellfaces = (long *)malloc((nf+1)*sizeof(long));
   if (gmv_meshdata.cellfaces == NULL) gmvrdmemerr2();
   gmv_meshdata.facetoverts = (long *)malloc((nf+1)*
                                                   sizeof(long));
   if (gmv_meshdata.facetoverts == NULL) gmvrdmemerr2();
   gmv_meshdata.faceverts = (long *)malloc(nv*sizeof(long));
   if (gmv_meshdata.faceverts == NULL) gmvrdmemerr2();
   gmv_meshdata.facecell1 = (long *)malloc(nf*sizeof(long));
   gmv_meshdata.facecell2 = (long *)malloc(nf*sizeof(long));
   if (gmv_meshdata.facecell1 == NULL || 
       gmv_meshdata.facecell2 == NULL)
      gmvrdmemerr2();
   gmv_meshdata.vfacepe = (long *)malloc(nf*sizeof(long));
   gmv_meshdata.vfaceoppface = (long *)malloc(nf*sizeof(long));
   gmv_meshdata.vfaceoppfacepe = (long *)malloc(nf*sizeof(long));
   if (gmv_meshdata.vfacepe == NULL || 
       gmv_meshdata.vfaceoppface == NULL || 
       gmv_meshdata.vfaceoppfacepe == NULL)
      gmvrdmemerr2();

   /*  Loop over the number of cells, and fill gmvmesh_data.  */
   for (icell = 0; icell < nc; icell++)
     {

      /*  Determine cell vertices.  */
      izc = icell / nxyc;
      ixy = icell - izc * nxyc;
      iyc = ixy / nxc;
      ixc = ixy - iyc * nxc;

      itemp[0] = ixc + iyc*nxv + izc*nxyv + 1;
      itemp[1] = itemp[0] + 1;
      itemp[2] = itemp[1] + nxv;
      itemp[3] = itemp[0] + nxv;
      itemp[4] = itemp[0] + nxyv;
      itemp[5] = itemp[1] + nxyv;
      itemp[6] = itemp[2] + nxyv;
      itemp[7] = itemp[3] + nxyv;

      /*  Determine cell faces.  */
      kf = nfaces * icell;
      gmv_meshdata.celltoface[icell] = nfaces * icell;
      for (ii = 0; ii < nfaces; ii++)
        {
         jf = kf + ii;
         gmv_meshdata.cellfaces[jf] = jf;
         kv = jf * nfv;
         gmv_meshdata.facetoverts[jf] = kv;
         for (j = 0; j < nfv; j++)
           {

            /*  Determine face vertices.  */
            kk = (ii*nfv) + j;
            jj = hexfverts[kk] - 1;
            if (struct2dflag == 1) jj = quadfverts[kk] - 1;
            gmv_meshdata.faceverts[kv+j] = itemp[jj];
           }
         gmv_meshdata.facecell1[jf] = icell + 1;
         jj = ii;
         if (struct2dflag == 1) jj = ii + 1;
         switch (jj)
           {
            case 0: icell2 = icell - nxyc;
                    gmv_meshdata.facecell2[jf] = icell2 + 1;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 5;
                    if (izc == 0) 
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
            case 1: icell2 = icell - nxc;
                    gmv_meshdata.facecell2[jf] = icell2 + 1;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 3;
                    if (struct2dflag == 1)
                       gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 2;
                    if (iyc == 0)
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
            case 2: icell2 = icell + 1;
                    gmv_meshdata.facecell2[jf] = icell2 + 1;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 4;
                    if (struct2dflag == 1)
                       gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 3;
                    if (ixc+1 == nxc)
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
            case 3: icell2 = icell + nxc;
                    gmv_meshdata.facecell2[jf] = icell2 + 1;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 1;
                    if (struct2dflag == 1)
                       gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 0;
                    if (iyc+1 == nyc)
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
            case 4: icell2 = icell - 1;
                    gmv_meshdata.facecell2[jf] = icell2 + 1;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 2;
                    if (struct2dflag == 1)
                       gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 1;
                    if (ixc == 0)
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
            case 5: icell2 = icell + nxyc;
                    gmv_meshdata.facecell2[jf] = icell + 12;
                    gmv_meshdata.vfaceoppface[jf] = (icell2 * nfaces) + 0;
                    if (icell+nxyc >= nc)
                       gmv_meshdata.facecell2[jf] = 0;
                    break;
           }
         if (gmv_meshdata.facecell2[jf] == 0) 
            gmv_meshdata.vfaceoppface[jf] = -1;
         gmv_meshdata.vfacepe[jf] = 0;
         gmv_meshdata.vfaceoppfacepe[jf] = 0;
        }
     }
   gmv_meshdata.celltoface[nc] = nf;
   gmv_meshdata.cellfaces[nf] = nf;
   gmv_meshdata.facetoverts[nf] = nv;
}


void struct2face()
{
  int nxv, nyv, nzv, nxyv, nxc, nyc, nxyc, itemp[8], izc, ixy, iyc, ixc,
      nc, nf, nv, struct2dflag, nfaces, nfv, kf, jf, icell, icell2, ii, jj,
      kk, kv, j, totf, lf, i;
  int nfxs, nfys, nxfaces, nyfaces,
      xface, yface, zface, iface[6];
  short hexfverts[24] = {1,4,3,2, 1,2,6,5, 2,3,7,6, 3,4,8,7,
                         4,1,5,8, 5,6,7,8};

   /*  Generate a set of vface data from a structured grid.  */
   nxv = gmv_meshdata.nxv;
   nyv = gmv_meshdata.nyv;
   nzv = gmv_meshdata.nzv;

   /*  Check for 2D structured (nzv = 1). */
   gmv_meshdata.intype = FACES;
   struct2dflag = 0;
   if (nzv == 1)
      struct2dflag = 1;

   /*  Determine the number of cells, faces and vertices.  */
   nc = (nxv-1) * (nyv-1) * (nzv-1);
   nf = (nxv*(nyv-1))*(nzv-1) + (nyv*(nxv-1))*(nzv-1) + (nxv-1)*(nyv-1)*nzv;
   totf = nc * 6;
   nfaces = 6;
   nv = nf * 4;
   nfv = 4;
   if (struct2dflag == 1)
     {
      nc = (nxv-1) * (nyv-1);
      nfaces = 1;
     }
   gmv_meshdata.ncells = nc;
   gmv_meshdata.nfaces = nf;
   gmv_meshdata.totfaces = totf;
   gmv_meshdata.totverts = nv;

   /*  Set boundary info.  */
   nxyv = nxv * nyv;
   nxc = nxv - 1;
   nyc = nyv - 1;
   nxyc = nxc * nyc;

   /*  Allocate memory needed to fill mesh data.  */
   gmv_meshdata.celltoface = (long *)malloc((nc+1)*sizeof(long));
   if (gmv_meshdata.celltoface == NULL) gmvrdmemerr2();
   gmv_meshdata.cellfaces = (long *)malloc((totf+1)*sizeof(long));
   if (gmv_meshdata.cellfaces == NULL) gmvrdmemerr2();
   gmv_meshdata.facetoverts = (long *)malloc((nf+1)*
                                                   sizeof(long));
   if (gmv_meshdata.facetoverts == NULL) gmvrdmemerr2();
   gmv_meshdata.faceverts = (long *)malloc(nv*sizeof(long));
   if (gmv_meshdata.faceverts == NULL) gmvrdmemerr2();
   gmv_meshdata.facecell1 = (long *)malloc(nf*sizeof(long));
   gmv_meshdata.facecell2 = (long *)malloc(nf*sizeof(long));
   if (gmv_meshdata.facecell1 == NULL || 
       gmv_meshdata.facecell2 == NULL)
      gmvrdmemerr2();
   for (i = 0; i < nf; i++)
      {
       gmv_meshdata.facecell1[i] = -1; 
       gmv_meshdata.facecell2[i] = -1; 
      }

   /*  Loop over the number of cells, and fill gmvmesh_data.  */
   for (icell = 0; icell < nc; icell++)
     {

      /*  Determine cell vertices.  */
      izc = icell / nxyc;
      ixy = icell - izc * nxyc;
      iyc = ixy / nxc;
      ixc = ixy - iyc * nxc;

      itemp[0] = ixc + iyc*nxv + izc*nxyv + 1;
      itemp[1] = itemp[0] + 1;
      itemp[2] = itemp[1] + nxv;
      itemp[3] = itemp[0] + nxv;
      itemp[4] = itemp[0] + nxyv;
      itemp[5] = itemp[1] + nxyv;
      itemp[6] = itemp[2] + nxyv;
      itemp[7] = itemp[3] + nxyv;

      /*  Determine cell faces.  */
      nfxs = nxv * (nyv-1);
      nfys = nyv * (nxv-1);
      nxfaces = nfxs * (nzv-1);
      nyfaces = nfys * (nzv-1);
      xface = iyc*nxv + ixc + nfxs*izc;
      yface = iyc*nxc + ixc + nfys*izc + nxfaces;
      zface = icell + nxfaces + nyfaces;
      iface[0] = zface;
      iface[1] = yface;
      iface[2] = xface + 1;
      iface[3] = yface + nxc;
      iface[4] = xface;
      iface[5] = zface + nxyc;
      kf = nfaces * icell;
      gmv_meshdata.celltoface[icell] = nfaces * icell;
      for (ii = 0; ii < nfaces; ii++)
        {
         lf = iface[ii];
         jf = kf + ii;
         gmv_meshdata.cellfaces[jf] = lf;
         kv = lf * nfv;

         /*  See if this face has been filled.  */
         if (gmv_meshdata.facecell1[lf] >= 0) continue;

         gmv_meshdata.facetoverts[lf] = kv;
         for (j = 0; j < nfv; j++)
           {

            /*  Determine face vertices.  */
            kk = (ii*nfv) + j;
            jj = hexfverts[kk] - 1;
            gmv_meshdata.faceverts[kv+j] = itemp[jj];
           }
         gmv_meshdata.facecell1[lf] = icell + 1;
         switch (ii)
           {
            case 0: icell2 = icell - nxyc;
                    gmv_meshdata.facecell2[lf] = icell2 + 1;
                    if (izc == 0) 
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
            case 1: icell2 = icell - nxc;
                    gmv_meshdata.facecell2[lf] = icell2 + 1;
                    if (iyc == 0)
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
            case 2: icell2 = icell + 1;
                    gmv_meshdata.facecell2[lf] = icell2 + 1;
                    if (ixc+1 == nxc)
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
            case 3: icell2 = icell + nxc;
                    gmv_meshdata.facecell2[lf] = icell2 + 1;
                    if (iyc+1 == nyc)
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
            case 4: icell2 = icell - 1;
                    if (ixc == 0)
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
            case 5: icell2 = icell + nxyc;
                    gmv_meshdata.facecell2[lf] = icell + 12;
                    if (icell+nxyc >= nc)
                       gmv_meshdata.facecell2[lf] = 0;
                    break;
           }
        }
     }
   gmv_meshdata.celltoface[nc] = totf;
   gmv_meshdata.cellfaces[totf] = nf;
   gmv_meshdata.facetoverts[nf] = nv;
}




/* ************************************************************* */
/*                                                               */
/*  Fortran interfaces to gmvread.                               */
/*                                                               */
/* ************************************************************* */

#if 0
void
#ifdef hp
fgmvread_open(char * filnam)
#elif CRAY
FGMVREAD_OPEN(char * filnam)
#else
fgmvread_open_(char * filnam)
#endif
{
  int i;
  char charbuffer[200];

   i = 0;
   while (*(filnam+i) != ' ')
     {
      charbuffer[i] = *(filnam+i);
     }
   charbuffer[i] = '\0';

   gmvread_open(charbuffer);
}


void
#ifdef hp
fgmvread_open_fromfileskip(char * filnam)
#elif CRAY
FGMVREAD_OPEN_FROMFILESKIP(char * filnam)
#else
fgmvread_open_fromfileskip_(char * filnam)
#endif
{
  int i;
  char charbuffer[200];

   i = 0;
   while (*(filnam+i) != ' ')
     {
      charbuffer[i] = *(filnam+i);
     }
   charbuffer[i] = '\0';

   gmvread_open_fromfileskip(charbuffer);
}


void
#ifdef hp
fgmvread_close()
#elif unicos
FGMVREAD_CLOSE()
#else
fgmvread_close_()
#endif
 {
    gmvread_close();
 }


void
#ifdef hp
fgmvread_data(int *keyword, int *datatype, long *num,
              long *num2, char name1[20], long *ndoubledata1, 
              double *doubledata1, long *ndoubledata2, 
              double *doubledata2, long *ndoubledata3, 
              double *doubledata3, long *nlongdata1, 
              long *longdata1, long *nlongdata2, 
              long *longdata2, int *nchardata1, char *chardata1, 
              int *nchardata2, char *chardata2)
#elif unicos
FGMVREAD_DATA(int *keyword, int *datatype, long *num,
              long *num2, char name1[20], long *ndoubledata1, 
              double *doubledata1, long *ndoubledata2, 
              double *doubledata2, long *ndoubledata3, 
              double *doubledata3, long *nlongdata1, 
              long *longdata1, long *nlongdata2, 
              long *longdata2, int *nchardata1, char *chardata1, 
              int *nchardata2, char *chardata2)
#else
fgmvread_data_(int *keyword, int *datatype, long *num,
               long *num2, char name1[20], long *ndoubledata1, 
               double *doubledata1, long *ndoubledata2, 
               double *doubledata2, long *ndoubledata3, 
               double *doubledata3, long *nlongdata1, 
               long *longdata1, long *nlongdata2, 
               long *longdata2, int *nchardata1, char *chardata1, 
               int *nchardata2, char *chardata2)
#endif
{

   gmvread_data();

   *keyword = gmv_data.keyword;
   *datatype = gmv_data.datatype;
   *num = gmv_data.num;
   *num2 = gmv_data.num2;
   name1 = gmv_data.name1;
   *ndoubledata1 = gmv_data.ndoubledata1;
   doubledata1 = gmv_data.doubledata1;
   *ndoubledata2 = gmv_data.ndoubledata2;
   doubledata2 = gmv_data.doubledata2;
   *ndoubledata3 = gmv_data.ndoubledata3;
   doubledata3 = gmv_data.doubledata3;
   *nlongdata1 = gmv_data.nlongdata1;
    longdata1 = gmv_data.longdata1;
   *nlongdata2 = gmv_data.nlongdata2;
    longdata2 = gmv_data.longdata2;
    *nchardata1 = gmv_data.nchardata1;
    chardata1 = gmv_data.chardata1;
    *nchardata2 = gmv_data.nchardata2;
    chardata2 = gmv_data.chardata2;
}


void
#ifdef hp
fgmvread_mesh(long *nnodes, long *ncells, long *nfaces,
              long *totfaces, long *totverts, int *intype,
              int *nxv, int *nyv, int *nzv, double *x, double *y, 
              double *z, long *celltoface, long *cell_faces,
              long *facetoverts, long *faceverts,
              long *facecell1, long *facecell2)
#elif unicos
FGMVREAD_MESH(long *nnodes, long *ncells, long *nfaces,
              long *totfaces, long *totverts, int *intype,
              int *nxv, int *nyv, int *nzv, double *x, double *y, 
              double *z, long *celltoface, long *cell_faces,
              long *facetoverts, long *faceverts,
              long *facecell1, long *facecell2)
#else
fgmvread_mesh_(long *nnodes, long *ncells, long *nfaces,
               long *totfaces, long *totverts, int *intype,
               int *nxv, int *nyv, int *nzv, double *x, double *y, 
               double *z, long *celltoface, long *cell_faces,
               long *facetoverts, long *faceverts,
               long *facecell1, long *facecell2)
#endif
{

   gmvread_mesh();

   *nnodes = gmv_meshdata.nnodes;
   *ncells = gmv_meshdata.ncells;
   *nfaces = gmv_meshdata.nfaces;
   *totfaces = gmv_meshdata.totfaces;
   *totverts = gmv_meshdata.totverts;
   *intype = gmv_meshdata.intype;
   *nxv = gmv_meshdata.nxv;
   *nyv = gmv_meshdata.nyv;
   *nzv = gmv_meshdata.nzv;
   x = gmv_meshdata.x;
   y = gmv_meshdata.y;
   z = gmv_meshdata.z;
   celltoface = gmv_meshdata.celltoface;
   cell_faces = gmv_meshdata.cellfaces;
   facetoverts = gmv_meshdata.facetoverts;
   faceverts = gmv_meshdata.faceverts;
   facecell1 = gmv_meshdata.facecell1;
   facecell2 = gmv_meshdata.facecell2;
}
#endif


/*  Code for gmv ray reader.  */
static long numrays;

void swapbytes(void *from, int size, int nitems),
     readray(FILE *gmvrayin, int ftype),
     readrayids(FILE *gmvrayin, int ftype),
     gmvrayrdmemerr(),  endfromfile();
int ioerrtst2(FILE *gmvrayin);
static FILE *gmvrayinGlobal;

void readrays(FILE* gmvrayin, int ftype);
void readrayids(FILE* gmvrayin, int ftype);

int chk_rayend(FILE *gmvrayin);

/* Mark C. Miller, Wed Aug 22, 2012: fix leak of filnam */
int gmvrayread_open(char *filnam)
{
   /*                                        */
   /*  Open and check a GMV ray input file.  */
   /*                                        */
   int chkend;
   char magic[MAXKEYWORDLENGTH+64], filetype[MAXKEYWORDLENGTH+64];
   char * slash;
   int alloc_filnam = 0;

   /* check for the path - if include open and save if not append */
#ifdef _WIN32
   slash = strrchr( filnam,  '\\' );
#else
   slash = strrchr( filnam,  '/' );
#endif
   if( file_path != NULL && slash == NULL )
      {
       /* append the path and check again*/
       size_t len = strlen(file_path) + strlen(filnam) + 1;
       char *temp = (char*)malloc(len *sizeof(char));
       strcpy(temp, file_path);
       strcat(temp, filnam);
       free( filnam );
       filnam = (char*)malloc(len *sizeof(char));
       alloc_filnam = 1;
       strcpy(filnam, temp);
       free(temp);
      }
   else if( file_path == NULL && slash != NULL)
      {
       size_t pos = slash - filnam + 1;
       file_path = (char*) malloc ( (pos+1) *sizeof(char));
       strncpy( file_path, filnam, pos );
       file_path[pos] = 0;
      }
   else if( file_path == NULL && slash == NULL )
      {
          fprintf(stderr,"Error with the path");
          gmv_data.errormsg = (char *)malloc(20 * sizeof(char));
          snprintf(gmv_data.errormsg,20,"Error with the path");
          return 1;
      }
   
   gmvrayinGlobal = fopen(filnam, "r");
   if(gmvrayinGlobal == NULL)
      {
       fprintf(stderr,"GMV cannot open file %s\n",filnam);
       errormsgvarlen = (int)strlen(filnam);
       gmv_data.errormsg = (char *)malloc((22 + errormsgvarlen) * sizeof(char));
       snprintf(gmv_data.errormsg,22 + errormsgvarlen,"GMV cannot open file %s",filnam);
       if (alloc_filnam) free(filnam);
       return 1;
      }
    
   /*  Read header. */
   binread(magic,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvrayinGlobal);
   if (strncmp(magic,"gmvrays",7) != 0)
      {
       fprintf(stderr,"This is not a GMV ray input file.\n");
       gmv_data.errormsg = (char *)malloc(34 * sizeof(char));
       snprintf(gmv_data.errormsg,34,"This is not a GMV ray input file.");
       if (alloc_filnam) free(filnam);
       return 2;
      }

   /*  Check that gmv input file has "endray".  */
   if (strncmp(magic,"gmvrayinput",8) == 0)
      {
       chkend = chk_rayend(gmvrayinGlobal);
       if (!chkend)
         {
          fprintf(stderr,"Error - endray not found.\n");
          gmv_data.errormsg = (char *)malloc(26 * sizeof(char));
          snprintf(gmv_data.errormsg,26,"Error - endray not found.");
          if (alloc_filnam) free(filnam);
          return 3;
         }
      }

   /*  Read file type and set ftype: 0-ieee binary, 1-ascii text,  */
   /*  0-ieeei4r4 binary, 2-ieeei4r8 binary, 3-ieeei8r4 binary,    */
   /*  or 4-ieeei8r8 binary.                                       */

   binread(filetype,charsize, CHAR, (long)MAXKEYWORDLENGTH, gmvrayinGlobal);

   /*  Rewind the file and re-read header for file type.  */
   ftypeGlobal = -1;
   if (strncmp(filetype,"ascii",5) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype," ascii",6) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"  ascii",7) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"   ascii",8) == 0) ftypeGlobal = ASCII;
   if (strncmp(filetype,"ieee",4) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieee",5) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r4",8) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype," ieeei4r4",9) == 0) ftypeGlobal = IEEEI4R4;
   if (strncmp(filetype,"ieeei4r8",8) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype," ieeei4r8",9) == 0) ftypeGlobal = IEEEI4R8;
   if (strncmp(filetype,"ieeei8r4",8) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype," ieeei8r4",9) == 0) ftypeGlobal = IEEEI8R4;
   if (strncmp(filetype,"ieeei8r8",8) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype," ieeei8r8",9) == 0) ftypeGlobal = IEEEI8R8;
   if (strncmp(filetype,"iecxi4r4",8) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype," iecxi4r4",9) == 0) ftypeGlobal = IECXI4R4;
   if (strncmp(filetype,"iecxi4r8",8) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype," iecxi4r8",9) == 0) ftypeGlobal = IECXI4R8;
   if (strncmp(filetype,"iecxi8r4",8) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype," iecxi8r4",9) == 0) ftypeGlobal = IECXI8R4;
   if (strncmp(filetype,"iecxi8r8",8) == 0) ftypeGlobal = IECXI8R8;
   if (strncmp(filetype," iecxi8r8",9) == 0) ftypeGlobal = IECXI8R8;

   /*  Determine character input size.  */
   charsize_in = 8;
   if (ftypeGlobal == ASCII || ftypeGlobal > IEEEI8R8) charsize_in = 32;

   /*  Reset IECX types back to regular types.  */
   if (ftypeGlobal == IECXI4R4) ftypeGlobal = IEEEI4R4;
   if (ftypeGlobal == IECXI4R8) ftypeGlobal = IEEEI4R8;
   if (ftypeGlobal == IECXI8R4) ftypeGlobal = IEEEI8R4;
   if (ftypeGlobal == IECXI8R8) ftypeGlobal = IEEEI8R8;

   /*  Check for valid file type.  */
   if (ftypeGlobal == -1)
     {
      fprintf(stderr,"Invalid GMV RAY input file type.  Type must be:\n");
      fprintf(stderr,
       "  ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8,\n");
      fprintf(stderr,
       "  iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8.\n");
      gmv_data.errormsg = (char *)malloc(141 * sizeof(char));
      snprintf(gmv_data.errormsg,141,"Invalid GMV RAY input file type.  Type must be: %s%s",
               "ascii, ieee, ieeei4r4, ieeei4r8, ieeei8r4, ieeei8r8, ",
               "iecxi4r4, iecxi4r8, iecxi8r4, iecxi8r8.");
      if (alloc_filnam) free(filnam);
      return 4;
     }

   /*  Check that machine can read I8 types.  */
   if (ftypeGlobal == IEEEI8R4 || ftypeGlobal == IEEEI8R8)
     {
      if (sizeof(long) < 8)
        {
         fprintf(stderr,"Cannot read 64bit I* types on this machine.\n");
         gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
         snprintf(gmv_data.errormsg,44,"Cannot read 64bit I* types on this machine.");
         if (alloc_filnam) free(filnam);
         return 4;
        }
     }

#ifdef BEFORE_TERRY_JORDAN_WINDOWS_CHANGES
   rewind(gmvrayinGlobal);
#endif
   /*re-open the file with the proper translation mode*/
   fclose(gmvrayinGlobal);
   if( ftypeGlobal != ASCII)
   {
    gmvrayinGlobal = fopen(filnam,"rb");
   }
   else
   {
    /* To be able to read on Windows ASCII files with both */
    /* CR and CRLF endings open file in binary mode.       */
    /* On Unix, the "b" option is ignored (at least since  */
    /* C90).                                               */
    gmvrayinGlobal = fopen(filnam,"rb");
   }

   if (ftypeGlobal != ASCII)
     {
      binread(magic,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvrayinGlobal);
      binread(filetype,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvrayinGlobal);
     }
   if (ftypeGlobal == ASCII) { int res = fscanf(gmvrayinGlobal,"%s%s",magic,filetype); (void) res; }

   if (alloc_filnam) free(filnam);
   return 0;
}


void gmvrayread_close()
{
   fclose(gmvrayinGlobal);
}


int ioerrtst2(FILE * gmvrayin)
{
   /*                                      */
   /*  Test input file for eof and error.  */
   /*                                      */

   if ((feof(gmvrayin) != 0) || (ferror(gmvrayin) != 0))
     {
      fprintf(stderr,"I/O error while reading gmv ray input file.\n");
      gmv_data.errormsg = (char *)malloc(44 * sizeof(char));
      snprintf(gmv_data.errormsg,44,"I/O error while reading gmv ray input file.");
      gmvray_data.nvars = -1;
      return 1;
     }
   return 0;
}


void gmvrayread_data()
{
  char keyword[MAXKEYWORDLENGTH+64];
  int iendLocal, curr_keywordLocal;

   /*  Zero gmvray_data and free structure arrays.  */
   gmvray_data.nrays = 0;
   gmvray_data.nvars = 0;
   FREE(gmvray_data.varnames);  
   FREE(gmvray_data.rayids);  
   FREE(gmvray_data.gmvrays);  

   /*  Read and process keyword based data until endray found.  */
   iendLocal = 0;
   while (iendLocal == 0)
     {
      if (ftypeGlobal != ASCII)
        {
         binread(keyword,charsize,CHAR,(long)MAXKEYWORDLENGTH,gmvrayinGlobal);
         *(keyword+MAXKEYWORDLENGTH)=(char)0;
        }
      if (ftypeGlobal == ASCII) { int res = fscanf(gmvrayinGlobal,"%s",keyword); (void) res; }

      if ((feof(gmvrayinGlobal) != 0) | (ferror(gmvrayinGlobal) != 0)) iendLocal = 1;

      if (strncmp(keyword,"endray",6) == 0)
        {
         curr_keywordLocal = GMVEND;
         iendLocal = 1;
        }
      else if (strncmp(keyword,"rays",5) == 0) curr_keywordLocal = RAYS;
      else if (strncmp(keyword,"rayids",7) == 0) curr_keywordLocal = RAYIDS;
      else curr_keywordLocal = INVALIDKEYWORD;

      /*  If invalid keyword, send error.  */
      if (curr_keywordLocal == INVALIDKEYWORD)
        {
         gmvray_data.nvars = -1;
         fprintf(stderr,"Error, %s is an invalid keyword.\n",keyword);
         errormsgvarlen = (int)strlen(keyword);
         gmv_data.errormsg = (char *)malloc((31 + errormsgvarlen) * sizeof(char));
         snprintf(gmv_data.errormsg,31 + errormsgvarlen,"Error, %s is an invalid keyword.",keyword);
         return;
        }

      /*  Read current keyword data.  */
      switch (curr_keywordLocal)
        {
         case(RAYS):
            readrays(gmvrayinGlobal,ftypeGlobal);
            break;   
         case(RAYIDS):
            readrayids(gmvrayinGlobal,ftypeGlobal);
            break;
        }

      /*  Check for error.  */
      if (gmvray_data.nvars == -1)
        {
         fclose(gmvrayinGlobal);
         return;
        }
     }

   if (iendLocal) fclose(gmvrayinGlobal);

   if (gmvray_data.nvars == -1) fclose(gmvrayinGlobal);
}



void readrays(FILE* gmvrayin, int ftype)
{
  int i, j=0, k, iswap=0, jswap=0, iray, npts=0, nvarin;
  int lrays, lrayvars;
  int *rayids;
  double *x, *y, *z, *field, *tmpdouble;
  float *tmpfloat = NULL; /* TODO: check fix for uninitialized pointer */
  char vname[MAXCUSTOMNAMELENGTH], *varnames;
  short vartype[NRAYVARS];
  struct gmvray *gmvrays;
  const char *rtype_str[4] = {"Points","Segments"};

   if (ftype == ASCII)
     {
      int res = fscanf(gmvrayin,"%d %d",&lrays,&lrayvars); (void) res;
      if (ioerrtst2(gmvrayin)) return;
     }
   else
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {         
         binread(&lrays,longlongsize,LONGLONG,(long)1,gmvrayin);
         binread(&lrayvars,longlongsize,LONGLONG,(long)1,gmvrayin);
       }
      else
        {         
         binread(&i,intsize,INT,(long)1,gmvrayin);
         binread(&j,intsize,INT,(long)1,gmvrayin);
         iswap = i;
         jswap = j;
         swapbytes(&iswap, intsize, 1);
         swapbytes(&jswap, intsize, 1);
         lrays = i;
         lrayvars = j;
        }
     }

   /*  Check for byte swapping.  */
   if (ftype != ASCII && lrays < 0 && (lrayvars < 0 || lrayvars > NRAYVARS))
     {
      swapbytes_on = 1;
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         swapbytes(&lrays, longlongsize, 1);
         swapbytes(&lrayvars, longlongsize, 1);
        }
      else
        {
         lrays = iswap;
         lrayvars = jswap;
        }
     }

   if (printon)
      printf("Reading %d rays.\n",lrays);

   /*  Allocate and read variable 8 or 32 char. names and types.  */
   varnames = (char *)malloc(lrayvars*MAXCUSTOMNAMELENGTH*sizeof(char));
   if (varnames == NULL)
     {
      gmvrdmemerr();
      return;
     }
   for (i = 0; i < lrayvars; i++)
     {

      if (ftype != ASCII)
        {
         binread(vname, charsize_in*charsize, CHAR, (long)1, gmvrayin);
         if (ioerrtst2(gmvrayin)) return;
         binread(&j,intsize,INT,(long)1,gmvrayin);
         if (ioerrtst2(gmvrayin)) return;
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvrayin,"%s %d",vname,&j); (void) res;
         if (ioerrtst2(gmvrayin)) return;
        }
      
      strncpy(&varnames[i*MAXCUSTOMNAMELENGTH],vname,MAXCUSTOMNAMELENGTH-1);
      *(varnames+i*MAXCUSTOMNAMELENGTH+charsize_in) = (char) 0;
      vartype[i] = j;
      if (printon)
        {
         printf("    %s  (%s)\n",vname,rtype_str[j]);
        }
     }

   /*  Set static part of gmvray_data.  */
   gmvray_data.nrays = lrays;
   gmvray_data.nvars = lrayvars;
   gmvray_data.varnames = varnames;
   for (i = 0; i < lrayvars; i++)
     {
      gmvray_data.vartype[i] = vartype[i];
     }

   /*  Allocate and fill rayids.  */
   rayids = (int *)malloc(lrays*sizeof(int));
   if (rayids == NULL)
     {
      gmvrayrdmemerr();
      return;
     }
   for (i = 0; i < lrays; i++) rayids[i] = i + 1;
   gmvray_data.rayids = rayids;

   /*  Allocate lrays gmvrays structures.  */
   gmvrays = (struct gmvray *)malloc(lrays*sizeof(struct gmvray));

   /*  Loop through lrays rays to read ray data.  */
   for (iray = 0; iray < lrays; iray++)
     {

      /*  Read the number of points for this ray.  */
      if (ftype != ASCII)
        {
         binread(&npts,intsize,INT,(long)1,gmvrayin);
         if (ioerrtst2(gmvrayin)) return;
        }
      if (ftype == ASCII)
        {
         int res = fscanf(gmvrayin,"%d",&npts); (void) res;
         if (ioerrtst2(gmvrayin)) return;
        }
      gmvrays[iray].npts = npts;

      if (printon)
         printf("  Reading ray %d with %d points\n",iray+1,npts);

      /*  Allocate and read npts x,y,z arrays.  */
      x = (double *)malloc(npts*sizeof(double));
      y = (double *)malloc(npts*sizeof(double));
      z = (double *)malloc(npts*sizeof(double));
      if (x == NULL || y == NULL || z == NULL)
        {
         gmvrayrdmemerr();
         return;
        }
      if (ftype != ASCII)
        {
         if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
           {
            tmpdouble = (double *)malloc((3*npts)*sizeof(double));
            if (tmpdouble == NULL)
              {
               gmvrayrdmemerr();
               return;
              }
            binread(tmpdouble,doublesize,DOUBLE,3*npts,gmvrayin);
            if (ioerrtst2(gmvrayin)) return;
            for (i = 0; i < npts; i++)
              {
               x[i] = tmpdouble[i];
               y[i] = tmpdouble[npts+i];
               z[i] = tmpdouble[2*npts+i];
              }
            FREE(tmpdouble);
           }
         else
           {
            tmpfloat = (float *)malloc((3*npts)*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrayrdmemerr();
               return;
              }
            binread(tmpfloat,floatsize,FLOAT,3*lrays,gmvrayin);
            if (ioerrtst2(gmvrayin)) return;
            if (node_inp_type == 0)  /*  nodes type  */
              {
               for (i = 0; i < npts; i++)
                 {
                  x[i] = tmpfloat[i];
                  y[i] = tmpfloat[npts+i];
                  z[i] = tmpfloat[2*npts+i];
                 }
              }
            FREE(tmpfloat);
           }
        }
      if (ftype == ASCII)
        {
         tmpdouble = (double *)malloc((3*npts)*sizeof(double));
         if (tmpdouble == NULL)
              {
               gmvrayrdmemerr();
               return;
              }
         rdfloats(tmpdouble,3*npts,gmvrayin);
         for (i = 0; i < npts; i++)
           {
            x[i] = tmpdouble[i];
            y[i] = tmpdouble[npts+i];
            z[i] = tmpdouble[2*npts+i];
           }
         FREE(tmpdouble);
        }
      gmvrays[iray].x = x;
      gmvrays[iray].y = y;
      gmvrays[iray].z = z;

      /*  Read variable data for this ray.  */
      if (lrayvars > 0)
        {
         if (ftype != ASCII && (ftype == IEEEI4R4 || ftype == IEEEI8R4))
           {
            tmpfloat = (float *)malloc(npts*sizeof(float));
            if (tmpfloat == NULL)
              {
               gmvrayrdmemerr();
               return;
              }
           }
        }
      for (k = 0; k < lrayvars; k++)
        {
         
         /*  Determine the number of variable elements to read.  */
         nvarin = npts;
         if (vartype[k] == 1) nvarin = npts - 1;

         field = (double *)malloc(nvarin*sizeof(double));
         if (field == NULL)
           {
            gmvrayrdmemerr();
            return;
           }

         if (ftype != ASCII)
           {
            if (ftype == IEEEI4R8 || ftype == IEEEI8R8)
              {
               binread(field,doublesize,DOUBLE,(long)nvarin,gmvrayin);
               ioerrtst2(gmvrayin);
              }
            else
              {
               binread(tmpfloat,floatsize,FLOAT,(long)nvarin, gmvrayin);
               ioerrtst2( gmvrayin);
               for (i = 0; i < nvarin; i++) 
                  field[i] = tmpfloat[i];
              }
           }
         if (ftype == ASCII) rdfloats(field, (long)nvarin, gmvrayin);    

         if (gmv_data.keyword == GMVERROR) return;

         gmvrays[iray].field[k] = field;
        }
      if (ftype != ASCII || ftype == IEEEI4R4 || ftype == IEEEI8R4)
         free(tmpfloat);
     }
   gmvray_data.gmvrays = gmvrays;

   if ((feof(gmvrayin) != 0) | (ferror(gmvrayin) != 0))
     {
      fprintf(stderr,"I/O error while reading rays.\n");
      gmv_data.errormsg = (char *)malloc(30 * sizeof(char));
      snprintf(gmv_data.errormsg,30,"I/O error while reading rays.");
      gmvray_data.nvars = -1;
      return;
     }
}


void readrayids(FILE* gmvrayin, int ftype)
{
  /*                                              */
  /*  Read and set alternate node numbers (ids).  */
  /*                                              */
  int i, *lrayids = NULL;
  long *tmpids;

   /*  Allocate ray ids.  */
   FREE(gmvray_data.rayids);
   lrayids=(int *)malloc(numrays*sizeof(int));
   if (lrayids == NULL)
     {
      gmvrayrdmemerr();
      return;
     }

   /*  Read node ids.  */
   if (ftype != ASCII)
     {
      if (ftype == IEEEI8R4 || ftype == IEEEI8R8)
        {
         tmpids=(long *)malloc(numrays*sizeof(long));
         if (tmpids == NULL)
           {
            gmvrayrdmemerr();
            return;
           }
         binread(tmpids,longlongsize,LONGLONG,numrays,gmvrayin);
         for (i = 0; i < numrays; i++)
            lrayids[i] = tmpids[i];
         free(tmpids);
        }
      else
        {
         binread(lrayids,intsize,INT,numnodes,gmvrayin);
        }
      if (ioerrtst2(gmvrayin)) return;
     }
   if (ftype == ASCII) rdints(lrayids,numrays,gmvrayin);

   gmvray_data.rayids = lrayids;
}


void gmvrayrdmemerr()
{
  /*                 */
  /*  Memory error.  */
  /*                 */

   fprintf(stderr,"Not enough memory to read gmv ray data.\n");
   gmv_data.errormsg = (char *)malloc(40 * sizeof(char));
   snprintf(gmv_data.errormsg,40,"Not enough memory to read gmv ray data.");
   gmvray_data.nvars = -1;
}


int chk_rayend(FILE *fin)
{
  /*                                             */
  /*  Check for rayend in a GMV ray input file.  */
  /*                                             */
  int i, chkend;
  long int currpos;
  char rdend[21];
  size_t res;

   /*  Get the current file position.  */
   /* currpos = ftell(fin); */
   currpos = 8;

   /*  Read the last 20 characters of the file.  */
   fseek(fin, -20L, 2);
   res = fread(rdend,sizeof(char), 20, fin); (void) res;

   /*  Check the 20 characters for endray.  */
   chkend = 0;
   for (i = 0; i < 15; i++)
      if (strncmp((rdend+i),"endray",6) == 0) chkend = 1;

   /*  Reset file position.  */
   fseek(fin, currpos, 0);

   return chkend;
}
