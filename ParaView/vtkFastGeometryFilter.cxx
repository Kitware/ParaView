/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFastGeometryFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkFastGeometryFilter.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkStructuredGridGeometryFilter.h"





//----------------------------------------------------------------------------
vtkFastGeometryFilter* vtkFastGeometryFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkFastGeometryFilter");
  if(ret)
    {
    return (vtkFastGeometryFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkFastGeometryFilter;
}


//----------------------------------------------------------------------------
vtkFastGeometryFilter::vtkFastGeometryFilter()
{
}

//----------------------------------------------------------------------------
vtkFastGeometryFilter::~vtkFastGeometryFilter()
{
}



//----------------------------------------------------------------------------
// Information for output type of PolyData
void vtkFastGeometryFilter::ExecuteInformation()
{
}

//----------------------------------------------------------------------------
void vtkFastGeometryFilter::Execute()
{
  vtkDataSet *input= this->GetInput();
  int numCells = input->GetNumberOfCells();
  int *ext;

  if (numCells == 0)
    {
    return;
    }

  switch (input->GetDataObjectType())
    {
    //case  VTK_POLY_DATA:
      //this->PolyDataExecute();
      //return;
    case  VTK_UNSTRUCTURED_GRID:
      {
      this->UnstructuredGridExecute();
      return;
      }
    case VTK_STRUCTURED_GRID:
      {
      vtkStructuredGrid *grid = vtkStructuredGrid::SafeDownCast(input);
      ext = grid->GetExtent();      
      this->StructuredExecute(grid, ext);
      return;
      }
    case VTK_IMAGE_DATA:      
      {
      vtkImageData *image = vtkImageData::SafeDownCast(input);
      ext = image->GetExtent();      
      this->StructuredExecute(image, ext);
      return;
      }
    }

  vtkErrorMacro("We do not handle " << input->GetClassName() << " yet.");
}



//----------------------------------------------------------------------------
// It is a pain that structured data sets do not share a common super class
// other than data set, and data set does not allow access to extent!
void vtkFastGeometryFilter::StructuredExecute(vtkDataSet *input, int *ext)
{
  int *wholeExt;
  int cellArraySize, numPoints;
  vtkCellArray *outStrips;
  vtkPoints *outPoints;

  // Cell Array Size is a pretty good estimate.  
  // Does not consider direction of strip.

  wholeExt = input->GetWholeExtent();

  // Lets figure out how many cells and points we are going to have.
  // It may be overkill comptuing the exact amount, but we can do it, so ...
  cellArraySize = numPoints = 0;
  // xMin face
  if (ext[0] == wholeExt[0] && ext[2] != ext[3] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
  // xMax face
  if (ext[1] == wholeExt[1] && ext[1] != ext[0] && ext[2] != ext[3] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
    }
  // yMin face
  if (ext[2] == wholeExt[2] && ext[0] != ext[1] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
  // yMax face
  if (ext[3] == wholeExt[3] && ext[3] != ext[2] && ext[0] != ext[1] && ext[4] != ext[5])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[5]-ext[4]+1);
    }
  // zMin face
  if (ext[4] == wholeExt[4] && ext[0] != ext[1] && ext[2] != ext[3])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }
  // zMax face
  if (ext[5] == wholeExt[5] && ext[5] != ext[4] && ext[0] != ext[1] && ext[2] != ext[3])
    {
    cellArraySize += 2*(ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    numPoints += (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1);
    }
  outStrips = vtkCellArray::New();
  outStrips->Allocate(cellArraySize);
  outPoints = vtkPoints::New();
  outPoints->Allocate(numPoints);
  this->GetOutput()->SetPoints(outPoints);
  outPoints->Delete();
  outPoints = NULL;
  this->GetOutput()->SetStrips(outStrips);
  outStrips->Delete();
  outStrips = NULL;

  //newPolys->Allocate(newPolys->EstimateSize(numStrips,ptsPerStrip));


  // Allocate attributes for copying.
  this->GetOutput()->GetPointData()->CopyAllocate(input->GetPointData());
  this->GetOutput()->GetCellData()->CopyAllocate(input->GetCellData());
  //pointData->CopyData(pd,pts[j],outCount);

  // xMin face
  this->ExecuteFace(input, 0, ext, 0,1,2);
  // xMax face
  this->ExecuteFace(input, 1, ext, 0,2,1);
  // yMin face
  this->ExecuteFace(input, 0, ext, 1,2,0);
  // yMax face
  this->ExecuteFace(input, 1, ext, 1,0,2);
  // zMin face
  this->ExecuteFace(input, 0, ext, 2,0,1);
  // zMax face
  this->ExecuteFace(input, 1, ext, 2,1,0);

}

//----------------------------------------------------------------------------
// I tried to make this general for any face, but it required too many args.
void vtkFastGeometryFilter::ExecuteFace(vtkDataSet *input, int maxFlag, int *ext,
                                        int aAxis, int bAxis, int cAxis)
{
  vtkPolyData  *output;
  vtkPoints    *outPts;
  vtkCellArray *outStrips;
  vtkPointData *inPD, *outPD;
  vtkCellData  *inCD, *outCD;
  int          *wholeExt;
  int          pInc[3];
  int          qInc[3];
  int          cOutInc;
  float        pt[3];
  int          inStartPtId;
  int          inStartCellId;
  int          outStartPtId;
  int          outPtId;
  int          inId, outId;
  int          ib, ic;
  int          aA2, bA2, cA2;
  int          rotatedFlag;
  int          *stripArray;
  int          stripArrayIdx;

  output = this->GetOutput();
  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  wholeExt = input->GetWholeExtent();
  pInc[0] = 1;
  pInc[1] = (ext[1]-ext[0]+1);
  pInc[2] = (ext[3]-ext[2]+1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1]-ext[0];
  qInc[2] = (ext[3]-ext[2]) * qInc[1];

  // Tempoprary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2+1] || ext[cA2] == ext[cA2+1])
    {
    return;
    }
  if (maxFlag)
    { // max faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2+1] || ext[aA2+1] < wholeExt[aA2+1])
      {
      return;
      } 
    }
  else
    {
    if (ext[aA2] > wholeExt[aA2])
      {
      return;
      }
    }

  // Lets rotate the image to make b the longest axis.
  // This will make the tri strips longer.
  rotatedFlag = 0;
  if (ext[bA2+1]-ext[bA2] < ext[cA2+1]-ext[cA2])
    {
    int tmp;
    rotatedFlag = 1;
    tmp = cAxis;
    cAxis = bAxis;
    bAxis = tmp;
    bA2 = bAxis * 2;
    cA2 = cAxis * 2;
    }
  
  // Assuming no ghost cells ...
  inStartPtId = inStartCellId = 0;
  if (maxFlag)
    {
    inStartPtId = pInc[aAxis]*(ext[aA2+1]-ext[aA2]);
    inStartCellId = qInc[aAxis]*(ext[aA2+1]-ext[aA2]-1);
    }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2+1]; ++ic)
    {
    for (ib = ext[bA2]; ib <= ext[bA2+1]; ++ib)
      {
      inId = inStartPtId + ib*pInc[bAxis] + ic*pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD,inId,outId);
      }
    }

  // Do the cells.
  cOutInc = ext[bA2+1] - ext[bA2] + 1;

  // Tri Strips (no cell data ...).
  // Allocate the temporary array user to create the tri strips.
  stripArray = new int[2*(ext[bA2+1]-ext[bA2]+1)];
  // Make the cells for this face.
  outStrips = output->GetStrips();
  for (ic = ext[cA2]; ic < ext[cA2+1]; ++ic)
    {
    // Fill in the array describing the strips.
    stripArrayIdx = 0;
    outPtId = outStartPtId + ic*cOutInc;
    if (rotatedFlag)
      {
      for (ib = ext[bA2]; ib <= ext[bA2+1]; ++ib)
        {
        stripArray[stripArrayIdx++] = outPtId+cOutInc;
        stripArray[stripArrayIdx++] = outPtId;
        ++outPtId;
        }
      }
    else
      { // Faster to justto dupicate the inner most loop.
      for (ib = ext[bA2]; ib <= ext[bA2+1]; ++ib)
        {
        stripArray[stripArrayIdx++] = outPtId;
        stripArray[stripArrayIdx++] = outPtId+cOutInc;
        ++outPtId;
        }
      }
    outStrips->InsertNextCell(stripArrayIdx, stripArray);
    }
  delete [] stripArray;

  // Old method for creating quads (needed for cell data.).
  // We should put the option back in.
  /*
  for (ic = ext[cA2]; ic < ext[cA2+1]; ++ic)
    {
    for (ib = ext[bA2]; ib < ext[bA2+1]; ++ib)
      {
      outPtId = outStartPtId + ib + ic*cOutInc;
      inId = inStartCellId + ib*qInc[bAxis] + ic*qInc[cAxis];

      //newPolys->InsertNextCell(ptsPerStrip, ptidx);

      outId = outPolys->InsertNextCell(4);
      outPolys->InsertCellPoint(outPtId);
      outPolys->InsertCellPoint(outPtId+cOutInc);
      outPolys->InsertCellPoint(outPtId+cOutInc+1);
      outPolys->InsertCellPoint(outPtId+1);
      // Copy cell data.
      outCD->CopyData(inCD,inId,outId);
      }
    }
  */
}



