/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleGridExtent.cxx
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
#include "vtkInteractorStyleGridExtent.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h" 
#include "vtkCellPicker.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
vtkInteractorStyleGridExtent *vtkInteractorStyleGridExtent::New() 
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkInteractorStyleGridExtent");
  if(ret)
    {
    return (vtkInteractorStyleGridExtent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkInteractorStyleGridExtent;
}

//----------------------------------------------------------------------------
vtkInteractorStyleGridExtent::vtkInteractorStyleGridExtent() 
{
  this->StructuredGrid = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorStyleGridExtent::~vtkInteractorStyleGridExtent() 
{
  this->SetStructuredGrid(NULL);
}


//----------------------------------------------------------------------------
void vtkInteractorStyleGridExtent::SetStructuredGrid(vtkStructuredGrid *grid)
{
  if (grid == this->StructuredGrid)
    {
    return;
    }
  if (this->StructuredGrid)
    {
    this->StructuredGrid->UnRegister(this);
    this->StructuredGrid = NULL;
    }
  if (grid)
    {
    grid->Register(this);
    this->StructuredGrid = grid;
    // This will clip the extent with the grid extent.
    this->SetExtent(this->Extent);
    }
}


//--------------------------------------------------------------------------
int *vtkInteractorStyleGridExtent::GetWholeExtent() 
{
  if (this->StructuredGrid == NULL)
    {
    return NULL;
    }
  // We return extent instead of whole extent because the following methods 
  // require the extent to be up to date.
  return this->StructuredGrid->GetExtent();
}


//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleGridExtent::GetWorldSpot(int spotId, float spot[3]) 
{
  int id, ix, iy, iz;
  int *ext, xInc, yInc, zInc;

  if (this->StructuredGrid == NULL)
    {
    vtkErrorMacro("No grid defined.");
    return;
    }

  // Decode the corner from the spot id.
  if (spotId >= 4)
    {
    iz = this->Extent[5];
    spotId -= 4;
    }
  else
    {
    iz = this->Extent[4];
    }
  if (spotId >= 2)
    {
    iy = this->Extent[3];
    spotId -= 2;
    }
  else
    {
    iy = this->Extent[2];
    }
  if (spotId >= 1)
    {
    ix = this->Extent[1];
    }
  else
    {
    ix = this->Extent[0];
    }

  // Make sure the spot is in bounds.
  ext = this->StructuredGrid->GetExtent();
  if (ix < ext[0])
    {
    ix = ext[0];
    }
  if (ix > ext[1])
    {
    ix = ext[1];
    }

  if (iy < ext[2])
    {
    iy = ext[2];
    }
  if (iy > ext[3])
    {
    iy = ext[3];
    }

  if (iz < ext[4])
    {
    iz = ext[4];
    }
  if (iz > ext[5])
    {
    iz = ext[5];
    }

  xInc = 1;
  yInc = ext[1]-ext[0]+1;
  zInc = yInc * (ext[3]-ext[2]+1);
  id = (ix-ext[0])*xInc + (iy-ext[2])*yInc + (iz-ext[4])*zInc;

  this->StructuredGrid->GetPoints()->GetPoint(id, spot);
}

//--------------------------------------------------------------------------
// This assumes the grid is up to date.
void vtkInteractorStyleGridExtent::GetSpotAxes(int spotId, double *v0, 
                                           double *v1, double *v2) 
{
  int id, i0, i1, i2;
  int *ext, inc0, inc1, inc2;
  int delta;
  float *pa, *pb;

  if (this->StructuredGrid == NULL)
    {
    vtkErrorMacro("No grid defined.");
    return;
    }

  // Decode the corner from the spot id.
  i0 = this->Extent[0];
  i1 = this->Extent[2];
  i2 = this->Extent[4];
  if (spotId >= 4)
    {
    i2 = this->Extent[5];
    spotId -= 4;
    }
  if (spotId >= 2)
    {
    i1 = this->Extent[3];
    spotId -= 2;
    }
  if (spotId >= 1)
    {
    i0 = this->Extent[1];
    }

  
  // Make sure the spot is in bounds.
  ext = this->StructuredGrid->GetExtent();
  if (i0 < ext[0])
    {
    i0 = ext[0];
    }
  if (i0 > ext[1])
    {
    i0 = ext[1];
    }
  if (i1 < ext[2])
    {
    i1 = ext[2];
    }
  if (i1 > ext[3])
    {
    i1 = ext[3];
    }
  if (i2 < ext[4])
    {
    i2 = ext[4];
    }
  if (i2 > ext[5])
    {
    i2 = ext[5];
    }

  inc0 = 1;
  inc1 = ext[1]-ext[0]+1;
  inc2 = inc1 * (ext[3]-ext[2]+1);
  id = (i0-ext[0])*inc0 + (i1-ext[2])*inc1 + (i2-ext[4])*inc2;

  delta = 0;
  if (i0 > ext[0])
    {
    ++delta;
    pa = this->StructuredGrid->GetPoints()->GetPoint(id-inc0);
    }
  else
    {
    pa = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (i0 < ext[1])
    {
    ++delta;
    pb = this->StructuredGrid->GetPoints()->GetPoint(id+inc0);
    }
  else
    {
    pb = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (ext[0] == ext[1])
    {
    v0[0] = v0[1] = v0[2] = 0.0;
    }
  else
    {
    v0[0] = (pb[0] - pa[0]) / (double)(delta);
    v0[1] = (pb[1] - pa[1]) / (double)(delta);
    v0[2] = (pb[2] - pa[2]) / (double)(delta);
    }

  delta = 0;
  if (i1 > ext[2])
    {
    ++delta;
    pa = this->StructuredGrid->GetPoints()->GetPoint(id-inc1);
    }
  else
    {
    pa = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (i1 < ext[3])
    {
    ++delta;
    pb = this->StructuredGrid->GetPoints()->GetPoint(id+inc1);
    }
  else
    {
    pb = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (ext[2] == ext[3])
    {
    v1[0] = v1[1] = v1[2] = 0.0;
    }
  else
    {
    v1[0] = (pb[0] - pa[0]) / (double)(delta);
    v1[1] = (pb[1] - pa[1]) / (double)(delta);
    v1[2] = (pb[2] - pa[2]) / (double)(delta);
    }

  delta = 0;
  if (i2 > ext[4])
    {
    ++delta;
    pa = this->StructuredGrid->GetPoints()->GetPoint(id-inc2);
    }
  else
    {
    pa = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (i2 < ext[5])
    {
    ++delta;
    pb = this->StructuredGrid->GetPoints()->GetPoint(id+inc2);
    }
  else
    {
    pb = this->StructuredGrid->GetPoints()->GetPoint(id);
    }
  if (ext[4] == ext[5])
    {
    v2[0] = v2[1] = v2[2] = 0.0;
    }
  else
    {
    v2[0] = (pb[0] - pa[0]) / (double)(delta);
    v2[1] = (pb[1] - pa[1]) / (double)(delta);
    v2[2] = (pb[2] - pa[2]) / (double)(delta);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGridExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkInteractorStyleExtent::PrintSelf(os,indent);

  os << indent << "StructuredGrid: (" << this->StructuredGrid << ")" << endl;

}
