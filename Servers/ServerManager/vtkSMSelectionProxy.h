/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionProxy - A proxy for a vtkSelection.
// .SECTION Description
// Apart from being a proxy for a vtkSelection, this class encapsulates 
// the logic to create a vtkSelection. 

#ifndef __vtkSMSelectionProxy_h
#define __vtkSMSelectionProxy_h

#include "vtkSMProxy.h"

class vtkCollection;
class vtkSelection;
class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkSMSelectionProxy : public vtkSMProxy
{
public:
  static vtkSMSelectionProxy* New();
  vtkTypeRevisionMacro(vtkSMSelectionProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the render module in which the selection is made.
  void SetRenderModule(vtkSMRenderModuleProxy* rm);
  vtkGetObjectMacro(RenderModule, vtkSMRenderModuleProxy);

  // Description:
  // Get/Set the selection region (in display coordinates).
  vtkSetVector4Macro(Selection, int);
  vtkGetVector4Macro(Selection, int);

  // Description:
  // Get/Set the selection modes. Supported selection modes are
  // FRUSTRUM and SURFACE. Default is SURFACE.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
  void SetModeToFrustrum() { this->SetMode(FRUSTRUM); }
  void SetModeToSurface() { this->SetMode(SURFACE); }
  //BTX
  enum SelectionModes
    {
    SURFACE,
    FRUSTRUM
    };
  //ETX

  // Description:
  // Get/Set the selection type. A selection can be a GEOMETRY selection
  // or a SOURCE selection. Default is SOURCE.
  vtkSetMacro(Type, int);
  vtkGetMacro(Type, int);
  void SetTypeToGeometry() { this->SetType(GEOMETRY); }
  void SetTypeToSource() { this->SetType(SOURCE); }
  //BTX
  enum SelectionTypes
    {
    SOURCE,
    GEOMETRY
    };
  //ETX
  
  // Generates/Regenerates the selection. This method must be called explicitly
  // to create the selection. Fires vtkCommand::StartEvent and vtkCommand::EndEvent
  // at start and end of this method call.
  void UpdateSelection();

  // Fills the collection with the selected sources.
  // UpdateSelection() selection must be called explicitly to update the selection
  // before using this method.
  void GetSelectedSourceProxies(vtkCollection* collection);

  // Description:
  // This is a convenience method. Although this proxy provides a Camera subproxy,
  // whose properties define the view point for the selection, in most cases the 
  // the current render module view should be used. To simply this, this method is provided,
  // is simply copies the Camera properties from the render module over to
  // this proxy so that the two are same.
  void UpdateCameraPropertiesFromRenderModule();


  // Description:
  // Overridden to update the SelectionUpToDate flag. The flag is cleared when 
  // any of the properties of this proxy itself are modified and set when UpdateSelection
  // is called. Note that this proxy is typically a consumer of a render module proxy. However,
  // we don't clear the SelectionUpToDate flag if properties on the render module change.
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

  // Description:
  // Get if selection has been updated since the properties of this proxy
  // last changed.
  vtkGetMacro(SelectionUpToDate, int);
protected:
  vtkSMSelectionProxy();
  ~vtkSMSelectionProxy();

  // Overridden to avoid creation of the camera subproxy, since
  // the camera subproxy is merely used for the data.
  virtual void CreateVTKObjects(int numObjects);

  int Selection[4];
  int Mode;
  int Type;
  vtkSMRenderModuleProxy* RenderModule;

  // Description:
  // The method creates a surface selection for the surfaces visible within
  // the specified region. The region must be specified in display coordinates.
  // It returns a new selection proxy
  // that represents the selection on the data server. This selection proxy 
  // represents a geometry selection.
  void SelectOnSurface(int in_rect[4],
    vtkSMRenderModuleProxy* renderModule, vtkSMProxy* geomSelectionProxy);

  // Description:
  // This method take a on-surface selection and sets it up to include the
  // grometry proxy IDs and original source proxy IDs. This is essential
  // since a selection must have these two IDs however on-surface selection
  // generates a selection object which doesn't have any other information
  // execept that of the selection prop IDs.
  void ConvertSelection(vtkSelection* sel, vtkSMRenderModuleProxy* rmp);

  // Description:
  // This method sends the client side selection to the server side objects represented
  // by the proxy. This is used to send the client-side selection obtained on surface 
  // selection over to the data server.
  void SendSelection(vtkSelection* sel, vtkSMProxy* selProxy);


  // Description:
  // This method creates a selection that includes everything with
  // the frustrum indicated by the display rectangle. The rectangle must
  // be specified in display coordinates. 
  void SelectInFrustrum(int display_rectangle[4], 
    vtkSMRenderModuleProxy* renderModule, vtkSMProxy* geomSelectionProxy);

  // Description
  // Convers a geometry selection to a source selection.
  void ConvertGeometrySelectionToSource(
    vtkSMProxy* geomSelectionProxy, vtkSMProxy* sourceSelectionProxy);

  // Description:
  // Given a sel, fills up the collection with proxies that are selected
  // in the selection.
  void FillSources(vtkSelection* sel, vtkSMRenderModuleProxy* rmp, vtkCollection*);

  // Description:
  // This is the client side selection object. This is maintained so that
  // the GetSelectedSourceProxies() query can be answered. In FRUSTRUM mode,
  // this selection object is fetched from the server on first
  // GetSelectedSourceProxies() call. For SURFACE mode, the selection
  // yields a client-side selection object.
  vtkSelection* ClientSideSelection;
  void SetClientSideSelection(vtkSelection*);

  void UpdateRenderModuleCamera(bool reset);

  // Flag used to determine if the selection is up-to-date when this
  // proxy's properties.
  int SelectionUpToDate;
private:
  vtkSMSelectionProxy(const vtkSMSelectionProxy&); // Not implemented.
  void operator=(const vtkSMSelectionProxy&); // Not implemented.
};

#endif

