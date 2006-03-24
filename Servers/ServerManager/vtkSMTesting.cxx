/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTesting.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTesting.h"

#include "vtkObjectFactory.h"
#include "vtkTesting.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"

vtkStandardNewMacro(vtkSMTesting);
vtkCxxRevisionMacro(vtkSMTesting, "1.1");
vtkCxxSetObjectMacro(vtkSMTesting, RenderModuleProxy, vtkSMRenderModuleProxy);
//-----------------------------------------------------------------------------
vtkSMTesting::vtkSMTesting()
{
  this->RenderModuleProxy = 0;
  this->Testing = vtkTesting::New();
}

//-----------------------------------------------------------------------------
vtkSMTesting::~vtkSMTesting()
{
  this->SetRenderModuleProxy(0);
  this->Testing->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMTesting::AddArgument(const char* arg)
{
  this->Testing->AddArgument(arg);
}

//-----------------------------------------------------------------------------
int vtkSMTesting::RegressionTest(float thresh)
{
  int res = vtkTesting::FAILED;
  if (this->RenderModuleProxy)
    {
    this->Testing->SetRenderWindow(this->RenderModuleProxy->GetRenderWindow());
    res = this->Testing->RegressionTest(thresh);
    }
  return res;
}

//-----------------------------------------------------------------------------
void vtkSMTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "RenderModuleProxy: " << this->RenderModuleProxy << endl;
}
