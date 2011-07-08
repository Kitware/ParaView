#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// #include <stdio.h>
// #include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
  if (argc!=7)
    {
    cerr 
      << "usage:" << endl
      << "  " << argv[0] << " /path/to/f /path/to/g /path/to/h nx ny nz" << endl
      << endl;
    return 1;
    }

  int i=0;
  char *fnf=argv[++i];
  char *fng=argv[++i];
  char *fnh=argv[++i];
  int ni=atoi(argv[++i]);
  int nj=atoi(argv[++i]);
  int nk=atoi(argv[++i]);

  size_t n=ni*nj*nk;

  int fdf=open(fnf,O_RDONLY);
  if (fdf<0)
    {
    string estr=strerror(errno);
    cerr
      << "Error: Failed to open " << fnf << "." << endl
      << estr << endl;
    return 1;
    }
  float *f=(float *)malloc(n*sizeof(float));
  read(fdf,f,n*sizeof(float));
  close(fdf);

  int fdg=open(fng,O_RDONLY);
  if (fdf<0)
    {
    string estr=strerror(errno);
    cerr
      << "Error: Failed to open " << fng << "." << endl
      << estr << endl;
    return 1;
    }
  float *g=(float *)malloc(n*sizeof(float));
  read(fdg,g,n*sizeof(float));
  close(fdg);

  float *h=(float *)malloc(n*sizeof(float));

  for (size_t i=0; i<n; ++i)
    {
    h[i]=f[i]+g[i];
    }

  const int mode=S_IRUSR|S_IWUSR|S_IRGRP;
  const int flags=O_WRONLY|O_CREAT|O_TRUNC;
  int fdh=open(fnh,flags,mode);
  if (fdh<0)
    {
    string estr=strerror(errno);
    cerr
      << "Error: Failed to open " << fnh << "." << endl
      << estr << endl;
    return 1;
    }
  write(fdh,h,n*sizeof(float));
  close(fdh);

  free(f);
  free(g);
  free(h);

  return 0;
}
