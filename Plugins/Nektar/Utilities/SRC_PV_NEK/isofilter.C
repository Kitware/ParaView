/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/isofilter.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/07/09 08:16:34 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#include <iostream>
#include <istream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

#include <veclib.h>
#include <nektar.h>
#include <gen_utils.h>

#include "isoutils.h"

using namespace std;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "isofilter";
char *usage  = "isofilter:  [options  -r file[.rea]]  input[.iso]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-i #    ... Number of smoothing iterations [Default = 100]\n"
  "-f #    ... Smoothing factor [Default = 0.6]\n"
  "-e #    ... Seocnd Smoothing factor [Default = -f value]\n"
  "-g      ... Gloablly condense requires -r field \n";

static void parse_util_args (int argc, char *argv[], FileList *f);
static void setup (FileList *f, Element_List* &U);
static void read(std::ifstream &in, vector<Iso*> &zones, double &val);

main (int argc, char *argv[]){
  vector<Iso *> zones;
  Iso G;
  double  val;
  FileList  f;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  std::ifstream infile(f.in.name);

  read(infile, zones, val);

  if(option("GlobalCondense")){
    Element_List *master;

    if(!f.rea.name){
      cerr << "must specify -r file[.rea] to run -g option \n";
      exit(1);
    }

    setup (&f, master);

    G.globalcondense(zones.size(),&(zones[0]),master);
    G.separate_regions();

    // write zones;
    if(f.out.name){
      std::ofstream out("f.out.name");

      out << "TITLE = \"Isocontour Level: " << val <<"\"\n";
      cout << "VARIABLES = \"x\" \"y\" \"z\" \"val\"" <<endl;
      G.write(out,val,iparam("SmoothMinCells"));
    }
    else{
      cout << "TITLE = \"Isocontour Level: " << val <<"\"\n";
      cout << "VARIABLES = \"x\" \"y\" \"z\" \"val\"" <<endl;
      G.write(cout,val,iparam("SmoothMinCells"));
    }
  }
  else{

      for(int i = 0; i <  zones.size(); ++i)
    zones[i]->smooth(iparam("SmoothIter"),dparam("SmoothFac"),
         -dparam("SmoothFac1"));

    // write zones;
    if(f.out.name){
      std::ofstream out;("f.out.name");
      //std::ostream out(outfile);
      out << "TITLE = \"Isocontour Level: " << val <<"\"\n";
      cout << "VARIABLES = \"x\" \"y\" \"z\" \"val\"" <<endl;
      for(int i = 0; i < zones.size(); ++i)
  zones[i]->write(out,val,iparam("SmoothMinCells"));
    }
    else{
      cout << "TITLE = \"Isocontour Level: " << val <<"\"\n";
      cout << "VARIABLES = \"x\" \"y\" \"z\" \"val\"" <<endl;
      for(int i = 0; i < zones.size(); ++i)
  zones[i]->write(cout,val,iparam("SmoothMinCells"));
    }
  }


  return 0;
}

static void read(std::ifstream &in, vector<Iso*> &zones, double &val){
  Iso *zone;
  char buf[256];

  in.getline(buf,256);
  in.getline(buf,256);

  while(!in.eof()){
    in.getline(buf,256);
    if(in.eof()) break;
    if(strstr(buf,"ZONE")){
      int eid;
      char *p;
      p = strstr(buf,"Elmt=");
      sscanf(p,"Elmt=%d",&eid);
      zone = new Iso [1];
      zone->readzone(in,val);
      zone->set_eid(eid);
      zones.push_back(zone);
    }
  }
}

Gmap *gmap;

void setup (FileList *f, Element_List* &U){
    double val;
    extern Element_List *Mesh;

    ReadParams  (f->rea.fp);

    if(val = dparam("SQ_PNT_TOL"))
    {
  set_SQ_PNT_TOL(val);
    }


  /* Generate the list of elements */
  Mesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  gmap = GlobalNumScheme(Mesh, (Bndry *)NULL);
  U= LocalMesh(Mesh,strtok(f->rea.name,"."));
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

  iparam_set("Nout", UNSET);
  dparam_set("SmoothFac",0.6);
  iparam_set("SmoothIter", 100);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'i':
  if (*++argv[0])
    iparam_set("SmoothIter", atoi(*argv));
  else {
    iparam_set("SmoothIter", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'f':
  if (*++argv[0])
    dparam_set("SmoothFac", atof(*argv));
  else {
    dparam_set("SmoothFac", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;

      case 'e':
  if (*++argv[0])
    dparam_set("SmoothFac1", atof(*argv));
  else {
    dparam_set("SmoothFac1", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;

      case 'c':
  if (*++argv[0])
    iparam_set("SmoothMinCells", atoi(*argv));
  else {
    iparam_set("SmoothMinCells", atoi(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'g':
  option_set("GlobalCondense",1);
  break;
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }

  if(!dparam("SmoothFac1"))
      dparam_set("SmoothFac1",dparam("SmoothFac"));


  /* open input file */

  if ((*argv)[0] == '-') {
    f->in.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.iso", *argv);
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
