// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSIMetaReaderProxy
 *
 * vtkSISourceProxy is the server-side helper for a vtkSMSourceProxy.
 * It adds support to handle various vtkAlgorithm specific Invoke requests
 * coming from the client. vtkSISourceProxy also inserts post-processing filters
 * for each output port from the vtkAlgorithm. These post-processing filters
 * deal with things like parallelizing the data etc.
 */

#ifndef vtkSIMetaReaderProxy_h
#define vtkSIMetaReaderProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSISourceProxy.h"

class vtkAlgorithm;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIMetaReaderProxy : public vtkSISourceProxy
{
public:
  static vtkSIMetaReaderProxy* New();
  vtkTypeMacro(vtkSIMetaReaderProxy, vtkSISourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIMetaReaderProxy();
  ~vtkSIMetaReaderProxy() override;

  void OnCreateVTKObjects() override;

  /**
   * Read xml-attributes.
   */
  bool ReadXMLAttributes(vtkPVXMLElement* element) override;

  // This is the name of the method used to set the file name on the
  // internal reader. See vtkFileSeriesReader for details.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  char* FileNameMethod;

private:
  vtkSIMetaReaderProxy(const vtkSIMetaReaderProxy&) = delete;
  void operator=(const vtkSIMetaReaderProxy&) = delete;
};

#endif
