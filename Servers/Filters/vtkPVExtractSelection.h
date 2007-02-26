/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractSelection - performs the extraction operation indicated
// by the vtkSelection
// .SECTION Description
// vtkPVExtractSelection takes in a list of data sets and a desired 
// extraction operation and performs the extraction. The particular operation
// is designated by the vtkSelection input. Internally, it uses 
// vtkExtractSelection to select cells to do its work. The output is a 
// vtkSelection tree. Each node in the tree contains a SOURCE_ID (the proxy 
// id of the producer of the dataset), a PROP_ID (if assigned with AddProp), 
// the PROCESS_ID and the selection list array.
//
// .SECTION See Also
// vtkExtractSelection vtkSelection vtkSMSelectionProxy

#ifndef __vtkPVExtractSelection_h
#define __vtkPVExtractSelection_h

#include "vtkObject.h"

//BTX
class vtkAlgorithm;
class vtkExtractSelection;
class vtkProp;
class vtkSelection;
struct vtkPVExtractSelectionInternals;
//ETX

class VTK_EXPORT vtkPVExtractSelection : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVExtractSelection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkPVExtractSelection *New();

  // Description:
  // Clears props and datasets.
  void Initialize();

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
  // Set the selection object that says what type of extraction to do
  // and what its parameters are.
  void SetInputSelection(vtkSelection* selection);

  // Description:
  // Get the selection object that says what type of extraction to do
  // and what its parameters are.
  vtkGetObjectMacro(InputSelection, vtkSelection);

  // Description:
  // Set the selection object to be used to store the results of the
  // selection operation.
  void SetOutputSelection(vtkSelection* selection);

  // Description:
  // Get the selection object to be used to store the results of the
  // selection operation.
  vtkGetObjectMacro(OutputSelection, vtkSelection);

  // Description:
  // Performs the selection.
  void Select();

protected:
  vtkPVExtractSelection();
  ~vtkPVExtractSelection();

  vtkExtractSelection *AtomExtractor;
  vtkPVExtractSelectionInternals* Internal;

  vtkSelection* InputSelection;
  vtkSelection* OutputSelection;

  int selType;
  double PointCoords[3];
private:
  vtkPVExtractSelection(const vtkPVExtractSelection&);  // Not implemented.
  void operator=(const vtkPVExtractSelection&);  // Not implemented.
};

#endif
