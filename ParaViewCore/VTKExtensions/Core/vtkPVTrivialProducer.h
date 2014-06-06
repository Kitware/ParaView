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
// .NAME vtkPVTrivialProducer - specialized subclass of vtkTrivialProducer that
// preserves the information about the whole extent of the data object.
// .SECTION Description
// vtkPVTrivialProducer is specialized subclass of vtkTrivialProducer that
// can manage time requests.

#ifndef __vtkPVTrivialProducer_h
#define __vtkPVTrivialProducer_h

#include "vtkTrivialProducer.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

struct vtkPVTrivialProducerInternal;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVTrivialProducer : public vtkTrivialProducer
{
public:
  static vtkPVTrivialProducer* New();
  vtkTypeMacro(vtkPVTrivialProducer, vtkTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the data object that is "produced" by this producer.  It is
  // never really modified.
  virtual void SetOutput(vtkDataObject* output);

  // Description:
  // Set the output data object as well as time information
  // for the requests.
  virtual void SetOutput(vtkDataObject* output, double time);

  // Description:
  // Process upstream/downstream requests trivially.  The associated
  // output data object is never modified, but it is queried to
  // fulfill requests.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);
//BTX
protected:
  vtkPVTrivialProducer();
  ~vtkPVTrivialProducer();

  // Description:
  // Used to store any time step information. It assumes that the
  // time steps are ordered oldest to most recent.
  vtkPVTrivialProducerInternal* Internals;

private:
  vtkPVTrivialProducer(const vtkPVTrivialProducer&); // Not implemented
  void operator=(const vtkPVTrivialProducer&); // Not implemented
//ETX
};

#endif
