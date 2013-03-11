#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

class Grid
{
public:
  Grid();
  void Initialize(const unsigned int numPoints[3], const double spacing[3] )
  {
    if(numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
      {
      std::cerr << "Must have a non-zero amount of points in each direction.\n";
      }
    // create the points -- slowest in the x and fastest in the z directions
    double coord[3] = {0,0,0};
    for(unsigned int i=0;i<numPoints[0];i++)
      {
      coord[0] = i*spacing[0];
      for(unsigned int j=0;j<numPoints[1];j++)
        {
        coord[1] = j*spacing[1];
        for(unsigned int k=0;k<numPoints[2];k++)
          {
          coord[2] = k*spacing[2];
          // add the coordinate to the end of the vector
          std::copy(coord, coord+3, std::back_inserter(this->Points));
          }
        }
      }
    // create the hex cells
    unsigned int cellPoints[8];
    for(unsigned int i=0;i<numPoints[0]-1;i++)
      {
      for(unsigned int j=0;j<numPoints[1]-1;j++)
        {
        for(unsigned int k=0;k<numPoints[2]-1;k++)
          {
          cellPoints[0] = i*numPoints[1]*numPoints[2] +
            j*numPoints[2] + k;
          cellPoints[1] = (i+1)*numPoints[1]*numPoints[2] +
            j*numPoints[2] + k;
          cellPoints[2] = i*numPoints[1]*numPoints[2] +
            (j+1)*numPoints[2] + k;
          cellPoints[3] = (i+1)*numPoints[1]*numPoints[2] +
            (j+1)*numPoints[2] + k;
          cellPoints[4] = i*numPoints[1]*numPoints[2] +
            j*numPoints[2] + k+1;
          cellPoints[5] = (i+1)*numPoints[1]*numPoints[2] +
            j*numPoints[2] + k+1;
          cellPoints[6] = i*numPoints[1]*numPoints[2] +
            (j+1)*numPoints[2] + k+1;
          cellPoints[7] = (i+1)*numPoints[1]*numPoints[2] +
            (j+1)*numPoints[2] + k+1;
          std::copy(cellPoints, cellPoints+8, std::back_inserter(this->Cells));
          }
        }
      }
  }
  size_t GetNumberOfPoints()
  {
    return this->Points.size()/3;
  }
  size_t GetNumberOfCells()
  {
    return this->Cells.size()/8;
  }
  double* GetPointsArray()
  {
    if(this->Points.empty())
      {
      return NULL;
      }
    return &(this->Points[0]);
  }
  double* GetPoint(size_t pointId)
  {
    if(pointId > this->Points.empty())
      {
      return NULL;
      }
    return &(this->Points[pointId*3]);
  }
  unsigned int* GetCellPoints(size_t cellId)
  {
    if(cellId > this->Cells.empty())
      {
      return NULL;
      }
    return &(this->Cells[cellId*8]);
  }
private:
  std::vector<double> Points;
  std::vector<unsigned int> Cells;
};

class Attributes
{
// A class for generating and storing point and cell fields.
// Velocity is stored at the points and pressure is stored
// for the cells.
public:
  Attributes()
  {
    this->GridPtr = NULL;
  }
  void Initialize(Grid* grid)
  {
    this>GridPtr = Grid;
  }
  void UpdateFields(double time)
  {
    size_t numPoints = this->GridPtr->GetNumberOfPoints();
    this->Velocity.resize(numPoints);
    for(size_t pt=0;pt<numPoints;pt++)
      {
      double* coord = this->GridPtr.GetPoint(pt);
      this->Velocity[pt] = coord[1]*time;
      }
    std::fill(this->Velocity.begin()+numPoints, this->Velocity.end()+3*numPoints, 0.);
    size_t numCells = this->GridPtr->GetNumberOfCells();
    this->Pressure.resize(numCells);
    std::fill(this->Pressure.begin(), this->Pressure.end(), 1.);
  }

  double* GetVelocityArray()
  {
    if(this->Velocity.empty())
      {
      return NULL;
      }
  }

private:
  std::vector<double> Velocity;
  std::vector<float> Pressure;
  Grid* GridPtr;
};
#endif
