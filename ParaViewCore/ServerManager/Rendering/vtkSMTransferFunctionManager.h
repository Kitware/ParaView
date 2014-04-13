/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunctionManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTransferFunctionManager - manages transfer functions i.e. color
// lookuptables and opacity piecewise functions for ParaView applications.
// .SECTION Description
// vtkSMTransferFunctionManager manages transfer functions i.e. color
// lookuptables and opacity piecewise functions for ParaView applications.
// vtkSMTransferFunctionManager implements the ParaView specific mechanism for
// managing such transfer function proxies where there's one transfer function
// created and maintained per data array name.
//
// vtkSMTransferFunctionManager has no state. You can create as many instances as
// per your choosing to call the methods. It uses the session proxy manager to
// locate proxies registered using specific names under specific groups. Thus,
// the state is maintained by the proxy manager itself.

#ifndef __vtkSMTransferFunctionManager_h
#define __vtkSMTransferFunctionManager_h

#include "vtkSMObject.h"
#include "vtkPVServerManagerRenderingModule.h" // needed for export macro

class vtkSMProxy;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMTransferFunctionManager : public vtkSMObject
{
public:
  static vtkSMTransferFunctionManager* New();
  vtkTypeMacro(vtkSMTransferFunctionManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns a color transfer function proxy instance for mapping a data array
  // with the given name. If none exists in the given
  // session, a new instance will be created and returned.
  virtual vtkSMProxy* GetColorTransferFunction(const char* arrayName,
    vtkSMSessionProxyManager* pxm);

  // Description:
  // Returns a opacity transfer function proxy (aka Piecewise Function) instance
  // for mapping a data array with the given name. If
  // none exists in the given session, a new instance will be created and
  // returned.
  virtual vtkSMProxy* GetOpacityTransferFunction(const char* arrayName,
    vtkSMSessionProxyManager* pxm);

  // Description:
  // Returns the scalar-bar (color-legend) representation corresponding to the
  // transfer function for the view (currently only render-views are supported).
  // Thus returns an existing proxy, if present, otherwise a new one is created,
  // if possible.
  virtual vtkSMProxy* GetScalarBarRepresentation(
    vtkSMProxy* colorTransferFunctionProxy, vtkSMProxy* view);

  // Description:
  // Iterates over all "known" transfer function proxies and request each one of
  // them to update its range using data information currently available.
  // If \c extend is true, the transfer function is expanded to accommodate
  // current data range rather then resetting it to the range.
  void ResetAllTransferFunctionRangesUsingCurrentData(
    vtkSMSessionProxyManager* pxm, bool extend=false);

  enum UpdateScalarBarsMode
    {
    HIDE_UNUSED_SCALAR_BARS = 0x01,
    SHOW_USED_SCALAR_BARS = 0x02
    };

  // Description:
  // Updates the scalar bar visibility in the view. Based on the specified mode,
  // scalars bars representing non-represented arrays can be automatically hidden;
  // and those corresponding to arrays used for coloring can be automatically shown.
  virtual bool UpdateScalarBars(vtkSMProxy* viewProxy, unsigned int mode);

  // Description:
  // Hides the scalar bar, if any, for the lutProxy in the view if it's not
  // being used. Returns true if the scalar bar visibility was changed.
  virtual bool HideScalarBarIfNotNeeded(vtkSMProxy* lutProxy, vtkSMProxy* view);

//BTX
protected:
  vtkSMTransferFunctionManager();
  ~vtkSMTransferFunctionManager();

private:
  vtkSMTransferFunctionManager(const vtkSMTransferFunctionManager&); // Not implemented
  void operator=(const vtkSMTransferFunctionManager&); // Not implemented
//ETX
};

#endif
