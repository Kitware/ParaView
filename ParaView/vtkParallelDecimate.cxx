/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelDecimate.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkParallelDecimate.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkDecimatePro.h"
#include "vtkCleanPolyData.h"
#include "vtkAppendPolyData.h"

vtkParallelDecimate* vtkParallelDecimate::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkParallelDecimate");
  if(ret)
    {
    return (vtkParallelDecimate*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkParallelDecimate;
}

vtkParallelDecimate::vtkParallelDecimate()
{
  this->Application = NULL;
}

vtkParallelDecimate::~vtkParallelDecimate()
{
}

void vtkParallelDecimate::Execute()
{
  vtkMultiProcessController *controller;
  int numProcs, myId, i;
  vtkPolyData *polyData = vtkPolyData::New();
  vtkCleanPolyData *clean = vtkCleanPolyData::New();
  vtkDecimatePro *decimate = vtkDecimatePro::New();
  vtkAppendPolyData *append = vtkAppendPolyData::New();
  vtkPVApplication *pvApp = this->GetApplication();
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *pd = vtkPolyData::New();
  vtkPolyData *temp;
  
  controller = pvApp->GetController();

  if (controller == NULL)
    {
    vtkErrorMacro("You must have a controller before calling Decimate.");
    return;
    }

  numProcs = controller->GetNumberOfProcesses();
  myId = controller->GetLocalProcessId();

  pd->ShallowCopy(input);

  for (i = 0; i < ceil(log(numProcs)); i++)
    {
    if ((myId % (int)pow(2, i)) == 0)
      {
      clean->SetInput(pd);
      clean->Update();
      decimate->SetInput(clean->GetOutput());
      decimate->SetTargetReduction(0.5);
      decimate->BoundaryVertexDeletionOff();
      decimate->PreserveTopologyOn();
      decimate->SplittingOff();
      decimate->Update();
      temp = decimate->GetOutput();
      if ((myId % (int)pow(2, i+1)) != 0)
	{
	controller->Send(temp, myId - pow(2, i), 10);
	}
      else
	{
	if ((myId + pow(2, i)) < numProcs)
	  {
	  controller->Receive(polyData, myId + pow(2, i), 10);
	  append->AddInput(temp);
	  append->AddInput(polyData);
	  append->Update();
	  temp = append->GetOutput();
	  }
	}
      }
    }
  
  if (myId == 0)
    {
    clean->SetInput(temp);
    clean->Update();
    decimate->SetInput(clean->GetOutput());
    decimate->SetTargetReduction(0.5);
    decimate->BoundaryVertexDeletionOff();
    decimate->PreserveTopologyOn();
    decimate->SplittingOff();
    decimate->Update();
    output->ShallowCopy(decimate->GetOutput());
    }
  
  polyData->Delete();
  polyData = NULL;
  clean->Delete();
  clean = NULL;
  decimate->Delete();
  decimate = NULL;
  append->Delete();
  append = NULL;
  pd->Delete();
  pd = NULL;
}

void vtkParallelDecimate::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Application (" << this->Application << ")\n";
}
