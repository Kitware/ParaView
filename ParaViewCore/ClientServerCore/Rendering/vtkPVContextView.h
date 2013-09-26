/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContextView
// .SECTION Description
// vtkPVContextView adopts vtkContextView so that it can be used in ParaView
// configurations.

#ifndef __vtkPVContextView_h
#define __vtkPVContextView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVView.h"

class vtkAbstractContextItem;
class vtkChart;
class vtkChartRepresentation;
class vtkContextView;
class vtkInformationIntegerKey;
class vtkRenderWindow;
class vtkSelection;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVContextView : public vtkPVView
{
public:
  vtkTypeMacro(vtkPVContextView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender();

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender();

  // Description:
  // Get the context view.
  vtkGetObjectMacro(ContextView, vtkContextView);

  // Description:
  // Get the context item.
  virtual vtkAbstractContextItem* GetContextItem() = 0;

  // Description:
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  // Overridden to ensure that in multi-client configurations, same set of
  // representations are "dirty" on all processes to avoid race conditions.
  virtual void Update();

  // Description:
  // Set or get whether offscreen rendering should be used during
  // CaptureWindow calls. On Apple machines, this flag has no effect.
  vtkSetMacro(UseOffscreenRenderingForScreenshots, bool);
  vtkBooleanMacro(UseOffscreenRenderingForScreenshots, bool);
  vtkGetMacro(UseOffscreenRenderingForScreenshots, bool);

  // Description:
  // Get/Set whether to use offscreen rendering for all rendering. This is
  // merely a suggestion. If --use-offscreen-rendering command line option is
  // specified, then setting this flag to 0 on that process has no effect.
  // Setting it to true, however, will ensure that even is
  // --use-offscreen-rendering is not specified, it will use offscreen
  // rendering.
  virtual void SetUseOffscreenRendering(bool);
  vtkBooleanMacro(UseOffscreenRendering, bool);
  vtkGetMacro(UseOffscreenRendering, bool);

  // Description:
  // Representations can use this method to set the selection for a particular
  // representation. Subclasses override this method to pass on the selection to
  // the chart using annotation link. Note this is meant to pass selection for
  // the local process alone. The view does not manage data movement for the
  // selection.
  virtual void SetSelection(vtkChartRepresentation* repr,
    vtkSelection* selection) = 0;

//BTX
protected:
  vtkPVContextView();
  ~vtkPVContextView();

  // Description:
  // Actual rendering implementation.
  virtual void Render(bool interactive);

  // Description:
  // Callbacks called when the primary "renderer" in the vtkContextView
  // starts/ends rendering. Note that this is called on the renderer, hence
  // before the rendering cleanup calls like SwapBuffers called by the
  // render-window.
  void OnStartRender();
  void OnEndRender();

  vtkContextView* ContextView;
  vtkRenderWindow* RenderWindow;

  bool UseOffscreenRenderingForScreenshots;
  bool UseOffscreenRendering;
private:
  vtkPVContextView(const vtkPVContextView&); // Not implemented
  void operator=(const vtkPVContextView&); // Not implemented
//ETX
};

#endif
