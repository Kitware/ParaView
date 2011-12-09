/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/rea2metis.C,v $
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
#include <hotel.h>
#include <gen_utils.h>

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "nekprepart";
char *usage  = "nekprepart:  [options]  input[.rea]\n";
char *author = "";
char *rcsid  = "";
char *help   = "";

/* ---------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f);
static void Connect(Element_List *rea, FILE *out);
Element_List *ReadMesh (FILE *fp, char* session_name);
main (int argc, char *argv[])
{
  FileList       f;
  Element_List *EL;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);
  option_set("FAMOFF",1);
  iparam_set("MODES", 3);

  EL = ReadMesh(f.rea.fp, "null");

  Connect(EL,f.out.fp);

  return;
}

static void Connect(Element_List *EL, FILE *out){
  Edge *ed;
  Element  *E;
  register int i;
  int      ncon = 0;

  if(EL->fhead->dim() == 3){
    ncon = 0;
    for(E = EL->fhead;E;E=E->next){
#if 0
      for(i = 0; i < E->Nedges; ++i)
  for(ed = E->edge[i].base;ed;ed=ed->link)
    ncon += (ed->eid != E->id) ? 1:0;
#endif

      for(i = 0; i < E->Nfaces; ++i)
  if(E->face[i].link && E->face[i].link->eid != E->id)
    ++ncon; // += (E->face[i].link) ? 1:0;
    }

    fprintf (out, "%d %d\n", EL->nel, ncon/2);

    for(E = EL->fhead;E;E=E->next){
#if 0
      for(i = 0; i < E->Nedges; ++i)
  for(ed = E->edge[i].base;ed;ed=ed->link)
    if(ed->eid != E->id)
      fprintf(out, "%d ", ed->eid + 1);
#endif
      for(i = 0; i < E->Nfaces; ++i)
  if(E->face[i].link)
    if(E->face[i].link->eid != E->id)
      fprintf(out, "%d ", E->face[i].link->eid + 1);
      fprintf (out, "\n");
    }
  }
  else{
    ncon = 0;
    for(E = EL->fhead;E;E=E->next)
      for(i = 0; i < E->Nedges; ++i)
  ncon += (E->edge[i].base) ? 1:0;

    fprintf (out, "%d %d\n", EL->nel, ncon/2);

    for(E = EL->fhead;E;E=E->next){
      for(i = 0; i < E->Nedges; ++i){
  if(E->edge[i].link)
    fprintf(out, "%d ", E->edge[i].link->eid + 1);
  else if(E->edge[i].base)
    fprintf(out, "%d ", E->edge[i].base->eid + 1);
      }
      fprintf (out, "\n");
    }
  }
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

  option_set("SingMesh",NULL);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      default:
        fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
        break;
      }
  }

  /* open the .rea file */

  if ((*argv)[0] == '-') {
    f->rea.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.rea", *argv);
      if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
        fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
                prog, *argv, fname);
        exit(1);
      }
    }
    f->rea.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
             f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
             f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}
