/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRIncrementalResampleHelper.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRIncrementalResampleHelper.h -- Resamples AMR data incrementally.
//
// .SECTION Description
//  A helper object that provides functionality for resampling AMR dataset
//  into a user-supplied ROI.
//
// .SECTION Caveats
//  The code assumes 3-D cell-centered AMR data.
//
// .SECTION See Also
//  vtkOverlappingAMR
#ifndef VTKAMRINCREMENTALRESAMPLEHELPER_H_
#define VTKAMRINCREMENTALRESAMPLEHELPER_H_

#include "vtkObject.h"

// Forward declarations
class vtkAMRDomainParameters;
class vtkAMRProcessedBlocksList;
class vtkOverlappingAMR;
class vtkUniformGrid;
class vtkMultiProcessController;
class vtkFieldData;
class vtkCellData;

class VTK_EXPORT vtkAMRIncrementalResampleHelper : public vtkObject
{
public:
  static vtkAMRIncrementalResampleHelper *New();
  vtkTypeMacro(vtkAMRIncrementalResampleHelper,vtkObject);
  void PrintSelf(ostream &oss, vtkIndent indent);

  // Description:
  // Set & Get macro for the multi-process controller
  vtkSetMacro(Controller, vtkMultiProcessController*);
  vtkGetMacro(Controller, vtkMultiProcessController*);

  // Description:
  // Gets the grid generated on this process.
  vtkGetMacro(Grid,vtkUniformGrid*);

  // Description:
  // Initializes this class instance with the AMR domain parameters
  void Initialize(vtkOverlappingAMR *amrmetadata);

  // Description:
  // Sets pointer to the user AMR data. The user, may update the AMR data
  // and call Update() to deposit the updated data on to the ROI grid.
  // It is suggested that level 0 is always populated.
  vtkSetMacro(AMRData,vtkOverlappingAMR*);

  // Description:
  // Set the ROI to the region corresponding to the box with the given bounds
  // and use the given number of samples, N, along each dimension to resolve
  // features. The bounds vector is given as, (xmin,xmax,ymin,ymax,kmin,kmax)
  // and the number of samples, N[i], corresponds to the number of samples in
  // the I,J,K directions respectively for all i in [0,2].
  void UpdateROI(double bounds[6], int N[3]);

  // Description:
  // Loops through the given AMR data and updates the resampled grid
  void Update();

  // Description:
  // For debugging, use this method to write out the sampled uniform grid at the
  // given location.
  bool WriteGrid(const char* filename);

protected:
  vtkAMRIncrementalResampleHelper();
  virtual ~vtkAMRIncrementalResampleHelper();

  // Description:
  // This method is called from within UpdateROI which initializes the output
  // grid of this process with the
  void InitializeGrid();

  // Description:
  // Initializes the user-supplied field data, F, of the specified size, with
  // the arrays provided in the src field.
  void InitializeGridFields(vtkFieldData *F,vtkIdType size,vtkFieldData *src );

  // Description:
  // Copies the data to the target from the given source.
  void CopyData( vtkFieldData *target, vtkIdType targetIdx,
    vtkCellData *src, vtkIdType srcIdx );

  // Description:
  // Transfer solution from a candidate donor grid at the given level.
  void TransferSolutionFromGrid( vtkUniformGrid *donorGrid, int level );

  // Description:
  // For the given grid and box bounds, min,max, compute the corresponding
  // extent on  the grid.
  void ComputeExtent(vtkUniformGrid *G,double min[3],double max[3],int ext[6]);

  // Description:
  // Return a reference grid from the user-supplied AMR data used to determine
  // which arrays are loaded etc.
  vtkUniformGrid* GetReferenceGrid();

  // Description:
  // Inserts the given block index to the list of processed blocks
  void MarkBlockAsProcessed(const int blockIdx);

  // Description:
  // Checks if the block has been processed
  bool HasBlockBeenProcessed(const int blockIdx);

  // Description:
  // Clears list of processed blocks
  void ClearProcessedBlocks();

  int NumberOfSamples[3]; // The desired number of samples to produce (supplied)
  double ROIBounds[6]; // The bounds of the ROI (supplied)
  double h[3]; // The grid spacing (computed)
  vtkOverlappingAMR *Metadata; // The AMR metadata (supplied)
  vtkOverlappingAMR *AMRData; // The actual AMR data (supplied)

  vtkAMRProcessedBlocksList *ProcessedBlocks; // List of blocks already processed

  vtkUniformGrid *Grid; // The grid owned by this process
  vtkMultiProcessController* Controller; // The controller used.

private:
  vtkAMRIncrementalResampleHelper(const vtkAMRIncrementalResampleHelper&); // Not implemented
  void operator=(const vtkAMRIncrementalResampleHelper&); // Not implemented
};
#endif /* VTKAMRINCREMENTALRESAMPLEHELPER_H_ */
