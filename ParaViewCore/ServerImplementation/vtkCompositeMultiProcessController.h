/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeMultiProcessController.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeMultiProcessController
// .SECTION Description
// vtkCompositeMultiProcessController offer a composite implementation of
// vtkMultiProcessController that allow us to deal with collaborative
// MultiProcessController by switching transparently underneath the active one
// and forwarding the method call to that specific one.
// RMICallBack are simply set on all of the MultiProcessController inside the
// composite.

#ifndef __vtkCompositeMultiProcessController_h
#define __vtkCompositeMultiProcessController_h

#include "vtkMultiProcessController.h"
#include "vtkWeakPointer.h" // Use to keep Active vtkMultiProcessController

class VTK_EXPORT vtkCompositeMultiProcessController : public vtkMultiProcessController
{
public:
  static vtkCompositeMultiProcessController* New();
  vtkTypeMacro(vtkCompositeMultiProcessController, vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  //  --------------------- Composite API -------------------------------

  // Description:
  // Append the given controller to the composite set.
  // We focus on vtkSocketController because most of the API method are empty
  void RegisterController(vtkMultiProcessController* controller);

  // Description:
  // Remove the given controller to the composite set.
  void UnRegisterController(vtkMultiProcessController* controller);

  // Description:
  // Remove the active controller and return the number of registered controler
  // left.
  int UnRegisterActiveController();

  // Description:
  // Allow server to broadcast to connected client some data but it skip
  // the connection that is the author of the previous network call.
  virtual void TriggerRMI2NonActives(int remote, void* data, int length, int tag);

  //  --------------- vtkMultiProcessController API ----------------------
  // Make sure inner vtkSocketController are initialized
  virtual void Initialize();
  virtual void Initialize(int*,char***)     {this->Initialize();};
  virtual void Initialize(int*,char***,int) {this->Initialize();};
  virtual void Finalize() {};             // Empty: Same as vtkSocketController
  virtual void Finalize(int) {};          // Empty: Same as vtkSocketController
  virtual void SingleMethodExecute() {};  // Empty: Same as vtkSocketController
  virtual void MultipleMethodExecute() {};// Empty: Same as vtkSocketController
  virtual void CreateOutputWindow() {};   // Empty: Same as vtkSocketController


  virtual vtkCommunicator* GetCommunicator();

  //  --------------- RMIs Overloaded API -------------------
//BTX
  // Description:
  // These methods are a part of the newer API to add multiple rmi callbacks.
  // When the RMI is triggered, all the callbacks are called
  // Adds a new callback for an RMI. Returns the identifier for the callback.
  virtual unsigned long AddRMICallback(vtkRMIFunctionType, void* localArg, int tag);

  // Description:
  // These methods are a part of the newer API to add multiple rmi callbacks.
  // When the RMI is triggered, all the callbacks are called
  // Removes all callbacks for the tag.
  virtual void RemoveAllRMICallbacks(int tag);
  virtual int RemoveFirstRMI(int tag)
    {
    vtkWarningMacro("RemoveRMICallbacks will remove all...");
    this->RemoveAllRMICallbacks(tag);
    return 1;
    }
  virtual void RemoveRMICallbacks(int tag)
    {
    vtkWarningMacro("RemoveRMICallbacks will remove all...");
    this->RemoveAllRMICallbacks(tag);
    }

protected:
  vtkCompositeMultiProcessController();
  ~vtkCompositeMultiProcessController();

private:
  vtkCompositeMultiProcessController(const vtkCompositeMultiProcessController&); // Not implemented
  void operator=(const vtkCompositeMultiProcessController&); // Not implemented

  class vtkCompositeInternals;
  vtkCompositeInternals* Internal;
  friend class vtkCompositeInternals;

  vtkWeakPointer<vtkMultiProcessController> ActiveController;
//ETX
};

#endif
