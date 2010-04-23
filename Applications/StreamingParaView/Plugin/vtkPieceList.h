/*=========================================================================

  Program:   ParaView
  Module:    vtkPieceList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPieceList - A set of vtkPieces
// .SECTION Description

#ifndef __vtkPieceList_h
#define __vtkPieceList_h

#include "vtkObject.h"

class vtkPiece;
class vtkInternals;

class VTK_EXPORT vtkPieceList : public vtkObject
{
public:
  static vtkPieceList* New();
  vtkTypeMacro(vtkPieceList, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //Description
  //Add a piece to the list.
  void AddPiece(vtkPiece *Piece);

  //Description:
  //Get the n'th piece.
  vtkPiece *GetPiece(int n);

  //Description:
  //Removes the n'th piece.
  void RemovePiece(int n);

  //Description:
  //GetPiece followed by RemovePiece.
  //WARNING Caller must eventually call Delete on the returned piece.
  vtkPiece *PopPiece(int n = 0);

  //Description:
  //Removes all of the Pieces
  void Clear();

  //Description:
  //Get the number of pieces that have been added.
  int GetNumberOfPieces();

  //Description:
  //Sort the list into decreasing Priority order (highest priority first).
  void SortPriorities();

  //Description:
  //Get the number of pieces that have non zero Priority.
  int GetNumberNonZeroPriority();

  //Description:
  //Removes all Pieces and then deep copies the pieces from other.
  void CopyPieceList(vtkPieceList *other);

  //Description:
  //Moves all entries from other to me.
  //WARNING: This does not do any sorting.
  void MergePieceList(vtkPieceList *other);
  
  //Description:
  //For debugging.
  void Print();

  void Serialize();
  void GetSerializedList(double **buffer, int *sz);
  void UnSerialize(double *buffer);

protected:
  vtkPieceList();
  ~vtkPieceList();

  vtkInternals *Internals;

  //Description:
  void CopyInternal(vtkPieceList *other, int merge);

private:
  vtkPieceList(const vtkPieceList&); // Not implemented
  void operator=(const vtkPieceList&); // Not implemented
};

#endif
