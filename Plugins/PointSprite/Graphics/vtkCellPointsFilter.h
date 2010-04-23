/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellPointsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCellPointsFilter - Generate a Polydata Pointset from any Dataset.
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description
// vtkCellPointsFilter takes any dataset as input, it extracts the point
// locations of all cells in the input and creates a polydata output with these
// points set. It can optionally generate vertex cells for each point.
// This filter is generally used to create a test point array from some arbitrary
// data.
//
// .SECTION See Also
//

#ifndef _vtkCellPointsFilter_h
#define _vtkCellPointsFilter_h

#include "vtkPolyDataAlgorithm.h"


class vtkAppendPolyData;
class vtkCompositeDataSet;

class VTK_EXPORT vtkCellPointsFilter : public vtkPolyDataAlgorithm
{
  public:
    // Description:
    // Standard Type-Macro
    vtkTypeMacro(vtkCellPointsFilter,vtkPolyDataAlgorithm);

    // Description:
    // Create an instance of vtkCellPointsFilter
    static vtkCellPointsFilter *New();

    // Description:
    // if VertexCells is true (default) then each point is represented by a
    // vertex cell in the output dataset. If false then the points are generated,
    // but no cells are generated.
    vtkGetMacro(VertexCells,int);
    vtkSetMacro(VertexCells,int);
    vtkBooleanMacro(VertexCells,int);
    //
  protected:
     vtkCellPointsFilter();
    ~vtkCellPointsFilter();
    //
    virtual int FillInputPortInformation(int port, vtkInformation* info);
    virtual int RequestInformation(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector);
    virtual int RequestCompositeData(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector);
    virtual int RequestData(vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector);

    // Create a default executive.
    virtual vtkExecutive* CreateDefaultExecutive();
    int CheckAttributes(vtkDataObject* input);

    void ExecuteSimple(vtkDataSet *input, vtkPolyData *output);
    int  ExecuteCompositeDataSet(vtkCompositeDataSet* input,
          vtkAppendPolyData* append);

    //
    int VertexCells;
    int GenerateGroupScalars;
    int CurrentGroup;
};

#endif
