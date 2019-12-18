/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiProcessControllerHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMultiProcessControllerHelper
 * @brief   collection of assorted helper
 * routines dealing with communication.
 *
 * vtkMultiProcessControllerHelper is collection of assorted helper
 * routines dealing with communication.
*/

#ifndef vtkMultiProcessControllerHelper_h
#define vtkMultiProcessControllerHelper_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkSmartPointer.h"              // needed for vtkSmartPointer.

#include <vector> // needed for std::vector

class vtkDataObject;
class vtkMultiProcessController;
class vtkMultiProcessStream;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkMultiProcessControllerHelper : public vtkObject
{
public:
  static vtkMultiProcessControllerHelper* New();
  vtkTypeMacro(vtkMultiProcessControllerHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reduce the stream to all processes calling the (*operation) for reduction.
   * The operation is assumed to be commutative.
   */
  static int ReduceToAll(vtkMultiProcessController* controller, vtkMultiProcessStream& data,
    void (*operation)(vtkMultiProcessStream& A, vtkMultiProcessStream& B), int tag);

  /**
   * Utility method to merge pieces received from several processes. It does not
   * handle all data types, and hence not meant for non-paraview specific use.
   * Returns a new instance of data object containing the merged result on
   * success, else returns NULL. The caller is expected to release the memory
   * from the returned data-object.
   */
  static vtkDataObject* MergePieces(vtkDataObject** pieces, unsigned int num_pieces);

  /**
   * Overload where the merged pieces are combined into result.
   */
  static bool MergePieces(
    std::vector<vtkSmartPointer<vtkDataObject> >& pieces, vtkDataObject* result);

protected:
  vtkMultiProcessControllerHelper();
  ~vtkMultiProcessControllerHelper() override;

private:
  vtkMultiProcessControllerHelper(const vtkMultiProcessControllerHelper&) = delete;
  void operator=(const vtkMultiProcessControllerHelper&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkMultiProcessControllerHelper.h
