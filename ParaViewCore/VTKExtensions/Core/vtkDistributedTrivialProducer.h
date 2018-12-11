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
/**
 * @class   vtkDistributedTrivialProducer
 *
 * Extend vtkPVTrivialProducer to allow distributed code to easily populate
 * a trivial producer with local DataObject while using a regular Proxy Model.
*/

#ifndef vtkDistributedTrivialProducer_h
#define vtkDistributedTrivialProducer_h

#include "vtkPVTrivialProducer.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro"

struct vtkPVTrivialProducerStaticInternal;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkDistributedTrivialProducer : public vtkPVTrivialProducer
{
public:
  static vtkDistributedTrivialProducer* New();
  vtkTypeMacro(vtkDistributedTrivialProducer, vtkPVTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Provide a global method to store a data object across processes and allow
   * a given instance of TrivialProducer to use it based on its registered key.
   */
  static void SetGlobalOutput(const char* key, vtkDataObject* output);

  /**
   * Release a given Global output if a valid key (not NULL) is provided,
   * otherwise the global output map will be cleared.
   */
  static void ReleaseGlobalOutput(const char* key);

  /**
   * Update the current instance to use a previously registered global data object
   * as current output.
   */
  virtual void UpdateFromGlobal(const char* key);

protected:
  vtkDistributedTrivialProducer();
  ~vtkDistributedTrivialProducer() override;

private:
  vtkDistributedTrivialProducer(const vtkDistributedTrivialProducer&) = delete;
  void operator=(const vtkDistributedTrivialProducer&) = delete;

  static vtkPVTrivialProducerStaticInternal* InternalStatic;
};

#endif
