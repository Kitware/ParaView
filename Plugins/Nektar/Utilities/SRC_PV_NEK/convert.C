/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/convert.C,v $
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
#include "polylib.h"
#include <nektar.h>
#include <gen_utils.h>

/* Each of the following strings MUST be defined */

char *prog   = "convert";
char *usage  = "convert: -s|-a input[.fld]\n";
char *author = "C. Evangelinos";
char *rcsid  = "$Revision: 1.2 $";
char *help   = "This program will convert a fieldfile\n"
#if defined(_CRAY) && !defined(_CRAYMPP)
"-s:  to IEEE binary little-endian from CRAY binary (default)\n"
#else
"-s:  from IEEE binary little-endian to big-endian and vice versa (default)\n"
#endif
"-a:  to ASCII\n"
"It is assumed that the binary file and the machine the program runs on\n"
"are of the same endianness for conversions of binary fieldfiles.\n";
/* ---------------------------------------------------------------------- */

#if defined(_CRAY) && !defined(_CRAYMPP)
int CRAY2IEG(int *type, int *num, void *foreign, int *bitoff, void *cray);
#endif

#ifndef OTYPE
#if defined(i860) || defined (__alpha) || defined (__WIN32__) || (defined(linux) && defined(i386))
#define OTYPE "ieee_big_endian"
#endif
#
#if defined(_CRAY) && !defined (_CRAYMPP)
#define OTYPE "ieee_big_endian"
#endif /* ........... Cray Y-MP ........... */
#
#ifndef OTYPE
#define OTYPE "ieee_little_endian"
#endif /* default case in the absence of any other TYPE */
#endif /* ifndef TYPE */

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
  "%-5c HyCompress          " "State 'p' = physical, 't' transformed\n",
  "%-7d %-7d %-7d   " "Number of Elements; Dim of run; Lmax\n",
  "%-25d "            "Step\n",
  "%-25.6g "          "Time\n",
  "%-25.6g "          "Time step\n",
  "%-25.6g "          "Kinvis;\n",
  "%-25s "            "Fields Written\n",
  "%-25s "            "Format\n"
  };

extern "C"
{
#ifdef _CRAY
void Sbrev (int n, short *vect);
#else
void Ibrev (int n, int *vect);
#endif
void Dbrev (int n, double *vect);
}

static void parse_util_args (int argc, char *argv[], FileList *f);
static int  writeFieldConv (FILE *fp, Field *f);

main (int argc, char *argv[])
{
  int       dump=0,shuff;
  Field     fld;
  FileList  f;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld, '\0', sizeof (Field));

  readField(f.in.fp, &fld);

  writeFieldConv (f.out.fp, &fld);

  return 0;


}

