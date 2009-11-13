/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoDistillerSTD.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoDistillerSTD.cxx

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
#include "vtkCellType.h"
#include "vtkCosmoDistillerSTD.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkAlgorithmOutput.h"

#include "vtkTimerLog.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkCosmoDistillerSTD, "1.2");
vtkStandardNewMacro(vtkCosmoDistillerSTD);

/****************************************************************************/
vtkCosmoDistillerSTD::vtkCosmoDistillerSTD()
{
  this->SetNumberOfInputPorts(2);
}

/****************************************************************************/
vtkCosmoDistillerSTD::~vtkCosmoDistillerSTD()
{
}

//---------------------------------------------------------------------------
void vtkCosmoDistillerSTD::SetSourceConnection(vtkAlgorithmOutput *algOutput)
{
  this->SetInputConnection(1, algOutput);
}

/****************************************************************************/
int vtkCosmoDistillerSTD::RequestData(vtkInformation* vtkNotUsed(request),
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  vtkDataSet *input0 = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet *input1 = vtkDataSet::GetData(inputVector[1]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  vtkDataArray *maskArray = this->GetInputArrayToProcess(0,input0);
  vtkDataArray *sourceArray = this->GetInputArrayToProcess(1,input1);
  if (maskArray == NULL || sourceArray == NULL)
    {
    return 1;
    }

  if (maskArray->GetDataType() != VTK_INT || sourceArray->GetDataType() != VTK_INT)
    {
    vtkErrorMacro("The mask and source arrays must be integer arrays!");
    return 0;
    }

  // number of points in the mask data
  int npts = input0->GetNumberOfPoints();

  vtkIntArray *iMask = vtkIntArray::SafeDownCast(maskArray);
  vtkIntArray *iSource = vtkIntArray::SafeDownCast(sourceArray);

  // the range of mask values
  double range[2];
  iMask->GetRange(range);
  int maxMaskIndex = int(range[1] - range[0]);
  int minMask = (int)range[0];
  int maxMask = (int)range[1];

  // convert the mask array into a full range bool array
  bool *value_mask = new bool[maxMaskIndex+1];
  memset(value_mask, 0, sizeof(bool) * (maxMaskIndex+1));
  for (int i=0; i<npts; i++)
    {
    value_mask[iMask->GetValue(i)-minMask] = true;
    }

  // number of particles in the source data
  int npart = input1->GetNumberOfPoints();

  vtkPoints *points = vtkPoints::New();

  // allocate point arrays in the output
  int pointArrays = input1->GetPointData()->GetNumberOfArrays();
  for (int i = 0; i < pointArrays; i++)
    {
    vtkDataArray *array = input1->GetPointData()->GetArray(i);
    vtkDataArray *outArray = vtkDataArray::CreateDataArray(array->GetDataType());
    outArray->SetName(array->GetName());
    outArray->SetNumberOfComponents(array->GetNumberOfComponents());

    output->GetPointData()->AddArray(outArray);
    outArray->Delete();
    }

  for (int i = 0; i < npart; i++)
    {
    int sourceVal = iSource->GetValue(i);

    // skip this particle?
    if (sourceVal < minMask || sourceVal > maxMask || !value_mask[sourceVal-minMask])
      {
      continue;
      }

    points->InsertNextPoint(input1->GetPoint(i));

    // copy the corresponding point's attributes from source to output
    for (int j = 0; j < pointArrays; j++)
      {
      vtkDataArray *inputArray = input1->GetPointData()->GetArray(j);
      vtkDataArray *outArray = output->GetPointData()->GetArray(j);
      outArray->InsertNextTuple(inputArray->GetTuple(i)); 
      }
    }

  output->SetPoints(points);
  points->Delete();

  // cleanup
  delete[] value_mask;

  return 1;
}

/****************************************************************************/
void vtkCosmoDistillerSTD::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

/****************************************************************************/
void vtkCosmoDistillerSTD::SetSourceArrayToProcess(
  int vtkNotUsed(idx), int vtkNotUsed(port), int connection,
  int fieldAssociation, const char *name)
{
  this->Superclass::SetInputArrayToProcess(1, 1, connection, 
                                           fieldAssociation, name);
}
