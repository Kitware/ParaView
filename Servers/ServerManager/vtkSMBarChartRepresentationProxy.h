/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBarChartRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMBarChartRepresentationProxy_h
#define __vtkSMBarChartRepresentationProxy_h

#include "vtkSMChartRepresentationProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkQtChartTableRepresentation;
class vtkSMBarChartViewProxy;

class VTK_EXPORT vtkSMBarChartRepresentationProxy : public vtkSMChartRepresentationProxy
{
public:
  static vtkSMBarChartRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMBarChartRepresentationProxy, vtkSMChartRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkGetObjectMacro(VTKRepresentation, vtkQtChartTableRepresentation);

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Superclass::Update(); }
  virtual void Update(vtkSMViewProxy* view);

  // Description:
  // Set visibility of the representation.
  // Don't call directly. This method must be called through properties alone.
  void SetVisibility(int visible);

//BTX
protected:
  vtkSMBarChartRepresentationProxy();
  ~vtkSMBarChartRepresentationProxy();

  vtkWeakPointer<vtkSMBarChartViewProxy> ChartViewProxy;
  vtkQtChartTableRepresentation* VTKRepresentation;
  int Visibility;
private:
  vtkSMBarChartRepresentationProxy(const vtkSMBarChartRepresentationProxy&); // Not implemented
  void operator=(const vtkSMBarChartRepresentationProxy&); // Not implemented
//ETX
};

#endif

