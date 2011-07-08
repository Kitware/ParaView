#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;
#include <fstream>
using std::ofstream;
#include <sstream>
using std::ostringstream;
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <fcntl.h>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>

#include "Tuple.hxx"

//*****************************************************************************
bool IsNan(float v_x, float v_y, float v_z)
{
  return isnan(v_x) || isnan(v_y) || isnan(v_z);
}

//*****************************************************************************
void Init(float v_0, float *v_x, float *v_y, float *v_z, int n)
{
  for (int i=0; i<n; ++i)
    {
    v_x[i]=v_0;
    v_y[i]=v_0;
    v_z[i]=v_0;
    }
}

//*****************************************************************************
float theta(float x, float y)
{
  float theta=atan(fabs(y)/fabs(x));
  float pi=3.14159265358979;

   if (x<=0 && y>=0) // 2nd
    {
    theta=pi-theta;
    }
  else
  if (x<=0 && y<=0) // 3rd
    {
    theta+=pi;
    }
  else
  if (x>=0 && y<=0) // 4th
    {
    theta=2*pi-theta;
    }

  return theta;
}

//*****************************************************************************
void InitSink(int *domain, int *patch, float *v_x, float *v_y, float *v_z)
{
  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  int nk=domain[5]-domain[4]+1;
  // translate so center of vortex is on center of patch
  float itr=ni/2.0; 
  float jtr=nj/2.0;
  float ktr=nk/2.0;   
 
  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        float x=i-itr;
        float y=j-jtr;
        float z=k-ktr;
        float r=sqrt(x*x+y*y+z*z);
        r=r<1E-3?1.0:r;

        int q=k*ni*nj+j*ni+i;

        // unit rays into the origin.
        v_x[q]=-x/r;
        v_y[q]=-y/r;
        v_z[q]=-z/r;
        }
      }
    }
}

//*****************************************************************************
void InitPseudoVortex(int *domain, int *patch, float *v_x, float *v_y, float *v_z)
{
  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  int nk=domain[5]-domain[4]+1;
  // translate so center of vortex is on center of patch
  float itr=ni/2.0; 
  float jtr=nj/2.0;
  // only set values inside a cylinder
  float rmax=(patch[1]-patch[0]+1)/2.0;
  // make small dz.
  float dz=nk/100.0;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        float x=i-itr;
        float y=j-jtr;
        float z=k;

        int q=k*ni*nj+j*ni+i;

        float r=sqrt(x*x+y*y);
        if (r>rmax) // restrict to cylinder
          {
          continue;
          }
        if (r<2.0) // avoid singularity at X=0,0,0
          {
          v_x[q]=0.0;
          v_y[q]=0.0;
          v_z[q]=0.0;
          continue;
          }

        float t=theta(x,y);
        // superpose vector from helix tangent
        v_x[q]=-sin(t);
        v_y[q]=cos(t);
        // v_z[q]=-dz;
        v_z[q]=0.0;
        }
      }
    }
}

//*****************************************************************************
void InitRankineVortex(
      int *domain, 
      int *patch,
      float *v_x,
      float *v_y,
      float *v_z,
      float v_th0, // angular velcity
      float v_z0,  // velocity along k hat
      float R)     // vortex radius 
{
  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  int nk=domain[5]-domain[4]+1;

  // translate so center of vortex is on center of patch
  float itr=ni/2.0; 
  float jtr=nj/2.0;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        int q=k*ni*nj+j*ni+i;
        float x=i-itr;
        float y=j-jtr;
        float r=sqrt(x*x+y*y);
        float a;
        if (r<=1E-3)
          {
          v_x[q]=0.0;
          v_y[q]=0.0;
          v_z[q]=v_z0;
          continue;
          }
        else
        if (r<=R)
          {
          a=v_th0*r;
          }
        else
          {
          a=v_th0*R*R/r;
          }
        float t=theta(x,y);
        v_x[q]=-a*sin(t);
        v_y[q]= a*cos(t);
        v_z[q]=v_z0;
        }
      }
    }
}

//*****************************************************************************
bool Find(
      int *domain,
      int *patch,
      float *v_x,
      float *v_y,
      float *v_z,
      float val)
{
  bool has=false;

  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  int nk=domain[5]-domain[4]+1;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        int q=k*ni*nj+j*ni+i;
        if (v_x[q]==val || v_y[q]==val || v_z[q]==val)
          {
          has=true;
          cerr 
            << __LINE__ << " FOUND val=" << val
            << " at " << Tuple<int>(i,j,k) << endl;
          }
        }
      }
    }
  return has;
}

