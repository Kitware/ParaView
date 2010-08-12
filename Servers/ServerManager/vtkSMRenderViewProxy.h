/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderViewProxy - implementation for View that includes
// render window and renderers.
// .SECTION Description
// vtkSMRenderViewProxy is a 3D view consisting for a render window and two
// renderers: 1 for 3D geometry and 1 for overlayed 2D geometry.

#ifndef __vtkSMRenderViewProxy_h
#define __vtkSMRenderViewProxy_h

#include "vtkSMViewProxy.h"

class vtkCamera;
class vtkCollection;
class vtkImageData;
class vtkPVClientServerIdCollectionInformation;
class vtkPVGenericRenderWindowInteractor;
class vtkPVOpenGLExtensionsInformation;
class vtkRenderer;
class vtkRenderWindow;
class vtkSelection;
class vtkSMHardwareSelector;
class vtkSMRenderViewHelper;
class vtkSMRepresentationProxy;
class vtkTimerLog;

class VTK_EXPORT vtkSMRenderViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderViewProxy* New();
  vtkTypeMacro(vtkSMRenderViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Callback for the triangle strips check button
  void SetUseTriangleStrips(int val);

  // Description:
  // Keys used to specify view rendering requirements.
  static vtkInformationIntegerKey* USE_LOD();
  static vtkInformationIntegerKey* LOD_RESOLUTION();
  static vtkInformationIntegerKey* USE_COMPOSITING();
  static vtkInformationIntegerKey* USE_ORDERED_COMPOSITING();
  
  // Description:
  // Callback for the immediate mode rendering check button
  void SetUseImmediateMode(int val);

  // Description:
  // Set the LOD Threshold.
  vtkSetMacro(LODThreshold, double);
  vtkGetMacro(LODThreshold, double);

  // Description:
  // Get/Set the LOD Resolution.
  void SetLODResolution(int);
  int GetLODResolution();
   
  // Description:
  // Access to the rendering-related objects for the GUI.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(Renderer2D, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  vtkGetObjectMacro(Interactor, vtkPVGenericRenderWindowInteractor);
  vtkGetObjectMacro(ActiveCamera, vtkCamera);

  vtkGetObjectMacro(RendererProxy, vtkSMProxy);
  vtkGetObjectMacro(Renderer2DProxy, vtkSMProxy);
  vtkGetObjectMacro(RenderWindowProxy, vtkSMProxy);
  vtkGetObjectMacro(InteractorProxy, vtkSMProxy);
  vtkGetObjectMacro(CenterAxesProxy, vtkSMProxy);
  vtkGetObjectMacro(OrientationWidgetProxy, vtkSMProxy);

  // Description:
  // Convenience method to set the background color.
  void SetBackgroundColorCM(double rgb[3]);

  // Description:
  // Controls whether the render module invokes abort check events.
  vtkSetMacro(RenderInterruptsEnabled,int);
  vtkGetMacro(RenderInterruptsEnabled,int);
  vtkBooleanMacro(RenderInterruptsEnabled,int);

  // Description:
  // Compute the bounding box of all visible props.
  void ComputeVisiblePropBounds(double bounds[6]);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

  // Description:
  // Reset camera to the given bounds.
  void ResetCamera(double bds[6]);

  // Description:
  // Reset camera. Interally calls ComputeVisiblePropBounds
  // to obtain the bounds.
  void ResetCamera();
  
  // Description:
  // This method calls UpdateInformation on the Camera Proxy
  // and sets the Camera properties according to the info
  // properties.
  void SynchronizeCameraProperties();

  // Description:
  // Generate a screenshot from the render window.
  // Obsolete, use API without writer name.
  int WriteImage(const char* filename, const char* writerName, int magnification);
  int WriteImage(const char* filename, const char* writerName)
    { return this->WriteImage(filename, writerName, 1); }

  // Description:
  // Generate a screenshot from the render window.
  // The file format is determined from the extension of the file to write.
  int WriteImage(const char* filename, int magnification);

  // Description:
  // Enable/Disable the LightKit.
  void SetUseLight(int enable);

  // Description:
  // Returns the dimensions of the first node of the render server (in
  // the argument size). Returns 1 on success, 0 on failure.
  int GetServerRenderWindowSize(int size[2]);

  // Description:
  // Called when saving server manager state.
  // Overridden to SynchronizeCameraProperties before saving the properties.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);
  
  // Description:
  // Un-hide the method. See vtkSMProxy for description.
    virtual vtkPVXMLElement* SaveState(
        vtkPVXMLElement* root,
        vtkSMPropertyIterator *iter,
        int saveSubProxies)
      {
      return this->Superclass::SaveState(root,iter,saveSubProxies);
      }

  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Set or get whether offscreen rendering should be used during
  // CaptureWindow calls.
  vtkSetClampMacro(UseOffscreenRenderingForScreenshots, int, 0, 1);
  vtkBooleanMacro(UseOffscreenRenderingForScreenshots, int);
  vtkGetMacro(UseOffscreenRenderingForScreenshots, int);

  // Description:
  // Set or get whether capture should be done as
  // StillRender or InteractiveRender inside CaptureWindow calls.
  vtkSetClampMacro(UseInteractiveRenderingForSceenshots, int, 0, 1);
  vtkBooleanMacro(UseInteractiveRenderingForSceenshots, int);
  vtkGetMacro(UseInteractiveRenderingForSceenshots, int);

  // Description:
  // Switches the render window into an offscreen mode.
  void SetUseOffscreen(int offscreen);
  int GetUseOffscreen();

  // Description:
  // Enable measurement of Polygons Per Second
  vtkSetClampMacro(MeasurePolygonsPerSecond, int, 0, 1);
  vtkBooleanMacro(MeasurePolygonsPerSecond, int);
  vtkGetMacro(MeasurePolygonsPerSecond, int);
  
  // Description:
  // Reset the tracking of polygons per second
  void ResetPolygonsPerSecondResults();

  // Description:
  // Get the last/maximum/average number of polygons per second
  vtkGetMacro(LastPolygonsPerSecond, double);
  vtkGetMacro(MaximumPolygonsPerSecond, double);
  vtkGetMacro(AveragePolygonsPerSecond, double);

  // Description:
  // Checks if color depth is sufficient to support selection.
  // If not, will return 0 and any calls to SelectVisibleCells will 
  // quietly return an empty selection.
  virtual bool IsSelectionAvailable();

  //BTX
  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the 
  // error message otherwise 0.
  virtual const char* IsSelectVisibleCellsAvailable();
  virtual const char* IsSelectVisiblePointsAvailable();
  //ETX

  // Description:
  // Return a list of visible cells within the provided screen area.
  virtual vtkSelection *SelectVisibleCells(unsigned int x0, unsigned int y0, 
    unsigned int x1, unsigned int y1, int ofPoints);

  // Description:
  // Creates a surface selection. Returns if selection was successful.
  // selectedRepresentations (if non-null) is filled with the representations
  // that were selected in the process.
  // surfaceSelection (if non-null) is filled with the client-side surface
  // selection objects for the selected representations.
  // \c multiple_selections indicates if multiple representations can be
  // selected by this operation; if false, only the representation with the
  // maximum pixel count covered in the selection region will get selected.
  bool SelectOnSurface(unsigned int x0, unsigned int y0,
    unsigned int x1, unsigned int y1,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    vtkCollection* surfaceSelection=0,
    bool multiple_selections=true,
    bool ofPoints = false);

  // Description:
  // Creates a frustum selection. Returns if selection was successful.
  bool SelectFrustum(unsigned int x0, unsigned int y0,
    unsigned int x1, unsigned int y1,
    vtkCollection* selectedRepresentations,
    vtkCollection* selectionSources,
    vtkCollection* frustumSelection=0,
    bool multiple_selections=true,
    bool ofPoints = false);

  // Description:
  // Locates the representation at the given location, if any, and returns it.
  // Returns NULL, if the location does not have a valid representation visible.
  // The implementation currently uses hardware selection alone. Hence it is
  // supported only on nodes that can support hardware selection.
  vtkSMRepresentationProxy* Pick(unsigned int x, unsigned int y);

  // Description:
  // Methods called by Representation proxies to add/remove the
  // actor proxies to appropriate renderer.
  // Avoid calling these methods directly outside representation proxies.
  virtual void AddPropToRenderer(vtkSMProxy* proxy);
  virtual void AddPropToRenderer2D(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer2D(vtkSMProxy* proxy);

  // Description:
  // Returns the information object for this render module can can provide information
  // about server-side extensions supported.
  vtkPVOpenGLExtensionsInformation* GetOpenGLExtensionsInformation();

  // Description:
  // Create a default representation for the given output port of source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int opport);

  // Description:
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. When
  // user requests the creation of a view class, the application can call this
  // method on a prototype instantaiated for the requested class and the
  // determine the actual xmlname for the view to create.
  // Overridden to choose the correct type of render view.
 virtual const char* GetSuggestedViewType(vtkIdType connectionID);

  // Description:
  // Called by AddRepresentation(). Subclasses can override to add 
  // observers etc. //DDM TODO Do I have to make this public?
  virtual void AddRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Reset the camera clipping range.
  void ResetCameraClippingRange();

//BTX
protected:
  vtkSMRenderViewProxy();
  ~vtkSMRenderViewProxy();

  // Description:
  // Called at the start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects();

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int dataType);

  // Description:
  // Set the LOD decision.
  void SetUseLOD(bool use_lod);

  // Description:
  // Get whether the view module is currently using LOD.
  bool GetUseLOD();

  // Description:
  // Determines if the LOD must be used for rendering. The difference between 
  // GetUseLOD() and GetLODDecision() is that GetUseLOD() indicates if the
  // most recent render decided to use LOD or not, while GetLODDecision()
  // uses the current geometry sizes to indicate if their sum is above
  // the LOD threshold.
  virtual bool GetLODDecision();

  // Description:
  // Called to process events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);

  // Description:
  // Synchornizes the properties of the 2D and 3D renders before
  // each render.
  void SynchronizeRenderers();

  // Description:
  // Updates the position and scale for the center axes.
  void UpdateCenterAxesPositionAndScale();

  // Description:
  // Returns a new selection consisting of all the selections with the given
  // prop id in the surfaceSelection.
  vtkSelection* NewSelectionForProp(vtkSelection* surfaceSelection, 
    vtkClientServerID propId);

  // Collection of props added to the renderer.
  vtkCollection* RendererProps; 

  // Collection of props added to the renderer2D.
  vtkCollection* Renderer2DProps;
 
  vtkSMProxy* InteractorStyleProxy;
  vtkSMProxy* RendererProxy;
  vtkSMProxy* Renderer2DProxy;
  vtkSMProxy* ActiveCameraProxy;
  vtkSMProxy* RenderWindowProxy;
  vtkSMProxy* InteractorProxy;
  vtkSMProxy* LightKitProxy;
  vtkSMProxy* LightProxy;
  vtkSMProxy* CenterAxesProxy;
  vtkSMProxy* OrientationWidgetProxy;

  // Pointer to client side objects,
  // for convienience.
  vtkRenderer* Renderer2D;
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkPVGenericRenderWindowInteractor* Interactor;
  vtkCamera* ActiveCamera;
  vtkSMRenderViewHelper* RenderViewHelper;
  
  int RenderInterruptsEnabled;
  int UseTriangleStrips;
  int ForceTriStripUpdate;
  int UseImmediateMode;
  double LODThreshold;

public:  
  // Description:
  // Method called before/after Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();
  virtual void EndStillRender();

  // Description:
  // Methods called before/after Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();
  virtual void EndInteractiveRender();

  // Description:
  // Performs actual rendering. Called for both still render and interactive 
  // render.
  virtual void PerformRender();

  // Description:
  // For internal use only. 
  void CalculatePolygonsPerSecond(double time); //DDM TODO Do I have to make this public?

protected:
  vtkTimerLog *RenderTimer;
  double LastPolygonsPerSecond;
  double MaximumPolygonsPerSecond;
  double AveragePolygonsPerSecond;
  double AveragePolygonsPerSecondAccumulated;
  vtkIdType AveragePolygonsPerSecondCount;
  int MeasurePolygonsPerSecond;
  int UseOffscreenRenderingForScreenshots;
  bool LightKitAdded;
  int UseInteractiveRenderingForSceenshots;

  // Description:
  // Get the number of polygons this render module is rendering
  vtkIdType GetTotalNumberOfPolygons();

  // Description:
  // The hardware selector is created the first time it's needed.
  // vtkSMHardwareSelector has the ability to cache buffers. This makes it
  // possible to do repeated selections without having to re-capture the
  // buffers. We clear the buffers if camera changes or any of the selection
  // parameters change or the window size changes.
  vtkSMHardwareSelector* HardwareSelector;
private:
  vtkSMRenderViewProxy(const vtkSMRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMRenderViewProxy&); // Not implemented.

  vtkPVOpenGLExtensionsInformation* OpenGLExtensionsInformation;
//ETX
};


#endif

