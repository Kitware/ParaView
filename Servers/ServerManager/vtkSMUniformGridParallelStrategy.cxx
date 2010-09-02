/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUniformGridParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMUniformGridParallelStrategy);
//----------------------------------------------------------------------------
vtkSMUniformGridParallelStrategy::vtkSMUniformGridParallelStrategy()
{
  this->SetEnableLOD(true);
  this->SetKeepLODPipelineUpdated(true);
}

//----------------------------------------------------------------------------
vtkSMUniformGridParallelStrategy::~vtkSMUniformGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
int vtkSMUniformGridParallelStrategy::GetMoveMode()
{
  return vtkMPIMoveData::PASS_THROUGH;
}

//----------------------------------------------------------------------------
int vtkSMUniformGridParallelStrategy::GetLODMoveMode()
{
  return vtkMPIMoveData::COLLECT;
}

//----------------------------------------------------------------------------
void vtkSMUniformGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


