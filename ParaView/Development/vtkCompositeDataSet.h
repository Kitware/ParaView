/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataSet -
// .SECTION Description

#ifndef __vtkCompositeDataSet_h
#define __vtkCompositeDataSet_h

#include "vtkDataObject.h"

class vtkCompositeDataIterator;
class vtkCompositeDataVisitor;

class VTK_EXPORT vtkCompositeDataSet : public vtkDataObject
{
public:
  vtkTypeRevisionMacro(vtkCompositeDataSet,vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return a new iterator (has to be deleted by user)
  virtual vtkCompositeDataIterator* NewIterator() = 0;

  // Description:
  // This returns a vtkCompositeDataVisitor. Sub-classes can
  // return this method to return the appropriate visitor.
  virtual vtkCompositeDataVisitor* NewVisitor() = 0;

  // Description:
  // Return class name of data type (see vtkSystemIncludes.h for
  // definitions).
  virtual int GetDataObjectType() {return VTK_COMPOSITE_DATA_SET;}

  // Description:
  // Restore data object to initial state,
  virtual void Initialize();

  //------------------------------------------------------------
  // Partial dataset interface:
  //
  // Description:
  // Returns the total number of points.
  vtkIdType GetNumberOfPoints();

  // Description:
  // Returns the total number of points.
  vtkIdType GetNumberOfCells();

  // Description:
  // Returns the total bounds.
  double* GetBounds();

  // Description:
  // For streaming.  User/next filter specifies which piece the want updated.
  // The source of this data has to return exactly this piece.
  void SetUpdateExtent(int piece, int numPieces, int ghostLevel);
  void SetUpdateExtent(int piece, int numPieces)
    {this->SetUpdateExtent(piece, numPieces, 0);}
  void GetUpdateExtent(int &piece, int &numPieces, int &ghostLevel);

  // Description:
  // Call superclass method to avoid hiding
  // Since this data type does not use 3D extents, this set method
  // is useless but necessary since vtkDataSetToDataSetFilter does not
  // know what type of data it is working on.
  void SetUpdateExtent( int x1, int x2, int y1, int y2, int z1, int z2 )
    { this->Superclass::SetUpdateExtent( x1, x2, y1, y2, z1, z2 ); };
  void SetUpdateExtent( int ext[6] )
    { this->Superclass::SetUpdateExtent( ext ); };

protected:
  vtkCompositeDataSet();
  ~vtkCompositeDataSet();

  // Description:
  // Compute these only when necessary (uses mtime)
  void ComputeNumberOfPoints();
  void ComputeNumberOfCells();
  void ComputeBounds();

  double Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfCells;
  vtkTimeStamp ComputeBoundsTime; // Time at which bounds, center, etc. computed
  vtkTimeStamp ComputeNPtsTime;
  vtkTimeStamp ComputeNCellsTime;

private:
  vtkCompositeDataSet(const vtkCompositeDataSet&);  // Not implemented.
  void operator=(const vtkCompositeDataSet&);  // Not implemented.
};

#endif

