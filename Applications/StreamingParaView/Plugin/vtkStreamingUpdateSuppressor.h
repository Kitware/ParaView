/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingUpdateSuppressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamingUpdateSuppressor - 
// .SECTION Description 

#ifndef __vtkStreamingUpdateSuppressor_h
#define __vtkStreamingUpdateSuppressor_h

#include "vtkPVUpdateSuppressor.h"

class vtkPieceList;
class vtkDoubleArray;
class vtkMPIMoveData;

class VTK_EXPORT vtkStreamingUpdateSuppressor : public vtkPVUpdateSuppressor
{
public:
  vtkTypeMacro(vtkStreamingUpdateSuppressor,vtkPVUpdateSuppressor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkStreamingUpdateSuppressor *New();

  // Description:
  // Force update on the input.
  virtual void ForceUpdate();

  //Description:
  //Define the current pass and the number of passes for streaming.
  //NumberOfPasses is essentially number of pieces local to this processor
  //Pass is essentially the current Piece
  vtkSetMacro(Pass, int);
  vtkGetMacro(Pass, int);
  vtkSetMacro(NumberOfPasses, int);
  vtkGetMacro(NumberOfPasses, int);
  void SetPassNumber(int Pass, int NPasses);

  //Description:
  //Determine the piece number for a given render pass
  //if Unspecified, will use this->Pass
  int GetPiece(int Pass = -1); 

  //Description:
  //Get a hold of my PieceList pointer
  vtkGetObjectMacro(PieceList, vtkPieceList);

  //Description:
  //Sets my PieceList pointer to other (with ref counting)
  void SetPieceList(vtkPieceList *other);
  
  //Description:
  //Returns the number of pieces with non zero priorities in PieceList
  vtkGetMacro(MaxPass, int);

  //Description:
  //Computes a priority for every piece by filter characteristics
  void ComputePriorities();

  //Description:
  //Reset the piece ordering.
  void ClearPriorities();

  //Description:
  //Used to copy list from server to client
  void SerializePriorities();
  vtkDoubleArray *GetSerializedPriorities();
  void UnSerializePriorities(double *);

  //Description:
  //Set in parallel runs to ensure communication when pieces are reused.
  void SetMPIMoveData(vtkMPIMoveData *mp)
  {
    this->MPIMoveData = mp;
  }
  //Description:
  //Called in parallel runs to ensure communication when pieces are reused.
  void MarkMoveDataModified();

//BTX
protected:
  vtkStreamingUpdateSuppressor();
  ~vtkStreamingUpdateSuppressor();

  //Description:
  //Used in parallel to let all processors agree on number of pieces to process.
  void MergePriorities();

  //Description:
  //Number of passes to stream over
  int NumberOfPasses;
  
  //Description:
  //Current pass in working on
  int Pass;

  //Description:
  //The ordered list of pieces, which says which piece corresponds to which pass
  vtkPieceList *PieceList;

  //Description:
  //The number of non cullable pieces so we can quit before all NumberOfPasses
  int MaxPass;

  //A copy of the piece list which server sends to client to stay in synch.
  vtkDoubleArray *SerializedPriorities;
  
  vtkMPIMoveData *MPIMoveData;

private:
  vtkStreamingUpdateSuppressor(const vtkStreamingUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkStreamingUpdateSuppressor&);  // Not implemented.

  enum
    {
    PRIORITY_COMMUNICATION_TAG=197001
    };

  //Description:
  //For debugging, helps to identify which US is spewing out messages
  void PrintPipe(vtkAlgorithm *);
//ETX
};

#endif
