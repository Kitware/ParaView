/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveRepresentation - Superclass for representations that
// can stream the display of their inputs.
// .SECTION Description

#ifndef __vtkSMAdaptiveRepresentation_h
#define __vtkSMAdaptiveRepresentation_h

#include "vtkSMPVRepresentationProxy.h"

class vtkSMViewProxy;
class vtkInformation;
class vtkSMAdaptiveOutlineRepresentation;

class VTK_EXPORT vtkSMAdaptiveRepresentation : 
  public vtkSMPVRepresentationProxy
{
public:
  static vtkSMAdaptiveRepresentation* New();
  vtkTypeMacro(vtkSMAdaptiveRepresentation,
    vtkSMPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Clears the data object cache in the streaming display pipeline.
  virtual void ClearStreamCache();

  virtual bool AddToView(vtkSMViewProxy *view);

  // Description:
  // Set piece bounds visibility. This flag is considered only if
  // this->GetVisibility() == true, otherwise, cube axes is not shown.
  void SetPieceBoundsVisibility(int);
  vtkGetMacro(PieceBoundsVisibility, int);

  // Description:
  // Tells server where camera is so it can prioritize
  virtual void SetViewState(double *camera, double *frustum);

  // Description:
  // This tells the display pipeline that a new wend is starting.
  virtual void PrepareFirstPass();

  // Description:
  // This tells the display pipeline that the next piece is starting
  virtual void PrepareAnotherPass();

  // Description:
  // This tells the display pipeline to choose the next most important piece to render.
  virtual void ChooseNextPiece();

  // Description:
  // Obtain state flags that server computes about multipass render progress.
  vtkGetMacro(AllDone, int);
  vtkGetMacro(WendDone, int);
  vtkSetMacro(AllDone, int);
  vtkSetMacro(WendDone, int);

  // Description:
  // This tells the display pipeline that a render pass is done
  virtual void FinishPass();

  // Description:
  // This tells the display pipeline to split and refine some pieces.
  virtual void Refine();
  // Description:
  // This tells the display pipeline to coarsen and join some pieces.
  virtual void Coarsen();

  // Desciption:
  // Tells the rep not to allow refinement
  vtkSetMacro(Locked,int);
  vtkGetMacro(Locked,int);

  // Description:
  // Tells the rep not to refine past this resolution
  void SetMaxDepth(int maxD);
  int GetMaxDepth();

//BTX
  virtual bool UpdateRequired();
  virtual void SetViewInformation(vtkInformation*);
  virtual void SetVisibility(int visible);
  virtual void Update(vtkSMViewProxy* view);
  virtual void SetUpdateTime(double time);
  virtual void SetUseViewUpdateTime(bool);
  virtual void SetViewUpdateTime(double time);

protected:
  vtkSMAdaptiveRepresentation();
  ~vtkSMAdaptiveRepresentation();

  virtual bool EndCreateVTKObjects();
  virtual bool RemoveFromView(vtkSMViewProxy* view); 

  vtkSMAdaptiveOutlineRepresentation* PieceBoundsRepresentation;
  int PieceBoundsVisibility;

  int AllDone;
  int WendDone;
  int Locked;
private:
  vtkSMAdaptiveRepresentation(const vtkSMAdaptiveRepresentation&); // Not implemented
  void operator=(const vtkSMAdaptiveRepresentation&); // Not implemented
//ETX
};

#endif

