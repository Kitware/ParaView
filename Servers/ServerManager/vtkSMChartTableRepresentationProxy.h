/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartTableRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartTableRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMChartTableRepresentationProxy_h
#define __vtkSMChartTableRepresentationProxy_h

#include "vtkSMChartRepresentationProxy.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkQtChartRepresentation;
class vtkSMChartViewProxy;
class vtkSMChartNamedOptionsModelProxy;

class VTK_EXPORT vtkSMChartTableRepresentationProxy : public vtkSMChartRepresentationProxy
{
public:
  static vtkSMChartTableRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMChartTableRepresentationProxy, vtkSMChartRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the underlying VTK representation.
  vtkGetObjectMacro(VTKRepresentation, vtkQtChartRepresentation);

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

  // Description:
  // Get the number of series in this representation
  int GetNumberOfSeries();

  // Description:
  // Get the name of the series with the given index.  Returns 0 is the index
  // is out of range.  The returned pointer is only valid until the next call
  // to GetSeriesName.
  const char* GetSeriesName(int series);

  // Description:
  // Set the series to use as the X-axis.
  void SetXAxisSeriesName(const char* name);

  // Description:
  // When set, the array index will be used for X axis, otherwise the array
  // identified by XArrayName will be used.
  void SetUseIndexForXAxis(bool use_index);

//BTX
protected:
  vtkSMChartTableRepresentationProxy();
  ~vtkSMChartTableRepresentationProxy();

  virtual bool EndCreateVTKObjects();

  // Description:
  // Called by SetXAxisSeriesName and SetUseIndexForXAxis to update the
  // KeyColumn for the internal chart.
  void UpdateXSeriesName();

  vtkWeakPointer<vtkSMChartViewProxy> ChartViewProxy;
  vtkQtChartRepresentation* VTKRepresentation;
  vtkSMChartNamedOptionsModelProxy* OptionsProxy;
  int Visibility;
  bool UseIndexForXAxis;
  char* XSeriesName;
  vtkSetStringMacro(XSeriesName);

private:
  vtkSMChartTableRepresentationProxy(const vtkSMChartTableRepresentationProxy&); // Not implemented
  void operator=(const vtkSMChartTableRepresentationProxy&); // Not implemented
//ETX
};

#endif

