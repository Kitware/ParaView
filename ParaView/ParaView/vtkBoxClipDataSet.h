/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoxClipDataSet.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//.NAME vtkBoxClipDataSet - clip an unstructured grid
//Clipping means that is actually 'cuts' through the cells of the dataset,
//returning tetrahedral cells inside of the box.
//The output of this filter is an unstructured grid.
//
//The vtkBoxClipDataSet will triangulate all types of 3D cells (i.e, create tetrahedra).
//This is necessary to preserve compatibility across face neighbors.
//
//To use this filter,you can decide if you will be clipping with a box or a hexahedral box.
//1) Set orientation 
//   if(SetOrientation(0)): box (parallel with coordinate axis)
//      SetBoxClip(xmin,xmax,ymin,ymax,zmin,zmax)  
//   if(SetOrientation(1)): hexahedral box (Default)
//      SetBoxClip(n[0],o[0],n[1],o[1],n[2],o[2],n[3],o[3],n[4],o[4],n[5],o[5])  
//      n[] normal of each plane
//      o[] point on the plane 
//2) Apply the GenerateClipScalarsOn() 
//3) Execute clipping  Update();

#ifndef __vtkBoxClipDataSet_h
#define __vtkBoxClipDataSet_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkGenericCell;            
class vtkCell3D;                     
class vtkDataArray;                    
class vtkCellArray;                   
class vtkPointData;                   
class vtkCellData;                    
class vtkPoints;                      
class vtkIdList;                    
class vtkPointLocator;

class VTK_EXPORT vtkBoxClipDataSet : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkBoxClipDataSet,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor of the clipping box.
  static vtkBoxClipDataSet *New();

  // Description
  // Specify the Box with which to perform the clipping. 
  // If the box is not parallel to axis, you need to especify  
  // normal vector of each plane and a point on the plane. 
  
  void SetBoxClip(float xmin,float xmax,float ymin,float ymax,float zmin,float zmax);
  void SetBoxClip(float *n0,float *o0,float *n1,float *o1,float *n2,float *o2,float *n3,float *o3,float *n4,float *o4,float *n5,float *o5);
  

  // Description:
  // If this flag is enabled, then the output scalar values will be 
  // interpolated, and not the input scalar data.
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);


  // Description:
  // Set the tolerance for merging clip intersection points that are near
  // the vertices of cells. This tolerance is used to prevent the generation
  // of degenerate primitives. Note that only 3D cells actually use this
  // instance variable.
  vtkSetClampMacro(MergeTolerance,float,0.0001,0.25);
  vtkGetMacro(MergeTolerance,float);
  
  // Description:
  // Return the Clipped output.
  vtkUnstructuredGrid *GetClippedOutput();
  virtual int GetNumberOfOutputs();

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Description:
  // Return the mtime also considering the locator.
  unsigned long GetMTime();

  // Description:
  // If you want to clip by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

  void SetOrientation(unsigned int i)
          {Orientation = i; };

  unsigned int GetOrientation()
          {return  Orientation;};
  void MinEdgeF(unsigned int *id_v, vtkIdType *cellIds,unsigned int *edgF );
  void PyramidToTetra(vtkIdType *pyramId, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void WedgeToTetra(vtkIdType *wedgeId, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void TetraGrid(vtkIdType typeobj, vtkIdType npts, vtkIdType *cellIds,vtkCellArray *newCellArray);
  void CreateTetra(vtkIdType npts,vtkIdType *cellIds,vtkCellArray *newCellArray);
  void ClipBox(vtkPoints *newPoints,vtkGenericCell *cell, 
               vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
               vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
               vtkCellData *outCD);
  void ClipHexahedron(vtkPoints *newPoints,vtkGenericCell *cell,
               vtkPointLocator *locator, vtkCellArray *tets,vtkPointData *inPD, 
               vtkPointData *outPD,vtkCellData *inCD,vtkIdType cellId,
               vtkCellData *outCD);

protected:
  vtkBoxClipDataSet();
  ~vtkBoxClipDataSet();

  void Execute();
  
  vtkPointLocator *Locator;
  int GenerateClipScalars;

  float MergeTolerance;

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  float    BoundBoxClip[3][2];
  unsigned int Orientation;
  float    n_pl[6][3];
  float    o_pl[6][3];

private:
  vtkBoxClipDataSet(const vtkBoxClipDataSet&);  // Not implemented.
  void operator=(const vtkBoxClipDataSet&);  // Not implemented.
};

#endif
