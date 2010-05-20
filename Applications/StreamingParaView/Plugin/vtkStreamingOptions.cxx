/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingOptions.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingOptions.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamingOptions);

class vtkStreamingOptionsInternal {
public:
  vtkStreamingOptionsInternal() {
    this->EnableStreamMessages = false;
    this->StreamedPasses = 16;
    this->UsePrioritization = true;
    this->UseViewOrdering = true;
    this->PieceCacheLimit = 16;
    this->PieceRenderCutoff = 16;
  }

  bool EnableStreamMessages;
  int StreamedPasses;
  bool UsePrioritization;
  bool UseViewOrdering;
  int PieceCacheLimit;
  int PieceRenderCutoff;
};

vtkStreamingOptionsInternal TheInstance;

//----------------------------------------------------------------------------
vtkStreamingOptions::vtkStreamingOptions()
{
}

//----------------------------------------------------------------------------
vtkStreamingOptions::~vtkStreamingOptions()
{
}

//----------------------------------------------------------------------------
int vtkStreamingOptions::GetStreamedPasses()
{
  return TheInstance.StreamedPasses;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetStreamedPasses(int arg)
{
  TheInstance.StreamedPasses = arg;
}

//----------------------------------------------------------------------------
bool vtkStreamingOptions::GetEnableStreamMessages()
{
  return TheInstance.EnableStreamMessages;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetEnableStreamMessages(bool arg)
{
  TheInstance.EnableStreamMessages = arg;
}

//----------------------------------------------------------------------------
bool vtkStreamingOptions::GetUsePrioritization()
{
  return TheInstance.UsePrioritization;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetUsePrioritization(bool arg)
{
  TheInstance.UsePrioritization = arg;
}

//----------------------------------------------------------------------------
bool vtkStreamingOptions:: GetUseViewOrdering()
{
  return TheInstance.UseViewOrdering;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetUseViewOrdering(bool arg)
{
  TheInstance.UseViewOrdering = arg;
}

//----------------------------------------------------------------------------
int vtkStreamingOptions::GetPieceCacheLimit()
{
  return TheInstance.PieceCacheLimit;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetPieceCacheLimit(int arg)
{
  TheInstance.PieceCacheLimit = arg;
}

//----------------------------------------------------------------------------
int vtkStreamingOptions::GetPieceRenderCutoff()
{
  return TheInstance.PieceRenderCutoff;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::SetPieceRenderCutoff(int arg)
{
  TheInstance.PieceRenderCutoff = arg;
}

//----------------------------------------------------------------------------
void vtkStreamingOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

