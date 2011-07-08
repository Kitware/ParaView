/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include <iostream>
using std::cerr;
using std::endl;
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// This script generates an array of val the same size as the other
// arrays in the dataset, example for use in the projection. bz=0.
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  if (argc!=6)
    {
    cerr
      << "Usage:" << endl
      << argv[0] << " nx ny nz val /out/file/name" << endl
      << endl;
    return 1;
    }

  unsigned int nx=atoi(argv[1]);
  unsigned int ny=atoi(argv[2]);
  unsigned int nz=atoi(argv[3]);
  float val=atof(argv[4]);

  unsigned long n=nx*ny*nz;
  unsigned long nb=n*sizeof(float);

  const int flags=O_RDWR|O_CREAT|O_TRUNC;
  const int mode=(S_IRWXU&~S_IXUSR)|S_IRGRP|S_IROTH;
  int fd=open(argv[5],flags,mode);
  if (fd<0)
    {
    perror("Error: failed to open the file.");
    cerr
      << argv[5] << endl
      << endl;
    return 1;
    }
  lseek(fd,nb-1,SEEK_SET);
  write(fd,"",1);

  float *data=(float *)mmap(0,nb,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if (data==MAP_FAILED)
    {
    perror("Error: failed to map the region.");
    cerr << endl;
    return 1;
    }

  for (unsigned long i=0; i<n; ++i)
    {
    data[i]=val;
    }

  munmap(data,nb);
  close(fd);

  return 0;
}
