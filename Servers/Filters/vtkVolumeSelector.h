/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeSelector - Given a frustrum and a list of datasets, creates a selection
// .SECTION Description
// vtkVolumeSelector selects cells that are in the given frustrum from a
// list of datasets. Internally, it uses vtkFrustumExtractor to select
// cells. The output is a vtkSelection tree. Each node in the tree contains
// a SOURCE_ID (the proxy id of the producer of the dataset), a PROP_ID (if
// assigned with AddProp), the PROCESS_ID and the selection list array.
//
// .SECTION See Also
// vtkFrustumExtractor

#ifndef __vtkVolumeSelector_h
#define __vtkVolumeSelector_h

#include "vtkObject.h"

//BTX
class vtkAlgorithm;
class vtkFrustumExtractor;
class vtkProp;
class vtkSelection;
struct vtkVolumeSelectorInternals;
//ETX

class VTK_EXPORT vtkVolumeSelector : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkVolumeSelector,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkVolumeSelector *New();

  // Description:
  // Clears props and datasets.
  void Initialize();

  // Description:
  // Clears datasets.
  void ClearDataSets();

  // Description:
  // Tries to obtain a dataset from the pointer, and adds the dataset
  // to the output.
  void AddDataSet(vtkAlgorithm* source);

  // Description:
  // Clears original sources.
  void ClearOriginalSources();

  // Description:
  // Add an original source to the list. These are used to populate
  // the selection. Make sure that these are added in the same order
  // as datasets.
  void AddOriginalSource(vtkAlgorithm* source);

  // Description:
  // Clears props.
  void ClearProps();

  // Description:
  // Add a prop to the internal list. These are used to populate the
  // selection. Make sure that props are added in the same order as
  // datasets.
  void AddProp(vtkProp* prop);

  // Description:
  // Given eight vertices, creates a frustum.
  void CreateFrustum(double vertices[32]);

  // Description:
  // Set the selection object to be used to store the results of the
  // selection operation.
  void SetSelection(vtkSelection* selection);

  // Description:
  // Get the selection object to be used to store the results of the
  // selection operation.
  vtkGetObjectMacro(Selection, vtkSelection);

  // Description:
  // Sets/gets the intersection test type.
  // Off extracts only those points and cells that are completely within the 
  // frustum. On extracts all of the above as well as those cells that cross 
  // the frustum along with all of their points.
  // On is the default.
  void SetExactTest(int et);
  int GetExactTest();

  // Description:
  // Performs the selection.
  void Select();

protected:
  vtkVolumeSelector();
  ~vtkVolumeSelector();

  vtkFrustumExtractor *AtomExtractor;
  vtkVolumeSelectorInternals* Internal;
  vtkSelection* Selection;

private:
  vtkVolumeSelector(const vtkVolumeSelector&);  // Not implemented.
  void operator=(const vtkVolumeSelector&);  // Not implemented.
};

#endif
