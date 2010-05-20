/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSurfaceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSurfaceRepresentationProxy - representation that can be used to
// show a 3D surface in a render view.
// .SECTION Description
// vtkSMSurfaceRepresentationProxy is a concrete representation that can be used
// to render the surface in a vtkSMRenderViewProxy. It uses a
// vtkPVGeometryFilter to convert non-polydata input to polydata that can be
// rendered. It supports rendering the data as a surface, wireframe or points.

#ifndef __vtkSMSurfaceRepresentationProxy_h
#define __vtkSMSurfaceRepresentationProxy_h

#include "vtkSMPropRepresentationProxy.h"

// Making this a #define to match those in vtkProperty.
#define VTK_SURFACE_WITH_EDGES 3

class VTK_EXPORT vtkSMSurfaceRepresentationProxy : 
  public vtkSMPropRepresentationProxy
{
public:
  static vtkSMSurfaceRepresentationProxy* New();
  vtkTypeMacro(vtkSMSurfaceRepresentationProxy, vtkSMPropRepresentationProxy);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Views typically support a mechanism to create a selection in the view
  // itself, eg. by click-and-dragging over a region in the view. The view
  // passes this selection to each of the representations and asks them to
  // convert it to a proxy for a selection which can be set on the view. 
  // It a representation does not support selection creation, it should simply
  // return NULL. This method returns a new vtkSMProxy instance which the 
  // caller must free after use.
  // This implementation converts a prop selection to a selection source.
  virtual vtkSMProxy* ConvertSelection(vtkSelection* input);

  // Description:
  // Returns true is opactity < 1.0
  virtual bool GetOrderedCompositingNeeded();

  // Description:
  // Set the scalar coloring mode
  virtual void SetColorAttributeType(int type);

  // Description:
  // Set the scalar color array name. If array name is 0 or "" then scalar
  // coloring is disabled.
  virtual void SetColorArrayName(const char* name);

  // Description:
  // Set the ambient coefficient. This is used only when representation type is
  // Surface.
  void SetAmbient(double a)
    {
    if (this->Ambient != a)
      {
      this->Ambient = a;
      this->UpdateShadingParameters();
      this->Modified();
      }
    }

  // Description:
  // Set the diffuse coefficient. This is used only when representation type is
  // Surface.
  void SetDiffuse(double d)
    {
    if (this->Diffuse != d)
      {
      this->Diffuse = d;
      this->UpdateShadingParameters();
      this->Modified();
      }
    }

  // Description:
  // Set the specular coefficient. This is used only when representation type is
  // Surface.
  void SetSpecular(double d)
    {
    if (this->Specular != d)
      {
      this->Specular = d;
      this->UpdateShadingParameters();
      this->Modified();
      }
    }

  // Description:
  // Set the representation type.
  // repr can be VTK_SURFACE or VTK_WIREFRAME or VTK_POINTS or
  // VTK_SURFACE_WITH_EDGES.
  virtual void SetRepresentation(int repr);

  // Description:
  // Returns the proxy for the prop.
  vtkGetObjectMacro(Prop3D, vtkSMProxy);

  // Description:
  // Returns the proxy for the vtkProperty.
  virtual vtkSMProxy *GetPropertyProxy() 
    { return this->Property; }

  // Description:
  // HACK: vtkSMAnimationSceneGeometryWriter needs acces to the processed data
  // so save out. This method should return the proxy that goes in as the input
  // to strategies (eg. in case of SurfaceRepresentation, it is the geometry
  // filter).
  virtual vtkSMProxy* GetProcessedConsumer()
    { return (vtkSMProxy*)(this->GeometryFilter); }

  // Description:
  // Check if this representation has the prop by checking its vtkClientServerID
  virtual bool HasVisibleProp3D(vtkProp3D* prop);

  // Description:
  // Called to set the view information object.
  // Don't call this directly, it is called by the View.
  // Overridden to add modification observer.
  virtual void SetViewInformation(vtkInformation*);

  // Description:
  // SuppressLOD controls whether this representation will use LOD
  // when asked by the render view. It can be used to disable LOD of
  // selected representations. The default is 0.
  vtkSetMacro(SuppressLOD, int);
  vtkGetMacro(SuppressLOD, int);

  // Description:
  // Get the bounds and transform according to rotation, translation, and scaling.
  // Returns true if the bounds are "valid" (and false otherwise)
  virtual bool GetBounds(double bounds[6]);

//BTX
protected:
  vtkSMSurfaceRepresentationProxy();
  ~vtkSMSurfaceRepresentationProxy();

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
  // Given a surface selection for this representation, this returns a new
  // vtkSelection for the selected cells/points in the input of this
  // representation.
  virtual void ConvertSurfaceSelectionToVolumeSelection(
   vtkSelection* input, vtkSelection* output);

  // Description:
  // Internal method to update actual diffuse/specular/ambient coefficients used
  // based on the representation.
  virtual void UpdateShadingParameters();

  vtkSMSourceProxy* GeometryFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;

  double Ambient;
  double Diffuse;
  double Specular;

  int Representation;

  int SuppressLOD;
private:
  vtkSMSurfaceRepresentationProxy(const vtkSMSurfaceRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSurfaceRepresentationProxy&); // Not implemented

  void ProcessViewInformation();
  vtkCommand* ViewInformationObserver;
//ETX
};

#endif

