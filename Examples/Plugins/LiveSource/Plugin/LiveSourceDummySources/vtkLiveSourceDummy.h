#pragma once

#include "LiveSourceDummySourcesModule.h"
#include "vtkPolyDataAlgorithm.h"

#include <array>

class vtkPoints;
class vtkCellArray;

struct Coords
{
  double a, b, c;
  Coords(double pa = .0, double pb = .0, double pc = .0)
    : a{ pa }
    , b{ pb }
    , c{ pc }
  {
  }
};

struct TriangleBase
{
  std::array<Coords, 3> tri = { { { 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.5, 0.66 } } };

  Coords middle(Coords pl, Coords pr)
  {
    return Coords((pl.a + pr.a) / 2, (pl.b + pr.b) / 2, (pl.c + pr.c) / 2);
  }

  Coords next(Coords current)
  {
    int base = vtkMath::Random(0, 3);
    return middle(current, tri[base]);
  }
};

class LIVESOURCEDUMMYSOURCES_EXPORT vtkLiveSourceDummy : public vtkPolyDataAlgorithm
{
public:
  static vtkLiveSourceDummy* New();
  vtkTypeMacro(vtkLiveSourceDummy, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the maximum number of iteration before the live source stops.
   */
  vtkSetMacro(MaxIterations, int);
  vtkGetMacro(MaxIterations, int);
  ///@}

  /**
   * Check if the RequestUpdateExtent/RequestData need to be called again to refresh the output.
   * Return true if an update is needed.
   *
   * This method is required for Live Source.
   */
  bool GetNeedsUpdate();

protected:
  vtkLiveSourceDummy();
  ~vtkLiveSourceDummy() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  // This parameter can be modified by the user and serve to demonstrate how a live source can stop
  // updating
  int MaxIterations = 2000;
  // This parameter is used to store and update the current iteration
  int CurIteration = 0;

  vtkNew<vtkPoints> Points;
  vtkNew<vtkCellArray> Cells;
  Coords Pos{ .0, .0, .0 };
  TriangleBase Tri;

  vtkLiveSourceDummy(const vtkLiveSourceDummy&) = delete;
  void operator=(const vtkLiveSourceDummy&) = delete;
};
