/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPlotDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPlotDisplay - Superclass for XYPlot.
// .SECTION Description
// This class takes an input and displays a XYplot in the 2D renderer.
// For now, the display ignores geomery and creates a polyline from the 
// points in the order they appear in the input data.

#ifndef __vtkSMPlotDisplay_h
#define __vtkSMPlotDisplay_h


#include "vtkSMDisplay.h"


class vtkDataSet;
class vtkPVProcessModule;
class vtkPVDataInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkSMSourceProxy;
class vtkSMProxy;
class vtkPVColorMap;
class vtkPolyData;
class vtkXYPlotWidget;

class VTK_EXPORT vtkSMPlotDisplay : public vtkSMDisplay
{
public:
  static vtkSMPlotDisplay* New();
  vtkTypeRevisionMacro(vtkSMPlotDisplay, vtkSMDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkSMSourceProxy* input);

  // Description:
  // Not implemented yet.  Client still does this.
//BTX
  virtual void AddToRenderer(vtkClientServerID rendererID);
  virtual void RemoveFromRenderer(vtkClientServerID rendererID);
//ETX

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

//BTX
  // Description:
  // This also creates the vtk objects for the composite.
  // (actor, mapper, ...)
  virtual void SetProcessModule(vtkPVProcessModule *pm);
  vtkGetObjectMacro(ProcessModule,vtkPVProcessModule);
//ETX

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  // fixme: merge this with set input. (wait until berks proxy code.)
  //virtual void SetSource(vtkSMSourceProxy* source) {this->Source = source;}
  //vtkSMSourceProxy* GetSource() {return this->Source;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

  // Description:
  // Connect actor to 3D widget.  This only shows plot on the client.
  void ConnectWidgetAndActor(vtkXYPlotWidget* widget);

  // Description:
  // Callback for when the plot moves needs access to this display.
  // Maybe callback should be part of this object?
  vtkSMProxy* GetXYPlotActorProxy(){return this->XYPlotActorProxy;}

  //BTX
  // Description:
  // The Probe needs access to this to fill in the UI point values.
  vtkPolyData *GetCollectedData();
  //ETX

protected:
  vtkSMPlotDisplay();
  ~vtkSMPlotDisplay();
  
  virtual void RemoveAllCaches();

  // I might get rid of this reference.
  vtkSMSourceProxy* Source;
  vtkPVProcessModule *ProcessModule;

  int GeometryIsValid;
  int Visibility;

  vtkSMProxy* DuplicateProxy;
  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* XYPlotActorProxy;

  virtual void CreateVTKObjects(int num);

  void HSVtoRGB(float h, float s, float v, float *r, float *g, float *b);

  vtkSMPlotDisplay(const vtkSMPlotDisplay&); // Not implemented
  void operator=(const vtkSMPlotDisplay&); // Not implemented
};

#endif
