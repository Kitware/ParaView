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

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTesting.h"

vtkStandardNewMacro(vtkSMTesting);
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

    // We do the local partition id checks to make sure that testing works with
    // pvsynchronousbatch. The image comparison are done only on root node.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (pm->GetPartitionId() == 0)
      {
      res = this->Testing->RegressionTest(image, thresh);
      }
    else
      {
      res = vtkTesting::PASSED;
      }
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
