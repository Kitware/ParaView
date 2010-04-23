/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFaceHash.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFaceHash.h"

#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"





// First step: Use global point ids to build a face hash.
// Face hash keeps track of up to two cells that use the face.
// Hash remebers which point are associated with each face.
// Index by up to four point ids.
// After all cells are added to the hash, it will return the faces that
// only have a single cell (potential process boundaries.).
// This hash should be the same as the dataset surface filter.

// Initialize.
// Add face/cell (point ids) n(3 or 4). Returns a face ID.
// Request the number of cells for a face.
// Get cell ids of face/cellidx.
// Iterate over all faces.






//----------------------------------------------------------------------------
void vtkFaceHash::Initialize(vtkIdType numberOfPoints)
{
  this->Hash = new *vtkFaceHashQuad[numberOfPoints];
}

// Add face/cell (point ids) n(3 or 4). Returns a face ID.
// Request the number of cells for a face.
// Get cell ids of face/cellidx.
// Iterate over all faces.








static int sizeofFastQuad(int numPts)
{
  // account for size of ptArray
  return static_cast<int>(sizeof(vtkFastGeomQuad)+(numPts-4)*sizeof(vtkIdType));
}

vtkStandardNewMacro(vtkFaceHash);

//----------------------------------------------------------------------------
vtkFaceHash::vtkFaceHash()
{
  this->QuadHash = NULL;
  this->QuadHashLength = 0;

  // Quad allocation stuff.
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->FastGeomQuadArrays = NULL;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;
}

//----------------------------------------------------------------------------
vtkFaceHash::~vtkFaceHash()
{
  if (this->QuadHash)
    {
    this->DeleteQuadHash();
    }
}










