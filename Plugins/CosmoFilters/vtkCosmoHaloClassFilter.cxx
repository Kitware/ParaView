/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloClassFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloClassFilter.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkCosmoHaloClassFilter.h"

#include "vtkIntArray.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformationIntegerKey.h"

vtkCxxRevisionMacro(vtkCosmoHaloClassFilter, "1.1.4.1");
vtkStandardNewMacro(vtkCosmoHaloClassFilter);

vtkInformationKeyMacro(vtkCosmoHaloClassFilter, OUTPUT_NUMBER_OF_CLASSES, Integer);

void vtkCosmoHaloClassFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCosmoHaloClassFilter::vtkCosmoHaloClassFilter() : Superclass()
{
  this->bounds = vtkIntArray::New();
  this->NumberOfBounds = 0;
}

vtkCosmoHaloClassFilter::~vtkCosmoHaloClassFilter()
{
  this->bounds->Delete();
}

int vtkCosmoHaloClassFilter::RequestInformation(vtkInformation*,
                                                vtkInformationVector**,
                                                vtkInformationVector* outputVector)
{
  // set the number of classes information
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkCosmoHaloClassFilter::OUTPUT_NUMBER_OF_CLASSES(), this->NumberOfBounds+1);

  return 1;
}

int vtkCosmoHaloClassFilter::RequestData(vtkInformation*,
                                         vtkInformationVector** inputVector,
                                         vtkInformationVector* outputVector)
{
  if (this->NumberOfBounds == 0)
    {
    vtkErrorMacro("No halo size bounds specified!");
    return 0;
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *input= vtkUnstructuredGrid::SafeDownCast(
   inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid *output= vtkUnstructuredGrid::SafeDownCast(
   outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the input array to process
  // in current implementation this has no effect on the final result
  // classification is done on "haloSize" regardless
  vtkDataArray* dataArray = this->GetInputArrayToProcess(0, inputVector);
  if (dataArray == NULL)
    {
    vtkErrorMacro("No data array selected!");
    return 0;
    }

  if (strcmp("haloSize", dataArray->GetName()) != 0)
    {
    vtkErrorMacro("Currently only classification by halo size is suppported!");
    }


  // TODO: to classify halos by the user selected array, use "dataArray" instead
  vtkIntArray *haloSize = vtkIntArray::SafeDownCast(input->GetPointData()->GetArray("haloSize")) ;
  if (haloSize == NULL)
    {
    vtkErrorMacro("The input data set doesn't have haloSize!");
    return 0;
    }

  int npart = input->GetNumberOfPoints();
  vtkIntArray *haloClass = vtkIntArray::New();
  haloClass->SetName("haloClass");
  haloClass->SetNumberOfValues(npart);

  // classify halos according to their sizes
  for (int i = 0; i < npart; i++)
    {
    int size = haloSize->GetValue(i);
    if (size <= this->bounds->GetValue(0))
      {
      haloClass->SetValue(i, 0);
      }
    for (int k = 1; k < this->NumberOfBounds; k++)
      {
      if (size > this->bounds->GetValue(k-1) && size <= this->bounds->GetValue(k))
        {
        haloClass->SetValue(i, k);
        break;
        }
      }
    if (size > this->bounds->GetValue(this->NumberOfBounds-1))
      {
      haloClass->SetValue(i, this->NumberOfBounds);
      }

    }

  // copy existing fields from input
  output->ShallowCopy(input);
  // add the "haloClass" field to the output
  output->GetPointData()->AddArray(haloClass);
  haloClass->Delete();

  return 1;
}

void vtkCosmoHaloClassFilter::SetNumberOfBounds(int number)
{
  this->NumberOfBounds = number;
  this->bounds->Resize(number);
}

void vtkCosmoHaloClassFilter::SetBoundValue(int i, double value)
{
  this->bounds->SetValue(i, static_cast<int> (value));
  // update the modification time
  Modified();
}
