/*=========================================================================

  Program:   ParaView
  Module:    vtkRemoteWriterHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class  vtkRemoteWriterHelper
 * @brief  Helper to write data on client or data-server-root.
 *
 * vtkRemoteWriterHelper is designed to be used to write data that is being
 * generated on the client-side either directly on the client or on the
 * data-server root node. This is useful for saving data such as screenshots,
 * state files, etc. that are generated on the client-side but sometimes needs
 * to be saved remotely on the data-server root node instead.
 *
 * To use, simply create the ("misc", "NetworkImageWriter") proxy. The proxy
 * creates this class on client and data-server. Next, using the client-side
 * object, pass the input data to the client-instance and then set
 * `OutputDestination` property as needed. The actual writer to use must be set
 * using the "Writer" property. Now, when you call UpdatePipeline,
 * the vtkRemoteWriterHelper will transfer data if needed and then invoke the
 * writer's Write method on the target process.
*/

#ifndef vtkRemoteWriterHelper_h
#define vtkRemoteWriterHelper_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVSession.h"                   // for vtkPVSession::ServerFlags
#include "vtkRemotingServerManagerModule.h" // for exports

class vtkAlgorithm;
class vtkClientServerInterpreter;
class vtkDataObject;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkRemoteWriterHelper : public vtkDataObjectAlgorithm
{
public:
  static vtkRemoteWriterHelper* New();
  vtkTypeMacro(vtkRemoteWriterHelper, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the Image writer
   */
  void SetWriter(vtkAlgorithm* writer);
  vtkGetObjectMacro(Writer, vtkAlgorithm);
  //@}

  //@{
  /**
   * Get/Set the process on which the data must be written. Currently supported
   * values are vtkPVSession::CLIENT and vtkPVSession::DATA_SERVER_ROOT (or
   * vtkPVSession::DATA_SERVER). vtkPVSession::DATA_SERVER and
   * vtkPVSession::DATA_SERVER_ROOT are treated identically since the data is
   * only written on the root node.
   */
  vtkSetMacro(OutputDestination, vtkTypeUInt32);
  vtkGetMacro(OutputDestination, vtkTypeUInt32);
  //@}

  //@{
  /**
   * Set the interpreter to use to call methods on the writer. Initialized to
   * `vtkClientServerInterpreterInitializer::GetGlobalInterpreter()` in the
   * constructor.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp);
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);
  //@}

protected:
  vtkRemoteWriterHelper();
  ~vtkRemoteWriterHelper() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Internal method that setup the file name and delegates `Write` in this->Writer
   */
  void WriteLocally(vtkDataObject* input);

  vtkTypeUInt32 OutputDestination = vtkPVSession::CLIENT;
  vtkAlgorithm* Writer = nullptr;
  vtkClientServerInterpreter* Interpreter = nullptr;
};

#endif /* end of include guard: VTKNETWORKIMAGEWRITER_H_3BVHUERT */
