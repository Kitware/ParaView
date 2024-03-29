// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMTesting.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMViewProxy.h"
#include "vtkTesting.h"
#include "vtkTrivialProducer.h"

vtkStandardNewMacro(vtkSMTesting);
vtkCxxSetObjectMacro(vtkSMTesting, ViewProxy, vtkSMViewProxy);
//-----------------------------------------------------------------------------
vtkSMTesting::vtkSMTesting()
{
  this->ViewProxy = nullptr;
  this->Testing = vtkTesting::New();
}

//-----------------------------------------------------------------------------
vtkSMTesting::~vtkSMTesting()
{
  this->SetViewProxy(nullptr);
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

  if (this->ViewProxy)
  {
    vtkImageData* image = this->ViewProxy->CaptureWindow(1);

    // We do the local partition id checks to make sure that testing works with
    // pvsynchronousbatch. The image comparison are done only on root node.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (pm->GetPartitionId() == 0)
    {
      vtkTrivialProducer* producer = vtkTrivialProducer::New();
      producer->SetOutput(image);
      res = this->Testing->RegressionTest(producer, thresh);
      producer->Delete();
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
  os << "ViewProxy: " << this->ViewProxy << endl;
}
