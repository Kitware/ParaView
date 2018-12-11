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
/**
 * @class   vtkCompositeMultiProcessController
 *
 * vtkCompositeMultiProcessController offer a composite implementation of
 * vtkMultiProcessController that allow us to deal with collaborative
 * MultiProcessController by switching transparently underneath the active one
 * and forwarding the method call to that specific one.
 * RMICallBack are simply set on all of the MultiProcessController inside the
 * composite.
*/

#ifndef vtkCompositeMultiProcessController_h
#define vtkCompositeMultiProcessController_h

#include "vtkMultiProcessController.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkCompositeMultiProcessController
  : public vtkMultiProcessController
{
public:
  static vtkCompositeMultiProcessController* New();
  vtkTypeMacro(vtkCompositeMultiProcessController, vtkMultiProcessController);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //  --------------------- Composite API -------------------------------

  /**
   * Return a unique identifier for the given selected controller.
   * This is only used as information to id which client is currently talking
   * to the server. In fact it is used inside vtkPVServerInformation so the client
   * could know what is its unique ID in respect to the server.
   */
  int GetActiveControllerID();

  /**
   * Return the number of currently connected controllers.
   */
  int GetNumberOfControllers();

  /**
   * Return the id of the nth controller.
   */
  int GetControllerId(int idx);

  /**
   * Return the nth controller.
   */
  vtkMultiProcessController* GetController(int idx);

  /**
   * Promote the given controller (ID) to be the next master controller.
   * Making a controller to be the master one, doesn't change anything on the
   * controller itself. It is just a meta-data information that helps client
   * application to build master/slave collaborative mechanism on top
   */
  void SetMasterController(int id);

  /**
   * Return the ID of the designed "Master" controller. That master controller
   * is nothing else than a tag that can only be set on a single controller at
   * a time.
   */
  int GetMasterController();

  /**
   * Append the given controller to the composite set.
   * We focus on vtkSocketController because most of the API method are empty
   */
  void RegisterController(vtkMultiProcessController* controller);

  /**
   * Remove the given controller to the composite set.
   */
  void UnRegisterController(vtkMultiProcessController* controller);

  /**
   * Remove the active controller and return the number of registered controller
   * left.
   */
  int UnRegisterActiveController();

  /**
   * Provides access to the active controller.
   */
  vtkMultiProcessController* GetActiveController();

  /**
   * Allow server to broadcast data to all connected client with our without
   * sending to the active client
   */
  virtual void TriggerRMI2All(int remote, void* data, int length, int tag, bool sendToActiveToo);

  //  --------------- vtkMultiProcessController API ----------------------
  // Make sure inner vtkSocketController are initialized
  virtual void Initialize();
  void Initialize(int*, char***) override { this->Initialize(); };
  void Initialize(int*, char***, int) override { this->Initialize(); };
  void Finalize() override{};              // Empty: Same as vtkSocketController
  void Finalize(int) override{};           // Empty: Same as vtkSocketController
  void SingleMethodExecute() override{};   // Empty: Same as vtkSocketController
  void MultipleMethodExecute() override{}; // Empty: Same as vtkSocketController
  void CreateOutputWindow() override{};    // Empty: Same as vtkSocketController

  vtkCommunicator* GetCommunicator() override;

  //  --------------- RMIs Overloaded API -------------------

  /**
   * These methods are a part of the newer API to add multiple rmi callbacks.
   * When the RMI is triggered, all the callbacks are called
   * Adds a new callback for an RMI. Returns the identifier for the callback.
   */
  unsigned long AddRMICallback(vtkRMIFunctionType, void* localArg, int tag) override;

  //@{
  /**
   * These methods are a part of the newer API to add multiple rmi callbacks.
   * When the RMI is triggered, all the callbacks are called
   * Removes all callbacks for the tag.
   */
  void RemoveAllRMICallbacks(int tag) override;
  int RemoveFirstRMI(int tag) override
  {
    vtkWarningMacro("RemoveRMICallbacks will remove all...");
    this->RemoveAllRMICallbacks(tag);
    return 1;
  }
  bool RemoveRMICallback(unsigned long observerTagId) override;
  //@}

  enum EventId
  {
    CompositeMultiProcessControllerChanged = 2345
  };

protected:
  vtkCompositeMultiProcessController();
  ~vtkCompositeMultiProcessController() override;

private:
  vtkCompositeMultiProcessController(const vtkCompositeMultiProcessController&) = delete;
  void operator=(const vtkCompositeMultiProcessController&) = delete;

  class vtkCompositeInternals;
  vtkCompositeInternals* Internal;
  friend class vtkCompositeInternals;
};

#endif
