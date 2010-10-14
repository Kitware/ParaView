/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMRepresentationProxy_h
#define __vtkSMRepresentationProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMRepresentationProxy : public vtkSMSourceProxy
{
public:
  static vtkSMRepresentationProxy* New();
  vtkTypeMacro(vtkSMRepresentationProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Calls MarkDirty() and invokes ModifiedEvent.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Returns information about the data that is finally rendered by this
  // representation.
  virtual vtkPVDataInformation* GetRepresentedDataInformation();

  // Description:
  // Calls Update() on all sources. It also creates output ports if
  // they are not already created.
  virtual void UpdatePipeline();

  // Description:
  // Calls Update() on all sources with the given time request.
  // It also creates output ports if they are not already created.
  virtual void UpdatePipeline(double time);

//BTX
protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy();

  virtual void UpdatePipelineInternal(double time, bool doTime);

  virtual void CreateVTKObjects();

  virtual void RepresentationUpdated();

  // Description:
  // Mark the data information as invalid.
  virtual void InvalidateDataInformation();

  // Overridden to restore this->Servers flag state.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

private:
  vtkSMRepresentationProxy(const vtkSMRepresentationProxy&); // Not implemented
  void operator=(const vtkSMRepresentationProxy&); // Not implemented

  bool RepresentedDataInformationValid;
  vtkPVDataInformation* RepresentedDataInformation;

  // Description:
  // When ViewTime changes, we mark all inputs modified so that they fetch the
  // updated data information.
  void ViewTimeChanged();
  friend class vtkSMViewProxy;

  friend class vtkSMComparativeViewProxy;
  void ClearMarkedModified() { this->MarkedModified = false; }
  bool MarkedModified;
//ETX
};

#endif
