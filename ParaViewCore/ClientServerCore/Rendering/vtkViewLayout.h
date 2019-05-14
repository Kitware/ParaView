/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkViewLayout
 * @brief   used by vtkSMViewLayoutProxy.
 *
 * vtkViewLayout is the server-side object corresponding to
 * vtkSMViewLayoutProxy. vtkSMViewLayoutProxy corresponds to a single layout
 * of views. In ParaView UI, this corresponds to a single tab. In tile-display
 * mode, the server-side only has "1 tab", in a manner of speaking. Thus, we
 * need to ensure that the server shows the views laid out in the active tab.
 * This class helps vtkSMViewLayoutProxy do that.
 *
 * In addition, in tile display mode this class handles setting of window size
 * and positions for each view known to the layout before any of view renders.
 * Additionally, it also handles pasting back rendering results on the processes
 * specific render window (see vtkPVProcessWindow), if any, if appropriate for
 * current mode e.g. in tile display mode, CAVE mode, client-server mode when
 * PV_DEBUG_REMOTE_RENDERING is set or on root node in batch mode.
 */

#ifndef vtkViewLayout_h
#define vtkViewLayout_h

#include "vtkNew.h" // for vtkNew
#include "vtkObject.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkVector.h"                            // for vtkVector2i
#include <memory>                                 // for std::unique_ptr

class vtkPVView;
class vtkRenderWindow;
class vtkViewLayoutProp;
class vtkViewport;
class vtkPVComparativeView;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkViewLayout : public vtkObject
{
public:
  static vtkViewLayout* New();
  vtkTypeMacro(vtkViewLayout, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This method is called to make this layout the one to be shown on the tile
   * display, if any. This will deactivate the previously chosen layout, if any.
   */
  void ShowOnTileDisplay();

  //@{
  /**
   * Add/remove views in this layout.
   */
  void RemoveAllViews();
  void AddView(vtkPVView*, const double* viewport);
  void AddView(vtkPVComparativeView*, const double* viewport);
  //@}

  /**
   * Intended for testing and debugging. When called, this will save the layout
   * results to an png file.
   */
  bool SaveAsPNG(int rank, const char* fname);

  //@{
  /**
   * Set the color to use for separator between views in multi-view
   * configurations when saving images.
   *
   * The arguments are the components of the red, green, and blue channels from 0.0 to 1.0.
   */
  vtkSetVector3Macro(SeparatorColor, double);
  vtkGetVector3Macro(SeparatorColor, double);
  //@}

  //@{
  /**
   * Get/Set the separator width (in pixels) to use for separator between views
   * in multi-view configurations.
   */
  vtkSetClampMacro(SeparatorWidth, int, 0, VTK_INT_MAX);
  vtkGetMacro(SeparatorWidth, int);
  //@}

protected:
  vtkViewLayout();
  ~vtkViewLayout() override;

  //@{
  /**
   * Set the tile dimensions. Default is (1, 1).
   */
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);
  //@}

  //@{
  /**
   * Set the tile mullions in pixels. Use negative numbers to indicate overlap
   * between tiles.
   */
  vtkSetVector2Macro(TileMullions, int);
  vtkGetVector2Macro(TileMullions, int);
  //@}

  void UpdateLayout(vtkObject*, unsigned long, void*);
  void UpdateLayoutForTileDisplay(vtkRenderWindow*);
  void UpdateLayoutForCAVE(vtkRenderWindow*);

  void UpdateDisplay(vtkObject*, unsigned long, void*);

  void Paint(vtkViewport*);

  /**
   * Returns true if any of the layout needs active stereo for current render.
   */
  bool NeedsActiveStereo() const;

private:
  vtkViewLayout(const vtkViewLayout&) = delete;
  void operator=(const vtkViewLayout&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  vtkNew<vtkViewLayoutProp> Prop;

  bool InTileDisplay;
  int TileDimensions[2];
  int TileMullions[2];
  bool InCave;
  bool DisplayResults;
  int SeparatorWidth;
  double SeparatorColor[3];

  friend class vtkViewLayoutProp;
};

#endif
