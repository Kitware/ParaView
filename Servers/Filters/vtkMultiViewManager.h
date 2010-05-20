/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiViewManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiViewManager - manager for multiview.
// .SECTION Description
// vtkMultiViewManager is used in conjunction with a parallel render manager to
// enable multview support when using parallel rendering. There are many
// components that need to work together to make multiview work, one aspect of
// which is making sure that each views renders are located correctly in the
// render window. This class manages the position of renderers in the render
// window.
//
// All render views that constitute the multi view should share the multiview
// manager instance on the server side. These render views also share their
// render windows on the server side. Each however will have its own set of
// renderers. Each render view is assigned a unique ID which it uses to register
// its renderers with the multiview manager. 
//
// Before each view renders, it must notify the multiview manager which view is
// the active view so that the multiview manager can make the renderers
// belonging to that render view active for the current render.

#ifndef __vtkMultiViewManager_h
#define __vtkMultiViewManager_h

#include "vtkObject.h"

class vtkCommand;
class vtkRenderer;
class vtkRendererCollection;
class vtkRenderWindow;

class VTK_EXPORT vtkMultiViewManager : public vtkObject
{
public:
  static vtkMultiViewManager* New();
  vtkTypeMacro(vtkMultiViewManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the render window which is managed by this multiview manager.
  // Once the render window is set, we start observer start render events on the
  // render window to update the renderer viewports based on the specified
  // dimensions before actual rendering is triggerred.
  void SetRenderWindow(vtkRenderWindow* renWin);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Add/Remove renders managed by this manager. At the start of each render,
  // the manager will update the viewports of only those renders that it is
  // aware of.
  void AddRenderer(int id, vtkRenderer*);
  void RemoveRenderer(int id, vtkRenderer*);
  void RemoveAllRenderers(int id);
  void RemoveAllRenderers();

  // Description:
  // Returns a collection of renderers added with the given id.
  // Return NULL is none exists.
  vtkRendererCollection* GetRenderers(int id);

  // Description:
  // Before any render request, the view must make itself active.
  vtkSetMacro(ActiveViewID, int);
  vtkGetMacro(ActiveViewID, int);


  // Description:
  // IceTRenderer does not work well with vtkWindowToImageFilter with
  // magnification > 1 and there are multiple views. To solve the problem we
  // hide all other views and only render the active renderers.
  void StartMagnificationFix();
  void EndMagnificationFix();

//BTX
protected:
  vtkMultiViewManager();
  ~vtkMultiViewManager();


  void StartRenderCallback();

  vtkRendererCollection* GetActiveRenderers();

  vtkRenderWindow* RenderWindow;
  vtkCommand* Observer;
  int ActiveViewID;

  class vtkRendererMap;
  vtkRendererMap* RendererMap;

  int OriginalRenderWindowSize[2];
  double OriginalViewport[4];
  bool FixViewport;
private:
  vtkMultiViewManager(const vtkMultiViewManager&); // Not implemented
  void operator=(const vtkMultiViewManager&); // Not implemented
//ETX
};

#endif