static int writeFieldConv (FILE *fp, Field *f)
{
  int  nfields, ntot;
  char buf[BUFSIZ];
  register int i, n;
#ifdef _CRAY
  short *ssize, *spllinfo;
#endif
  char **fmt;

  if(option("COMPRESSWRITE"))
    fmt = hdr_fmt_comp;
  else
    fmt = hdr_fmt;

  /* Write the header */
  fprintf (fp, fmt[0],  f->name);
  fprintf (fp, fmt[1],  f->created);
  fprintf (fp, fmt[2],  f->state);
  if(f->nz > 1)
    fprintf (fp, fmt[3],  f->nel, f->dim, f->lmax, f->nz);
  else
    fprintf (fp, fmt[3],  f->nel, f->dim, f->lmax);
  fprintf (fp, fmt[4],  f->step);
  fprintf (fp, fmt[5],  f->time);
  fprintf (fp, fmt[6],  f->time_step);
  if(f->nz > 1)
    fprintf (fp, fmt[7],  f->kinvis, f->lz);
  else
    fprintf (fp, fmt[7],  f->kinvis);
  fprintf (fp, fmt[8],  f->type);
  fprintf (fp, fmt[9],  (option("binary")) ? "binary-"OTYPE : "ascii");
  f->format = strdup((option("binary")) ? "binary-"OTYPE : "ascii");

  /* Write the field files  */
  if(f->state == 't'){
    if(f->nz > 1)
      ntot = f->nz*data_len(f->nel,f->size,f->nfacet,f->dim);
    else
      ntot = data_len(f->nel,f->size,f->nfacet,f->dim);
  }
  else{
    fprintf(stderr,"Not implemented\n");
    exit(-1);
  }

  nfields = (int) strlen (f->type);

  int cnt = isum(f->nel,f->nfacet,1);

  switch (tolower(*f->format)) {
  case 'a':
    for(i = 0; i < f->nel; ++i)
      fprintf(fp,"%d ",f->nfacet[i]);
    fputc('\n',fp);

    for(i = 0; i < cnt; ++i)
      fprintf(fp,"%d ",f->size[i]);
    fputc('\n',fp);

#ifdef PARALLEL
    for(i = 0; i < pllinfo.nloop; ++i)
      fprintf(fp,"%d ",pllinfo.eloop[i]);
    fputc ('\n', fp);
#endif

    for (i = 0; i < ntot; i++) {
      for (n = 0; n < nfields; n++)
  fprintf (fp, "%#16.10g ", f->data[n][i]);
      fputc ('\n', fp);
    }


    break;

  case 'b':
#ifdef _CRAY
    // int on Crays is 64b, short 32b - use short for portable fieldfiles
    ssize = (short *) calloc(f->nel, sizeof(short));
    for (n = 0; n < f->nel; n++)
      ssize[n] = (short) f->nfacet[n];
#ifdef _CRAYMPP // Do this only on CRAY MPPs
    Sbrev(f->nel,ssize);
#endif
    fwrite(ssize, sizeof(short),  f->nel, fp);
    free (ssize);

    ssize = (short *) calloc(cnt, sizeof(short));
    for (n = 0; n < cnt; n++)
      ssize[n] = (short) f->size[n];
#ifdef _CRAYMPP // Do this only on CRAY MPPs
    Sbrev(cnt,ssize);
#endif
    fwrite(ssize, sizeof(short),  cnt, fp);
    free (ssize);
#else
    Ibrev(f->nel,f->nfacet);
    fwrite(f->nfacet, sizeof(int),  f->nel, fp);
    Ibrev(cnt,f->size);
    fwrite(f->size, sizeof(int),  cnt, fp);
#endif

#ifdef PARALLEL
#ifdef _CRAY
    // int on Crays is 64b, short 32b - use short for portable fieldfiles
    spllinfo = (short *) calloc(pllinfo.nloop, sizeof(short));
    for (n = 0; n < pllinfo.nloop; n++)
      spllinfo[n] = (short) pllinfo.eloop[n];
#ifdef _CRAYMPP // Do this only on CRAY MPPs
    Sbrev(pllinfo.nloop,spllinfo);
#endif
    fwrite(spllinfo, sizeof(short), pllinfo.nloop, fp);
    free (spllinfo);
#else
    Ibrev(pllinfo.nloop,pllinfo.eloop);
    fwrite(pllinfo.eloop, sizeof(int),  pllinfo.nloop, fp);
#endif
#endif

#if defined(_CRAY) && !defined(_CRAYMPP)
    double *convfdata = (void *) calloc(ntot, sizeof(double));
    int convzero = 0, ierr;
    for (n = 0; n < nfields; n++) {
      // Convert to IEEE big-endian
      ierr = CRAY2IEG(&convtype, &ntot, convfdata, &convzero,
          (void *) f->data[n]);
      if (fwrite (convfdata, sizeof(double), ntot, fp) != ntot) {
  fprintf  (stderr, "error writing field %c", f->type[n]);
  exit(-1);
      }
    }
#else
    for (n = 0; n < nfields; n++) {
      Dbrev(ntot,f->data[n]);
      if (fwrite (f->data[n], sizeof(double), ntot, fp) != ntot) {
  fprintf  (stderr, "error writing field %c", f->type[n]);
  exit(-1);
      }
    }
#endif

    break;

  default:
    sprintf  (buf, "unknown format -- %s", f->format);
    fprintf(stderr,buf);
    exit(-1);
    break;
  }

  fflush (fp);
  return nfields;
}

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f)
{
  char  c;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }

  while (--argc && (*++argv)[0] == '-') {
    switch (c = *++argv[0]) {

    case 's':
      break;
    default:
      fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
      break;
    }
  }

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

/* Byte-reversal routines */

void Dbrev (int n, double *x)
{
  char  *cx;
  register int i;
  register char d1, d2, d3, d4;

  for (i = 0; i < n; i++, x++) {
    cx = (char*) x;
    d1    = cx[0];
    cx[0] = cx[7];
    cx[7] = d1;
    d2    = cx[1];
    cx[1] = cx[6];
    cx[6] = d2;
    d3    = cx[2];
    cx[2] = cx[5];
    cx[5] = d3;
    d4    = cx[3];
    cx[3] = cx[4];
    cx[4] = d4;
  }
  return;
}

#ifndef _CRAY
void Ibrev (int n, int *x)
{
  char  *cx;
  register int i;
  register char d1, d2;

  for (i = 0; i < n; i++, x++) {
    cx = (char*) x;
    d1    = cx[0];
    cx[0] = cx[3];
    cx[3] = d1;
    d2    = cx[1];
    cx[1] = cx[2];
    cx[2] = d2;
  }
  return;
}
#else
void Sbrev (int n, short *x)
{
  char  *cx;
  register int i;
  register char d1, d2;

  for (i = 0; i < n; i++, x++) {
    cx = (char*) x;
    d1    = cx[0];
    cx[0] = cx[3];
    cx[3] = d1;
    d2    = cx[1];
    cx[1] = cx[2];
    cx[2] = d2;
  }
  return;
}
#endif