//----------------------------------------------------------------------------
void vtkFastGeometryFilter::UnstructuredGridExecute()
{
  int cellId, i, j, newCellId;
  vtkDataSet *input= this->GetInput();
  int numPts=input->GetNumberOfPoints();
  int numCells=input->GetNumberOfCells();
  vtkGenericCell *cell;
  vtkCell *face;
  float *x;
  vtkIdList *cellIds;
  vtkIdList *pts;
  vtkPoints *newPts;
  int ptId;
  int npts, pt;
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  // ghost cell stuff
  unsigned char  updateLevel = (unsigned char)(output->GetUpdateGhostLevel());
  unsigned char  *cellGhostLevels = NULL;
  
  if (numCells == 0)
    {
    return;
    }

  vtkFieldData* fd = input->GetCellData()->GetFieldData();
  vtkDataArray* temp = 0;
  if (fd)
    {
    temp = fd->GetArray("vtkGhostLevels");
    }
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkDebugMacro("No appropriate ghost levels field available.");
    }
  else
    {
    cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
    }
  
  cellIds = vtkIdList::New();
  pts = vtkIdList::New();

  vtkDebugMacro(<<"Executing geometry filter");

  cell = vtkGenericCell::New();


  // Allocate
  //
  newPts = vtkPoints::New();
  newPts->Allocate(numPts,numPts/2);
  output->Allocate(4*numCells,numCells/2);
  outputPD->CopyAllocate(pd,numPts,numPts/2);
  outputCD->CopyAllocate(cd,numCells,numCells/2);

  // Traverse cells to extract geometry
  //
  int abort=0;
  int progressInterval = numCells/20 + 1;
  for(cellId=0; cellId < numCells && !abort; cellId++)
    {
    //Progress and abort method support
    if ( !(cellId % progressInterval) )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress ((float)cellId/numCells);
      abort = this->GetAbortExecute();
      }
    
    input->GetCell(cellId,cell);
    switch (cell->GetCellDimension())
      {
      // create new points and then cell
      case 0: case 1: case 2:
        
        npts = cell->GetNumberOfPoints();
        pts->Reset();
        for ( i=0; i < npts; i++)
          {
          ptId = cell->GetPointId(i);
          x = input->GetPoint(ptId);
          pt = newPts->InsertNextPoint(x);
          outputPD->CopyData(pd,ptId,pt);
          pts->InsertId(i,pt);
          }
        newCellId = output->InsertNextCell(cell->GetCellType(), pts);
        outputCD->CopyData(cd,cellId,newCellId);
        break;
       case 3:
        for (j=0; j < cell->GetNumberOfFaces(); j++)
          {
          face = cell->GetFace(j);
          input->GetCellNeighbors(cellId, face->PointIds, cellIds);
          if ( cellIds->GetNumberOfIds() <= 0)
            {
            npts = face->GetNumberOfPoints();
            pts->Reset();
            for ( i=0; i < npts; i++)
              {
              ptId = face->GetPointId(i);
              x = input->GetPoint(ptId);
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd,ptId,pt);
              pts->InsertId(i,pt);
              }
            newCellId = output->InsertNextCell(face->GetCellType(), pts);
            outputCD->CopyData(cd,cellId,newCellId);
            }
          }
      break;
      } //switch
    } //for all cells

  vtkDebugMacro(<<"Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  cell->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  //free storage
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();
}


