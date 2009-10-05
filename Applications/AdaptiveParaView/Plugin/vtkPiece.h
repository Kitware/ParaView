/*=========================================================================

  Program:   ParaView
  Module:    vtkPiece.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPiece - A handle for meta information about one streamed piece
// .SECTION Description

#ifndef __vtkPiece_h
#define __vtkPiece_h

#include "vtkObject.h"

class vtkBoundingBox;
class vtkDataObject;
class vtkRenderer;

class VTK_EXPORT vtkPiece : public vtkObject
{
public:
  static vtkPiece* New();
  vtkTypeRevisionMacro(vtkPiece, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(Piece, int);
  vtkGetMacro(Piece, int);
  vtkSetMacro(NumPieces, int);
  vtkGetMacro(NumPieces, int);
  vtkSetMacro(Priority, double);
  vtkGetMacro(Priority, double);
  vtkSetMacro(Resolution, double);
  vtkGetMacro(Resolution, double);

  void Serialize(double *buffer, double **nextptr);
  void UnSerialize(double *buffer, double **nextptr);

  //Decription:
  //Copies everything
  void CopyPiece(vtkPiece *other);

  //Description:
  //convenience for sorting
  bool ComparePriority(vtkPiece *other)
    {
      return this->Priority > other->Priority;
    }

protected:
  vtkPiece();
  ~vtkPiece();

  int Piece;
  int NumPieces;
  double Priority;
  double Resolution;

private:
  vtkPiece(const vtkPiece&); // Not implemented
  void operator=(const vtkPiece&); // Not implemented
};

#endif
