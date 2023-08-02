// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
 * To use, simply create the ("misc", "RemoteWriterHelper") proxy. The proxy
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

  ///@{
  /**
   * Get/Set the Image writer
   */
  void SetWriter(vtkAlgorithm* writer);
  vtkGetObjectMacro(Writer, vtkAlgorithm);
  ///@}

  ///@{
  /**
   * Get/Set the process on which the data must be written. Currently supported
   * values are vtkPVSession::CLIENT and vtkPVSession::DATA_SERVER_ROOT (or
   * vtkPVSession::DATA_SERVER). vtkPVSession::DATA_SERVER and
   * vtkPVSession::DATA_SERVER_ROOT are treated identically since the data is
   * only written on the root node.
   */
  vtkSetMacro(OutputDestination, vtkTypeUInt32);
  vtkGetMacro(OutputDestination, vtkTypeUInt32);
  ///@}

  ///@{
  /**
   * If set to true, this helper will attempt writing the file in the background in parallel.
   * As of today, only images can go this path (.jpeg, .png, etc.). Otherwise, writing a file will
   * happen serially in all circumstances.
   */
  vtkSetMacro(TryWritingInBackground, bool);
  vtkGetMacro(TryWritingInBackground, bool);
  vtkBooleanMacro(TryWritingInBackground, bool);
  ///@}

  /**
   * Writer States
   *
   * START and END are used only by vtkGenericMovieWriter subclasses, while WRITE
   * is used by any writer.
   */
  enum
  {
    START = 0,
    WRITE = 1,
    END = 2
  };

  ///@{
  /**
   * Set the state of the writer.
   *
   * Make sure that the given writer has a Start and End functions if you want to use
   * START and END states.
   *
   * The default is WRITE.
   */
  vtkSetClampMacro(State, int, START, END);
  vtkGetMacro(State, int);
  ///@}

  ///@{
  /**
   * Set the interpreter to use to call methods on the writer. Initialized to
   * `vtkClientServerInterpreterInitializer::GetGlobalInterpreter()` in the
   * constructor.
   */
  void SetInterpreter(vtkClientServerInterpreter* interp);
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);
  ///@}

  vtkRemoteWriterHelper(const vtkRemoteWriterHelper&) = delete;
  void operator=(const vtkRemoteWriterHelper&) = delete;

  /**
   * Wait until `fileName` has finished being written. If the file has been written in the
   * background in parallel, this thread might hang if the file is not finished being written.
   * Otherwise, there is no waiting.
   *
   * @param fileName File name to wait for. It can be provided with its absolute or relative path
   * regardless.
   */
  static void Wait(const std::string& fileName);

  /**
   * Wait for all jobs saving screenshot to finish.
   */
  static void Wait();

  /**
   * Write the data.
   */
  int Write();

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
  int State = WRITE;
  vtkAlgorithm* Writer = nullptr;
  vtkClientServerInterpreter* Interpreter = nullptr;
  bool TryWritingInBackground = false;
};

#endif /* end of include guard: vtkRemoteWriterHelper_h */