//----------------------------------------------------------------------------
void vtkFastGeometryFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  int piece, numPieces, ghostLevels;
  
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro("No Input");
    return;
    }
  piece = output->GetUpdatePiece();
  numPieces = output->GetUpdateNumberOfPieces();
  ghostLevels = output->GetUpdateGhostLevel();
  
  if (numPieces > 1)
    {
    ++ghostLevels;
    }

  this->GetInput()->SetUpdateExtent(piece, numPieces, ghostLevels);

  this->GetInput()->RequestExactExtentOn();
}



//----------------------------------------------------------------------------
void vtkFastGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);


}

//========================================================================
// Tris are now degenerate quads so we only need one hash table.
// We might want to change the method names from QuadHash to just Hash.


// Helper classes for hashing faces.
class vtkFastGeomQuad 
{
  public:
  int p0;
  int p1;
  int p2;
  int p3;
  unsigned char Hide;
  vtkFastGeomQuad *Next;
};



//----------------------------------------------------------------------------
void vtkFastGeometryFilter::InitializeQuadHash(int numPoints)
{
  int i;

  if (this->QuadHash)
    {
    this->DeleteQuadHash();
    }
  this->QuadHash = new vtkFastGeomQuad*[numPoints];
  this->QuadHashLength = numPoints;
  for (i = 0; i < numPoints; ++i)
    {
    this->QuadHash[i] = NULL;
    }
}


