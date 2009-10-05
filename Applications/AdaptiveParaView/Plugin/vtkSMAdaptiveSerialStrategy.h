/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveSerialStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveSerialStrategy
// .SECTION Description
//

#ifndef __vtkSMAdaptiveSerialStrategy_h
#define __vtkSMAdaptiveSerialStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMAdaptiveSerialStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMAdaptiveSerialStrategy* New();
  vtkTypeRevisionMacro(vtkSMAdaptiveSerialStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tells the strategy where the camera is so that pieces can be sorted and rejected
  virtual void SetViewState(double *camera, double *frustum);

  // Description:
  // Clears the data object cache in the streaming display pipeline.
  virtual void ClearStreamCache(); //TODO RENAME

  // Description:
  // Clears the data object cache in the streaming display pipeline.
  virtual void EmptyCache();

  // Description:
  // Sets up to begin rendering at the current resolution.
  virtual void PrepareFirstPass();

  // Description:
  // Sets up to begin rendering the next piece.
  virtual void PrepareAnotherPass();

  // Description:
  // Tells server to decide what piece it is going to show next.
  virtual void ChooseNextPiece();

//BTX
  // Description:
  // Asks server what piece it is going to show next.
  virtual void GetPieceInfo(int *P, int *NP, double *R, double *PRIORITY, bool *HIT, bool *APPEND);

  // Description:
  // Asks server what piece it is going to show next.
  virtual void GetStateInfo(bool *ALLDONE, bool *WENDDONE);
//ETX

  // Description:
  // Used to share current piece with piece bounds representation
  virtual void SetNextPiece(int Piece, int NumPieces, double Res, double pri, bool hit, bool append);

  // Description:
  // Tells server that a render pass is done
  virtual void FinishPass();

  // Description:
  // Tells server to split and refine some pieces.
  virtual void Refine();

  // Description:
  // Tells server to merge and join some pieces.
  virtual void Coarsen();

  // Description:
  // Sets a maximum refinement limit for this thing
  virtual void SetMaxDepth(int maxD);
  virtual void GetMaxDepth(int *maxD);

//BTX
protected:
  vtkSMAdaptiveSerialStrategy();
  ~vtkSMAdaptiveSerialStrategy();

  // Description:
  // Overridden to swap in AdaptiveUpdateSuppresors and add the PieceCache.
  virtual void BeginCreateVTKObjects();

  // Description:
  // Overridden to insert piececache in front of render pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input, int outputport);
  
  // Description:
  // Overridden to insert piececache in front of LOD render pipeline.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input, int outputport);

  // Description:
  // Overridden to gather information incrementally.
  virtual void GatherInformation(vtkPVInformation*);

  // Description:
  // Overridden to gather information incrementally.
  virtual void GatherLODInformation(vtkPVInformation*);

  // Description:
  // Overridden to clean piececache too.
  virtual void InvalidatePipeline();

  vtkSMSourceProxy* PieceCache;
  vtkSMSourceProxy* ViewSorter;
    
  int IsFinished;
private:
  vtkSMAdaptiveSerialStrategy(const vtkSMAdaptiveSerialStrategy&); // Not implemented
  void operator=(const vtkSMAdaptiveSerialStrategy&); // Not implemented
//ETX
};

#endif

