#include<vtkIOStream.h"
#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/string>
using vtkstd::string;

ostream &operator<<(ostream &os, vector<string> v)
{
  os << "[";
  size_t n=v.size();
  if (n>0)
    {
    os << v[0];
    for (size_t i=1; i<n; ++i)
      {
      os << ", " << v[i];
      }
    }
  os << "]";
  return os;
}

ostream &operator<<(ostream &os, vector<double> v)
{
  os << "[";
  size_t n=v.size();
  if (n>0)
    {
    os << v[0];
    for (size_t i=1; i<n; ++i)
      {
      os << ", " << v[i];
      }
    }
  os << "]";
  return os;
}

ostream &operator<<(ostream &os, vector<int> v)
{
  os << "[";
  size_t n=v.size();
  if (n>0)
    {
    os << v[0];
    for (size_t i=1; i<n; ++i)
      {
      os << ", " << v[i];
      }
    }
  os << "]";
  return os;
}

ostream &PrintD3(ostream &os, const double *v3)
{
  vector<double> dv(3,0.0);
  dv[0]=v3[0];
  dv[1]=v3[1];
  dv[2]=v3[2];
  os << dv;
  return os;
}

ostream &PrintD6(ostream &os, const double *v6)
{
  vector<double> dv(6,0.0);
  dv[0]=v6[0];
  dv[1]=v6[1];
  dv[2]=v6[2];
  dv[3]=v6[3];
  dv[4]=v6[4];
  dv[5]=v6[5];
  os << dv;
  return os;
}

