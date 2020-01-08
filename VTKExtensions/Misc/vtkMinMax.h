/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMinMax.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMinMax
 * @brief   Finds the min, max, or sum of all of its input data
 * attributes.
 *
 *
 * This filter lets the user choose from a set of operations and then runs
 * that operation on all of the attribute data of its input(s). For example
 * if MIN is requested, it finds the minimum values in all of its input data
 * arrays. If this filter has multiple input data sets attached to its
 * first input port, it will run the operation on each input data set in
 * turn, producing a global minimum value over all the inputs. In parallel
 * runs this filter REQUIRES ghost arrays to skip redundant
 * information. The output of this filter will always be a single vtkPolyData
 * that contains exactly one point and one cell (a VTK_VERTEX).
*/

#ifndef vtkMinMax_h
#define vtkMinMax_h

#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro
#include "vtkPolyDataAlgorithm.h"

class vtkFieldData;
class vtkAbstractArray;
class vtkUnsignedCharArray;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkMinMax : public vtkPolyDataAlgorithm
{
public:
  static vtkMinMax* New();
  vtkTypeMacro(vtkMinMax, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Selects the operation to perform on the data.
   * min/max, sum...
   */

  enum Operations
  {
    MIN = 0,
    MAX = 1,
    SUM = 2
  };

  vtkSetClampMacro(Operation, int, MIN, SUM);
  vtkGetMacro(Operation, int);
  void SetOperation(const char* op);

  //@{
  /**
   * A diagnostic that should be zero.
   * One indicates that some array didn't match up exactly.
   */
  vtkGetMacro(MismatchOccurred, int);
  //@}

  //@{
  /**
   * Contains a flag for each component of each (Point or Cell) array
   * that indicates if any of the results were never initialized.
   */
  vtkGetStringMacro(FirstPasses);
  void FlagsForPoints();
  void FlagsForCells();
  //@}

  // temp for debugging
  const char* Name;
  vtkIdType Idx;

protected:
  vtkMinMax();
  ~vtkMinMax() override;

  // overridden to allow multiple inputs to port 0
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // run the algorithm
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // helper methods to break up the work
  void OperateOnField(vtkFieldData* id, vtkFieldData* od);
  void OperateOnArray(vtkAbstractArray* ia, vtkAbstractArray* oa);

  // choice of operation to perform
  int Operation;

  // for keeping track of data initialization on the first value
  int ComponentIdx;
  char* CFirstPass;
  char* PFirstPass;
  char* FirstPasses;

  // for deciding what cells and points to ignore
  vtkUnsignedCharArray* GhostArray;

  // a flag that indicates if values computed could be inaccurate
  int MismatchOccurred;

private:
  vtkMinMax(const vtkMinMax&) = delete;
  void operator=(const vtkMinMax&) = delete;
};

#endif
