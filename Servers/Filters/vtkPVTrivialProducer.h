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
// preserves the information about the whole extent of the data object.
// In REQUEST_INFORMATION pass, vtkTrivialProducer tells the downstream pipeline
// that the whole extent of the data is exactly equal to the extent of the data.
// That way, no filters downstream can ask for more that what's available.
// However, in some cases, we still want to provide the downstream pipeline
// information that the data is only a chunk of bigger dataset eg.
// vtkPVGeometryFilter to avoid false boundaries between structured blocks.
// Hence we use vtkPVTrivialProducer in that case. vtkPVTrivialProducer uses
// vtkPVTrivialExtentTranslator which converts any request for the whole extent
// to match extent if the data available to the producer.

#ifndef __vtkPVTrivialProducer_h
#define __vtkPVTrivialProducer_h

#include "vtkTrivialProducer.h"

class vtkPVTrivialExtentTranslator;

class VTK_EXPORT vtkPVTrivialProducer : public vtkTrivialProducer
{
public:
  static vtkPVTrivialProducer* New();
  vtkTypeMacro(vtkPVTrivialProducer, vtkTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the data object that is "produced" by this producer.  It is
  // never really modified.
  // Overridden to pass the output to the vtkPVTrivialExtentTranslator.
  virtual void SetOutput(vtkDataObject* output);

  // Description:
  // Set the whole extent to use for the data this producer is producing.
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  // Description:
  // If the output of the filter is topologically regular and
  // this filter is used in parallel with the grid using partitioned
  // subextents then each process will only know about its own
  // subextent.  This function does an allreduce to make sure
  // that each process knows the subextent of every process.
  void GatherExtents();

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

  virtual void ReportReferences(vtkGarbageCollector*);
  vtkPVTrivialExtentTranslator* PVExtentTranslator;
  int WholeExtent[6];
private:
  vtkPVTrivialProducer(const vtkPVTrivialProducer&); // Not implemented
  void operator=(const vtkPVTrivialProducer&); // Not implemented
//ETX
};

#endif
