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
/**
 * @class   vtkSelectionConverter
 * @brief   converts one selection type to another
 *
 * vtkSelectionConverter can be used to convert from one selection type
 * to another. Currently, it only supports conversion from a 'surface'
 * geometry selection to a 'volume' selection. It does this by looking for a
 * pedigree array called vtkOriginalCellIds that says what 3D cell produced
 * each selected 2D surface cell. The input selection must have
 * SOURCE_ID() and ORIGINAL_SOURCE_ID() properties set. The SOURCE_ID()
 * corresponds to the geometry filter whereas the ORIGINAL_SOURCE_ID()
 * corresponds to the input of the geometry filter. The output selection
 * will have SOURCE_ID() corresponding to the input of the geometry filter
 * (what was ORIGINAL_SOURCE_ID()).
 * @sa
 * vtkSelection
*/

#ifndef vtkSelectionConverter_h
#define vtkSelectionConverter_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

class vtkSelection;
class vtkSelectionNode;
class vtkCompositeDataIterator;
class vtkDataSet;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkSelectionConverter : public vtkObject
{
public:
  static vtkSelectionConverter* New();
  vtkTypeMacro(vtkSelectionConverter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convert input selection and store it in output. Currently, the
   * input selection must be a geometry selection and the output
   * is a volume selection.
   * If \c global_ids is set, then the selection is converted to global
   * ids selection.
   */
  void Convert(vtkSelection* input, vtkSelection* output, int global_ids);

protected:
  vtkSelectionConverter();
  ~vtkSelectionConverter() override;

  void Convert(vtkSelectionNode* input, vtkSelection* output, int global_ids);

  vtkDataSet* LocateDataSet(vtkCompositeDataIterator* iter, unsigned int index);

private:
  vtkSelectionConverter(const vtkSelectionConverter&) = delete;
  void operator=(const vtkSelectionConverter&) = delete;

  class vtkKeyType;
};

#endif
