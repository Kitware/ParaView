/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCinemaExtractWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMCinemaExtractWriterProxy
 * @brief extracts writer subclass to write Cinema database
 *
 * vtkSMCinemaExtractWriterProxy is an image extracts writer to write to a
 * Cinema database.
 */

#ifndef vtkSMCinemaExtractWriterProxy_h
#define vtkSMCinemaExtractWriterProxy_h

#include "vtkRemotingCinemaModule.h" // for export macros
#include "vtkSMImageExtractWriterProxy.h"

class VTKREMOTINGCINEMA_EXPORT vtkSMCinemaExtractWriterProxy : public vtkSMImageExtractWriterProxy
{
public:
  static vtkSMCinemaExtractWriterProxy* New();
  vtkTypeMacro(vtkSMCinemaExtractWriterProxy, vtkSMImageExtractWriterProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Overridden to forward to the Cinema database exporting code.
   */
  bool Write(vtkSMExtractsController* extractor) override;
  //@}

protected:
  vtkSMCinemaExtractWriterProxy();
  ~vtkSMCinemaExtractWriterProxy() override;

private:
  vtkSMCinemaExtractWriterProxy(const vtkSMCinemaExtractWriterProxy&) = delete;
  void operator=(const vtkSMCinemaExtractWriterProxy&) = delete;
};

#endif
