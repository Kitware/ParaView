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
// .NAME vtkPieceList - A Priority Queue of vtkPieces
// .SECTION Description
// This implements a collection of vtkPieces. The list can be sorted to 
// maintain the list in priority order, so that the next piece popped
// will always be the piece with the highest priority. PieceLists can 
// be merged (draining one to add to another), copied, and serialized.

#ifndef __vtkPieceList_h
#define __vtkPieceList_h

#include "vtkObject.h"

class vtkPiece;
class vtkInternals;

class VTK_EXPORT vtkPieceList : public vtkObject
{
public:
  static vtkPieceList* New();
  vtkTypeRevisionMacro(vtkPieceList, vtkObject);
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
  bool IsEmpty() {return this->GetNumberOfPieces() == 0;}

  //Description:
  //Sort the list into decreasing Priority order (highest priority first).
  void SortPriorities();

  //Description:
  //Get the number of important pieces, ie those with non zero Priority.
  int GetNumberNonZeroPriority();

  //Description:
  //Removes all of self's Pieces and then deep copies other's.
  void CopyPieceList(vtkPieceList *other);

  //Description:
  //Moves all entries from other to me.
  //WARNING: This does not do any sorting.
  void MergePieceList(vtkPieceList *other);  

  //Description:
  //Share lists over the network.
  //Call Serialize, then call GetSerialized list to access the result.
  //Call Unserialize elsewhere to recover the list from the buffer.
  void Serialize();
  void UnSerialize(double *buffer);
  void GetSerializedList(double **buffer, int *sz);

  //Description:
  //For debugging.
  void Print();

protected:
  vtkPieceList();
  ~vtkPieceList();

  vtkInternals *Internals;

  void CopyInternal(vtkPieceList *other, int merge);

private:
  vtkPieceList(const vtkPieceList&); // Not implemented
  void operator=(const vtkPieceList&); // Not implemented
};

#endif