//----------------------------------------------------------------------------
void vtkFaceHash::InitializeQuadHash(vtkIdType numPoints)
{
  vtkIdType i;

  if (this->QuadHash)
    {
    this->DeleteQuadHash();
    }

  // Prepare our special quad allocator (for efficiency).
  this->InitFastGeomQuadAllocation(numPoints);

  this->QuadHash = new vtkFastGeomQuad*[numPoints];
  this->QuadHashLength = numPoints;
  for (i = 0; i < numPoints; ++i)
    {
    this->QuadHash[i] = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkFaceHash::DeleteQuadHash()
{
  vtkIdType i;

  this->DeleteAllFastGeomQuads();

  for (i = 0; i < this->QuadHashLength; ++i)
    {
    this->QuadHash[i] = NULL;
    }

  delete [] this->QuadHash;
  this->QuadHash = NULL;
  this->QuadHashLength = 0;
}

//----------------------------------------------------------------------------
void vtkFaceHash::InsertQuadInHash(vtkIdType a, vtkIdType b,
                                               vtkIdType c, vtkIdType d, 
                                               vtkIdType sourceId)
{
  vtkIdType tmp;
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
    if (quad->numPts == 4 && c == quad->ptArray[2])
      { 
      // Check boh orders for b and d.
      if ((b == quad->ptArray[1] && d == quad->ptArray[3]) || (b == quad->ptArray[3] && d == quad->ptArray[1]))
        {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do.  Hide any quad shared by two or more cells.
        return;
        }
      }
    quad = *end;
    }
  
  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(4);
  quad->Next = NULL;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  quad->ptArray[3] = d;
  *end = quad;
}

//----------------------------------------------------------------------------
void vtkFaceHash::InsertTriInHash(vtkIdType a, vtkIdType b,
                                              vtkIdType c, vtkIdType sourceId)
{
  vtkIdType tmp;
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
  // We can't put the second smnallest in b because it might change the order
  // of the verticies in the final triangle.

  // Look for existing tri in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
    {
    end = &(quad->Next);
    // a has to match in this bin.
    if (quad->numPts == 3)
      { 
      if ((b == quad->ptArray[1] && c == quad->ptArray[2]) || (b == quad->ptArray[2] && c == quad->ptArray[1]))
        {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do. Hide any tri shared by two or more cells.
        return;
        }
      }
    quad = *end;
    }
  
  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(3);
  quad->Next = NULL;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  *end = quad;
}

// Insert a polygon into the hash.  
// Input: an array of vertex ids
//        the start index of the polygon in the array
//        the end index of the polygon in the array
//        the cellId of the polygon
//----------------------------------------------------------------------------
void vtkFaceHash::InsertPolygonInHash(vtkIdType* ids,
                                                  int numPts, vtkIdType sourceId)
{
  vtkFastGeomQuad *quad, **end;

  // find the index to the smallest id
  vtkIdType offset = 0;
  for(int i=1; i<numPts; i++)
    {
    if(ids[i] < ids[offset])
      {
      offset = i;
      }
    }

  // copy ids into ordered array with smallest id first
  vtkIdType* tab = new vtkIdType[numPts];
  for(int i=0; i<numPts; i++)
    {
    tab[i] = ids[(offset+i)%numPts];
    }
  
  // Look for existing hex in the hash;
  end = this->QuadHash + tab[0];
  quad = *end;
  while (quad)
    {
    end = &(quad->Next);
    // a has to match in this bin.
    // first just check the polygon size.
    bool match = true;
    if (numPts == quad->numPts)
      {
      if ( tab[1] == quad->ptArray[1])
        {
        // if the first two points match loop through forwards
        // checking all points
        for (int i = 2; i < numPts; i++)
          {
          if ( tab[i] != quad->ptArray[i])
            {
            match = false;
            break;
            }
          }
        }
      else if (tab[numPts-1] == quad->ptArray[1])
        {
        // the first two points match with the opposite sense.
        // loop though comparing the correct sense
        for (int i = 2; i < numPts; i++)
          {
          if ( tab[numPts - i] != quad->ptArray[i])
            {
            match = false;
            break;
            }
          }
        }
      else
        {
        match = false;
        }
      }
    else
      {
      match = false;
      }

      if (match)
        {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do. Hide any tri shared by two or more cells.
        return;
        }
    quad = *end;
    }
  
  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(numPts);
  // mark the structure as a polygon
  quad->Next = NULL;
  quad->SourceId = sourceId;
  for (int i = 0; i < numPts; i++)
    {
      quad->ptArray[i] = tab[i];
    }
  *end = quad;

  delete [] tab;
}





//----------------------------------------------------------------------------
void vtkFaceHash::InitQuadHashTraversal()
{
  this->QuadHashTraversalIndex = 0;
  this->QuadHashTraversal = this->QuadHash[0];
}

//----------------------------------------------------------------------------
vtkFastGeomQuad *vtkFaceHash::GetNextVisibleQuadFromHash()
{
  vtkFastGeomQuad *quad;

  quad = this->QuadHashTraversal;

  // Move till traversal until we have a quad to return.
  // Note: the current traversal has not been returned yet.
  while (quad == NULL || quad->SourceId == -1)
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

















































//----------------------------------------------------------------------------
void vtkFaceHash::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//========================================================================
// Tris are now degenerate quads so we only need one hash table.
// We might want to change the method names from QuadHash to just Hash.



//----------------------------------------------------------------------------
int vtkFaceHash::UnstructuredGridExecute(vtkDataSet *dataSetInput,
                                                     vtkPolyData *output)
{
  vtkCellArray *newVerts;
  vtkCellArray *newLines;
  vtkCellArray *newPolys;
  vtkPoints *newPts;
  vtkIdType *ids;
  int progressCount;
  vtkIdType cellId;
  int i, j;
  vtkIdType *cellPointer;
  int cellType;
  vtkUnstructuredGrid *input = vtkUnstructuredGrid::SafeDownCast(dataSetInput);
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkIdType numCells=input->GetNumberOfCells();
  vtkGenericCell *cell;
  int numFacePts, numCellPts;
  vtkIdType inPtId, outPtId;
  vtkPointData *inputPD = input->GetPointData();
  vtkCellData *inputCD = input->GetCellData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkIdType outPts[6];
  vtkFastGeomQuad *q;
  unsigned char* cellTypes = input->GetCellTypesArray()->GetPointer(0);

  // These are for the default case/
  vtkIdList *pts;
  vtkPoints *coords;
  vtkCell *face;
  int flag2D = 0;
  
  pts = vtkIdList::New();  
  coords = vtkPoints::New();
  // might not be necessary to set the data type for coords
  // but certainly safer to do so
  coords->SetDataType(input->GetPoints()->GetData()->GetDataType());
  cell = vtkGenericCell::New();

  this->NumberOfNewCells = 0;
  this->InitializeQuadHash(numPts);

  // Allocate
  //
  newPts = vtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetData()->GetDataType());
  newPts->Allocate(numPts);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(4*numCells,numCells/2);
  newVerts = vtkCellArray::New();
  newLines = vtkCellArray::New();

  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(inputPD, numPts, numPts/2);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(inputCD, numCells, numCells/2);

  if (this->PassThroughCellIds)
    {
    this->OriginalCellIds = vtkIdTypeArray::New();
    this->OriginalCellIds->SetName("vtkOriginalCellIds");
    this->OriginalCellIds->SetNumberOfComponents(1);
    }
  if (this->PassThroughPointIds)
    {
    this->OriginalPointIds = vtkIdTypeArray::New();
    this->OriginalPointIds->SetName("vtkOriginalPointIds");
    this->OriginalPointIds->SetNumberOfComponents(1);
    }

  // First insert all points.  Points have to come first in poly data.
  cellPointer = input->GetCells()->GetPointer();
  for(cellId=0; cellId < numCells; cellId++)
    {
    // Direct access to cells.
    cellType = cellTypes[cellId];
    numCellPts = cellPointer[0];
    ids = cellPointer+1;
    // Move to the next cell.
    cellPointer += (1 + *cellPointer);

    // A couple of common cases to see if things go faster.
    if (cellType == VTK_VERTEX || cellType == VTK_POLY_VERTEX)
      {
      newVerts->InsertNextCell(numCellPts);
      for (i = 0; i < numCellPts; ++i)
        {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD); 
        newVerts->InsertCellPoint(outPtId);
        }
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    }
  
  // Traverse cells to extract geometry
  //
  progressCount = 0;
  int abort=0;
  vtkIdType progressInterval = numCells/20 + 1;

  // First insert all points lines in output and 3D geometry in hash.
  // Save 2D geometry for second pass.
  // initialize the pointer to the cells for fast traversal.
  cellPointer = input->GetCells()->GetPointer();
  for(cellId=0; cellId < numCells && !abort; cellId++)
    {
    //Progress and abort method support
    if ( progressCount >= progressInterval )
      {
      vtkDebugMacro(<<"Process cell #" << cellId);
      this->UpdateProgress (static_cast<double>(cellId)/numCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
      }
    progressCount++;

    // Direct access to cells.
    cellType = cellTypes[cellId];
    numCellPts = cellPointer[0];
    ids = cellPointer+1;
    // Move to the next cell.
    cellPointer += (1 + *cellPointer);

    // A couple of common cases to see if things go faster.
    if (cellType == VTK_VERTEX || cellType == VTK_POLY_VERTEX)
      {
      // Do nothing.  This case was handled in the previous loop.
      }
    else if (cellType == VTK_LINE || cellType == VTK_POLY_LINE)
      {
      newLines->InsertNextCell(numCellPts);
      for (i = 0; i < numCellPts; ++i)
        {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
        newLines->InsertCellPoint(outPtId);
        }
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    else if (cellType == VTK_HEXAHEDRON)
      {
      this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
      this->InsertQuadInHash(ids[0], ids[3], ids[2], ids[1], cellId);
      this->InsertQuadInHash(ids[0], ids[4], ids[7], ids[3], cellId);
      this->InsertQuadInHash(ids[1], ids[2], ids[6], ids[5], cellId);
      this->InsertQuadInHash(ids[2], ids[3], ids[7], ids[6], cellId);
      this->InsertQuadInHash(ids[4], ids[5], ids[6], ids[7], cellId);
      }
    else if (cellType == VTK_VOXEL)
      {
      this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
      this->InsertQuadInHash(ids[0], ids[2], ids[3], ids[1], cellId);
      this->InsertQuadInHash(ids[0], ids[4], ids[6], ids[2], cellId);
      this->InsertQuadInHash(ids[1], ids[3], ids[7], ids[5], cellId);
      this->InsertQuadInHash(ids[2], ids[6], ids[7], ids[3], cellId);
      this->InsertQuadInHash(ids[4], ids[5], ids[7], ids[6], cellId);
      }
    else if (cellType == VTK_TETRA)
      {
      this->InsertTriInHash(ids[0], ids[1], ids[3], cellId);
      this->InsertTriInHash(ids[0], ids[2], ids[1], cellId);
      this->InsertTriInHash(ids[0], ids[3], ids[2], cellId);
      this->InsertTriInHash(ids[1], ids[2], ids[3], cellId);
      }
    else if (cellType == VTK_PENTAGONAL_PRISM)
      {
      // The quads :
      this->InsertQuadInHash (ids[0], ids[1], ids[6], ids[5], cellId);
      this->InsertQuadInHash (ids[1], ids[2], ids[7], ids[6], cellId);
      this->InsertQuadInHash (ids[2], ids[3], ids[8], ids[7], cellId);
      this->InsertQuadInHash (ids[3], ids[4], ids[9], ids[8], cellId);
      this->InsertQuadInHash (ids[4], ids[0], ids[5], ids[9], cellId);
      this->InsertPolygonInHash(ids, 5, cellId);
      this->InsertPolygonInHash(&ids[5], 5, cellId);
      }
    else if (cellType == VTK_HEXAGONAL_PRISM)
      {
      // The quads :
      this->InsertQuadInHash(ids[0], ids[1], ids[7], ids[6], cellId);
      this->InsertQuadInHash(ids[1], ids[2], ids[8], ids[7], cellId);
      this->InsertQuadInHash(ids[2], ids[3], ids[9], ids[8], cellId);
      this->InsertQuadInHash(ids[3], ids[4], ids[10], ids[9], cellId);
      this->InsertQuadInHash(ids[4], ids[5], ids[11], ids[10], cellId);
      this->InsertQuadInHash(ids[5], ids[0], ids[6], ids[11], cellId);
      this->InsertPolygonInHash (ids, 6, cellId);
      this->InsertPolygonInHash (&ids[6], 6, cellId);
      }
    else if (cellType == VTK_PIXEL || cellType == VTK_QUAD ||
             cellType == VTK_TRIANGLE || cellType == VTK_POLYGON ||
             cellType == VTK_TRIANGLE_STRIP ||
             cellType == VTK_QUADRATIC_TRIANGLE ||
             cellType == VTK_QUADRATIC_QUAD ||
             cellType == VTK_QUADRATIC_LINEAR_QUAD ||
             cellType == VTK_BIQUADRATIC_QUAD )
      { // save 2D cells for second pass
      flag2D = 1;
      }
    else
      // Default way of getting faces. Differentiates between linear
      // and higher order cells.
      {
      input->GetCell(cellId,cell);
      if ( cell->IsLinear() )
        {
        if (cell->GetCellDimension() == 3)
          {
          int numFaces = cell->GetNumberOfFaces();
          for (j=0; j < numFaces; j++)
            {
            face = cell->GetFace(j);
            numFacePts = face->GetNumberOfPoints();
            if (numFacePts == 4)
              {
              this->InsertQuadInHash(face->PointIds->GetId(0),
                                     face->PointIds->GetId(1),
                                     face->PointIds->GetId(2),
                                     face->PointIds->GetId(3), cellId);
              }
            else if (numFacePts == 3)
              {
              this->InsertTriInHash(face->PointIds->GetId(0),
                                    face->PointIds->GetId(1),
                                    face->PointIds->GetId(2), cellId);
              }
            else
              {
              this->InsertPolygonInHash(face->PointIds->GetPointer(0),
                                        face->PointIds->GetNumberOfIds(),
                                        cellId);
              }
            } // for all cell faces
          } // if 3D
        else
          {
          vtkDebugMacro("Missing cell type.");
          }
        } // a linear cell type

      else //process nonlinear cells via triangulation
        {
        if ( cell->GetCellDimension() == 1 )
          {
          cell->Triangulate(0,pts,coords);
          for (i=0; i < pts->GetNumberOfIds(); i+=2)
            {
            newLines->InsertNextCell(2);
            inPtId = pts->GetId(i);
            this->RecordOrigCellId(this->NumberOfNewCells, cellId);
            outputCD->CopyData( cd, cellId, this->NumberOfNewCells++ );
            outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD); 
            newLines->InsertCellPoint(outPtId);
            inPtId = pts->GetId(i+1);
            outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD); 
            newLines->InsertCellPoint(outPtId);
            }
          }
        else if ( cell->GetCellDimension() == 2 )
          {
          vtkWarningMacro(<< "2-D nonlinear cells must be processed with all other 2-D cells.");
          } 
        else //3D nonlinear cell
          {
          vtkIdList *cellIds = vtkIdList::New();
          int numFaces = cell->GetNumberOfFaces();
          for (j=0; j < numFaces; j++)
            {
            face = cell->GetFace(j);
            input->GetCellNeighbors(cellId, face->PointIds, cellIds);
            if ( cellIds->GetNumberOfIds() <= 0)
              {
              // FIXME: Face could not be consistent. vtkOrderedTriangulator is a better option
              face->Triangulate(0,pts,coords);
              for (i=0; i < pts->GetNumberOfIds(); i+=3)
                {
                this->InsertTriInHash(pts->GetId(i), pts->GetId(i+1),
                                      pts->GetId(i+2), cellId);
                }
              }
            }
          cellIds->Delete();
          } //3d cell
        } //nonlinear cell
      } // Cell type else.
    } // for all cells.
  
  // It would be possible to add these (except for polygons with 5+ sides)
  // to the hashes.  Alternatively, the higher order 2d cells could be handled
  // in the following loop.

  // Now insert 2DCells.  Because of poly datas (cell data) ordering,
  // the 2D cells have to come after points and lines.
  // initialize the pointer to the cells for fast traversal.
  cellPointer = input->GetCells()->GetPointer();
  for(cellId=0; cellId < numCells && !abort && flag2D; cellId++)
    {  
    // Direct acces to cells.
    cellType = input->GetCellType(cellId);
    numCellPts = cellPointer[0];
    ids = cellPointer+1;
    // Move to the next cell.
    cellPointer += (1 + *cellPointer);

    // A couple of common cases to see if things go faster.
    if (cellType == VTK_PIXEL)
      { // Do we really want to insert the 2D cells into a hash?
      pts->Reset();
      pts->InsertId(0, this->GetOutputPointId(ids[0], input, newPts, outputPD));
      pts->InsertId(1, this->GetOutputPointId(ids[1], input, newPts, outputPD));
      pts->InsertId(2, this->GetOutputPointId(ids[3], input, newPts, outputPD));
      pts->InsertId(3, this->GetOutputPointId(ids[2], input, newPts, outputPD));
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    else if (cellType == VTK_POLYGON || cellType == VTK_TRIANGLE || cellType == VTK_QUAD)
      {
      pts->Reset();
      for ( i=0; i < numCellPts; i++)
        {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD); 
        pts->InsertId(i, outPtId);
        }
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    else if (cellType == VTK_TRIANGLE_STRIP)
      {
      // Change strips to triangles so we do not have to worry about order.
      int toggle = 0;
      vtkIdType ptIds[3];
      // This check is not really necessary.  It was put here because of another (now fixed) bug.
      if (numCellPts > 1)
        {
        ptIds[0] = this->GetOutputPointId(ids[0], input, newPts, outputPD);
        ptIds[1] = this->GetOutputPointId(ids[1], input, newPts, outputPD);
        for (i = 2; i < numCellPts; ++i)
          {
          ptIds[2] = this->GetOutputPointId(ids[i], input, newPts, outputPD);
          newPolys->InsertNextCell(3, ptIds);
          this->RecordOrigCellId(this->NumberOfNewCells, cellId);
          outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
          ptIds[toggle] = ptIds[2];
          toggle = !toggle;
          }
        }
      }
    else if ( cellType == VTK_QUADRATIC_TRIANGLE
           || cellType == VTK_QUADRATIC_QUAD
           || cellType == VTK_BIQUADRATIC_QUAD
           || cellType == VTK_QUADRATIC_LINEAR_QUAD)
      {
      input->GetCell( cellId, cell );
      cell->Triangulate( 0, pts, coords );
      for ( i=0; i < pts->GetNumberOfIds(); i+=3 )
        {
        outPts[0] = this->GetOutputPointId( pts->GetId(i),   input, newPts, outputPD );
        outPts[1] = this->GetOutputPointId( pts->GetId(i+1), input, newPts, outputPD );
        outPts[2] = this->GetOutputPointId( pts->GetId(i+2), input, newPts, outputPD );
        newPolys->InsertNextCell( 3, outPts );
        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        }
      }
    } // for all cells.


  // Now transfer geometry from hash to output (only triangles and quads).
  this->InitQuadHashTraversal();
  while ( (q = this->GetNextVisibleQuadFromHash()) )
    {
    // handle all polys
    for (i = 0; i < q->numPts; i++)
      {
      q->ptArray[i] = this->GetOutputPointId(q->ptArray[i], input, newPts, outputPD);
      }
    newPolys->InsertNextCell(q->numPts, q->ptArray);
    this->RecordOrigCellId(this->NumberOfNewCells, q->SourceId);
    outputCD->CopyData(inputCD, q->SourceId, this->NumberOfNewCells++);
    }

  if (this->PassThroughCellIds)
    {
    outputCD->AddArray(this->OriginalCellIds);
    }
  if (this->PassThroughPointIds)
    {
    outputPD->AddArray(this->OriginalPointIds);
    }

  // Update ourselves and release memory
  //
  cell->Delete();
  coords->Delete();
  pts->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
  if (newVerts->GetNumberOfCells() > 0)
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();
  newVerts = NULL;
  if (newLines->GetNumberOfCells() > 0)
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  //free storage
  output->Squeeze();
  if (this->OriginalCellIds != NULL)
    {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = NULL;
    }
  if (this->OriginalPointIds != NULL)
    {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = NULL;
    }
  int ghostLevels = output->GetUpdateGhostLevel();
  if (this->PieceInvariant)
    {
    output->RemoveGhostCells(ghostLevels+1);
    }

  this->DeleteQuadHash();

  return 1;
}

//----------------------------------------------------------------------------
void vtkFaceHash::InitFastGeomQuadAllocation(vtkIdType numberOfCells)
{
  int idx;

  this->DeleteAllFastGeomQuads();
  // Allocate 100 pointers to arrays.
  // This should be plenty (unless we have triangle strips) ...
  this->NumberOfFastGeomQuadArrays = 100;
  this->FastGeomQuadArrays = new unsigned char*[this->NumberOfFastGeomQuadArrays];
  // Initalize all to NULL;
  for (idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
    {
    this->FastGeomQuadArrays[idx] = NULL;
    }
  // Set pointer to the begining.
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;

  // size the chunks based on the size of a quadrilateral
  int quadSize = sizeofFastQuad(4);

  // Lets keep the chunk size relatively small.
  if (numberOfCells < 100)
    {
    this->FastGeomQuadArrayLength = 50 * quadSize;
    }
  else
    {
    this->FastGeomQuadArrayLength = (numberOfCells / 2) * quadSize;
    }
}

//----------------------------------------------------------------------------
void vtkFaceHash::DeleteAllFastGeomQuads()
{
  int idx;

  for (idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
    {
    if (this->FastGeomQuadArrays[idx])
      {
      delete [] this->FastGeomQuadArrays[idx];
      this->FastGeomQuadArrays[idx] = NULL;
      }
    }
  if (this->FastGeomQuadArrays)
    {
    delete [] this->FastGeomQuadArrays;
    this->FastGeomQuadArrays = NULL;
    }
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;
}

//----------------------------------------------------------------------------
vtkFastGeomQuad* vtkFaceHash::NewFastGeomQuad(int numPts)
{
  if (this->FastGeomQuadArrayLength == 0)
    {
    vtkErrorMacro("Face hash allocation has not been initialized.");
    return NULL;
    }
 
  // see if there's room for this one 
  int polySize = sizeofFastQuad(numPts);
  if(this->NextQuadIndex + polySize > this->FastGeomQuadArrayLength)
    {
    ++(this->NextArrayIndex);
    this->NextQuadIndex = 0;
    }
  
  // Although this should not happen often, check first.
  if (this->NextArrayIndex >= this->NumberOfFastGeomQuadArrays)
    {
    int idx, num;
    unsigned char** newArrays;
    num = this->NumberOfFastGeomQuadArrays * 2;
    newArrays = new unsigned char*[num];
    for (idx = 0; idx < num; ++idx)
      {
      newArrays[idx] = NULL;
      if (idx < this->NumberOfFastGeomQuadArrays)
        {
        newArrays[idx] = this->FastGeomQuadArrays[idx];
        }
      }
    delete [] this->FastGeomQuadArrays;
    this->FastGeomQuadArrays = newArrays;
    this->NumberOfFastGeomQuadArrays = num;
    }

  // Next: allocate a new array if necessary.
  if (this->FastGeomQuadArrays[this->NextArrayIndex] == NULL)
    {
    this->FastGeomQuadArrays[this->NextArrayIndex] 
      = new unsigned char[this->FastGeomQuadArrayLength];
    }
  
  vtkFastGeomQuad* q = reinterpret_cast<vtkFastGeomQuad*>
    (this->FastGeomQuadArrays[this->NextArrayIndex] + this->NextQuadIndex);
  q->numPts = numPts;

  this->NextQuadIndex += polySize;

  return q;
}

