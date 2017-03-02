#ifndef VTKPVRANDOMPOINTSSTREAMINGSOURCE_H
#define VTKPVRANDOMPOINTSSTREAMINGSOURCE_H

#include "vtkMultiBlockDataSetAlgorithm.h"

// Description:
// This class generateds a multiblock datastructure of random points in space.
// The dataset generated is an octree of blocks, with one block on the first
// level, eight on the second, etc...

class vtkPVRandomPointsStreamingSource : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkPVRandomPointsStreamingSource, vtkMultiBlockDataSetAlgorithm);
  static vtkPVRandomPointsStreamingSource* New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Sets/Gets the number of levels of detail to create.  Each level will
  // have eight time the number of blocks as the level above and as all blocks
  // have the same number of points, a level has eight times the points of the
  // level above it.
  vtkSetClampMacro(NumLevels, int, 1, 6);
  vtkGetMacro(NumLevels, int);

  // Description:
  // Sets/Gets the number of points per block in the generated data set
  vtkSetMacro(PointsPerBlock, int);
  vtkGetMacro(PointsPerBlock, int);

  // Description:
  // Sets/Gets the random seed used to generate the points
  vtkSetMacro(Seed, int);
  vtkGetMacro(Seed, int);

protected:
  vtkPVRandomPointsStreamingSource();
  virtual ~vtkPVRandomPointsStreamingSource();

  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int NumLevels;
  int PointsPerBlock;
  int Seed;
  class vtkInternals;
  vtkInternals* Internal;
};

#endif // VTKPVRANDOMPOINTSSTREAMINGSOURCE_H
