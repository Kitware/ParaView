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
  vtkSetVector4Macro(ScreenRectangle, int);
  vtkGetVector4Macro(ScreenRectangle, int);

  // Description:
  // Get/Set the selected ids.
  vtkSetMacro(Ids, int);
  vtkGetMacro(Ids, int);

  // Description:
  // Get/Set the selected points.
  vtkSetVector3Macro(Points, double);
  vtkGetVector3Macro(Points, double);

  // Description:
  // Get/Set the selected thresholds.
  vtkSetVector2Macro(Thresholds, double);
  vtkGetVector2Macro(Thresholds, double);

  // Description:
  // Get/Set the selection algorithm. Supported selection algorithms are
  // ColorBuffer selection, Frustum extraction, Ids lookup, Point containment
  // lookup, and threshold extraction.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);
  void SetModeToSurface() { this->SetMode(SURFACE); }
  void SetModeToFrustum() { this->SetMode(FRUSTUM); }
  void SetModeToIds() { this->SetMode(IDS); }
  void SetModeToPoints() { this->SetMode(POINTS); }
  void SetModeToThresholds() { this->SetMode(THRESHOLDS); }
  //BTX
  enum SelectionModes
    {
    SURFACE,
    FRUSTUM,
    IDS,
    POINTS,
    THRESHOLDS
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

  int ScreenRectangle[4];
  int Ids;
  double Points[3];
  double Thresholds[2];

  int Mode;
  vtkSMRenderModuleProxy* RenderModule;

  // Description:
  // The does a color id selection to find cells drawn in the screen region 
  // bounded by ScreenRectangle. The region must be specified in display 
  // coordinates.
  // It returns a new selection proxy
  // that represents the selection on the data server. This selection proxy 
  // represents a geometry selection.
  void SelectOnSurface(
    vtkSMRenderModuleProxy* renderModule, 
    vtkSMProxy* outSel
    );

  // Description:
  // The does a frustum extraction to find all cells behind the screen region 
  // bounded by ScreenRectangle. The region must be specified in display 
  // coordinates.
  void SelectInFrustum(
    vtkSMRenderModuleProxy* renderModule, 
    vtkSMProxy* inSel,
    vtkSMProxy* outSel
    );

  // Description:
  // The does an id extraction to find just the cells matching Ids. 
  void SelectIds(
    vtkSMRenderModuleProxy* renderModule, 
    vtkSMProxy* inSel,
    vtkSMProxy* outSel
    );

  // Description:
  // The does a point extraction to find all cells that contain 
  // Points.
  void SelectPoints(
    vtkSMRenderModuleProxy* renderModule, 
    vtkSMProxy* inSel,
    vtkSMProxy* outSel
    );

  // Description:
  // The does a threshold extraction to find all cells whose data values
  // fall within the thresholds listed in Thresholds.
  void SelectThresholds(
    vtkSMRenderModuleProxy* renderModule, 
    vtkSMProxy* inSel,
    vtkSMProxy* outSel
    );

  // Description:
  // This method take a Surface selection and sets it up to include the
  // geometry proxy IDs and original source proxy IDs. This is essential
  // since a selection must have these two IDs however on-surface selection
  // generates a selection object which doesn't have any other information
  // execept that of the selection prop IDs.
  void ConvertSelection(vtkSelection* sel, vtkSMRenderModuleProxy* rmp);

  // Description:
  // This method sends the client side selection to the server side objects 
  // represented by the proxy. This is used to send the client-side color_id
  // selection results over to the data server.
  void SendSelection(vtkSelection* sel, vtkSMProxy* selProxy);

  // Description
  // Convers a geometry selection to a source selection.
  // That is it takes a selection including 2D polygonal "shell" ids and
  // converts it to one including 3D "outer edge" cell ids.
  // The outer edge cells are not necessarily voxels.
  void ConvertPolySelectionToVoxelSelection(
    vtkSMProxy* geomSelectionProxy, vtkSMProxy* sourceSelectionProxy);

  // Description:
  // Given a sel, fills up the collection with proxies that are selected
  // in the selection.
  void FillSources(vtkSelection* sel, vtkSMRenderModuleProxy* rmp, vtkCollection*);

  // Description:
  // This is the client side selection object. This is maintained so that
  // the GetSelectedSourceProxies() query can be answered. In FRUSTUM mode,
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

