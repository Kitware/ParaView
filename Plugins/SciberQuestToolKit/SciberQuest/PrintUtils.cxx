/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include<iostream>
#include<vector>
#include<string>
#include<map>

#include "vtkSciberQuestModule.h" // for export macro
#if defined PV_3_4_BUILD
  #include "vtkAMRBox_3.7.h"
#else
  #include "vtkAMRBox.h"
#endif

//*****************************************************************************
 VTKSCIBERQUEST_EXPORT  std::ostream &operator<<(std::ostream &os, const std::map<std::string,int> &m)
{
  std::map<std::string,int>::const_iterator it=m.begin();
  std::map<std::string,int>::const_iterator end=m.end();
  while (it!=end)
    {
    os << it->first << ", " << it->second << std::endl;
    ++it;
    }
  return os;
}

//*****************************************************************************
 VTKSCIBERQUEST_EXPORT std::ostream &operator<<(std::ostream &os, const std::vector<vtkAMRBox> &v)
{
  os << "\t[" << std::endl;
  size_t n=v.size();
  for (size_t i=0; i<n; ++i)
    {
    os << "\t\t"; v[i].Print(os) << std::endl;
    }
  os << "\t]" << std::endl;
  return os;
}

//*****************************************************************************
 VTKSCIBERQUEST_EXPORT std::ostream &operator<<(std::ostream &os, const std::vector<std::string> &v)
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
 VTKSCIBERQUEST_EXPORT std::ostream &operator<<(std::ostream &os, const std::vector<double> &v)
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
 VTKSCIBERQUEST_EXPORT std::ostream &operator<<(std::ostream &os, const std::vector<float> &v)
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
 VTKSCIBERQUEST_EXPORT std::ostream &operator<<(std::ostream &os, const std::vector<int> &v)
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
 VTKSCIBERQUEST_EXPORT std::ostream &PrintD3(std::ostream &os, const double *v3)
{
  std::vector<double> dv(3,0.0);
  dv[0]=v3[0];
  dv[1]=v3[1];
  dv[2]=v3[2];
  os << dv;
  return os;
}

//*****************************************************************************
 VTKSCIBERQUEST_EXPORT std::ostream &PrintD6(std::ostream &os, const double *v6)
{
  std::vector<double> dv(6,0.0);
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
 VTKSCIBERQUEST_EXPORT std::ostream &PrintI3(std::ostream &os, const int *v3)
{
  std::vector<int> dv(3,0);
  dv[0]=v3[0];
  dv[1]=v3[1];
  dv[2]=v3[2];
  os << dv;
  return os;
}

//*****************************************************************************
 VTKSCIBERQUEST_EXPORT std::ostream &PrintI6(std::ostream &os, const int *v6)
{
  std::vector<int> dv(6,0);
  dv[0]=v6[0];
  dv[1]=v6[1];
  dv[2]=v6[2];
  dv[3]=v6[3];
  dv[4]=v6[4];
  dv[5]=v6[5];
  os << dv;
  return os;
}
