/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSpreadSheetRepresentationProxy
// .SECTION Description
// vtkSMSpreadSheetRepresentationProxy is a representation proxy used for
// spreadsheet view. It uses vtkSMBlockDeliveryRepresentationProxy proxy to
// deliver the input data to the client. It uses another
// vtkSMBlockDeliveryRepresentationProxy to deliver the vtkSelection to the
// client, if one exists on the input.

#ifndef __vtkSMSpreadSheetRepresentationProxy_h
#define __vtkSMSpreadSheetRepresentationProxy_h

#include "vtkSMBlockDeliveryRepresentationProxy.h"

class vtkSelection;
class vtkSMBlockDeliveryRepresentationProxy;

class VTK_EXPORT vtkSMSpreadSheetRepresentationProxy : 
  public vtkSMBlockDeliveryRepresentationProxy
{
public:
  static vtkSMSpreadSheetRepresentationProxy* New();
  vtkTypeMacro(vtkSMSpreadSheetRepresentationProxy,
    vtkSMBlockDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. 
  // Overridden to forward the update request to the strategy if any. 
  // If subclasses don't use any strategy, they may want to override this
  // method.
  // Fires vtkCommand:StartEvent and vtkCommand:EndEvent and start and end of
  // the update if it is indeed going to cause some pipeline execution.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Get the data that was collected to the client. Overridden to return the
  // data in the cache, if any. This method does not update the representation
  // if data is obsolete, use GetBlockOutput() instead.
  virtual vtkSelection* GetSelectionOutput(vtkIdType block);

  // Description:
  // Indicates if the block for selection output is available on the client.
  virtual bool IsSelectionAvailable(vtkIdType blockid);

  // Description:
  // Set if the representation should deliver only the selected elements to the
  // client.
  void SetSelectionOnly(int);
  vtkGetMacro(SelectionOnly, int);
  vtkBooleanMacro(SelectionOnly, int);

//BTX
protected:
  vtkSMSpreadSheetRepresentationProxy();
  ~vtkSMSpreadSheetRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  // Overridden to setup view time link.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Create the data pipeline.
  virtual bool CreatePipeline(vtkSMSourceProxy* input, int outputport);
  
  void PassEssentialAttributes();
  void InvokeStartEvent();
  void InvokeEndEvent();
  vtkSMBlockDeliveryRepresentationProxy* SelectionRepresentation;

  int SelectionOnly;
private:
  vtkSMSpreadSheetRepresentationProxy(const vtkSMSpreadSheetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSpreadSheetRepresentationProxy&); // Not implemented
//ETX
};

#endif

