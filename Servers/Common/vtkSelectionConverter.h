/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionConverter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionConverter - converts one selection type to another
// .SECTION Description
// vtkSelectionConverter can be used to convert from one selection type
// to another. Currently, it only supports conversion from a 'surface'
// geometry selection to a 'volume' selection. It does this by looking for a
// pedigree array called vtkOriginalCellIds that says what 3D cell produced
// each selected 2D surface cell. The input selection must have
// SOURCE_ID() and ORIGINAL_SOURCE_ID() properties set. The SOURCE_ID()
// corresponds to the geometry filter whereas the ORIGINAL_SOURCE_ID()
// corresponds to the input of the geometry filter. The output selection
// will have SOURCE_ID() corresponding to the input of the geometry filter
// (what was ORIGINAL_SOURCE_ID()).
// .SECTION See Also
// vtkSelection

#ifndef __vtkSelectionConverter_h
#define __vtkSelectionConverter_h

#include "vtkObject.h"

class vtkSelection;
class vtkSelectionNode;
class vtkCompositeDataIterator;
class vtkDataSet;

class VTK_EXPORT vtkSelectionConverter : public vtkObject
{
public:
  static vtkSelectionConverter* New();
  vtkTypeMacro(vtkSelectionConverter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convert input selection and store it in output. Currently, the
  // input selection must be a geometry selection and the output
  // is a volume selection.
  // If \c global_ids is set, then the selection is converted to global
  // ids selection.
  void Convert(vtkSelection* input, vtkSelection* output, int global_ids);

//BTX
protected:
  vtkSelectionConverter();
  ~vtkSelectionConverter();

  void Convert(vtkSelectionNode* input, vtkSelection* output, int global_ids);

  vtkDataSet* LocateDataSet(vtkCompositeDataIterator* iter, unsigned int index);

private:
  vtkSelectionConverter(const vtkSelectionConverter&);  // Not implemented.
  void operator=(const vtkSelectionConverter&);  // Not implemented.

  class vtkKeyType;
//ETX
};

#endif
