/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleStrategy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleStrategy
// .SECTION Description
//

#ifndef __vtkSMSimpleStrategy_h
#define __vtkSMSimpleStrategy_h

#include "vtkSMRepresentationStrategy.h"

class VTK_EXPORT vtkSMSimpleStrategy : public vtkSMRepresentationStrategy
{
public:
  static vtkSMSimpleStrategy* New();
  vtkTypeRevisionMacro(vtkSMSimpleStrategy, vtkSMRepresentationStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()
    { return this->UpdateSuppressor; }

  // Description:
  // Get low resolution output.
  virtual vtkSMSourceProxy* GetLODOutput()
    { return this->UpdateSuppressorLOD; }

//BTX
protected:
  vtkSMSimpleStrategy();
  ~vtkSMSimpleStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void CreateVTKObjects();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input);

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the data pipeline (non-LOD).
  virtual void GatherInformation(vtkPVDataInformation*);

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the LOD pipeline.
  virtual void GatherLODInformation(vtkPVDataInformation*);

  // Description:
  // Update the LOD pipeline. Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  // This method need not worry about caching, since LODPipeline is never used
  // when caching is enabled.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  // This method should respect caching, if supported. Call
  // UseCache() to check if caching is to be employed.
  virtual void UpdatePipeline();

  vtkSMSourceProxy* UpdateSuppressor;
  vtkSMSourceProxy* UpdateSuppressorLOD;
  vtkSMSourceProxy* LODDecimator;

private:
  vtkSMSimpleStrategy(const vtkSMSimpleStrategy&); // Not implemented
  void operator=(const vtkSMSimpleStrategy&); // Not implemented
//ETX
};

#endif

