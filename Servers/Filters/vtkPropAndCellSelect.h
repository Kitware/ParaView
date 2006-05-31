/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAndCellSelect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPropAndCellSelect - 
// .SECTION Description

#ifndef __vtkPropAndCellSelect_h
#define __vtkPropAndCellSelect_h

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkSelectDataSets;
class vtkFrustumExtractor;
class vtkCompositeDataPipeline;

class VTK_EXPORT vtkPropAndCellSelect : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkPropAndCellSelect,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkPropAndCellSelect *New();

  //////////////////////////////

  // Description:
  // Empties the output.
  void Initialize();

  // Description:
  // Tries to obtain a dataset from the pointer, and adds the dataset to the
  // output.
  void AddDataSet(vtkObject *obj);

  // Description:
  // Returns the number of datasets that are in the output.
  int GetNumProps();  

  ///////////////////////////////////////////////////
  // Description:
  // Given eight vertices, creates a frustum.
  void CreateFrustum(double vertices[32]);

  // Description:
  // When On, this returns an unstructured grid that outlines selection area.
  // Off is the default.
  void SetShowBounds(int sb);
  int GetShowBounds();

  // Description:
  // Sets/gets the intersection test type.
  // Off extracts only those points and cells that are completely within the 
  // frustum. On extracts all of the above as well as those cells that cross 
  // the frustum along with all of their points.
  // On is the default.
  void SetExactTest(int et);
  int GetExactTest();

  // Description:
  // Sets/Gets the output data type.
  // If On, the input data set is shallow copied through, and two new
  // "vtkInsidedness" attribute arrays are added. If Off the output is 
  // a new vtkUnstructuredGrid containing only the structure that is inside.
  // Off is the default.
  void SetPassThrough(int pt);
  int GetPassThrough();

  // Description:
  // Sets/Gets the selection type.
  // 0 means select props, 1 means select surface, 2 means select volumes.
  void SetSelectionType(int st);
  int GetSelectionType();

protected:
  vtkPropAndCellSelect();
  ~vtkPropAndCellSelect();

  virtual int FillOutputPortInformation(int, vtkInformation *);


  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkCompositeDataPipeline *CompositeDataPipeline1;
  vtkSelectDataSets *PropPicker;
  vtkFrustumExtractor *AtomExtractor;


  int SelectionType;
private:
  vtkPropAndCellSelect(const vtkPropAndCellSelect&);  // Not implemented.
  void operator=(const vtkPropAndCellSelect&);  // Not implemented.
};

#endif
