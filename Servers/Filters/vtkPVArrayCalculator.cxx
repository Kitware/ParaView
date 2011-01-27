/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVArrayCalculator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArrayCalculator.h"

#include "vtkGraph.h"
#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"

#include <vtksys/ios/sstream>
#include <assert.h>

vtkStandardNewMacro( vtkPVArrayCalculator );
// ----------------------------------------------------------------------------
vtkPVArrayCalculator::vtkPVArrayCalculator()
{
}

// ----------------------------------------------------------------------------
vtkPVArrayCalculator::~vtkPVArrayCalculator()
{
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::UpdateArrayAndVariableNames
   ( vtkDataObject * vtkNotUsed(theInputObj), vtkDataSetAttributes * inDataAttrs )
{ 
  static  char   stringSufix[3][3] = { "_X", "_Y", "_Z" };
  unsigned long mtime = this->GetMTime();

  // Look at the data-arrays available in the input and register them as
  // variables with the superclass.
  // It's safe to call these methods in RequestData() since they don't call
  // this->Modified().
  this->RemoveAllVariables();
  
  // Add coordinate scalar and vector variables
  this->AddCoordinateScalarVariable( "coordsX", 0 );
  this->AddCoordinateScalarVariable( "coordsY", 1 );
  this->AddCoordinateScalarVariable( "coordsZ", 2 );
  this->AddCoordinateVectorVariable( "coords",  0, 1, 2 );
  
  // add non-coordinate scalar and vector variables
  int numberArays = inDataAttrs->GetNumberOfArrays(); // the input
  for (int j = 0; j < numberArays; j ++ )
    {
    vtkAbstractArray* array = inDataAttrs->GetAbstractArray(j);
    const char* array_name = array->GetName();
    int numberComps = array->GetNumberOfComponents();
   
    if ( numberComps == 1 )
      {
      this->AddScalarVariable( array_name, array_name, 0 );
      }
    else
      {
      for (int i = 0; i < numberComps; i ++ )
        {
        if (i < 3)
          {
          vtksys_ios::ostringstream var_name;
          var_name << array_name << stringSufix[i];
          this->AddScalarVariable(var_name.str().c_str(), array_name, i );
          }
        vtksys_ios::ostringstream var_name2;
        var_name2 << array_name << "_";
        if (array->GetComponentName(i))
          {
          var_name2 << array->GetComponentName(i);
          }
        else
          {
          var_name2 << i;
          }
        this->AddScalarVariable(var_name2.str().c_str(), array_name, i );
        }
      
      if ( numberComps == 3 )
        {
        this->AddVectorArrayName(array_name, 0, 1, 2 );
        }
      }
    }

  assert(this->GetMTime() == mtime &&
    "post: mtime cannot be changed in RequestData()");
  static_cast<void>(mtime); // added so compiler won't complain im release mode.
}

// ----------------------------------------------------------------------------
int vtkPVArrayCalculator::RequestData
  ( vtkInformation * request, vtkInformationVector ** inputVector,
                              vtkInformationVector  * outputVector )
{
  vtkDataObject  * input  = inputVector[0]->GetInformationObject( 0 )
                            ->Get( vtkDataObject::DATA_OBJECT() );
  
  vtkIdType    numTuples  = 0;
  vtkGraph   * graphInput = vtkGraph::SafeDownCast( input );
  vtkDataSet * dsInput    = vtkDataSet::SafeDownCast( input );
  vtkDataSetAttributes *  dataAttrs = NULL;
 
  if ( dsInput )
    {
    if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
         this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_POINT_DATA )
      {
      dataAttrs = dsInput->GetPointData();
      numTuples = dsInput->GetNumberOfPoints();
      }
    else
      {
      dataAttrs = dsInput->GetCellData();
      numTuples = dsInput->GetNumberOfCells();
      }
    }
  else 
  if ( graphInput )
    {
    if ( this->AttributeMode == VTK_ATTRIBUTE_MODE_DEFAULT ||
         this->AttributeMode == VTK_ATTRIBUTE_MODE_USE_VERTEX_DATA )
      {
      dataAttrs = graphInput->GetVertexData();
      numTuples = graphInput->GetNumberOfVertices();
      }
    else
      {
      dataAttrs = graphInput->GetEdgeData();
      numTuples = graphInput->GetNumberOfEdges();
      }
    }
  
  if ( numTuples > 0 )
    {
    // Let's update the (scalar and vector arrays / variables) names  to make
    // them consistent with those of the upstream calculator(s). This addresses
    // the scenarios where the user modifies the name of a calculator whose out-
    // put is the input of a (some) subsequent calculator(s) or the user changes
    // the input of a downstream calculator.
    this->UpdateArrayAndVariableNames( input, dataAttrs );
    }
  
  input      = NULL;
  dsInput    = NULL;
  dataAttrs  = NULL;
  graphInput = NULL;
  
  return this->Superclass::RequestData( request, inputVector, outputVector );
}

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
