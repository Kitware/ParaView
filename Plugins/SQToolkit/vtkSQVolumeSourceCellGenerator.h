/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __vtkSQVolumeSourceCellGenerator_h
#define __vtkSQVolumeSourceCellGenerator_h

#include "vtkSQCellGenerator.h"
#include "vtkCellType.h"

/// Plane sources that provide data on demand
/**
*/
class vtkSQVolumeSourceCellGenerator : public vtkSQCellGenerator
{
public:
  static vtkSQVolumeSourceCellGenerator *New();
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkSQVolumeSourceCellGenerator, vtkObject);

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()
    {
    return this->Resolution[0]*this->Resolution[1]*this->Resolution[2];
    }

  /**
  Return the cell type of the cell at id. 
  */
  virtual int GetCellType(vtkIdType id){ return VTK_HEXAHEDRON; }

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType id){ return 8;}

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enough. Return the number
  of points coppied.
  */
  virtual int GetCellPoints(vtkIdType cid, float *pts);

  /**
  Copy the point's indexes into the provided bufffer,
  buffer is expected to be large enough. Return the 
  number of points coppied. The index is unique across
  all processes but is not the same as the point id
  in a VTK dataset.
  */
  virtual int GetCellPointIndexes(vtkIdType cid, vtkIdType *idx);

  /**
  Set/Get plane cell resolution.
  */
  void SetResolution(int *r);
  void SetResolution(int rx, int ry, int rz);
  vtkGetVector3Macro(Resolution,int);

  /**
  Set/Get plane coordinates, origin,point1,point2.
  */
  void SetOrigin(double *o);
  void SetOrigin(double x, double y, double z);
  vtkGetVector3Macro(Origin,double);

  void SetPoint1(double *o);
  void SetPoint1(double x, double y, double z);
  vtkGetVector3Macro(Point1,double);

  void SetPoint2(double *o);
  void SetPoint2(double x, double y, double z);
  vtkGetVector3Macro(Point2,double);

  void SetPoint3(double *o);
  void SetPoint3(double x, double y, double z);
  vtkGetVector3Macro(Point3,double);

  /**
  Call this after any change to coordinates and resolution.
  */
  void ComputeDeltas();

protected:
  vtkSQVolumeSourceCellGenerator();
  virtual ~vtkSQVolumeSourceCellGenerator(){};

private:
  int Resolution[6];
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Point3[3];
  double Dx[3];
  double Dy[3];
  double Dz[3];
};

#endif