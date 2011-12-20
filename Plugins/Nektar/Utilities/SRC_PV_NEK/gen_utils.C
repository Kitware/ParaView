/*
 * Generic utility functions
 */

#include <nekscal.h>
#include <gen_utils.h>
#include <polylib.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
#include <memcheck.h>
#include <dbutils.h>
#endif

using namespace polylib;

/* Externals */

static char *genargs =
  "options:\n"
  "-h      ... print this help message\n"
  "-var    ... run with variable order (uses [].ord file)\n"
  "-v      ... be verbose about things\n";

/* ---------------------------------------------------------------------- *
 * generic_args() -- Generic command line arguments for all utilities     *
 *                                                                        *
 * This function processes the command line arguments common to all of    *
 * the utility programs.  Any arguments not processed by this function    *
 * are returned in an updated argv[].  It NEVER processes the last arg-   *
 * ument since that's reserved for the input file name.                   *
 *                                                                        *
 * The generic options are the following:                                 *
 *                                                                        *
 *     -h      ...  print a help message                                  *
 *     -v      ...  be verbose about things                               *
 *     -n #    ...  specify an N-order to interpolate to                  *
 *     -o file ...  send output to the named file                         *
 *     -m file ...  read the mesh from the named file                     *
 *     -r file ...  read the session (.rea) from the named file           *
 *                                                                        *
 * Your application doesn't have to support all of these options, but it  *
 * should list the ones it does in the external character array "help",   *
 * except for the -h and -v options (they're automatic).  If you're ap-   *
 * plication DOES support any of these features, it MUST use these argu-  *
 * ments.  Likewise, you shouldn't use these symbols for anything else    *
 * since that's makes things more confusing for everybody.                *
 *                                                                        *
 * Return value: number of arguments passed through to the application    *
 * ---------------------------------------------------------------------- */

