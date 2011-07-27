/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include<iostream>
#include<sstream>
#include<vector>
using namespace std;
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <mpi.h>

#include"FsUtils.h"

// this script computes temerature and pressure on H3D datasets.
// it works in a task parallel way assigning timesteps to processes.
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  MPI_Init(&argc,&argv);

  int procId=0;
  int nProcs=1;
  MPI_Comm_rank(MPI_COMM_WORLD,&procId);
  MPI_Comm_size(MPI_COMM_WORLD,&nProcs);

  int pathLen=0;
  char *path=0;

  int nLocalIds=0;
  vector<int> localIds;

  // master process determines the available work and dishes it out 
  // staically to the others. Doing this to avoid all processes
  // scanning the directory at the same time.
  if (procId==0)
    {
    vector<int> ids;

    if (argc!=2
      || !GetSeriesIds(argv[1],"tpar_",ids))
      {
      cerr << "Error. Usage:" << endl
           << argv[0] << "/path/to/time/series" << endl
           << endl;
      return 1;
      }

    // send the path to the data files.
    int pathLen=strlen(argv[1]);
    if (argv[1][pathLen-1]=='/')
      {
      argv[1][pathLen-1]='\0';
      }
    MPI_Bcast(&pathLen,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(argv[1],pathLen+1,MPI_CHAR,0,MPI_COMM_WORLD); 
    path=argv[1];

    int nIds=ids.size();
    int nIdsPerProc=nIds/nProcs;
    int nIdsLeft=nIds%nProcs;

    // cerr << nIdsLeft << endl;

    int cnt[nProcs];
    int disp[nProcs];
    int at=0;
    for (int i=0; i<nProcs; ++i)
      {
      cnt[i]=nIdsPerProc+(i<nIdsLeft?1:0);
      disp[i]=at;
      at+=cnt[i];
      // cerr << cnt[i] << " " << disp[i] << endl;
      }

    // send the counts
    MPI_Scatter(cnt,1,MPI_INT,&nLocalIds,1,MPI_INT,0,MPI_COMM_WORLD);
    // send the ids
    localIds.resize(nLocalIds,0);
    MPI_Scatterv(&ids[0],cnt,disp,MPI_INT,&localIds[0],nLocalIds,MPI_INT,0,MPI_COMM_WORLD);
    }
  else
    {
    // recv the path to the data files
    MPI_Bcast(&pathLen,1,MPI_INT,0,MPI_COMM_WORLD);
    path=static_cast<char*>(malloc(pathLen+1));
    MPI_Bcast(path,pathLen+1,MPI_CHAR,0,MPI_COMM_WORLD);

    // recv the counts
    int notUsed;
    MPI_Scatter(&notUsed,1,MPI_INT,&nLocalIds,1,MPI_INT,0,MPI_COMM_WORLD);
    // recv the ids
    localIds.resize(nLocalIds,0);
    MPI_Scatterv(&notUsed,&notUsed,&notUsed,MPI_INT,&localIds[0],nLocalIds,MPI_INT,0,MPI_COMM_WORLD);
    }


  // cerr << procId << " " << nLocalIds << " -> ";
  // for (int i=0; i<nLocalIds; ++i)
  //   {
  //   cerr << localIds[i] << " ";
  //   }
  // cerr << endl;


  // do the work assigned to us.
  for (int id=0; id<nLocalIds; ++id)
    {
    // open the files
    ostringstream tparfn;
    tparfn << path << "/tpar_" << localIds[id] << ".gda";
    int tparfd=open(tparfn.str().c_str(),O_RDONLY);

    ostringstream tperpfn;
    tperpfn << path << "/tperp_" << localIds[id] << ".gda";
    int tperpfd=open(tperpfn.str().c_str(),O_RDONLY);

    ostringstream denfn;
    denfn << path << "/den_" << localIds[id] << ".gda";
    int denfd=open(denfn.str().c_str(),O_RDONLY);

    const int fmode=(S_IRWXU&~S_IXUSR)|S_IRGRP|S_IROTH;

    ostringstream tfn;
    tfn << path << "/t_" << localIds[id] << ".gda";
    int tfd=open(tfn.str().c_str(),O_RDWR|O_CREAT|O_TRUNC,fmode);

    ostringstream pfn;
    pfn << path << "/p_" << localIds[id]<< ".gda";
    int pfd=open(pfn.str().c_str(),O_RDWR|O_CREAT|O_TRUNC,fmode);

    if (tperpfd==-1 || tparfd==-1 || denfd==-1 || tfd==-1 || pfd==-1 )
      {
      perror("Error opening file(s).");
      cerr << "    " <<  (tparfd<0?"  ok  ":"failed") << " " << tparfn.str() << endl;
      cerr << "    " <<  (tperpfd<0?"  ok  ":"failed") << " " << tperpfn.str() << endl;
      cerr << "    " <<  (denfd<0?"  ok  ":"failed") << " " << denfn.str() << endl;
      cerr << "    " <<  (tfd<0?"  ok  ":"failed") << " " << tfn.str() << endl;
      cerr << "    " <<  (pfd<0?"  ok  ":"failed") << " " << pfn.str() << endl;
      cerr << "  skipping id " << localIds[id] << "." << endl;
      }
    // The input and output files have been successfully opened.
    else
      {
      struct stat tparss;
      fstat(tparfd,&tparss);
      size_t fsize=tparss.st_size;

      // map the input
      float *tpar
        = static_cast<float*>(mmap(0,fsize,PROT_READ,MAP_PRIVATE,tparfd,0));
      float *tperp
        = static_cast<float*>(mmap(0,fsize,PROT_READ,MAP_PRIVATE,tperpfd,0));
      float *den
        = static_cast<float*>(mmap(0,fsize,PROT_READ,MAP_PRIVATE,denfd,0));
      // resize the initially empty output
      lseek(tfd,fsize-1,SEEK_SET);
      write(tfd,"",1);
      lseek(pfd,fsize-1,SEEK_SET);
      write(pfd,"",1);
      // map the output.
      float *t
        = static_cast<float*>(mmap(0,fsize,PROT_WRITE,MAP_SHARED,tfd,0));
      float *p
        = static_cast<float*>(mmap(0,fsize,PROT_WRITE,MAP_SHARED,pfd,0));

      if (tpar==(float*)-1 || tperp==(float*)-1 || den==(float*)-1 || t==(float*)-1 || p==(float*)-1)
        {
        perror("Error mapping file(s).");
        cerr << "    " << (tpar<0?"  ok  ":"failed") << " " << tparfn.str() << endl;
        cerr << "    " <<  (tperp<0?"  ok  ":"failed") << " " << tperpfn.str() << endl;
        cerr << "    " <<  (den<0?"  ok  ":"failed") << " " << denfn.str() << endl;
        cerr << "    " <<  (t<0?"  ok  ":"failed") << " " << tfn.str() << endl;
        cerr << "    " <<  (p<0?"  ok  ":"failed") << " " << pfn.str() << endl;
        cerr << "  skipping id " << localIds[id] << "." << endl;
        }
      // files were successfully mapped into memory
      else
        {
        // calculate temnperature and pressure
        size_t nVals=fsize/sizeof(float);
        for (size_t i=0; i<nVals; ++i)
          {
          //   total temperature = (tpar + 2*tperp)/3
          //   pressure is just temp*den. 
          t[i]=(tpar[i]+2.0*tperp[i])/3.0;
          p[i]=den[i]*t[i];
          }

        // unmamp any memory that was mapped.
        if (tpar!=(float*)-1){ munmap(tpar,fsize); }
        if (tperp!=(float*)-1){ munmap(tperp,fsize); }
        if (den!=(float*)-1){ munmap(den,fsize); }
        if (t!=(float*)-1){ munmap(t,fsize); }
        if (p!=(float*)-1){ munmap(p,fsize); }

        // cerr << procId << ", " << localIds[id] << "|";
        cerr << ".";
        cerr.flush();
        }
      }
    // close any files we opened.
    if (tparfd!=-1){ close(tparfd); }
    if (tperpfd!=-1){ close(tperpfd); }
    if (denfd!=-1){ close(denfd); }
    if (tfd!=-1){ close(tfd); }
    if (pfd!=-1){ close(pfd); }
    }

  MPI_Barrier(MPI_COMM_WORLD);
  if (procId==0) cerr << endl;
  if (procId!=0) free(path);

  MPI_Finalize();

  return 0;
}