//----------------------------------------------------------------------------
void vtkFastGeometryFilter::DeleteQuadHash()
{
  vtkFastGeomQuad *quad, *next;
  int i;

  for (i = 0; i < this->QuadHashLength; ++i)
    {
    quad = this->QuadHash[i];
    this->QuadHash[i] = NULL;
    while (quad)
      {
      next = quad->Next;
      quad->Next = NULL;
      delete quad;
      quad = next;
      next = NULL;
      }
    }

  delete [] this->QuadHash;
  this->QuadHash = NULL;
  this->QuadHashLength = 0;
}



//----------------------------------------------------------------------------
void vtkFastGeometryFilter::InsertQuadInHash(int a, int b, int c, int d, int hidden)
{
  int tmp;
  vtkFastGeomQuad *quad, **end;


  // Reorder to get smallest id in a.
  if (b < a && b < c && b < d)
    {
    tmp = a;
    a = b;
    b = c;
    c = d;
    d = tmp;
    }
  else if (c < a && c < b && c < d)
    {
    tmp = a;
    a = c;
    c = tmp;
    tmp = b;
    b = d;
    d = tmp;
    }
  else if (d < a && d < b && d < c)
    {
    tmp = a;
    a = d;
    d = c;
    c = b;
    b = tmp;
    }

  // Look for existing quad in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
    {
    end = &(quad->Next);
    // a has to match in this bin.
    // c should be independant of point order.
    if (c == quad->p2)
      { 
      // Check boh orders for b and d.
      if ((b == quad->p1 && d == quad->p3) || (b == quad->p3 && d == quad->p1))
        {
        // We have a match.
        quad->Hide = 1;
        // That is all we need to do.  Hide any quad shared by two or more cells.
        return;
        }
      }
    }
  
  // Create a new quad and add it to the hash.
  quad = new vtkFastGeomQuad;
  quad->Next = NULL;
  quad->Hide = 0;
  quad->p0 = a;
  quad->p1 = b;
  quad->p2 = c;
  quad->p3 = d;
  *end = quad;
}


//----------------------------------------------------------------------------
void vtkFastGeometryFilter::InsertTriInHash(int a, int b, int c, int hidden)
{
  int tmp;
  vtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c)
    {
    tmp = a;
    a = b;
    b = c;
    c = tmp;
    }
  else if (c < a && c < b)
    {
    tmp = a;
    a = c;
    c = b;
    b = tmp;
    }

  // Look for existing tri in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
    {
    end = &(quad->Next);
    // a has to match in this bin.
    // Tris have p0 == p3 
    if (quad->p0 == quad->p3)
      { 
      // Check boh orders for b and c.
      if ((b == quad->p1 && c == quad->p2) || (b == quad->p2 && c == quad->p1))
        {
        // We have a match.
        quad->Hide = 1;
        // That is all we need to do.  Hide any tri shared by two or more cells.
        return;
        }
      }
    }
  
  // Create a new quad and add it to the hash.
  quad = new vtkFastGeomQuad;
  quad->Next = NULL;
  quad->Hide = 0;
  quad->p0 = a;
  quad->p1 = b;
  quad->p2 = c;
  quad->p3 = a;
  *end = quad;
}

//----------------------------------------------------------------------------
void vtkFastGeometryFilter::InitQuadHashTraversal()
{
  this->QuadHashTraversalIndex = 0;
  this->QuadHashTraversal = this->QuadHash[0];
}

//----------------------------------------------------------------------------
vtkFastGeomQuad *vtkFastGeometryFilter::GetNextVisibleQuadFromHash()
{
  vtkFastGeomQuad *quad;

  quad = this->QuadHashTraversal;

  // Move till traversal until we have a quad to return.
  // Note: the current traversal has not been returned yet.
  while (quad == NULL || quad->Hide)
    {
    if (quad)
      { // The quad must be hidden.  Move to the next.
      quad = quad->Next;
      }
    else
      { // must be the end of the linked list.  Move to the next bin.
      this->QuadHashTraversalIndex += 1;
      if ( this->QuadHashTraversalIndex >= this->QuadHashLength)
        { // There are no more bins.
        this->QuadHashTraversal = NULL;
        return NULL;
        }
      quad = this->QuadHash[this->QuadHashTraversalIndex];
      }
    }
  
  // Now we have a quad to return.  Set the traversal to the next entry.
  this->QuadHashTraversal = quad->Next;

  return quad;
}





