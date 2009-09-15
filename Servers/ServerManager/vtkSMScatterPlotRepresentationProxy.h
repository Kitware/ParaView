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

#include "vtkSMDataRepresentationProxy.h"
#include "vtkStdString.h" // needed for vtkStdString.
#include <vector>

class vtkSMScatterPlotViewProxy;

class VTK_EXPORT vtkSMScatterPlotRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMScatterPlotRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMScatterPlotRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }


  virtual void SetViewInformation(vtkInformation* info);
  
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
  // Get the bounds for the representation.  Returns true if successful.
  // Default implementation returns non-transformed bounds.
  // Reimplemented to use the Mapper bounds instead of the input bounds
  virtual bool GetBounds(double bounds[6]);
  
  // Description:
  // Update the CubeAxesActor with the exact bounds
  virtual void Update() { this->Superclass::Update(); }
  virtual void Update(vtkSMViewProxy* view);
  
  // Description:
  // Visibility controls the visibility of the representation AND the
  // CubeAxes actor
  virtual void SetVisibility(int visible);
  
  // Description:
  // Turn On/Off the visibility of the CubeAxes actor
  virtual void SetCubeAxesVisibility(int visible);

  // Description:
  // Set the X-Axis array to the ScatterPlot Mapper
  void SetXAxisArrayName(const char* name);

  // Description:
  // Set the Y-Axis array to the ScatterPlot Mapper
  void SetYAxisArrayName(const char* name);
  
  // Description:
  // Set the Z-Axis array to the ScatterPlot Mapper
  void SetZAxisArrayName(const char* name);

  // Description:
  // Set the Color array to the ScatterPlot Mapper
  void SetColorArrayName(const char* name);

  // Description:
  // Set the Glyph Scaling array to the ScatterPlot Mapper
  void SetGlyphScalingArrayName(const char* name);
  
  // Description:
  // Set the Glyph Source array to the ScatterPlot Mapper
  void SetGlyphMultiSourceArrayName(const char* name);

  // Description:
  // Set the Glyph Orientation array to the ScatterPlot Mapper
  void SetGlyphOrientationArrayName(const char* name);

  // Description:
  // Get the number of series in this representation
  int GetNumberOfSeries();

  // Description:
  // Get the name of the series with the given index. Returns "" is the index
  // is out of range.
  vtkStdString GetSeriesName(int series);
  
  // Description:
  // Returns the type of the series. Possible output
  // vtkDataObject::FIELD_ASSOCIATION_POINTS if series is a Point field array
  // vtkDataObject::FIELD_ASSOCIATION_CELLS if series is a Cell field array
  // vtkDataObject::NUMBER_OF_ASSOCIATIONS if series is a coordinate array or invalid
  int GetSeriesType(int series);
  
  // Description:
  // Returns the number of components of the series.
  int GetSeriesNumberOfComponents(int series);

//BTX
protected:
  // Description:
  // Protected constructor. Call vtkSMScatterPlotRepresentationProxy::New() to 
  // create an instance of vtkSMScatterPlotRepresentationProxy.
  vtkSMScatterPlotRepresentationProxy();
  
  // Description:
  // Protected destructor.
  virtual ~vtkSMScatterPlotRepresentationProxy();

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

  // Description:
  // Utility function that set arrayName to the id array of the mapper
  void SetArray(int id, const char* arrayName);

  vtkSMSourceProxy* FlattenFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;
  
  vtkSMProxy* CubeAxesActor;
  vtkSMProxy* CubeAxesProperty;
  int         CubeAxesVisibility;

  vtkSMSourceProxy* GlyphInput;
  unsigned int      GlyphOutputPort;
  
  struct vtkInternal;
  vtkInternal* Internal;
private:
  vtkSMScatterPlotRepresentationProxy(const vtkSMScatterPlotRepresentationProxy&); // Not implemented
  void operator=(const vtkSMScatterPlotRepresentationProxy&); // Not implemented
//ETX
};

#endif

