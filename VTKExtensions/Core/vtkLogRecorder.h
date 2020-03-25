/*=========================================================================

   Program: ParaView
   Module:  vtkLogRecorder.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#ifndef vtkLogRecorder_h
#define vtkLogRecorder_h

#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export

/**
 * @class vtkLogRecorder
 * @brief Record log messages at or below a given verbosity on a particular ParaView process/rank.
 *
 * This class can be used to record log messages at or below a verbosity specified by
 * SetVerbosity(). If the process is run as an MPI job, the log messages from a
 * rank enabled via SetRankEnabled() will be recorded.
 */
class VTKPVVTKEXTENSIONSCORE_EXPORT vtkLogRecorder : public vtkObject
{
public:
  static vtkLogRecorder* New();

  vtkTypeMacro(vtkLogRecorder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The callback function that listens for log message will not be added until logging is
   * enabled. The vtkLogRecorder object will query the multiprocess controller for its rank.
   */
  vtkLogRecorder();
  ~vtkLogRecorder() override;

  //@{
  /**
   * Set/get the verbosity of the entries recorded from the log.
   * Any messages with verbosity less than or equal to this verbosity
   * will be recorded.
   */
  void SetVerbosity(int verbosity);
  vtkGetMacro(Verbosity, int);
  //@}

  //@{
  /**
   * Set to enable or disable logging, only if the rank parameter is equal to the rank that
   * this log recorder is on.
   */
  void SetRankEnabled(int rank);
  void SetRankDisabled(int rank);
  //@}

  /**
   * Get the recorded logs.
   */
  const std::string& GetLogs() const;

  /**
   * Get the starting log.
   */
  const std::string& GetStartingLog() const { return this->StartingLog; }

  /**
   * Get the rank the log recorder is on.
   */
  vtkGetMacro(MyRank, int);

  /**
   * Clear any logs recorded.
   */
  void ClearLogs();

  /**
   * Set the verbosity of a ParaView logging category.
   */
  void SetCategoryVerbosity(int categoryIndex, int verbosity);

  /**
   * Clear custom verbosities set on all ParaView logging categories.
   */
  void ResetCategoryVerbosities();

private:
  vtkLogRecorder(const vtkLogRecorder&) = delete;
  void operator=(const vtkLogRecorder&) = delete;
  void EnableLoggingCallback();
  void DisableLoggingCallback();

  int Verbosity;
  std::string Logs;
  std::string CallbackName;
  std::string StartingLog;
  int MyRank;
  int RankEnabled = 0;
};

#endif // vtkLogRecorder_h
