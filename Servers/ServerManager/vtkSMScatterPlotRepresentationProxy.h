/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScatterPlotRepresentationProxy - representation to show images.
// .SECTION Description
// vtkSMScatterPlotRepresentationProxy is a concrete representation that can be used
// to render any poly data. 

#ifndef __vtkSMScatterPlotRepresentationProxy_h
#define __vtkSMScatterPlotRepresentationProxy_h

//#include "vtkSMPropRepresentationProxy.h"
#include "vtkSMDataRepresentationProxy.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkSMScatterPlotViewProxy;

class VTK_EXPORT vtkSMScatterPlotRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMScatterPlotRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMScatterPlotRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // virtual void AddInput(unsigned int inputPort,
//                         vtkSMSourceProxy* input,
//                         unsigned int outputPort,
//                         const char* method);
  
  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  // Description:
  // Set the scalar coloring mode
  void SetColorAttributeType(int type);

  // Description:
  // Set the scalar color array name. If array name is 0 or "" then scalar
  // coloring is disabled.
  //void SetColorArrayName(const char* name);

  // Description:
  // Get the bounds for the representation.  Returns true if successful.
  // Default implementation returns non-transformed bounds.
  // Overridden to take "UseXYPlane" property value into consideration.
  virtual bool GetBounds(double bounds[6]);

  virtual void Update() { this->Superclass::Update(); }
  virtual void Update(vtkSMViewProxy* view);
  
  void SetXAxisArrayName(const char* name);
  void SetYAxisArrayName(const char* name);
  void SetZAxisArrayName(const char* name);
  void SetColorArrayName(const char* name);
  void SetGlyphScalingArrayName(const char* name);
  void SetGlyphMultiSourceArrayName(const char* name);
  void SetGlyphOrientationArrayName(const char* name);
/*
  void SetThreeDMode(bool enable);
  void SetColorize(bool enable);
  void SetGlyphMode(int mode);
*/
  // Description:
  // Get the number of series in this representation
  int GetNumberOfSeries();

  // Description:
  // Get the name of the series with the given index.  Returns 0 is the index
  // is out of range.  The returned pointer is only valid until the next call
  // to GetSeriesName.
  //const char* GetSeriesName(int series);
  vtkStdString GetSeriesName(int series);

  int GetSeriesType(int series);

//BTX
protected:
  vtkSMScatterPlotRepresentationProxy();
  ~vtkSMScatterPlotRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  void SetArray(int array, const char* arrayName);

  //vtkSMProxy* GeometryFilter;
  vtkSMSourceProxy* FlattenFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;
  vtkSMScatterPlotViewProxy* ScatterPlotView;

private:
  vtkSMScatterPlotRepresentationProxy(const vtkSMScatterPlotRepresentationProxy&); // Not implemented
  void operator=(const vtkSMScatterPlotRepresentationProxy&); // Not implemented
//ETX
};

#endif