//*****************************************************************************
bool HasNans(int *domain, int *patch, float *v_x, float *v_y, float *v_z)
{
  bool has=false;

  int ni=domain[1]-domain[0]+1;
  int nj=domain[3]-domain[2]+1;
  int nk=domain[5]-domain[4]+1;
 
  // translate so center of vortex is on center of patch
  float itr=ni/2.0; 
  float jtr=nj/2.0;

  for (int k=patch[4]; k<=patch[5]; ++k)
    {
    for (int j=patch[2]; j<=patch[3]; ++j)
      {
      for (int i=patch[0]; i<=patch[1]; ++i)
        {
        int q=k*ni*nj+j*ni+i;
        float x=i-itr;
        float y=j-jtr;
        float z=k;
        if (IsNan(v_x[q],v_y[q],v_z[q]))
          {
          has=true;
          cerr 
            << __LINE__ << " ERROR NAN." << endl
            << "I=" << Tuple<int>(i,j,k) << endl
            << "X=" << Tuple<float>(x,y,z) << endl;
          }
        }
      }
    }
  return has;
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  // validate command tail
  if (argc<5)
    {
    cerr 
      << "Error: usage: " << argv[0] << "nx ny nz name ..." << endl
      << "Names:" << endl
      << "   pseudo" << endl
      << "   rankine" << endl
      << endl;
    return 1;
    }

  // domain size from command tail
  int nx=atoi(argv[1]);
  int ny=atoi(argv[2]);
  int nz=atoi(argv[3]);
  int domain[6]={0,nx-1,0,ny-1,0,nz-1};

  // allocate the vector field  
  int n=nx*ny*nz;
  float *v_x=(float *)malloc(n*sizeof(float));
  float *v_y=(float *)malloc(n*sizeof(float));
  float *v_z=(float *)malloc(n*sizeof(float));

  // initialize to an absurd value.
  float sentinel=-555.555;
  Init(sentinel,v_x,v_y,v_z,n);

  // initialize to requested dataset
  ostringstream comment;
  comment << "# " << argv[4] << endl;
  if (strcmp(argv[4],"pseudo")==0)
    {
    // get a sub patch
    int vorilo=(nx-1)/3;
    int vorihi=2*vorilo;
    int vorjlo=(ny-1)/3;
    int vorjhi=2*vorjlo;
    int patch[6]={vorilo,vorihi,vorjlo,vorjhi,0,nz-1};

    InitSink(domain,domain,v_x,v_y,v_z);
    InitPseudoVortex(domain,patch,v_x,v_y,v_z);
    }
  else
  if (strcmp(argv[4],"rankine")==0)
    {
    float R=sqrt(nx*nx+ny*ny)/8.0;
    float v_z0=-1.0;
    float v_th0=10.0;
    comment 
      << "# R    =" << R << endl
      << "# v_z0 =" << v_z0 << endl
      << "# v_th0=" << v_th0 << endl;
    InitRankineVortex(domain,domain,v_x,v_y,v_z,v_th0,v_z0,R);
    }
  else
    {
    cerr << "Error: Inavalid name " << argv[4] << endl;
    return 1;
    }

  // check for numerical problems.
  HasNans(domain,domain,v_x,v_y,v_z);
  Find(domain,domain,v_x,v_y,v_z,sentinel);

  // write out the dataset
  n*=sizeof(float);
  int mode=S_IRUSR|S_IWUSR|S_IRGRP;
  int flags=O_WRONLY|O_CREAT|O_TRUNC;

  umask(S_IROTH|S_IWOTH); 

  ostringstream fn;
  fn << argv[4] << "/";
  mkdir(fn.str().c_str(),S_IRWXU|S_IXGRP|S_IXOTH);

  fn.str("");
  fn << argv[4] << "/vix_0.gda";
  int xfd=open(fn.str().c_str(),flags,mode);
  write(xfd,v_x,n);
  close(xfd);

  fn.str("");
  fn << argv[4] << "/viy_0.gda";
  int yfd=open(fn.str().c_str(),flags,mode);
  write(yfd,v_y,n);
  close(yfd);

  fn.str("");
  fn << argv[4] << "/viz_0.gda";
  int zfd=open(fn.str().c_str(),flags,mode);
  write(zfd,v_z,n);
  close(zfd);

  fn.str("");
  fn << argv[4] << "/" << argv[4] << ".bov";
  ofstream os(fn.str().c_str());
  os
    << comment.str()
    << endl
    << "nx=" << nx << ", ny=" << ny << ", nz=" << nz << endl
    << endl;
  os.close();

  return 0;
}

