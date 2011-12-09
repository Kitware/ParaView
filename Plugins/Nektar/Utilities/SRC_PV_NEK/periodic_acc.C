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

char *prog   = "periodic_acc";
char *usage  = "periodic_acc:  [options]  -p nfiles  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
"-p #    ... the number of files in periodic time sequence (multiple of 2)  \n";

/* ---------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f);
static Field *sortHeaders(Field *fld,int nfld);

main (int argc, char *argv[]){
  int       i,j,k;
  int       nfiles,shuffled,chk = 0;
  Field     *flds, *fout;
  FileList  f;
  FILE     *fp;
  char      fname[BUFSIZ], tmpname[BUFSIZ], syscall[BUFSIZ];
  double    tmp;


  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

#ifdef DEBUG
  debug_out = stdout;
#endif

  nfiles = option("Nfiles");
  if(!nfiles||(nfiles%2)){
    fprintf(stderr,"Must specify number of files which is a multiple of 2\n");
    exit(1);
  }

  flds = (Field *)calloc(nfiles,sizeof(Field));

  /* determine file ending (i.e. .chk or .fld ) */
  /* determine input files name by checking it's existance */

  sprintf(fname,"%s",strtok(f.in.name,"."));
  sprintf(tmpname,"%s_0.fld",fname);

  if((fp = fopen(tmpname,"r")) == (FILE *) NULL){
    sprintf(tmpname,"%s_0.chk",fname);
    if((fp = fopen(tmpname,"r")) == (FILE *) NULL){
      fprintf(stderr,"%s: unable to open the input file -- "
        "%s_0.fld or  %s_0.chk \n",prog,fname,fname);
      exit(1);
    }
    else
      chk = 1;
  }
  else
    chk = 0;

  fclose(fp);

  /* read in all field files */
  for(i = 0; i < nfiles; ++i){
    /* open input files */
    if(chk)
      sprintf(tmpname,"%s_%d.chk",fname,i);
    else
      sprintf(tmpname,"%s_%d.fld",fname,i);

    if((fp = fopen(tmpname,"r")) == (FILE *) NULL)
      fprintf(stderr,"unable to open the input file -- %s",tmpname);
    else
      readField(fp, flds+i);
  }

  // Do Fourier Transform
  int     ndata = data_len(flds->nel,flds->size,flds->nfacet,flds->dim);
  double *data = dvector(0,nfiles-1);
  int nfields = (int) strlen(flds->type);


  for(k = 0; k < nfields; ++k){
    for(i = 0; i < ndata; ++i){
      for(j = 0;  j < nfiles; ++j)
  data[j] = flds[j].data[k][i];


      // take Fourier derivative;
      //transform to frequency space
      realft(nfiles/2, data, -1);

      for(j = 0; j < nfiles; j+=2){
  tmp = data[j+1];
  data[j+1] = (double)  (j/2)*data[j];
  data[j]   = (double) -(j/2)*tmp;
      }

      // transform back to physical space
      realft(nfiles/2, data, 1);

      // copy back data
      for(j = 0; j < nfiles; ++j){
  flds[j].data[k][i] = data[j];
      }
    }
  }


  /* Write out files */
  /* read in all field files */
  for(i = 0; i < nfiles; ++i){
    /* open input files */
    if(chk)
      sprintf(tmpname,"%s_acc_%d.chk",fname,i);
    else
      sprintf(tmpname,"%s_acc_%d.fld",fname,i);

    if((fp = fopen(tmpname,"w")) == (FILE *) NULL)
      fprintf(stderr,"unable to open the input file -- %s",tmpname);
    else {
      writeHeader(fp, flds+i);
      writeData  (fp, flds+i);
    }
  }

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
      case 'p':
  if (*++argv[0])
    option_set("Nfiles", atoi(*argv));
  else {
    option_set("Nfiles", atoi(*++argv));
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
