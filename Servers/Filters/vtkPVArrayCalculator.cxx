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

vtkCxxRevisionMacro( vtkPVArrayCalculator, "1.2" );
vtkStandardNewMacro( vtkPVArrayCalculator );

// ----------------------------------------------------------------------------
void vtkPVArrayCalculator::UpdateArrayAndVariableNames
   ( vtkDataObject * theInputObj, vtkDataSetAttributes * inDataAttrs )
{ 
  if ( !inDataAttrs )
    {
    return;
    }
    
  int     i, j;
  int     funcInvalid = 0; // an invalid function?
  int     varNotFound = 1; // a function variable not found from input?
  int     numberComps = 1;
  int     aray1Length = 0; // length of a scalar variable name
  int     numberArays = inDataAttrs->GetNumberOfArrays(); // the input
  char    tempVarName[100];
  char  * theAryName0 = NULL; // array name of the input
  char  * theAryName1 = NULL; // array name of the output
  static  char   stringSufix[3][3] = { "_X", "_Y", "_Z" };
  
  for ( j = 0; j < this->NumberOfVectorArrays && funcInvalid == 0; j ++ )
    {
    theAryName1 = this->VectorArrayNames[j];
    
    if (  strstr( this->Function, theAryName1 )  )
      {
      // the function references this vector, either whole or component(s)
      
      varNotFound = 1;
      
      // check if this vector can be found from the input
      for ( i = 0; i < numberArays && varNotFound == 1; i ++ )
        {
        theAryName0 = inDataAttrs->GetArray( i )->GetName();
        numberComps = inDataAttrs->GetArray( i )->GetNumberOfComponents();
        varNotFound = (  numberComps > 1 && 
                         strcmp( theAryName1, theAryName0 ) == 0  )
                      ? 0 : varNotFound;
        }
        
      funcInvalid = varNotFound;
      }
    
    theAryName0 = NULL;
    theAryName1 = NULL;
    }
  this->VectorsOutdated = funcInvalid; // to avoid a crash for whole vector
  this->VectorsOutdated = ( this->FunctionType == 1 && 
                            this->InputData &&
                            this->InputData != theInputObj
                          ) ? 1 : this->VectorsOutdated;
  this->InputData = theInputObj;
    
    
  for ( j = 0; j < this->NumberOfScalarArrays && funcInvalid == 0; j ++ )
    {
    theAryName1 = this->ScalarArrayNames[j];
    
    // Let's extract the main part of the scalar variable name, without suffix.
    // It is safe to assume that the components of any vector end with _X/Y/Z.
    aray1Length = static_cast< int > (  strlen( theAryName1 )  );
    if (   aray1Length > 2 &&
           (  strcmp( theAryName1 + aray1Length - 2, "_X" ) == 0 ||
              strcmp( theAryName1 + aray1Length - 2, "_Y" ) == 0 ||
              strcmp( theAryName1 + aray1Length - 2, "_Z" ) == 0
           )
       )
      {
      strcpy( tempVarName, theAryName1 );
      tempVarName[ aray1Length - 2 ] = '\0';
      theAryName1 = tempVarName;
      }
    
    if (  strstr( this->Function, theAryName1 )  )
      {
      // the function references this scalar variable, which might be a
      // component of a vector
      
      varNotFound = 1;
      
      // check if this scalable variable can be found from the input
      for ( i = 0; i < numberArays && varNotFound == 1; i ++ )
        {
        theAryName0 = inDataAttrs->GetArray( i )->GetName();
        varNotFound = (  strcmp( theAryName1, theAryName0 ) == 0  ) 
                      ? 0 : varNotFound;
        }
        
      funcInvalid = varNotFound;
      }
    
    theAryName0 = NULL;
    theAryName1 = NULL;
    }
  
  
  // As the function has been checked, the arrays and the associated variables
  // need to be updated to ensure consistency between the input and the output
  // regardless of any change made to either the name of an upstream calculator
  // or the input of an downstream calculator. Note that the following segment
  // simply employs AddScalarVariable() and AddVectorArrayName(), which may not
  // be an optimized solution, though the small number of arrays and variables
  // does not justify that effort.
  this->RemoveAllVariables();
  
  // Add coordinate scalar and vector variables
  this->AddCoordinateScalarVariable( "coordsX", 0 );
  this->AddCoordinateScalarVariable( "coordsY", 1 );
  this->AddCoordinateScalarVariable( "coordsZ", 2 );
  this->AddCoordinateVectorVariable( "coords",  0, 1, 2 );
  
  // add non-coordinate scalar and vector variables
  for ( j = 0; j < numberArays; j ++ )
    {
    theAryName0 = inDataAttrs->GetArray( j )->GetName();
    numberComps = inDataAttrs->GetArray( j )->GetNumberOfComponents();
   
    if ( numberComps == 1 )
      {
      this->AddScalarVariable( theAryName0, theAryName0, 0 );
      }
    else
      {
      for ( i = 0; i < numberComps; i ++ )
        {
        sprintf( tempVarName, "%s%s", theAryName0, stringSufix[i] );
        this->AddScalarVariable( tempVarName, theAryName0, i );
        }
      
      if ( numberComps == 3 )
        {
        this->AddVectorArrayName( theAryName0, 0, 1, 2 );
        }
      }
    
    theAryName0 = NULL;
    }
  
  // release a warning to ask the user to update the function, if necessary
  if ( funcInvalid || this->VectorsOutdated )
    {
    vtkGenericWarningMacro( << "Array " << this->ResultArrayName << " may be "
                            << "either missing or assigned with incorrect "
                            << "values since the function contains outdated "
                            << "scalar and / or vector variables. Please " 
                            << "update it with valid variables." << endl );
    }
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
  
  if ( numTuples < 1 )
    {
    vtkDebugMacro( "Empty data." );
    return 1;
    }
   
  // Let's update the (scalar and vector arrays / variables) names  to make
  // them consistent with those of the upstream calculator(s). This addresses
  // the scenarios where the user modifies the name of a calculator whose out-
  // put is the input of a (some) subsequent calculator(s) or the user changes
  // the input of a downstream calculator.
  this->UpdateArrayAndVariableNames( input, dataAttrs );
  
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
