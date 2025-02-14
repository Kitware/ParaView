// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2009 Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
/**
 * @class   vtkPCosmoReader
 * @brief   Read a binary cosmology data file
 *
 *
 * vtkPCosmoReader creates a vtkUnstructuredGrid from a binary cosmology file.
 *
 * A cosmo file is a record format file with no header.
 * One record per particle.
 *
 * Each record is 32 bytes, with fields (in order) for:
 *     x_position (float),
 *     x_velocity (float),
 *     y_position (float),
 *     y_velocity (float),
 *     z-position (float),
 *     z_velocity (float)
 *     mass (float)
 *     identification tag (int64_t)
 *
 * Total particle data can be split into per processor files, with each file
 * name ending in the processor number.
 *
 */

#ifndef vtkPCosmoReader_h
#define vtkPCosmoReader_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPCosmoReader : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPCosmoReader* New();
  vtkTypeMacro(vtkPCosmoReader, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the name of the cosmology particle binary file to read
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify the physical box dimensions size (rL)
   * (default 100.0)
   */
  vtkSetMacro(RL, float);
  vtkGetMacro(RL, float);
  ///@}

  ///@{
  /**
   * Specify the ghost cell spacing in Mpc (in rL units)
   * (edge boundary of processor box)
   * (default 5)
   */
  vtkSetMacro(Overlap, float);
  vtkGetMacro(Overlap, float);
  ///@}

  ///@{
  /**
   * Set the read mode (0 = one-to-one, 1 = default, round-robin)
   */
  vtkSetMacro(ReadMode, int);
  vtkGetMacro(ReadMode, int);
  ///@}

  /**
   * Set whether to byte-swap or not the data. Applicable only to Cosmo format.
   * By default, no byte-swapping is enabled.
   * vtkSetMacro(ByteSwap,int);
   * vtkGetMacro(ByteSwap,int);
   */

  ///@{
  /**
   * Set the filetype to Gadget or Cosmo read mode
   * (0 = Gadget, 1 = default, Cosmo)
   */
  vtkSetMacro(CosmoFormat, int);
  vtkGetMacro(CosmoFormat, int);
  ///@}

  ///@{
  /**
   * Set the communicator object for interprocess communication
   */
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController*);
  ///@}

protected:
  vtkPCosmoReader();
  ~vtkPCosmoReader();

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;

  char* FileName;  // Name of binary particle file
  float RL;        // The physical box dimensions (rL)
  float Overlap;   // The ghost cell boundary space
  int ReadMode;    // The reading mode
                   //  int ByteSwap; // Indicates whether to byte-swap data on read
  int CosmoFormat; // Enable cosmo format or gadget format

private:
  vtkPCosmoReader(const vtkPCosmoReader&) = delete;
  void operator=(const vtkPCosmoReader&) = delete;
};

#endif
