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

  // Description:
  // Overridden to reset this->MarkedModified flag.
  virtual void PostUpdateData();

  // Description:
  // Called after the view updates.
  virtual void ViewUpdated(vtkSMProxy* view);

//BTX
protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy();

  // Unlike subproxies in regular proxies, subproxies in representations
  // typically represent internal representations e.g. label representation,
  // representation for selection etc. In that case, if the internal
  // representation is modified, we need to ensure that any of our consumers is
  // a consumer of all our subproxies as well.
  virtual void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy);
  virtual void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy);
  virtual void RemoveAllConsumers();

  virtual void CreateVTKObjects();
  void OnVTKRepresentationUpdated();

  virtual void UpdatePipelineInternal(double time, bool doTime);

  // Description:
  // Mark the data information as invalid.
  virtual void InvalidateDataInformation();

  // Description:
  // Overridden to restore this->Servers flag state.
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

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

  friend class vtkPVComparativeView;
  void ClearMarkedModified() { this->MarkedModified = false; }
  bool MarkedModified;
//ETX
};

#endif
