/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPointLabelDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPointLabelDisplay - Collect the pick data.
// .SECTION Description
// This class takes an input and collects the data for display in the UI.
// It is responsible for displaying the labels on the points.

#ifndef __vtkSMPointLabelDisplay_h
#define __vtkSMPointLabelDisplay_h


#include "vtkSMDisplay.h"

class vtkDataSet;
class vtkPVProcessModule;
class vtkPVDataInformation;
class vtkProp;
class vtkSMProxy;
class vtkProperty;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;


class VTK_EXPORT vtkSMPointLabelDisplay : public vtkSMDisplay
{
public:
  static vtkSMPointLabelDisplay* New();
  vtkTypeRevisionMacro(vtkSMPointLabelDisplay, vtkSMDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Not implemented yet.  Client still does this.
  virtual void AddToRenderer(vtkPVRenderModule*);
  virtual void RemoveFromRenderer(vtkPVRenderModule*);
//ETX


  // Description:
  // Connects the parts data to the plot actor.
  // All point data arrays are ploted for now.
  // Data must already be updated.
  virtual void SetInput(vtkSMSourceProxy* input);

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

  //BTX
  // Description:
  // The Probe needs access to this to fill in the UI point values.
  vtkUnstructuredGrid *GetCollectedData();
  //ETX

protected:
  vtkSMPointLabelDisplay();
  ~vtkSMPointLabelDisplay();
  
  virtual void RemoveAllCaches();

  // I might get rid of this reference.
  //vtkSMSourceProxy* Source;
  vtkPVProcessModule *ProcessModule;

  int GeometryIsValid;
  int Visibility;

  vtkSMProxy* UpdateSuppressorProxy;
  vtkSMProxy* DuplicateProxy;
  vtkSMProxy* PointLabelMapperProxy;
  vtkSMProxy* PointLabelActorProxy;

  // This method gets called by SetProcessModule.
  virtual void CreateVTKObjects(int num);

  vtkSMPointLabelDisplay(const vtkSMPointLabelDisplay&); // Not implemented
  void operator=(const vtkSMPointLabelDisplay&); // Not implemented
};

#endif
