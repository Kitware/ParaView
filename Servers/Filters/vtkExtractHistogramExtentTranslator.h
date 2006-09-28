/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogramExtentTranslator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractHistogramExtentTranslator - extent translator for
// vtkExtractHistogram filter.

#ifndef __vtkExtractHistogramExtentTranslator_h
#define __vtkExtractHistogramExtentTranslator_h

#include "vtkExtentTranslator.h"

class VTK_EXPORT vtkExtractHistogramExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkExtractHistogramExtentTranslator* New();
  vtkTypeRevisionMacro(vtkExtractHistogramExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkExtractHistogramExtentTranslator();
  ~vtkExtractHistogramExtentTranslator();

  virtual int PieceToExtentThreadSafe(int vtkNotUsed(piece), 
                                      int vtkNotUsed(numPieces), 
                                      int vtkNotUsed(ghostLevel), 
                                      int *wholeExtent, int *resultExtent, 
                                      int vtkNotUsed(splitMode), 
                                      int vtkNotUsed(byPoints))
    {
    memcpy(resultExtent, wholeExtent, sizeof(int)*6);
    return 1;
    }
private:
  vtkExtractHistogramExtentTranslator(const vtkExtractHistogramExtentTranslator&); // Not implemented.
  void operator=(const vtkExtractHistogramExtentTranslator&); // Not implemented.
  
};

#endif

