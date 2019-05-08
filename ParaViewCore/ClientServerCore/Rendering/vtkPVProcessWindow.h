/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVProcessWindow
 * @brief a window associated with current process, if any.
 *
 * In certain configurations, a ParaView process may have a single
 * vtkRenderWindow per process that is used to show the rendering results. For
 * example, in CAVE or tile-display mode, each render-server (or pvserver)
 * process may show its results on a user-viewable window. This class provides
 * access to that window.
 *
 * It is a singleton, since each process can have at most one such window.
 */

#ifndef vtkPVProcessWindow_h
#define vtkPVProcessWindow_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkRenderWindow;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVProcessWindow : public vtkObject
{
public:
  vtkTypeMacro(vtkPVProcessWindow, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the render window for this process.
   * May return nullptr on processes where ParaView rules dictate no such
   * process specific window is necessary. The first time this method is called
   * the window may get created and initialized based on the configuration.
   */
  static vtkRenderWindow* GetRenderWindow();

  /**
   * This may be called to ensured that the shared window is created/initialized
   * at least once.
   */
  static void PrepareForRendering();

protected:
  vtkPVProcessWindow();
  ~vtkPVProcessWindow();

  static vtkRenderWindow* NewWindow();
  static vtkRenderWindow* NewTileDisplayWindow();
  static vtkRenderWindow* NewCAVEWindow();

private:
  vtkPVProcessWindow(const vtkPVProcessWindow&) = delete;
  void operator=(const vtkPVProcessWindow&) = delete;
};

// Implementation of Schwartz counter idiom to ensure that the
// singleton vtkRenderWindow is cleaned up correctly during finalization.
static class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVProcessWindowSingletonCleaner
{
public:
  vtkPVProcessWindowSingletonCleaner();
  ~vtkPVProcessWindowSingletonCleaner();

private:
  vtkPVProcessWindowSingletonCleaner(const vtkPVProcessWindowSingletonCleaner&) = delete;
  void operator=(const vtkPVProcessWindowSingletonCleaner&) = delete;
} PVProcessWindowSingletonCleaner;
#endif
