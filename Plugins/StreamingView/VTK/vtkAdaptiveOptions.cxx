/*=========================================================================

  Program:   ParaView
  Module:    vtkAdaptiveOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveOptions.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAdaptiveOptions);

class vtkAdaptiveOptionsInternal {
public:
  vtkAdaptiveOptionsInternal() {
    this->EnableStreamMessages = false;
    this->UsePrioritization = true;
    this->UseViewOrdering = true;
    this->PieceCacheLimit = 16;
    this->Height = 4;
    this->Degree = 8;
    this->Rate = 2;
    this->MaxSplits = -1;
    this->ShowOn = vtkAdaptiveOptions::REFINE;
  }

  bool EnableStreamMessages;
  bool UsePrioritization;
  bool UseViewOrdering;
  int PieceCacheLimit;
  int Height;
  int Degree;
  int Rate;
  int MaxSplits;
  int ShowOn;
};

vtkAdaptiveOptionsInternal TheInstance;

//----------------------------------------------------------------------------
vtkAdaptiveOptions::vtkAdaptiveOptions()
{
}

//----------------------------------------------------------------------------
vtkAdaptiveOptions::~vtkAdaptiveOptions()
{
}

//----------------------------------------------------------------------------
bool vtkAdaptiveOptions::GetEnableStreamMessages()
{
  return TheInstance.EnableStreamMessages;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetEnableStreamMessages(bool arg)
{
  TheInstance.EnableStreamMessages = arg;
}

//----------------------------------------------------------------------------
bool vtkAdaptiveOptions::GetUsePrioritization()
{
  return TheInstance.UsePrioritization;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetUsePrioritization(bool arg)
{
  TheInstance.UsePrioritization = arg;
}

//----------------------------------------------------------------------------
bool vtkAdaptiveOptions:: GetUseViewOrdering()
{
  return TheInstance.UseViewOrdering;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetUseViewOrdering(bool arg)
{
  TheInstance.UseViewOrdering = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetPieceCacheLimit()
{
  return TheInstance.PieceCacheLimit;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetPieceCacheLimit(int arg)
{
  if (arg < -1)
    {
    arg = -1;
    }
  TheInstance.PieceCacheLimit = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetHeight()
{
  return TheInstance.Height;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetHeight(int arg)
{
  if (arg < 1)
    {
    arg = 1;
    }
  TheInstance.Height = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetDegree()
{
  return TheInstance.Degree;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetDegree(int arg)
{
  if (arg < 2)
    {
    arg = 2;
    }
  TheInstance.Degree = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetRate()
{
  return TheInstance.Rate;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetRate(int arg)
{
  if (arg < 1)
    {
    arg = 1;
    }
  TheInstance.Rate = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetMaxSplits()
{
  return TheInstance.MaxSplits;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetMaxSplits(int arg)
{
  if (arg < -1)
    {
    arg = -1;
    }
  TheInstance.MaxSplits = arg;
}

//----------------------------------------------------------------------------
int vtkAdaptiveOptions::GetShowOn()
{
  return TheInstance.ShowOn;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::SetShowOn(int arg)
{
  TheInstance.ShowOn = arg;
}

//----------------------------------------------------------------------------
void vtkAdaptiveOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
