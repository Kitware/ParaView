/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/old2newfld.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include <nektar.h>
#include "Quad.h"
#include "Tri.h"
#include <gen_utils.h>

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "old2newfld";
char *usage  = "old2newfld:  [options] -r input.rea  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

static void setup (FileList *f, Element_List **U);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Write(Element_List **E, FILE *out, int nfields, Field *fld);
static int readField_old (FILE *fp, Field *f, Element *E);

main (int argc, char *argv[]){
  int       dump=0,nfields;
  Field     fld;
  FileList  f;
  Element_List *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld, '\0', sizeof (Field));

  master = (Element_List *) malloc(sizeof(Element_List *));
  setup (&f, &master);

  set_nfacet_list(master);
  dump = readField(f.in.fp, &fld);
  writeHeader(f.out.fp,&fld);
  writeData(f.out.fp,&fld);
  return 0;
}

Element_List *GMesh;
Gmap *gmap;

static void setup (FileList *f, Element_List **U)
{
  int i,k;
  Curve *curve;

  ReadParams  (f->rea.fp);

  /* Generate the list of elements */
  GMesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  gmap  = GlobalNumScheme(GMesh, (Bndry *)NULL);
  U[0]  = LocalMesh(GMesh,strtok(f->rea.name,"."));

  return;
}

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f)
{
  char  c;
  int   i;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }
  iparam_set("Porder",0);
  dparam_set("theta",0.3);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'b':
  option_set("Body",1);
  break;
      case 'f':
  option_set("FEstorage",1);
  break;
      case 'R':
  option_set("Range",1);
  break;
      case 'q':
  option_set("Qpts",1);
  break;
      case 't':
  if (*++argv[0])
    dparam_set("theta", atof(*argv));
  else {
    dparam_set("theta", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }
#if DIM == 2
  if(iparam("NORDER-req") == UNSET) iparam_set("NORDER-req",15);
#endif
  /* open input file */

  if ((*argv)[0] == '-') {
    f->in.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.fld", *argv);
      if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
  fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
    prog, *argv, fname);
    exit(1);
      }
    }
    f->in.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}


#ifndef BTYPE
#if defined(i860) || defined (__alpha) || defined (__WIN32__)
#define BTYPE "ieee_little_endian"
#endif
#
#if defined(_CRAY) && !defined (_CRAYMPP)
#define BTYPE "cray"
#endif /* ........... Cray Y-MP ........... */
#
#ifndef BTYPE
#define BTYPE "ieee_big_endian"
#endif /* default case in the absence of any other TYPE */
#endif /* ifndef TYPE */

#define DESCRIP 25
static char *hdr_fmt[] = {
  "%-25s "            "Session\n",
  "%-25s "            "Created\n",
  "%-5c (Hybrid)            " "State 'p' = physical, 't' transformed\n",
  "%-7d %-7d %-7d  "  "Number of Elements; Dim of run; Lmax\n",
  "%-25d "            "Step\n",
  "%-25.6g "          "Time\n",
  "%-25.6g "          "Time step\n",
  "%-25.6g "          "Kinvis;\n",
  "%-25s "            "Fields Written\n",
  "%-25s "            "Format\n"
  };

static char *hdr_fmt_comp[] = {
  "%-25s "            "Session\n",
  "%-25s "            "Created\n",
  "%-5c HyCompress              " "State 'p' = physical, 't' transformed\n",
  "%-7d %-7d %-7d  "  "Number of Elements; Dim of run; Lmax\n",
  "%-25d "            "Step\n",
  "%-25.6g "          "Time\n",
  "%-25.6g "          "Time step\n",
  "%-25.6g "          "Kinvis;\n",
  "%-25s "            "Fields Written\n",
  "%-25s "            "Format\n"
  };


#define BINARY   strings[0]     /* 0. Binary format file            */
#define ASCII    strings[1]     /* 1. ASCII format file             */
#define KINVIS   strings[2]     /* 2. Kinematic viscosity           */
#define DELT     strings[3]     /* 3. Time step                     */
#define CHECKPT  strings[4]        /* 4. Check-pointing option         */
#define BINFMT   strings[5]        /* 5. Binary format string          */

static char *strings [] = {
  "binary", "ascii", "KINVIS", "DELT", "checkpt",
  "binary-"BTYPE
};

static int checkfmt (char *arch)
{
  char        **p;
  static char *fmtlist[] = {
#if defined(ieeeb)                  /* ... IEEE big endian machine ..... */
    "ieee_big_endian",
    "sgi", "iris4d", "SGI", "IRIX", /* kept for compatibility purposes   */
    "IRIX64",                       /* ........ Silicon Graphics ....... */
    "AIX",                          /* .......... IBM RS/6000 .......... */
    "cm5",                          /* ........ Connection Machine ..... */
#endif
#
#if defined(ieeel)                  /* ... IEEE little endian machine .. */
    "ieee_little_endian",
    "i860",                         /* ........... Intel i860 .......... */
#endif
#
#if defined(ieee)                   /* ...... Generic IEEE machine ..... */
    "ieee", "sim860",               /* kept for compatibility purposes   */
#endif                              /* same as IEEE big endian           */
#
#if defined(_CRAY) && !defined (_CRAYMPP) /* ...... Cray PVP ........... */
    "cray", "CRAY",                 /* kept for compatibility purposes   */
#endif                              /* no conflict with T3* as it        */
#                                   /* precedes this line                */
     0 };   /* a NULL string pointer to signal the end of the list */

  for (p = fmtlist; *p; p++)
    if (strncmp (arch, *p, strlen(*p)) == 0)
      return 0;

  return 1;
}
