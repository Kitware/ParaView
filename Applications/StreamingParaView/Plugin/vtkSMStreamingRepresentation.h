/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingRepresentation - Superclass for representations that
// can stream the display of their inputs.
// .SECTION Description

#ifndef __vtkSMStreamingRepresentation_h
#define __vtkSMStreamingRepresentation_h

#include "vtkSMPVRepresentationProxy.h"

class vtkSMViewProxy;
class vtkInformation;

class VTK_EXPORT vtkSMStreamingRepresentation : 
  public vtkSMPVRepresentationProxy
{
public:
  static vtkSMStreamingRepresentation* New();
  vtkTypeMacro(vtkSMStreamingRepresentation,
    vtkSMPVRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Tells server side to work with a particular piece until further notice.
  virtual void SetPassNumber()
  { 
    this->SetPassNumber(0,0); 
  }
  virtual void SetPassNumber(int val)
  { 
    this->SetPassNumber(val,0); 
  }
  virtual void SetPassNumber(int val, int force);

  // Description:
  // Orders the pieces from most to least important.
  virtual int ComputePriorities();

  // Description:
  // Clears the data object cache in the streaming display pipeline.
  virtual void ClearStreamCache();

  virtual bool AddToView(vtkSMViewProxy *view);

  // Description:
  // Set piece bounds visibility. This flag is considered only if
  // this->GetVisibility() == true, otherwise, cube axes is not shown.
  void SetPieceBoundsVisibility(int);
  vtkGetMacro(PieceBoundsVisibility, int);

//BTX
  virtual bool UpdateRequired();
  virtual void SetViewInformation(vtkInformation*);
  virtual void SetVisibility(int visible);
  virtual void Update(vtkSMViewProxy* view);
  virtual void SetUpdateTime(double time);
  virtual void SetUseViewUpdateTime(bool);
  virtual void SetViewUpdateTime(double time);

  virtual void SetViewState(double *camera, double *frustum);

protected:
  vtkSMStreamingRepresentation();
  ~vtkSMStreamingRepresentation();

  virtual bool EndCreateVTKObjects();
  virtual bool RemoveFromView(vtkSMViewProxy* view); 

  vtkSMDataRepresentationProxy* PieceBoundsRepresentation;
  int PieceBoundsVisibility;

private:
  vtkSMStreamingRepresentation(const vtkSMStreamingRepresentation&); // Not implemented
  void operator=(const vtkSMStreamingRepresentation&); // Not implemented
//ETX
};

#endif

