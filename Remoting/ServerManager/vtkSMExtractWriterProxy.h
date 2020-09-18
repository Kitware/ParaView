/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractWriterProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMExtractWriterProxy
 * @brief abstract class defining extract writer proxy API
 *
 * vtkSMExtractWriterProxy is an abstract class that defines the API for extract
 * writer proxies. Such proxies are intended to generate extracts from a
 * "producer". The producer may be any support proxy.
 */

#ifndef vtkSMExtractWriterProxy_h
#define vtkSMExtractWriterProxy_h

#include "vtkSMProxy.h"

class vtkSMExtractsController;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMExtractWriterProxy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMExtractWriterProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Generate the extract.
   */
  virtual bool Write(vtkSMExtractsController* extractor) = 0;

  /**
   * Returns true if the provided proxy can be extracted/written by this writer.
   * Note that this method is generally called on a prototype proxy and hence the
   * writer-proxy would not have been fully instantiated.
   */
  virtual bool CanExtract(vtkSMProxy* proxy) = 0;

  /**
   * Returns true this extract writer proxy is generating and extract from the provided
   * `proxy`.
   */
  virtual bool IsExtracting(vtkSMProxy* proxy) = 0;

  //@{
  /**
   * This is convenience method that gets called by vtkSMExtractsController to set the
   * extract writer to extract the given proxy.
   */
  virtual void SetInput(vtkSMProxy* proxy) = 0;
  virtual vtkSMProxy* GetInput() = 0;
  //@}
protected:
  vtkSMExtractWriterProxy();
  ~vtkSMExtractWriterProxy() override;

  //@{
  /**
   * Subclasses can use these methods to convert a filename for data or image
   * extracts.
   */
  static std::string GenerateDataExtractsFileName(
    const std::string& fname, vtkSMExtractsController* extractor);
  static std::string GenerateImageExtractsFileName(
    const std::string& fname, vtkSMExtractsController* extractor);
  static std::string GenerateImageExtractsFileName(
    const std::string& fname, const std::string& cameraparams, vtkSMExtractsController* extractor);
  //@}

private:
  vtkSMExtractWriterProxy(const vtkSMExtractWriterProxy&) = delete;
  void operator=(const vtkSMExtractWriterProxy&) = delete;

  static std::string GenerateExtractsFileName(
    const std::string& fname, vtkSMExtractsController* extractor, const char* rootdir);
};

#endif
