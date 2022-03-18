/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPCGNSWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
Copyright (c) Maritime Research Institute Netherlands (MARIN)
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class vtkPCGNSWriter
 * @brief Writes CGNS file in parallel using serial I/O
 *
 * This writer writes (composite) datasets that may consist of
 *   - vtkStructuredGrid
 *   - vtkUnstructuredGrid
 *   - vtkPolydata
 *   - vtkCompositeDataSet
 *
 * The writer is intended to be used in a distributed MPI process
 * and lets each process write to the same CGNS file. The writing
 * is coordinated between processes and each process writes after
 * its predecessor. In that way it is writing files in parallel,
 * but using serial I/O.
 *
 * The reason that serial I/O appraoch is chosen is that the CGNS
 * library does have support for true parallel I/O, but that this
 * is not implemented for all cell types (notably not for
 * VTK_POLYGON or for VTK_POLYHEDRON).
 *
 */

#ifndef vtkPCGNSWriter_h
#define vtkPCGNSWriter_h

#include "vtkCGNSWriter.h"
#include "vtkPVVTKExtensionsIOParallelCGNSWriterModule.h" // for export macro
#include "vtkSmartPointer.h"                              // for vtkSmartPointer

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSIOPARALLELCGNSWRITER_EXPORT vtkPCGNSWriter : public vtkCGNSWriter
{
public:
  static vtkPCGNSWriter* New();
  vtkTypeMacro(vtkPCGNSWriter, vtkCGNSWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the MPI controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  virtual vtkMultiProcessController* GetController();
  //@}

protected:
  vtkPCGNSWriter();
  ~vtkPCGNSWriter() override = default;

  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override;

  int NumberOfPieces = 0;
  int RequestPiece = -1;

  vtkSmartPointer<vtkMultiProcessController> Controller;

private:
  vtkPCGNSWriter(const vtkPCGNSWriter&) = delete;
  void operator=(const vtkPCGNSWriter&) = delete;
};

#endif // vtkPCGNSWriter_h
