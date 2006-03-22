/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalCacheFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalCacheFilter - Filter that caches.
// .SECTION Description
// This is a more generalized for of vtkTemporalPickFilter 
// and vtkTemporalProbeFilter. When running in MPI mode,
// this filter assumes that all the input data is available
// only on the Root Node. The data on all other nodes is ignored.
// (This is true with vtkPickProbeFilter. It collects the data on
// root node.)

#ifndef __vtkTemporalCacheFilter_h
#define __vtkTemporalCacheFilter_h

#include "vtkPointSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkTemporalCacheFilter : public vtkPointSetAlgorithm
{
public:
  static vtkTemporalCacheFilter* New();
  vtkTypeRevisionMacro(vtkTemporalCacheFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set if this filter should use the collected cache or
  // simply behave as a pass-thru filter. By default, set to 0, hence
  // behaves as a pass-thru filter. This flag is ignored when 
  // CollectedData is empty, in that case, the filter always behaves
  // as a pass-thru filter.
  vtkSetMacro(UseCache, int);
  vtkGetMacro(UseCache, int);
  
  // Description:
  // Clear the collected data cache.
  void ClearCache();

  // Description:
  // Called to collect the data attribute (point/cell or field) at the input. 
  // Use SetAttributeToCollect to choose which attribute is collected.
  // A new point is added at (time, 0, 0) and the 0th tuple from the choosen attibute 
  // is collected as the point data associated with the newly inserted point.
  void CollectAttributeData(double time);
  
  //BTX
  enum 
    {
    POINT_DATA,
    CELL_DATA,
    FIELD_DATA
    };
  //ETX

  // Description:
  // Choose the attribute to collect. This filter can collect only 1 attribute.
  // By default, set to POINT_DATA.
  vtkSetClampMacro(AttributeToCollect, int, POINT_DATA, FIELD_DATA);
  vtkGetMacro(AttributeToCollect, int);
  void SetAttributeToCollectToPointData() { this->SetAttributeToCollect(POINT_DATA); }
  void SetAttributeToCollectToCellData() { this->SetAttributeToCollect(CELL_DATA); }
  void SetAttributeToCollectToFieldData() { this->SetAttributeToCollect(FIELD_DATA); }

  // Description:
  // Set the multiprocess controller. 
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
protected:
  vtkTemporalCacheFilter();
  ~vtkTemporalCacheFilter();

  void InitializeCollection(vtkPointSet* input);

  virtual int RequestData(vtkInformation *, vtkInformationVector **, 
    vtkInformationVector *);

  vtkPointSet* CollectedData;
  int UseCache;
  int AttributeToCollect;
  vtkMultiProcessController* Controller;

private:
  vtkTemporalCacheFilter(const vtkTemporalCacheFilter&); // Not implemented.
  void operator=(const vtkTemporalCacheFilter&); // Not implemented.
};
#endif

