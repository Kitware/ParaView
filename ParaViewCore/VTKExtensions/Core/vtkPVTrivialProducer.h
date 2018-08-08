/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialProducer.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVTrivialProducer
 * @brief   specialized subclass of vtkTrivialProducer that
 * preserves the information about the whole extent of the data object.
 *
 * vtkPVTrivialProducer is specialized subclass of vtkTrivialProducer that
 * can manage time requests.
*/

#ifndef vtkPVTrivialProducer_h
#define vtkPVTrivialProducer_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkTrivialProducer.h"

struct vtkPVTrivialProducerInternal;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVTrivialProducer : public vtkTrivialProducer
{
public:
  static vtkPVTrivialProducer* New();
  vtkTypeMacro(vtkPVTrivialProducer, vtkTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the data object that is "produced" by this producer.  It is
   * never really modified.
   */
  void SetOutput(vtkDataObject* output) override;

  /**
   * Set the output data object as well as time information
   * for the requests.
   */
  virtual void SetOutput(vtkDataObject* output, double time);

  /**
   * Process upstream/downstream requests trivially.  The associated
   * output data object is never modified, but it is queried to
   * fulfill requests.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkPVTrivialProducer();
  ~vtkPVTrivialProducer() override;

  /**
   * Used to store any time step information. It assumes that the
   * time steps are ordered oldest to most recent.
   */
  vtkPVTrivialProducerInternal* Internals;

private:
  vtkPVTrivialProducer(const vtkPVTrivialProducer&) = delete;
  void operator=(const vtkPVTrivialProducer&) = delete;
};

#endif
