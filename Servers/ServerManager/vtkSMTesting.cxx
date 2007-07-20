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
#include "vtkSMRenderViewProxy.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkSMTesting);
vtkCxxRevisionMacro(vtkSMTesting, "1.3");
vtkCxxSetObjectMacro(vtkSMTesting, RenderViewProxy, vtkSMRenderViewProxy);
//-----------------------------------------------------------------------------
vtkSMTesting::vtkSMTesting()
{
  this->RenderViewProxy = 0;
  this->Testing = vtkTesting::New();
}

//-----------------------------------------------------------------------------
vtkSMTesting::~vtkSMTesting()
{
  this->SetRenderViewProxy(0);
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
  if (this->RenderViewProxy)
    {
    vtkImageData* image = this->RenderViewProxy->CaptureWindow(1);
    res = this->Testing->RegressionTest(image, thresh);
    image->Delete();
    }
  return res;
}

//-----------------------------------------------------------------------------
void vtkSMTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "RenderViewProxy: " << this->RenderViewProxy << endl;
}
