/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProbe - A class to handle the UI for vtkProbeFilter
// .SECTION Description


#ifndef __vtkPVProbe_h
#define __vtkPVProbe_h

#include "vtkPVSource.h"

class vtkKWCheckButton;
class vtkKWLabel;
class vtkKWLabel;
class vtkKWOptionMenu;
class vtkKWWidget;
class vtkXYPlotWidget;
class vtkPVPlotDisplay;
class vtkXYPlotWidgetObserver;

class VTK_EXPORT vtkPVProbe : public vtkPVSource
{
public:
  static vtkPVProbe* New();
  vtkTypeRevisionMacro(vtkPVProbe, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Methods to call when this pv source is selected/deselected
  virtual void Deselect() { this->Deselect(1); }
  virtual void Deselect(int doPackForget);

  // Description:
  // Access to the ShowXYPlotToggle from Tcl
  vtkGetObjectMacro(ShowXYPlotToggle, vtkKWCheckButton);

  //BTX
  // Description:
  // Get the XY Plot widget.
  vtkGetObjectMacro(XYPlotWidget, vtkXYPlotWidget);
  //ETX
  
  // Description:
  // This method is called when event is triggered on the XYPlotWidget.
//BTX
  virtual void ExecuteEvent(vtkObject* wdg, unsigned long event,  
                            void* calldata);
//ETX

protected:
  vtkPVProbe();
  ~vtkPVProbe();
  
  vtkPVPlotDisplay* PlotDisplay;
  
  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  vtkKWLabel *DimensionalityLabel;
  vtkKWOptionMenu *DimensionalityMenu;
  vtkKWWidget *ProbeFrame;

  vtkKWWidget *SelectedPointFrame;
  vtkKWLabel *SelectedPointLabel;
  vtkKWLabel *PointDataLabel;
  
  vtkKWCheckButton *ShowXYPlotToggle;
  
  vtkXYPlotWidget* XYPlotWidget;
  vtkXYPlotWidgetObserver* XYPlotObserver;

  int InstanceCount;
  
  vtkPVProbe(const vtkPVProbe&); // Not implemented
  void operator=(const vtkPVProbe&); // Not implemented
};

#endif
