// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVEnSightMasterServerTranslator
 *
 */

#ifndef vtkPVEnSightMasterServerTranslator_h
#define vtkPVEnSightMasterServerTranslator_h

#include "vtkExtentTranslator.h"
#include "vtkPVVTKExtensionsIOEnSightModule.h" //needed for exports

class VTKPVVTKEXTENSIONSIOENSIGHT_EXPORT vtkPVEnSightMasterServerTranslator
  : public vtkExtentTranslator
{
public:
  static vtkPVEnSightMasterServerTranslator* New();
  vtkTypeMacro(vtkPVEnSightMasterServerTranslator, vtkExtentTranslator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the piece that should provide the data.  All other pieces
   * should provide empty data.
   */
  vtkGetMacro(ProcessId, int);
  vtkSetMacro(ProcessId, int);
  ///@}

  ///@{
  /**
   * Translates the piece matching ProcessId to the whole extent, and
   * all other pieces to empty.
   */
  int PieceToExtentThreadSafe(int piece, int numPieces, int ghostLevel, int* wholeExtent,
    int* resultExtent, int splitMode, int byPoints) override;

protected:
  vtkPVEnSightMasterServerTranslator();
  ~vtkPVEnSightMasterServerTranslator() override;
  ///@}

  // The process id on which this translator is running.
  int ProcessId;

private:
  vtkPVEnSightMasterServerTranslator(const vtkPVEnSightMasterServerTranslator&) = delete;
  void operator=(const vtkPVEnSightMasterServerTranslator&) = delete;
};

#endif
