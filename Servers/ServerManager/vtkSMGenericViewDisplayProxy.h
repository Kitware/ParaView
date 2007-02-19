/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGenericViewDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGenericViewDisplayProxy - proxy for any entity that must be rendered.
// .SECTION Description
// vtkSMGenericViewDisplayProxy is a sink for display objects. Anything that can
// be rendered has to be a vtkSMGenericViewDisplayProxy, otherwise it can't be added
// be added to the vtkSMRenderModule, and hence cannot be rendered.
// This can have inputs (but not required, for displays such as 3Dwidgets/ Scalarbar).
// This is an abstract class, merely defining the interface.
//  This class (or subclasses) has a bunch of 
// "convenience methods" (method names appended with CM). These methods
// do the equivalent of getting the property by the name and
// setting/getting its value. They are there to simplify using the property
// interface for display objects. When adding a method to the proxies
// that merely sets some property on the proxy, make sure to append the method
// name with "CM" - implying it's a convenience method. That way, one knows
// its purpose and will not be confused with a update-self property method.

#ifndef __vtkSMGenericViewDisplay_h
#define __vtkSMGenericViewDisplay_h

#include "vtkSMAbstractDisplayProxy.h"

class vtkDataObject;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMGenericViewDisplayProxy : public vtkSMAbstractDisplayProxy
{
public:
  static vtkSMGenericViewDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMGenericViewDisplayProxy, vtkSMAbstractDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method gets called to set input when using Input property.
  // Internally leads to a call to SetInput.
  virtual void AddInput(vtkSMSourceProxy* input, const char* method,
    int hasMultipleInputs);

  // Description:
  // Get the data that was collected to the client
  virtual vtkDataObject* GetOutput();

  // Description:
  // Visibility
  vtkSetClampMacro(Visibility, int, 0, 1);
  vtkBooleanMacro(Visibility, int);
  vtkGetMacro(Visibility, int);

  // Description:
  // Update the output
  virtual void Update(vtkSMAbstractViewModuleProxy*);
  virtual void Update() { this->Superclass::Update(); }

  // Description:
  // This method returns if the Update() or UpdateDistributedGeometry()
  // calls will actually lead to an Update. This is used by the render module
  // to decide if it can expect any pipeline updates.
  virtual int UpdateRequired();

  // Description:
  // Set the reduction algorithm type. Cannot be called before
  // objects are created.
  void SetReductionType(int type);
  //BTX
  enum ReductionTypeEnum
    {
    ADD = 0,
    POLYDATA_APPEND = 1,
    UNSTRUCTURED_APPEND = 2,
    FIRST_NODE_ONLY = 3,
    RECTILINEAR_GRID_APPEND=4
    };
  //ETX

  // Description:
  // Chains to superclass and update UpdateRequiredFlag.
  virtual void MarkModified(vtkSMProxy* modifiedProxy); 
protected:
  vtkSMGenericViewDisplayProxy();
  ~vtkSMGenericViewDisplayProxy();

  virtual void CreateVTKObjects(int numObjects);

  void SetInput(vtkSMProxy* input);

  int UpdateRequiredFlag;

private:
  vtkSMProxy* CollectProxy;
  vtkSMProxy *UpdateSuppressorProxy;
  vtkSMProxy* ReduceProxy;
  vtkSMProxy* PostProcessorProxy;

  int CanCreateProxy;
  int CollectionDecision;
  int Visibility;
  int ReductionType;

  vtkDataObject* Output;

  vtkSMGenericViewDisplayProxy(const vtkSMGenericViewDisplayProxy&); // Not implemented.
  void operator=(const vtkSMGenericViewDisplayProxy&); // Not implemented.
};



#endif


