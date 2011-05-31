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

class VTK_EXPORT vtkCompositeMultiProcessController : public vtkMultiProcessController
{
public:
  static vtkCompositeMultiProcessController* New();
  vtkTypeMacro(vtkCompositeMultiProcessController, vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent);

  //  --------------------- Composite API -------------------------------

  // Description:
  // Return a unique identifier for the given selected controller.
  // This is only used as information to id which client is currently talking
  // to the server. In fact it is used inside vtkPVServerInformation so the client
  // could know what is its unique ID in respect to the server.
  int GetActiveControllerID();

  // Description:
  // Return the number of currently connected controllers.
  int GetNumberOfControllers();

  // Description:
  // Return the id of the nth controller.
  int GetControllerId(int idx);

  // Description:
  // Promote the given controller (ID) to be the next master controller.
  // Making a controller to be the master one, doesn't change anything on the
  // controller itself. It is just a meta-data information that helps client
  // application to build master/slave collaborative mechanism on top
  void SetMasterController(int id);

  // Description:
  // Retrun the ID of the designed "Master" controller. That master controller
  // is nothing else than a tag that can only be set on a single controller at
  // a time.
  int GetMasterController();

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
  // Provides access to the active controller.
  vtkMultiProcessController* GetActiveController();

  // Description:
  // Allow server to broadcast data to all connected client with our without
  // sending to the active client
  virtual void TriggerRMI2All(int remote, void* data, int length, int tag,
                              bool sendToActiveToo);

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
  virtual bool RemoveRMICallback(unsigned long observerTagId);

  enum EventId
    {
    CompositeMultiProcessControllerChanged = 2345
    };

protected:
  vtkCompositeMultiProcessController();
  ~vtkCompositeMultiProcessController();

private:
  vtkCompositeMultiProcessController(const vtkCompositeMultiProcessController&); // Not implemented
  void operator=(const vtkCompositeMultiProcessController&); // Not implemented

  class vtkCompositeInternals;
  vtkCompositeInternals* Internal;
  friend class vtkCompositeInternals;
//ETX
};

#endif
