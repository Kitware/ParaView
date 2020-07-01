/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImageExtractWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMImageExtractWriterProxy
 * @brief extract writer for images or rendering results
 *
 * vtkSMImageExtractWriterProxy is intended to writing extracts using rendering
 * results from views. Default implementation uses a vtkSMSaveScreenshotProxy
 * provided as a subproxy named "Writer" to handle the image capture.
 *
 * This can generate images only from vtkSMViewProxy and subclasses.
 */

#ifndef vtkSMImageExtractWriterProxy_h
#define vtkSMImageExtractWriterProxy_h

#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkSMExtractWriterProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMImageExtractWriterProxy : public vtkSMExtractWriterProxy
{
public:
  static vtkSMImageExtractWriterProxy* New();
  vtkTypeMacro(vtkSMImageExtractWriterProxy, vtkSMExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Implementation for vtkSMExtractWriterProxy API.
   */
  bool Write(vtkSMExtractsController* extractor) override;
  bool CanExtract(vtkSMProxy* proxy) override;
  bool IsExtracting(vtkSMProxy* proxy) override;
  void SetInput(vtkSMProxy* proxy) override;
  vtkSMProxy* GetInput() override;
  //@}

protected:
  vtkSMImageExtractWriterProxy();
  ~vtkSMImageExtractWriterProxy();

  void CreateVTKObjects() override;

private:
  vtkSMImageExtractWriterProxy(const vtkSMImageExtractWriterProxy&) = delete;
  void operator=(const vtkSMImageExtractWriterProxy&) = delete;
};

#endif
