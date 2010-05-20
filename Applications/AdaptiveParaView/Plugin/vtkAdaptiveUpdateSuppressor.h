/*=========================================================================

  Program:   ParaView
  Module:    vtkAdaptiveUpdateSuppressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAdaptiveUpdateSuppressor - 
// .SECTION Description 

#ifndef __vtkAdaptiveUpdateSuppressor_h
#define __vtkAdaptiveUpdateSuppressor_h

#include "vtkPVUpdateSuppressor.h"

class vtkPiece;
class vtkPieceCacheFilter;
class vtkPieceList;
class vtkMPIMoveData;

class VTK_EXPORT vtkAdaptiveUpdateSuppressor : public vtkPVUpdateSuppressor
{
public:
  vtkTypeMacro(vtkAdaptiveUpdateSuppressor,vtkPVUpdateSuppressor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkAdaptiveUpdateSuppressor *New();

  // Description:
  // Force update on the input.
  virtual void ForceUpdate();

  //Description:
  //Clears everthing out, and reset to drawing fullest domain at lowest resolution
  void ClearPriorities();

  //Description:
  void PrepareFirstPass();

  //Description:
  void PrepareAnotherPass();

  //Description:
  void ChooseNextPiece();

  //Description:
  void FinishPass();

  //Description:
  void Refine();

  //Description:
  void Coarsen();

  //Description:
  //This holds the state of the adaptive resoution changing state machine
  vtkGetVector6Macro(StateInfo,int);

  //Description:
  //This holds information about the current piece to be processed
  vtkSetVector6Macro(PieceInfo,double);
  vtkGetVector6Macro(PieceInfo,double);

  //Description:
  //This filter needs access to the upstream cache so that pieces can be
  //removed from the cache whenever they are refined
  void SetCacheFilter(vtkPieceCacheFilter *pc)
  {
    this->PieceCacheFilter = pc;
  }

  //Description:
  //Set in parallel runs to ensure communication when pieces are reused.
  void SetMPIMoveData(vtkMPIMoveData *mp)
  {
    this->MPIMoveData = mp;
  }

  //Description:
  //A user specified maximum refinement tree depth
  vtkSetMacro(MaxDepth, int);
  vtkGetMacro(MaxDepth, int);

//BTX
protected:
  vtkAdaptiveUpdateSuppressor();
  ~vtkAdaptiveUpdateSuppressor();

  void Reap();

  //ask input's pipeline to compute the priority of the specified piece
  double ComputePriority(int n, int m, double res);

  //For diagnostics.
  int EnableStreamMessages;

  //parameters of refinement tree
  int Height;
  int Degree;
  int MaxSplits;
  int MaxDepth;
  
  //Description:
  //PriorityQueues of pieces
  vtkPieceList *PQ1;
  vtkPieceList *PQ2;
  vtkPieceList *ToSplit;
  vtkPieceList *NextFrame;
  vtkPieceList *ToDo;

  //returned information about what piece is being processed
  double PieceInfo[6]; //P,NP,RES,PRI,HIT, reserved

  //returned information about progress
  int StateInfo[6]; //ALLDONE, WENDDONE, reserved...

  //Description
  //Convenient access to caching filter upstream
  vtkPieceCacheFilter *PieceCacheFilter;
  //Convenient access to IPC filter upstream
  vtkMPIMoveData *MPIMoveData;

private:
  vtkAdaptiveUpdateSuppressor(const vtkAdaptiveUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkAdaptiveUpdateSuppressor&);  // Not implemented.

//ETX
};

#endif
