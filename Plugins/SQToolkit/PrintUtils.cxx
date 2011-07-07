/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include<iostream>
#include<vector>
#include<string>
#include<map>
using namespace std;

#if defined PV_3_4_BUILD
  #include "vtkAMRBox_3.7.h"
#else
  #include "vtkAMRBox.h"
#endif

//*****************************************************************************
 VTK_EXPORT  ostream &operator<<(ostream &os, const map<string,int> &m)
{
  map<string,int>::const_iterator it=m.begin();
  map<string,int>::const_iterator end=m.end();
  while (it!=end)
    {
    os << it->first << ", " << it->second << endl;
    ++it;
    }
  return os;
}

//*****************************************************************************
 VTK_EXPORT ostream &operator<<(ostream &os, const vector<vtkAMRBox> &v)
{
  os << "\t[" << endl;
  size_t n=v.size();
  for (size_t i=0; i<n; ++i)
    {
    os << "\t\t"; v[i].Print(os) << endl;
    }
  os << "\t]" << endl;
  return os;
}

//*****************************************************************************
 VTK_EXPORT ostream &operator<<(ostream &os, const vector<string> &v)
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

//*****************************************************************************
 VTK_EXPORT ostream &operator<<(ostream &os, const vector<double> &v)
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

//*****************************************************************************
 VTK_EXPORT ostream &operator<<(ostream &os, const vector<float> &v)
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


//*****************************************************************************
 VTK_EXPORT ostream &operator<<(ostream &os, const vector<int> &v)
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

//*****************************************************************************
 VTK_EXPORT ostream &PrintD3(ostream &os, const double *v3)
{
  vector<double> dv(3,0.0);
  dv[0]=v3[0];
  dv[1]=v3[1];
  dv[2]=v3[2];
  os << dv;
  return os;
}

//*****************************************************************************
 VTK_EXPORT ostream &PrintD6(ostream &os, const double *v6)
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

//*****************************************************************************
 VTK_EXPORT ostream &PrintI3(ostream &os, const int *v3)
{
  vector<int> dv(3,0);
  dv[0]=v3[0];
  dv[1]=v3[1];
  dv[2]=v3[2];
  os << dv;
  return os;
}

//*****************************************************************************
 VTK_EXPORT ostream &PrintI6(ostream &os, const int *v6)
{
  vector<int> dv(6,0);
  dv[0]=v6[0];
  dv[1]=v6[1];
  dv[2]=v6[2];
  dv[3]=v6[3];
  dv[4]=v6[4];
  dv[5]=v6[5];
  os << dv;
  return os;
}
