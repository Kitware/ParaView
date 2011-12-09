/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/p2sfld.C,v $
 * $Revision: 1.4 $
 * $Date: 2005/11/09 09:23:04 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <veclib.h>
#include <nektar.h>
#include <gen_utils.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
//#include <veclib.h>
//#include <nektar.h>
//#include <gen_utils.h>

/* Each of the following strings MUST be defined */
//static char  usage_[128];

char *prog   = "p2sfld";
char *usage  = "p2sfld:  [options]  -p Nfields  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
"-t      ... assume headers are defined in a .tot.hdr file"
"-p #    ... the number of seperate files from parallel output \n"
"-c      ... concatinated field\n"
"-d #    ... read dump # (1 - no. dumps) from [.fld] file.\n";

/* ---------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f);
static Field *sortHeaders(Field *fld,int nfld);

main (int argc, char *argv[]){
  register  int i;
  int       nfiles,shuffled;
  Field     *hfld, *fout;
  FileList  f;
  FILE     *fp;
  char      fname[BUFSIZ], tmpname[BUFSIZ];//, syscall[BUFSIZ];

  char* syscall = new char[ BUFSIZ * 10 ];

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

#ifdef DEBUG
  debug_out = stdout;
#endif

  nfiles = option("Nfiles");
  if(!nfiles){
    fprintf(stderr,"Must specify number of files with -p option\n");
    exit(1);
  }

  hfld = (Field *)calloc(nfiles,sizeof(Field));

  if(option("CONCATENATEDFILE")){
    /* open input file */

      sprintf(fname,"%s",f.in.name);
      sprintf(tmpname,"%s.tot.hdr",fname);
      if((fp = fopen(tmpname,"r")) == (FILE *) NULL){
  fprintf(stderr,"unable to open the input file -- %s",tmpname);
  // exit(1);
      }

      else{
        nfiles=0;
  while(readHeader (fp, hfld+nfiles, &shuffled))
    nfiles++;
  fclose(fp);
      }
  }

  else{


    /* determine file ending (i.e. .chk or .fld ) */
    /* determine input files name by checking it's existance */

    sprintf(fname,"%s",f.in.name);
    sprintf(tmpname,"%s.hdr.0",fname);

    if((fp = fopen(tmpname,"r")) == (FILE *) NULL){
      sprintf(fname,"%s.fld",f.in.name);
      sprintf(tmpname,"%s.hdr.0",fname);
      if((fp = fopen(tmpname,"r")) == (FILE *) NULL){
        fprintf(stderr,"%s: unable to open the input file -- "
        "%s.hdr.0 or  %s.fld.hdr.0 \n",prog,f.in.name,f.in.name);
      //      exit(1);
      }
    }

    fclose(fp);


    /* read in all field Header files */
    for(i = 0; i < nfiles; ++i){
      /* open input files */
      sprintf(tmpname,"%s.hdr.%d",fname,i);
      if((fp = fopen(tmpname,"r")) == (FILE *) NULL)
  fprintf(stderr,"unable to open the input file -- %s",tmpname);
      //exit(1);
      else{
  readHeader (fp, hfld+i, &shuffled);
        //printf(" readHeader file No. %d - done \n",i);
      fclose(fp);
      }

    }
  }

  fout = sortHeaders(hfld,nfiles);
  pllinfo.nprocs = 2;
  pllinfo.nloop  = fout->nel;
  pllinfo.eloop  = fout->emap;
  writeHeader(f.out.fp,fout);
  //printf("write Header file done \n");

  pllinfo.nprocs = 1;

  for(i = 0; i < nfiles; ++i)
    freeField(hfld+i);

  /* make a system call to cat the dat files onto the output file */
  if(f.out.name){ /* write info to file */
    if(option("CONCATENATEDFILE")){
      sprintf(syscall,"cat %s %s.tot.dat > %s.tmp",f.out.name,fname,f.out.name);
    }
    else{
      sprintf(syscall,"cat %s %s.dat.0 ",f.out.name,fname);

      for(i = 1; i < nfiles; ++i){
  if((hfld+i != ((Field* ) NULL)))
    sprintf(syscall,"%s %s.dat.%d ",syscall,fname, i);
      }

      sprintf(syscall,"%s > %s.tmp",syscall,f.out.name);
    }
    fprintf(stdout,"system call: %s\n",syscall);
    system(syscall);

    sprintf(syscall,"mv %s.tmp %s",f.out.name,f.out.name);
    fprintf(stdout,"system call: %s\n",syscall);
    system(syscall);
  }
  else{ /*  cat to stdout */
    if(option("CONCATENATEDFILE")){
      sprintf(syscall,"cat %s.tot.dat",fname);
    }
    else{
      sprintf(syscall,"cat %s.dat.0 ",fname);
      for(i = 1; i < nfiles; ++i){
  sprintf(syscall,"%s %s.dat.%d ",syscall,fname,i);
      }
    }
    system(syscall);
  }
  delete[] syscall;
  return 0;
}

static Field *sortHeaders(Field *fld, int nfld){
  register int i;
  Field *fout;
  int cnt,*size,*emap,*nfacet;

  fout = (Field *)calloc(1,sizeof(Field));

  memcpy(fout,fld,sizeof(Field));

  cnt   = 0;
  for(i = 0; i < nfld; ++i)
    if((fld+i != ((Field* ) NULL)))
      // if(fld[i])
      cnt   += fld[i].nel;

  fout->nel    = cnt;

  emap   = fout->emap   = ivector(0,fout->nel-1);
  nfacet = fout->nfacet = ivector(0,fout->nel-1);
  for(i = 0,cnt = 0; i < nfld; ++i){
    if((fld+i != ((Field* ) NULL))){
      icopy(fld[i].nel,fld[i].nfacet,1,nfacet,1);
      icopy(fld[i].nel,fld[i].emap,  1,emap,1);
      emap   += fld[i].nel;
      nfacet += fld[i].nel;
      cnt    += isum(fld[i].nel,fld[i].nfacet,1);
    }
  }

  size = fout->size = ivector(0,cnt-1);
  for(i = 0,cnt = 0; i < nfld; ++i){
    if((fld+i != ((Field* ) NULL))){
      cnt = isum(fld[i].nel,fld[i].nfacet,1);
      icopy(cnt,fld[i].size,1,size,1);
      size += cnt;
    }
  }

  for(i = 0,cnt = 0; i < nfld; ++i){
    if((fld+i != ((Field* ) NULL)))
      cnt += fld[i].nel;
  }

  return fout;
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

      case 'c':
  option_set("CONCATENATEDFILE",1);
  break;

      case 'd':
  if (*++argv[0])
    iparam_set("Dump", atoi(*argv));
  else {
    iparam_set("Dump", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'p':
  if (*++argv[0])
    option_set("Nfiles", atoi(*argv));
  else {
    option_set("Nfiles", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 't':
  option_set("Totheader",1);
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
