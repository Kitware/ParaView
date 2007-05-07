/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPipelineRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPipelineRepresentationProxy - Superclass for representations that
// have some data input.
// .SECTION Description
// vtkSMPipelineRepresentationProxy is a superclass for representations that
// consume data i.e. require some input.

#ifndef __vtkSMPipelineRepresentationProxy_h
#define __vtkSMPipelineRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMSourceProxy;

class VTK_EXPORT vtkSMPipelineRepresentationProxy : 
  public vtkSMRepresentationProxy
{
public:
  vtkTypeRevisionMacro(vtkSMPipelineRepresentationProxy, 
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkSMInputProperty requires that the consumer proxy support
  // AddInput() method. Hence, this method is defined. This method
  // sets up the input connection.
  void AddInput(vtkSMSourceProxy* input, const char* method,
    int hasMultipleInputs);

  // Description:
  // Get the data information for the represented data.
  // Representations that do not have significatant data representations such as
  // 3D widgets, text annotations may return NULL. Default implementation
  // returns NULL.
  virtual vtkPVDataInformation* GetDataInformation()
    { return 0; }

//BTX
protected:
  vtkSMPipelineRepresentationProxy();
  ~vtkSMPipelineRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects(int numObjects);

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Added to call InitializeStrategy() to give subclassess the opportunity to
  // set up pipelines involving compositing strategy it they support it.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Some representations may require lod/compositing strategies from the view
  // proxy. This method gives such subclasses an opportunity to as the view
  // module for the right kind of strategy and plug it in the representation
  // pipeline. Returns true on success. Default implementation suffices for
  // representation that don't use strategies.
  virtual bool InitializeStrategy(vtkSMViewProxy* vtkNotUsed(view))
    { return true; }


  // Description:
  // Set the representation strategy.
  void SetStrategy(vtkSMRepresentationStrategy*);

  vtkSMSourceProxy* InputProxy;
  vtkSMRepresentationStrategy* Strategy;
private:
  vtkSMPipelineRepresentationProxy(const vtkSMPipelineRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPipelineRepresentationProxy&); // Not implemented

  void SetInputProxy(vtkSMSourceProxy*);
//ETX
};

#endif

