/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleFieldDataToAttributeDataFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkSimpleFieldDataToAttributeDataFilter.h"
#include "vtkObjectFactory.h"

//--------------------------------------------------------------------------
vtkSimpleFieldDataToAttributeDataFilter* vtkSimpleFieldDataToAttributeDataFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSimpleFieldDataToAttributeDataFilter");
  if(ret)
    {
    return (vtkSimpleFieldDataToAttributeDataFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSimpleFieldDataToAttributeDataFilter;
}

// Instantiate object with no input and no defined output.
vtkSimpleFieldDataToAttributeDataFilter::vtkSimpleFieldDataToAttributeDataFilter()
{
  //this->AttributeType = 0;
  this->Attribute = 0;
  this->FieldName = NULL;
}

vtkSimpleFieldDataToAttributeDataFilter::~vtkSimpleFieldDataToAttributeDataFilter()
{
  this->SetFieldName(NULL);
}

void vtkSimpleFieldDataToAttributeDataFilter::Execute()
{
  int num;
  vtkFieldData *field;
  vtkDataArray *array = NULL;
  vtkDataSet *input;
  vtkDataSet *output;

  input = this->GetInput();
  output = this->GetOutput();

  output->ShallowCopy(input);

  field = this->GetInput()->GetPointData()->GetFieldData();
  if (field)
    {
    array = field->GetArray(this->FieldName);
    }

  if (array == NULL)
    {
    vtkErrorMacro("Could not find field array with name: " << this->FieldName);
    return;
    }

  num = array->GetNumberOfComponents();
  if (this->Attribute == 0)
    { // scalars
    // Create a scalars object around the array. 
    vtkScalars *scalars;
    scalars = vtkScalars::New();
    scalars->SetData(array);
    output->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    scalars = NULL;
    }
  else
    { // vectors
    // Create a vector object around the array. 
    vtkVectors *vects;
    vects = vtkVectors::New();
    vects->SetData(array);
    output->GetPointData()->SetVectors(vects);
    vects->Delete();
    vects = NULL;

    }
}


void vtkSimpleFieldDataToAttributeDataFilter::PrintSelf(ostream& os, 
                                                  vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Field Name: " << this->FieldName << endl;
  //os << indent << "AttributeType: " << this->AttributeType << endl;
  os << indent << "Attribute: " << this->Attribute << endl;


}
