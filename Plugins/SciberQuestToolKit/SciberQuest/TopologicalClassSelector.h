/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#ifndef __TopologicalClassSelector_h
#define __TopologicalClassSelector_h

class vtkDataSet;
class vtkThreshold;
class vtkAppendFilter;
class vtkUnstructuredGrid;

class TopologicalClassSelector
{
public:
  TopologicalClassSelector();
  ~TopologicalClassSelector();

  void Initialize();
  void Clear();

  void SetInput(vtkDataSet *input);
  vtkUnstructuredGrid *GetOutput();

  void AppendRange(double v0, double v1);

private:
  vtkDataSet *Input;
  vtkThreshold *Threshold;
  vtkAppendFilter *Append;
};

#endif

// VTK-HeaderTest-Exclude: TopologicalClassSelector.h
