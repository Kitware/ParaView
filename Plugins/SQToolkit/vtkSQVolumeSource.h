/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQVolumeSource -Create volume of hexahedral cells.
// .SECTION Description
// Creates a volume composed of hexahedra cells on a latice.
// This is the 3D counterpart to the plane source.

#ifndef __vtkSQVolumeSource_h
#define __vtkSQVolumeSource_h

#include "vtkUnstructuredGridAlgorithm.h"

class VTK_EXPORT vtkSQVolumeSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkSQVolumeSource *New();
  vtkTypeRevisionMacro(vtkSQVolumeSource,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the points defining edges of a 3D quarilateral.
  vtkSetVector3Macro(Origin,double);
  vtkGetVector3Macro(Origin,double);

  vtkSetVector3Macro(Point1,double);
  vtkGetVector3Macro(Point1,double);

  vtkSetVector3Macro(Point2,double);
  vtkGetVector3Macro(Point2,double);

  vtkSetVector3Macro(Point3,double);
  vtkGetVector3Macro(Point3,double);

  // Description:
  // Set the latice resolution in the given direction.
  vtkSetVector3Macro(Resolution,int);
  vtkGetVector3Macro(Resolution,int);

  // Description:
  // Toggle between immediate mode and demand mode. In immediate
  // mode requested geometry is gernerated and placed in the output
  // in demand mode a cell generator is placed in the pipeline and
  // a single cell is placed in the output.
  vtkSetMacro(ImmediateMode,int);
  vtkGetMacro(ImmediateMode,int);

protected:
  /// Pipeline internals.
  //int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQVolumeSource();
  ~vtkSQVolumeSource();

private:
  int ImmediateMode;
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Point3[3];
  int Resolution[3];

private:
  vtkSQVolumeSource(const vtkSQVolumeSource&);  // Not implemented.
  void operator=(const vtkSQVolumeSource&);  // Not implemented.
};

#endif
