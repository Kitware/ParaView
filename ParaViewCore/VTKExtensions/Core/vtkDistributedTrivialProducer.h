/*=========================================================================

  Program:   ParaView
  Module:    vtkDistributedTrivialProducer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDistributedTrivialProducer
// .SECTION Description
// Extend vtkPVTrivialProducer to allow distributed code to easily populate
// a trivial producer with local DataObject while using a regular Proxy Model.

#ifndef __vtkDistributedTrivialProducer_h
#define __vtkDistributedTrivialProducer_h

#include "vtkPVTrivialProducer.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro"

struct vtkPVTrivialProducerStaticInternal;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkDistributedTrivialProducer : public vtkPVTrivialProducer
{
public:
  static vtkDistributedTrivialProducer* New();
  vtkTypeMacro(vtkDistributedTrivialProducer, vtkPVTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provide a global method to store a data object accross processes and allow
  // a given instance of TrivialProducer to use it based on its registred key.
  static void SetGlobalOutput(const char* key, vtkDataObject* output);

  // Description:
  // Release a given Global output if a valid key (not NULL) is provided,
  // otherwise the global output map will be cleared.
  static void ReleaseGlobalOutput(const char* key);

  // Description:
  // Update the current instance to use a previously registred global data object
  // as current output.
  virtual void UpdateFromGlobal(const char* key);

//BTX
protected:
  vtkDistributedTrivialProducer();
  ~vtkDistributedTrivialProducer();

private:
  vtkDistributedTrivialProducer(const vtkDistributedTrivialProducer&); // Not implemented
  void operator=(const vtkDistributedTrivialProducer&); // Not implemented

  static vtkPVTrivialProducerStaticInternal* InternalStatic;
//ETX
};

#endif
