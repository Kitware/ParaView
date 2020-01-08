/*=========================================================================

  Program:   ParaView
  Module:    vtkCommunicationErrorCatcher.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCommunicationErrorCatcher
 * @brief   helper class to catch errors from
 * vtkMultiProcessController and vtkCommunicator.
 *
 * vtkCommunicationErrorCatcher is helper class designed to catch errors from
 * vtkCommunicator and vtkCommunicator. This is not a
 * vtkObject and hence is designed to be created on the stack directly for
 * watching error during a set of calls and not over the lifetime of the
 * controller/communicator. For that, simply add your own observers for
 * vtkCommand::ErrorEvent and intercept those.
 * Note that is vtkObject::GlobalWarningDisplay is off, this class will not
 * receive any ErrorEvents and hence will not report any errors that were
 * raised.
*/

#ifndef vtkCommunicationErrorCatcher_h
#define vtkCommunicationErrorCatcher_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkWeakPointer.h"               // needed for vtkWeakPointer.
#include <string>

class vtkCommunicator;
class vtkMultiProcessController;
class vtkObject;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkCommunicationErrorCatcher
{
public:
  vtkCommunicationErrorCatcher(vtkMultiProcessController*);
  vtkCommunicationErrorCatcher(vtkCommunicator*);
  virtual ~vtkCommunicationErrorCatcher();

  /**
   * Get the status of errors.
   */
  bool GetErrorsRaised() const { return this->ErrorsRaised; }

  /**
   * Get the combined error messages.
   */
  const std::string& GetErrorMessages() const { return this->ErrorMessages; }

private:
  vtkCommunicationErrorCatcher(const vtkCommunicationErrorCatcher&) = delete;
  void operator=(const vtkCommunicationErrorCatcher&) = delete;

  void Initialize();
  void OnErrorEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  vtkWeakPointer<vtkMultiProcessController> Controller;
  vtkWeakPointer<vtkCommunicator> Communicator;

  bool ErrorsRaised;
  std::string ErrorMessages;
  unsigned long ControllerObserverId;
  unsigned long CommunicatorObserverId;
};

#endif
// VTK-HeaderTest-Exclude: vtkCommunicationErrorCatcher.h
