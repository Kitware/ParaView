/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFaceHash.h


     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFaceHash - Save faces and links to cells.
// .SECTION Description
// vtkFaceHash indexes faces by it's point ids.  
// Faces have links back to the originating cells.
// API is setup for Triangles and Quads but you can use the 3 smallest 
// point ids of any polygon for faces with more than 4 points.

#ifndef __vtkFaceHash_h
#define __vtkFaceHash_h

#include "vtkObject.h"


//BTX
// Helper structure for hashing faces.
struct vtkFaceHashQuadStruct
{
  struct vtkFaceHashQuadStruct *Next;
  vtkIdType SourceId;
  int numPts;
  vtkIdType ptArray[4]; // actually a variable length array.  MUST be last
};
typedef struct vtkFaceHashQuadStruct vtkFaceHashQuad;
//ETX

class VTK_GRAPHICS_EXPORT vtkFaceHash : public vtkObject
{
public:
  static vtkFaceHash *New();
  vtkTypeMacro(vtkFaceHash,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  void Initialize(vtkIdType numberOfPoints);
  void AddFace


protected:
  vtkFaceHash();
  ~vtkFaceHash();


  void InitializeQuadHash(vtkIdType numPoints);
  void DeleteQuadHash();

  virtual void InsertQuadInHash(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d,
                        vtkIdType sourceId);
  virtual void InsertTriInHash(vtkIdType a, vtkIdType b, vtkIdType c,
                       vtkIdType sourceId);
  virtual void InsertPolygonInHash(vtkIdType* ids, int numpts,
                           vtkIdType sourceId);

  vtkFaceHashQuad **Hash;
  vtkIdType HashLength;

    
  // Better memory allocation for faces (hash)
  void InitFaceHashQuadAllocation(vtkIdType numberOfCells);
  vtkFaceHashQuad* NewFaceHashQuad(int numPts);
  void DeleteAllFaceHashQuads();
  
  // -----
  vtkIdType FaceHashQuadArrayLength;
  vtkIdType NumberOfFaceHashQuadArrays;
  unsigned char** FaceHashQuadArrays;  // store this data as an array of bytes

  // These indexes allow us to find the next available face.
  vtkIdType NextArrayIndex;
  vtkIdType NextQuadIndex;



private:
  vtkFaceHash(const vtkFaceHash&);  // Not implemented.
  void operator=(const vtkFaceHash&);  // Not implemented.
};

#endif
