/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlotDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPlotDisplay - Superclass for XYPlot.
// .SECTION Description
// This class takes an input and displays a XYplot in the 2D renderer.
// For now, the display ignores geomery and creates a polyline from the 
// points in the order they appear in the input data.

#ifndef __vtkPVPlotDisplay_h
#define __vtkPVPlotDisplay_h


#include "vtkPVDisplay.h"

#include "vtkClientServerID.h" // Needed for PropID ...

class vtkDataSet;
class vtkPVApplication;
class vtkPVDataInformation;
class vtkPolyDataMapper;
class vtkProp;
class vtkProperty;
class vtkPVPart;
class vtkPVColorMap;

class VTK_EXPORT vtkPVPlotDisplay : public vtkPVDisplay
{
public:
  static vtkPVPlotDisplay* New();
  vtkTypeRevisionMacro(vtkPVPlotDisplay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkPVPart* input);

  // Description:
  // Turns visibility on or off.
  virtual void SetVisibility(int v);
  vtkGetMacro(Visibility,int);

  // Description:
  // This also creates the vtk objects for the composite.
  // (actor, mapper, ...)
  virtual void SetPVApplication(vtkPVApplication *pvApp);
  vtkGetObjectMacro(PVApplication,vtkPVApplication);

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update();

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total);  

  // Description:
  // Not referenced counted.  I might get rid of this reference later.
  // fixme: merge this with set input. (wait until berks proxy code.)
  virtual void SetPart(vtkPVPart* part) {this->Part = part;}
  vtkPVPart* GetPart() {return this->Part;}

  // Description:
  // PVSource calls this when it gets modified.
  void InvalidateGeometry();

  // Description:
  // Needed to connect actor to 3D widget.
  vtkClientServerID GetXYPlotActorID() { return this->XYPlotActorID;}

protected:
  vtkPVPlotDisplay();
  ~vtkPVPlotDisplay();
  
  virtual void RemoveAllCaches();

  // I might get rid of this reference.
  vtkPVPart* Part;
  vtkPVApplication *PVApplication;

  int GeometryIsValid;
  int Visibility;

  vtkClientServerID DuplicatePolyDataID;
  vtkClientServerID UpdateSuppressorID;
  vtkClientServerID XYPlotActorID;

  // This method gets called by SetPVApplication.
  virtual void CreateParallelTclObjects(vtkPVApplication *pvApp);

  void HSVtoRGB(float h, float s, float v, float *r, float *g, float *b);

  vtkPVPlotDisplay(const vtkPVPlotDisplay&); // Not implemented
  void operator=(const vtkPVPlotDisplay&); // Not implemented
};

#endif
