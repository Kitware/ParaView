/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/addfld.C,v $
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
#include <gen_utils.h>

/* Each of the following strings MUST be defined */
//static char  usage_[128];

char *prog   = "addfld";
char *usage  = "addfld:  [options]  -s val  -m file1.fld file2[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
"-s #    ... the scaling value \n"
" output: file1 + val*file2\n";

/* ---------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f);
static Field *sortHeaders(Field *fld,int nfld);

main (int argc, char *argv[]){
  register  int i;
  Field     fld, fld1, *fout;
  FileList  f;
  FILE     *fp,*fp1;
  char      fname[BUFSIZ], tmpname[BUFSIZ], syscall[BUFSIZ];
  int      nfields, dlen;
  double    val;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  memset(&fld , '\0', sizeof (Field));
  memset(&fld1, '\0', sizeof (Field));

#ifdef DEBUG
  debug_out = stdout;
#endif

  val = dparam("Val");

  sprintf(fname,"%s",f.mesh.name);
  if((fp = fopen(fname,"r")) == (FILE *) NULL){
    sprintf(fname,"%s.fld",f.mesh.name);
    if((fp = fopen(fname,"r")) == (FILE *) NULL){
      fprintf(stderr,"%s: unable to open the input file -- "
        "%s or  %s.fld \n",prog,f.mesh.name,f.mesh.name);
      exit(1);
    }
  }
  readField  (fp, &fld);


  sprintf(fname,"%s",f.in.name);
  if((fp1 = fopen(fname,"r")) == (FILE *) NULL){
    sprintf(fname,"%s.fld",f.in.name);
    if((fp1 = fopen(fname,"r")) == (FILE *) NULL){
      fprintf(stderr,"%s: unable to open the input file -- "
        "%s or  %s.fld \n",prog,f.in.name,f.in.name);
      exit(1);
    }
  }
  readField (fp1, &fld1);

  dlen = data_len(fld1.nel,fld.size,fld.nfacet, fld.dim);
  nfields = (int) strlen (fld.type);

  for(i = 0; i < nfields; ++i)
    dsvtvp(dlen,val,fld1.data[i],1,fld.data[i],1,fld.data[i],1);

  writeHeader(f.out.fp,&fld);
  writeData(f.out.fp,&fld);

  return 0;
}


/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f){
  char  c;

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }

  iparam_set("Dump", 0);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 's':
  if (*++argv[0])
    dparam_set("Val", atoi(*argv));
  else {
    dparam_set("Val", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }
  f->in.name = strdup(*argv);
  /* open input file */

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}
