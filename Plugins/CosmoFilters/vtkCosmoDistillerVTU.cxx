/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoDistillerVTU.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoDistillerVTU.cxx

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
#include "vtkCosmoDistillerVTU.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"

//#include "vtkTimerLog.h"
//#include "vtkMath.h"

vtkCxxRevisionMacro(vtkCosmoDistillerVTU, "1.1.4.1");
vtkStandardNewMacro(vtkCosmoDistillerVTU);

/****************************************************************************/
vtkCosmoDistillerVTU::vtkCosmoDistillerVTU()
{
  rL = 0.0;
  XORG = YORG = ZORG = 0.0;
}

/****************************************************************************/
vtkCosmoDistillerVTU::~vtkCosmoDistillerVTU()
{
}

/****************************************************************************/
int vtkCosmoDistillerVTU::RequestData(vtkInformation* vtkNotUsed(request),
                                      vtkInformationVector** inputVector,
                                      vtkInformationVector* outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  // start timing
  //vtkTimerLog *timer = vtkTimerLog::New();
  //timer->StartTimer();

  // READ PARTICLES
  // initilaization
  int npart = input->GetNumberOfPoints();

#if 0
  // find appropriate XROG, YORG, and ZORG
  float minX = rL, minY = rL, minZ = rL;
  float maxX = 0,  maxY = 0,  maxZ = 0;

  for (int i=0; i<npart; i++) {
  double *point = input->GetPoint(i);
  float xx = (float) point[0];
  float yy = (float) point[1];
  float zz = (float) point[2];

  minX = vtkstd::min(minX, xx);
  minY = vtkstd::min(minY, yy);
  minZ = vtkstd::min(minZ, zz);

  maxX = vtkstd::max(maxX, xx);
  maxY = vtkstd::max(maxY, yy);
  maxZ = vtkstd::max(maxZ, zz);
  }

  if (maxX - minX >= rL / 2.0)
    {
    XORG = rL / 2.0;
    }

  if (maxY - minY >= rL / 2.0)
    {
    YORG = rL / 2.0;
    }

  if (maxZ - minZ >= rL / 2.0)
    {
    ZORG = rL / 2.0;
    }
#endif

  // writing
  output->Allocate(npart, npart);
  vtkPoints *points = vtkPoints::New();

  for (int i=0; i<npart; i++)
    {
    // reading
    double *point = input->GetPoint(i);
    float xx = (float) point[0];
    float yy = (float) point[1];
    float zz = (float) point[2];

    // change coordinate values
    xx = xx + XORG - (xx+XORG >= rL) * rL;
    yy = yy + YORG - (yy+YORG >= rL) * rL;
    zz = zz + ZORG - (zz+ZORG >= rL) * rL;

    // writing
    long vtkPointID = points->InsertNextPoint(xx, yy, zz);

    // need this
    vtkIdType nodeIds[1];
    nodeIds[0] = vtkPointID;
    //long vtkCellID =
    output->InsertNextCell (VTK_VERTEX, 1, nodeIds);
    }

  output->ShallowCopy(input);
  output->SetPoints(points);
  points->Delete();

  // end timing
  //timer->StopTimer();
  //cout << timer->GetElapsedTime() << " sec" << endl;
  //timer->Delete();

  return 1;
}

/****************************************************************************/
void vtkCosmoDistillerVTU::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "rL: " << this->rL << endl;
  os << indent << "XORG: " << this->XORG << endl;
  os << indent << "YORG: " << this->YORG << endl;
  os << indent << "ZORG: " << this->ZORG << endl;
}
