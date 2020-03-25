/*=========================================================================

   Program: ParaView
   Module:  vtkPVLogInformation.h

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

#ifndef vtkPVLogInformation_h
#define vtkPVLogInformation_h

#include "vtkClientServerStream.h"
#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" // needed for exports

/**
 * @class vtkPVLogInformation
 * @brief Gets the log of a specific rank as well as the verbosity level
 */
class VTKREMOTINGCORE_EXPORT vtkPVLogInformation : public vtkPVInformation
{
public:
  static vtkPVLogInformation* New();
  vtkTypeMacro(vtkPVLogInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  //@}

  /**
   * Get the recorded logs.
   */
  vtkGetMacro(Logs, const std::string&);

  /**
   * Get the starting log.
   */
  vtkGetMacro(StartingLogs, const std::string&);

  /**
   * Set the rank to get log from.
   */
  vtkSetMacro(Rank, int);

  /**
   * Get the verbosity level of the server.
   */
  vtkGetMacro(Verbosity, int);

protected:
  vtkPVLogInformation() = default;
  ~vtkPVLogInformation() override = default;

  int Rank = -1;
  std::string Logs;
  std::string StartingLogs;
  int Verbosity = 20;

  vtkPVLogInformation(const vtkPVLogInformation&) = delete;
  void operator=(const vtkPVLogInformation&) = delete;
};

#endif // vtkPVLogInformation_h
