// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef TestFunctions_h
#define TestFunctions_h

#include "vtkType.h"

class vtkUnstructuredGrid;
class vtkStructuredGrid;
class vtkPolyData;
class vtkMultiBlockDataSet;
class vtkImageData;
class vtkRectilinearGrid;

void Create(vtkUnstructuredGrid*, vtkIdType);
void Create(vtkStructuredGrid*, vtkIdType, vtkIdType, vtkIdType);
void Create(vtkPolyData*);
void CreatePolyhedral(vtkUnstructuredGrid*);
void Create(vtkImageData*, vtkIdType, vtkIdType, vtkIdType);
void Create(vtkRectilinearGrid*, vtkIdType, vtkIdType, vtkIdType);

int PolydataTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, const char*);
int UnstructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int, const char*);
int StructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int, int, int);
int MappedUnstructuredGridTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int);
int PolyhedralTest(vtkMultiBlockDataSet*, unsigned int, unsigned int);
int CellAndPointDataTest(vtkMultiBlockDataSet*, unsigned int, unsigned int, int);

#endif
