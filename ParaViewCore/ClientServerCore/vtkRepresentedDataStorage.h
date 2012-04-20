/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRepresentedDataStorage
// .SECTION Description
// vtkRepresentedDataStorage is used by views and representations in ParaView to
// manage datasets that are "represented" by the representations. Typically
// representations use data-store instead of directly use the data from its
// input to ensure that data delivery mechanisms can provide data to the
// processes that need it.

#ifndef __vtkRepresentedDataStorage_h
#define __vtkRepresentedDataStorage_h

#include "vtkObject.h"
#include "vtkWeakPointer.h"

class vtkAlgorithmOutput;
class vtkDataObject;
class vtkPVDataRepresentation;
class vtkPVRenderView;

//BTX
#include <vector>
//ETX

class VTK_EXPORT vtkRepresentedDataStorage : public vtkObject
{
public:
  static vtkRepresentedDataStorage* New();
  vtkTypeMacro(vtkRepresentedDataStorage, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // View uses these methods to register a representation with the storage. This
  // makes it possible for representations to communicate with the storage
  // directly using a self pointer, while enables views on different processes
  // to communicate information about representations using their unique ids.
  void RegisterRepresentation(int id, vtkPVDataRepresentation*);
  void UnRegisterRepresentation(vtkPVDataRepresentation*);

  // Description:
  void SetPiece(vtkPVDataRepresentation*, vtkDataObject* data, bool low_res);
  vtkAlgorithmOutput* GetProducer(vtkPVDataRepresentation*, bool low_res);
  void SetDeliverToAllProcesses(vtkPVDataRepresentation*, bool flag, bool low_res);

  void SetPiece(int, vtkDataObject* data, bool low_res);
  vtkAlgorithmOutput* GetProducer(int, bool low_res);

  // Description:
  // Returns the size for all visible geometry.
  unsigned long GetVisibleDataSize(bool low_res);

  void Deliver(int use_low_res, unsigned int size, int *keys);

  void SetView(vtkPVRenderView*);

//BTX
  bool NeedsDelivery(unsigned long timestamp,
    std::vector<int>& keys_to_deliver, bool use_low_res);

protected:
  vtkRepresentedDataStorage();
  ~vtkRepresentedDataStorage();

  vtkWeakPointer<vtkPVRenderView> View;

private:
  vtkRepresentedDataStorage(const vtkRepresentedDataStorage&); // Not implemented
  void operator=(const vtkRepresentedDataStorage&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
