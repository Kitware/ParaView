/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataExtractWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMDataExtractWriterProxy
 * @brief extract writers to write datasets
 *
 * vtkSMDataExtractWriterProxy is an extract writer intended to write extracts
 * using ParaView writer proxies. The actual writer to use is defined a "Writer"
 * subproxy.
 */

#ifndef vtkSMDataExtractWriterProxy_h
#define vtkSMDataExtractWriterProxy_h

#include "vtkSMExtractWriterProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDataExtractWriterProxy : public vtkSMExtractWriterProxy
{
public:
  static vtkSMDataExtractWriterProxy* New();
  vtkTypeMacro(vtkSMDataExtractWriterProxy, vtkSMExtractWriterProxy);
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
  vtkSMDataExtractWriterProxy();
  ~vtkSMDataExtractWriterProxy() override;

private:
  vtkSMDataExtractWriterProxy(const vtkSMDataExtractWriterProxy&) = delete;
  void operator=(const vtkSMDataExtractWriterProxy&) = delete;
};

#endif
