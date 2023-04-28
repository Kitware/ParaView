#pragma once

#include "LiveSourceDummyModule.h"
#include "vtkPolyDataAlgorithm.h"

#include <array>

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
  std::array<Coords, 3> tri = { { { 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.5, 0.5 } } };

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

class LIVESOURCEDUMMY_EXPORT vtkLiveSourceDummy : public vtkPolyDataAlgorithm
{
public:
  static vtkLiveSourceDummy* New();
  vtkTypeMacro(vtkLiveSourceDummy, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(MaxIterations, int);
  vtkGetMacro(MaxIterations, int);

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool GetNeedsUpdate();

protected:
  vtkLiveSourceDummy();
  virtual ~vtkLiveSourceDummy() {}

private:
  int MaxIterations = 2000;
  int CurIteration = 0;

  vtkNew<vtkPoints> Points;
  vtkNew<vtkCellArray> Cells;
  Coords Pos{ .0, .0, .0 };
  TriangleBase Tri;

  vtkLiveSourceDummy(const vtkLiveSourceDummy&) = delete;
  void operator=(const vtkLiveSourceDummy&) = delete;
};
