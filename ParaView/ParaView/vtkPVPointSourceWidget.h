/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointSourceWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPointSourceWidget -  a PointWidget which contains a separate point source
// This widget contains a vtkPointWidget as well as a vtkPointSource. 
// This vtkPointSource (which is created on all processes) can be used as 
// input or source to filters (for example as streamline seed).

#ifndef __vtkPVPointSourceWidget_h
#define __vtkPVPointSourceWidget_h

#include "vtkPVSourceWidget.h"

class vtkPVPointWidget;
class vtkPVVectorEntry;
class vtkPVWidgetProperty;

class VTK_EXPORT vtkPVPointSourceWidget : public vtkPVSourceWidget
{
public:
  static vtkPVPointSourceWidget* New();
  vtkTypeRevisionMacro(vtkPVPointSourceWidget, vtkPVSourceWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The underlying line widget.
  vtkGetObjectMacro(PointWidget, vtkPVPointWidget);

  // Description:
  // Controls the radius of the point cloud.
  vtkGetObjectMacro(RadiusWidget, vtkPVVectorEntry);

  // Description:
  // Controls the number of points in the point cloud.
  vtkGetObjectMacro(NumberOfPointsWidget, vtkPVVectorEntry);

  // Description:
  // Returns if any subwidgets are modified.
  virtual int GetModifiedFlag();

  // Description:
  // This method is called when the source that contains this widget
  // is selected.
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected.
  virtual void Deselect();

  // Description:
  // Create the point source in the VTK Tcl script.
  // Savea a point source (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void AcceptInternal(vtkClientServerID);
  //ETX

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

protected:
  vtkPVPointSourceWidget();
  ~vtkPVPointSourceWidget();


  vtkPVPointWidget* PointWidget;

  static int InstanceCount;

  vtkPVVectorEntry* RadiusWidget;
  vtkPVVectorEntry* NumberOfPointsWidget;
  vtkPVWidgetProperty *RadiusProperty;
  vtkPVWidgetProperty *NumberOfPointsProperty;

  vtkPVPointSourceWidget(const vtkPVPointSourceWidget&); // Not implemented
  void operator=(const vtkPVPointSourceWidget&); // Not implemented

};

#endif
