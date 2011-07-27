/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __CellCopier_h
#define __CellCopier_h

#include "IdBlock.h"

#include "vtkType.h"

#include <map>
using std::map;
using std::pair;

#include <vector>
using std::vector;

class DataArrayCopier;
class vtkDataSet;

/// Abstract interface for copying geometry and data between datasets.
/**
Copies specific cells and their associated data from one dataset
to another.
*/
class CellCopier
{
public:
  virtual ~CellCopier();

  /**
  Initialize the object with an input and an output. This
  will set up array copiers. Derived classes must make sure
  this method gets called if they override it.
  */
  virtual void Initialize(vtkDataSet *in, vtkDataSet *out);

  /**
  Copy a single cell and its associated point and cell data
  from iniput to output.
  */
  virtual int Copy(vtkIdType cellId)
    {
    IdBlock id(cellId);
    return this->Copy(id);
    }

  /**
  Copy a contiguous block of cells and their associated point
  and cell data from iniput to output. Derived classes must make
  sure this method gets called if they override it.
  */
  virtual int Copy(IdBlock &block)=0;

  /**
  Free resources used by the object. Dervied classes must makes
  sure this method gets called if they override it.
  */
  virtual void Clear()
    {
    this->ClearDataCopier();
    this->ClearPointIdMap();
    }

  /**
  Insure that point to insert is unique based on its inputId.
  The outputId must contain the id that will be used if the
  id is unique. If the id is unque this value is cached. If
  the id is not unique then outputId is set to the cached
  value and false is returned.
  */
  bool GetUniquePointId(vtkIdType inputId, vtkIdType &outputId);

protected:
  int CopyPointData(IdBlock &block);
  int CopyPointData(vtkIdType id);
  int CopyCellData(IdBlock &block);
  int CopyCellData(vtkIdType id);
  void ClearDataCopier();
  void ClearPointIdMap(){ this->PointIdMap.clear(); }

protected:
  map<vtkIdType,vtkIdType> PointIdMap;

  vector<DataArrayCopier *> PointDataCopier;
  vector<DataArrayCopier *> CellDataCopier;
};

#endif
