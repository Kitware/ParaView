/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTempTessellatorFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2003 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
#ifndef __vtkTempTessellatorFilter_h
#define __vtkTempTessellatorFilter_h

// .NAME vtkTempTessellatorFilter - approximate nonlinear FEM elements with simplices
// .SECTION Description
// This class approximates nonlinear FEM elements with linear simplices.
//
// <b>Warning</b>: This class is temporary and will go away at some point
// after ParaView 1.4.0.
//
// This filter rifles through all the cells in an
// input vtkDataSet. It tesselates each cell and uses the vtkStreamingTessellator
// and vtkDataSetSubdivisionAlgorithm classes to generate simplices that approximate the
// nonlinear mesh using some approximation metric (encoded in the particular
// vtkDataSetSubdivisionAlgorithm::EvaluateEdge implementation). The simplices are placed into
// the filter's output vtkDataSet object by the callback routines
// AddATetrahedron, AddATriangle, and AddALine, which are registered with the triangulator.
//
// The output mesh will have geometry and any fields specified as attributes
// in the input mesh's point data.
// The attribute's copy flags are honored, except for normals.
//
// .SECTION Internals
//
// The filter's main member function is Execute(). This function first calls
// SetupOutput() which allocates arrays and some temporary variables for the
// primitive callbacks (OutputTriangle and OutputLine which are called by
// AddATriangle and AddALine, respectively).
// Each cell is given an initial tesselation, which
// results in one or more calls to OutputTetrahedron, OutputTriangle or OutputLine to add
// elements to the OutputMesh. Finally, Teardown() is called to free the
// filter's working space.
//
// .SECTION See Also
// vtkDataSetToUnstructuredGridFilter vtkDataSet vtkStreamingTessellator vtkDataSetSubdivisionAlgorithm

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkDataArray;
class vtkDataSet;
class vtkDataSetSubdivisionAlgorithm;
class vtkPoints;
class vtkStreamingTessellator;
class vtkSubdivisionAlgorithm;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkTempTessellatorFilter : public vtkDataSetToUnstructuredGridFilter
{
  public:
    vtkTypeRevisionMacro(vtkTempTessellatorFilter,vtkDataSetToUnstructuredGridFilter);
    void PrintSelf( ostream& os, vtkIndent indent );

    static vtkTempTessellatorFilter* New();

    virtual void SetTessellator( vtkStreamingTessellator* );
    vtkGetObjectMacro(Tessellator, vtkStreamingTessellator);

    virtual void SetSubdivider( vtkDataSetSubdivisionAlgorithm* );
    vtkGetObjectMacro(Subdivider, vtkDataSetSubdivisionAlgorithm);

    virtual unsigned long GetMTime();

    vtkSetClampMacro(OutputDimension,int,1,3);
    vtkGetMacro(OutputDimension,int);
    int GetOutputDimension() const;

    // Description:
    // These are convenience routines for setting properties maintained by the
    // tessellator and subdivider. They are implemented here for ParaView's
    // sake.
    virtual void SetMaximumNumberOfSubdivisions( int num_subdiv_in );
    int GetMaximumNumberOfSubdivisions();
    virtual void SetChordError( double ce );
    double GetChordError();

    // Description:
    // These methods are for the ParaView client.
    virtual void ResetFieldCriteria();
    virtual void SetFieldCriterion( int field, double chord );

    // Description:
    // The adaptive tessellation will output vertices that are not shared
    // among cells, even where they should be. This can be corrected to
    // some extents with a vtkMergeFilter.
    // By default, the filter is off and vertices will not be shared.
    virtual void SetMergePoints( int OnOrOff );
    vtkBooleanMacro(MergePoints,int);
    vtkGetMacro(MergePoints,int);
    int GetMergePoints() const { return this->MergePoints; }

  protected:
    vtkTempTessellatorFilter();
    ~vtkTempTessellatorFilter();

    // Description:
    // Called by Execute to set up a multitude of member variables used by the per-primitive
    // output functions (OutputLine, OutputTriangle, and maybe one day... OutputTetrahedron).
    void SetupOutput();

    // Description:
    // Run the filter; produce a polygonal approximation to the grid.
    virtual void Execute();

    // Description:
    // Reset the temporary variables used during the filter's Execute() method.
    void Teardown();

    //BTX
    vtkStreamingTessellator* Tessellator;
    vtkDataSetSubdivisionAlgorithm* Subdivider;
    int OutputDimension;
    int MergePoints;

    // Description:
    // These member variables are set by SetupOutput for use inside the
    // callback members OutputLine and OutputTriangle.
    vtkUnstructuredGrid* OutputMesh;
    vtkPoints* OutputPoints;
    vtkDataArray** OutputAttributes;
    int* OutputAttributeIndices;

    static void AddALine( const double*, const double*, vtkSubdivisionAlgorithm*, void*, const void* );
    static void AddATriangle( const double*, const double*, const double*, vtkSubdivisionAlgorithm*, void*, const void* );
    static void AddATetrahedron( const double*, const double*, const double*, const double*, vtkSubdivisionAlgorithm*, void*, const void* );
    void OutputLine( const double*, const double* );
    void OutputTriangle( const double*, const double*, const double* );
    void OutputTetrahedron( const double*, const double*, const double*, const double* );
    //ETX

  private:
    vtkTempTessellatorFilter( const vtkTempTessellatorFilter& ); // Not implemented.
    void operator = ( const vtkTempTessellatorFilter& ); // Not implemented.
};

//BTX
inline int vtkTempTessellatorFilter::GetOutputDimension() const { return this->OutputDimension; }
//ETX

#endif // __vtkTempTessellatorFilter_h
