/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHFractal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFractal - A source to test my new CTH AMR data object.
// .SECTION Description
// vtkCTHFractal is a collection of image datas.  All have the same dimensions.
// Each block has a different origin and spacing.  It uses mandelbrot
// to create cell data.  I scale the fractal array to look like a volme fraction.
// I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHFractal_h
#define __vtkCTHFractal_h

#include "vtkCTHSource.h"

class vtkCTHData;
class vtkIntArray;

class VTK_EXPORT vtkCTHFractal : public vtkCTHSource
{
public:
  static vtkCTHFractal *New();

  vtkTypeRevisionMacro(vtkCTHFractal,vtkCTHSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Essentially the iso surface value.
  // The fractal array is scaled to map this value to 0.5 for use as a volume fraction.
  vtkSetMacro(FractalValue, float);
  vtkGetMacro(FractalValue, float);  

  // Description:
  // Any blocks touching a predefined line will be subdivided to this level.
  // Other blocks are subdivided so that neighboring blocks only differ
  // by one level.
  vtkSetMacro(MaximumLevel, int);
  vtkGetMacro(MaximumLevel, int);

  // Description:
  // XYZ dimensions of cells.
  vtkSetMacro(Dimensions, int);
  vtkGetMacro(Dimensions, int);

  // Description:
  // For testing ghost levels.
  vtkSetMacro(GhostLevels, int);
  vtkGetMacro(GhostLevels, int);
  vtkBooleanMacro(GhostLevels, int);

  // Description:
  // Make a 2D data set to test.
  vtkSetMacro(TwoDimensional, int);
  vtkGetMacro(TwoDimensional, int);
  vtkBooleanMacro(TwoDimensional, int);

  // Description:
  // Test the case when the blocks do not have the same sizes.
  // Adds 2 to the x extent of the far x blocks (level 1).
  vtkSetMacro(Asymetric,int);
  vtkGetMacro(Asymetric,int);

protected:
  vtkCTHFractal();
  ~vtkCTHFractal();

  int StartBlock;
  int EndBlock;
  int BlockCount;
  virtual void ExecuteInformation();
  virtual void Execute();
  void Traverse(int &blockId, int level, vtkCTHData* output, 
                int x0,int x1, int y0,int y1, int z0,int z1);

  int LineTest2(float x0, float y0, float z0, 
                float x1, float y1, float z1,
                double bds[6]); 
  int LineTest(float x0, float y0, float z0, 
               float x1, float y1, float z1,
               double bds[6], int level, int target); 

  void SetBlockInfo(int blockId, int level, int* ext);

  void AddVectorArray();
  void AddTestArray();
  void AddFractalArray();
  void AddBlockIdArray();
  void AddDepthArray();
  void AddGhostLevelArray();

  int MandelbrotTest(double x, double y);
  int TwoDTest(double bds[6], int level, int target);

private:
  void InternalImageDataCopy(vtkCTHFractal *src);

  int Asymetric;
  int MaximumLevel;
  int Dimensions;
  float FractalValue;
  int GhostLevels;
  vtkIntArray *Levels;
  int TwoDimensional;


private:
  vtkCTHFractal(const vtkCTHFractal&);  // Not implemented.
  void operator=(const vtkCTHFractal&);  // Not implemented.
};


#endif



