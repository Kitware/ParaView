/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdditionalFieldReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAdditionalFieldReader.h"

#include "vtkObjectFactory.h"

#include "vtkAbstractArray.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataObjectReader.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkSmartPointer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAdditionalFieldReader);

//----------------------------------------------------------------------------
vtkAdditionalFieldReader::vtkAdditionalFieldReader()
{
  this->FileName = NULL;
}

//----------------------------------------------------------------------------
vtkAdditionalFieldReader::~vtkAdditionalFieldReader()
{
  this->SetFileName(NULL);
}

//----------------------------------------------------------------------------
void vtkAdditionalFieldReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NULL") << endl;
}

//----------------------------------------------------------------------------
int vtkAdditionalFieldReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* din = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* dout = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkCompositeDataSet* cdout = vtkCompositeDataSet::SafeDownCast(dout);
  vtkCompositeDataIterator* cditer = NULL;
  dout->ShallowCopy(din);
  if (cdout)
  {
    cditer = cdout->NewIterator();
  }

  if (this->FileName && strlen(this->FileName))
  {
    vtkSmartPointer<vtkDataObjectReader> dor = vtkSmartPointer<vtkDataObjectReader>::New();
    dor->SetFileName(this->FileName);
    dor->Update();
    vtkDataObject* d = dor->GetOutput();
    if (!d)
    {
      return VTK_ERROR;
    }
    vtkFieldData* arrs = d->GetFieldData();
    vtkFieldData* ofd = NULL;
    for (int i = 0; i < arrs->GetNumberOfArrays(); i++)
    {
      if (cditer)
      {
        cditer->InitTraversal();
        while (!cditer->IsDoneWithTraversal())
        {
          vtkDataObject* dsout = cditer->GetCurrentDataObject();
          ofd = dsout->GetFieldData();
          ofd->AddArray(arrs->GetAbstractArray(i));
          cditer->GoToNextItem();
        }
      }
      else
      {
        ofd = dout->GetFieldData();
        ofd->AddArray(arrs->GetAbstractArray(i));
      }
    }
  }
  if (cditer)
  {
    cditer->Delete();
  }

  return VTK_OK;
}