int generic_args (int argc, char *argv[], FileList *f)
{
  int   appargc = 0;
  char *appargv [MAXARGS], **orig;
  char  c, fname[FILENAME_MAX];
  register int i;

#ifdef DEBUG
  init_debug();
/*  mallopt(M_DEBUG,1);*/
#endif

  /* Set up the default values for each file pointer */

  memset (f, '\0', sizeof(FileList));

  f -> in.fp    =  stdin;
  f -> out.fp   =  stdout;
  f -> mesh.fp  =  stdin;

  orig = argv;          /* save the orignal argument vector    */
  manager_init();       /* initialize the symbol table manager */

  /* Mark the following parameters with the UNSET flag. Otherwise, *
   * the parser will generate an undefined variable error.         */

  iparam_set("NORDER.REQ", UNSET);

  /* copy application name into save list */
  strcpy (appargv[appargc++] = (char *) malloc (strlen(*argv)+1), *argv);

  if(argc == 1) goto showUsage;

  while (--argc && appargc < MAXARGS)       /* start parsing options ... */
    if ((*++argv)[0] == '-' && strlen(*argv) > 1) {
      if (strcmp (*argv+1, "var") == 0) {
  option_set("variable",1);
  continue;
      }
      else if (strcmp (*argv+1, "old") == 0) {
  option_set("oldhybrid",1);
  continue;
      }
      switch (c = *++argv[0]) {
      case 'h':                               /* print the help string */
      showUsage:
  fputs(usage,   stderr);
  fputs(genargs, stderr);
  fputs(help,    stderr);
  exit (-1);
  break;
      case 'n':                        /* Queue an NR|NS-interpolation */
  if (*++argv[0])
    iparam_set("NORDER.REQ", atoi(*argv));
  else {
    iparam_set("NORDER.REQ", atoi(*++argv));
    argc--;
  }
  break;
      case 'm':                           /* read the mesh from a file */
  if (*++argv[0])
    strcpy(fname, *argv);
  else {
    strcpy(fname, *++argv);
    argc--;
  }
  if (!(f->mesh.fp = fopen(fname,"r")))
    if (!(f->mesh.fp = fopen(strcat (fname, ".mesh"), "r"))) {
      fprintf(stderr, "%s: unable to open the mesh file -- %s or %s\n",
        prog, *argv, fname);
      exit(1);
    }
  f->mesh.name = strdup(fname);
  break;

      case 'r':                           /* read the session file */
  if (*++argv[0])
    strcpy(fname, *argv);
  else {
    strcpy(fname, *++argv);
    argc--;
  }
  if (!(f->rea.fp = fopen(fname,"r")))
    if (!(f->rea.fp = fopen(strcat(fname,".rea"),"r"))) {
      fprintf(stderr,"%s: unable to open the session file -- %s or %s\n",
        prog, *argv, fname);
      exit(1);
    }
  f->rea.name = strdup(fname);

  /* Try to open the connectivity file.  This file is usually *
   * optional, so the application is responsible for error    *
   * checking.                                                */

  strcpy(fname + strlen(fname)-3, "mor");
  f->mor.fp   = fopen (fname,"r");
  f->mor.name = strdup(fname);
  break;

      case 'o':                         /* direct the output to a file */
  if (*++argv[0])
    strcpy(fname, *argv);
  else {
    strcpy(fname, *++argv);
    argc--;
  }
  if ((f->out.fp = fopen(fname,"w")) == (FILE*) NULL) {
    fprintf(stderr, "%s: unable to open the output file -- %s\n",
      prog, fname);
    exit(1);
  }
  f->out.name = strdup(fname);
  break;

      case 'v': {               /* be verbose and echo the version */
  float v;
  sscanf (rcsid, "%*s%f", &v);
  fprintf(stderr, "%s v%g -- by %s\n", prog, v, author);
  option_set("verbose", 1);
  break;
      }

      default:                  /* transfer it to the un-processed list */
  sprintf
    ( appargv[appargc++] = (char *) malloc(strlen(*argv)+2),
      "-%s", *argv );
  break;
      }

      /* Current argv[] is not an option, so just copy it to the *
       * application list to be processed later.                 */

    } else {
      strcpy (appargv[appargc++] = (char *) malloc (strlen(*argv)+1), *argv);
    }

  /* Now assign the application's arguments */

  while ((i = argc++) < appargc) orig[i] = appargv[i];

  return appargc;
}


/* interp face 1 to equispaced at QGmax */
void InterpToEqui(int nfs, int qa, int qb, double *in, double *out){
  int i;
  double *f1 = dvector(0,QGmax*QGmax-1);
  static double **im1,**im2;
  static int qa_store, qb_store;


  if(!((qa == qa_store)&&(qb == qb_store))){
    double *z1,*w;
    double *z2 = dvector(0,QGmax-1);

    if(im1){
      free_dmatrix(im1,0,0);
      free_dmatrix(im2,0,0);
    }

    im1 = dmatrix(0,QGmax-1,0,qa-1);
    im2 = dmatrix(0,QGmax-1,0,qb-1);

    for(i = 0; i < QGmax; ++i)
      z2[i] = 2.0*i/(QGmax-1.0) - 1.0;

    getzw(qa,&z1,&w,'a');
    igllm(im1,z1,z2,qa,QGmax);

    if(nfs == 3){
      getzw(qb,&z1,&w,'b');
      igrjm(im2,z1,z2,qb,QGmax,1.0,0.0);
    }
    else{
      getzw(qb,&z1,&w,'a');
      igllm(im2,z1,z2,qb,QGmax);
    }

    free(z2);

    qa_store = qa;
    qb_store = qb;
  }

  /* interpolate qa to QGmax */
  for(i = 0; i < qb; ++i)
    dgemv('T',qa,QGmax,1.0,*im1,qa,in+i*qa,1,0.0,f1+i*QGmax,1);

  /* interpolate qb to QGmax */
  for(i = 0; i < QGmax; ++i)
    dgemv('T',qb,QGmax,1.0,*im2,qb,f1+i,QGmax,0.0,out+i,QGmax);

  free(f1);
}
