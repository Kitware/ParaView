/*=========================================================================

Program:   ParaView
Module:    vtkSMExportProxyDepot.h

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMExportProxyDepot
 * @brief   access proxies that define catalyst export state
 *
 * vtkSMExportProxyDepot is a container for export proxies. These include
 * Writer proxies, SaveScreenShot proxies and the global Catalyst Options Proxy.
 * It is a helper class for the SMSessionProxyManager intended to standardize an
 * API for working with them.
*/

#ifndef vtkSMExportProxyDepot_h
#define vtkSMExportProxyDepot_h

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports

class vtkSMProxy;
class vtkSMSessionProxyManager;
class vtkSMSourceProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMExportProxyDepot : public vtkObject
{
public:
  static vtkSMExportProxyDepot* New();
  vtkTypeMacro(vtkSMExportProxyDepot, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Global options for the entire catalyst export.
   */
  vtkSMProxy* GetGlobalOptions();

  //@{
  /**
   * Options for exporting a source proxy.
   */
  bool HasWriterProxy(const char* group, const char* format);
  vtkSMSourceProxy* GetWriterProxy(vtkSMSourceProxy* filter, const char* group, const char* format);
  //@}

  //@{
  /**
   * Iterate through source proxy exports.
   */
  void InitNextWriterProxy();
  vtkSMSourceProxy* GetNextWriterProxy();
  //@}

  //@{
  /**
   * Options for exporting a screen shot.
   */
  bool HasScreenshotProxy(const char* group, const char* format);
  vtkSMProxy* GetScreenshotProxy(vtkSMProxy* view, const char* group, const char* format);
  //@}

  //@{
  /**
   * Iterate through screen shot exports.
   */
  void InitNextScreenshotProxy();
  vtkSMProxy* GetNextScreenshotProxy();
  //@}

protected:
  vtkSMExportProxyDepot();
  ~vtkSMExportProxyDepot() override;

private:
  vtkSMExportProxyDepot(const vtkSMExportProxyDepot&) = delete;
  void operator=(const vtkSMExportProxyDepot&) = delete;

  friend class vtkSMSessionProxyManager;
  vtkSMSessionProxyManager* Session;

  class Internal;
  Internal* Internals;
};

#endif
