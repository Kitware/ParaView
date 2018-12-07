/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPExtentTranslator.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPExtentTranslator
 * @brief   extent translator that collects information
 * about extents from multiple processes in parallel.
 *
 * vtkPExtentTranslator is used by vtkImageVolumeRepresentation to collect
 * information about image extents on all the ranks. This is resurrected version
 * of vtkPVTrivialExtentTranslator.
*/

#ifndef vtkPExtentTranslator_h
#define vtkPExtentTranslator_h

#include "vtkExtentTranslator.h"
#include "vtkPVClientServerCoreRenderingModule.h" // needed for export macro

class vtkDataSet;
class vtkPExtentTranslatorInternals;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPExtentTranslator : public vtkExtentTranslator
{
public:
  static vtkPExtentTranslator* New();
  vtkTypeMacro(vtkPExtentTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If DataSet is topologically regular, each process will only know
   * about its own subextent.  This function does an allreduce to make sure
   * that each process knows the subextent of every process.
   */
  void GatherExtents(vtkDataSet* dataset);

protected:
  vtkPExtentTranslator();
  ~vtkPExtentTranslator() override;
  int PieceToExtentThreadSafe(int vtkNotUsed(piece), int vtkNotUsed(numPieces),
    int vtkNotUsed(ghostLevel), int* wholeExtent, int* resultExtent, int vtkNotUsed(splitMode),
    int vtkNotUsed(byPoints)) override;

private:
  vtkPExtentTranslator(const vtkPExtentTranslator&) = delete;
  void operator=(const vtkPExtentTranslator&) = delete;

  vtkPExtentTranslatorInternals* Internals;
};

#endif
