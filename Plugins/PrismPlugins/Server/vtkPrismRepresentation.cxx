/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkInformation.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVRenderView.h"
#include "vtkQuadricClustering.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredDataDeliveryFilter.h"

vtkStandardNewMacro(vtkPrismRepresentation);
namespace
{
  class vtkPrismTransform : public vtkTransformFilter
  {
  public:
    vtkTypeRevisionMacro(vtkPrismTransform,vtkTransformFilter);
    void PrintSelf(ostream& os, vtkIndent indent);

    static vtkPrismTransform *New();


    vtkGetVectorMacro(PrismRange,double,3);

  protected:

    vtkPrismTransform()
    {
      PrismRange[0]=-1;
      PrismRange[1]=-1;
      PrismRange[2]=-1;


    }
    ~vtkPrismTransform()
    {

    }

    virtual int RequestData(vtkInformation *request, vtkInformationVector **inputVector, vtkInformationVector *outputVector)
    {
      vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

      vtkPointSet *input = vtkPointSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));

      if ( !input )
      {
        vtkDebugMacro( << "No input found." );
        return 0;
      }
      if(input->GetNumberOfPoints())
      {
        double scaleBounds[6];
        input->GetBounds(scaleBounds);

        this->PrismRange[0]=scaleBounds[1] - scaleBounds[0];
        this->PrismRange[1]=scaleBounds[3] - scaleBounds[2];
        this->PrismRange[2]=scaleBounds[5] - scaleBounds[4];
      }
      return this->Superclass::RequestData(request, inputVector, outputVector);

    }

  private:
    double PrismRange[3];

    vtkPrismTransform(const vtkPrismTransform&);  // Not implemented.
    void operator=(const vtkPrismTransform&);  // Not implemented.
  };
}
vtkCxxRevisionMacro(vtkPrismTransform , "1.11");
vtkStandardNewMacro(vtkPrismTransform );
//----------------------------------------------------------------------------
void vtkPrismTransform::PrintSelf(ostream& os, vtkIndent indent)
    {
    this->Superclass::PrintSelf(os,indent);

    os << indent << "Not Implemented: " << "\n";

    }
class vtkPrismRepresentation::MyInternal
    {
    public:
        vtkSmartPointer<vtkPrismTransform> ScaleTransform;
        vtkSmartPointer<vtkTransform> Transform;



        MyInternal()
            {

              this->ScaleTransform= vtkSmartPointer<vtkPrismTransform>::New();
              this->Transform= vtkSmartPointer<vtkTransform>::New();
              this->Transform->Scale(1,1,1);
              this->ScaleTransform->SetTransform(this->Transform);

            }
        ~MyInternal()
            {
            }
    };

//----------------------------------------------------------------------------
vtkPrismRepresentation::vtkPrismRepresentation()
{
    this->Internal = new MyInternal();
}

//----------------------------------------------------------------------------
vtkPrismRepresentation::~vtkPrismRepresentation()
{
  delete this->Internal;
}

 void vtkPrismRepresentation::GetPrismRange(double* range)
{
  this->Internal->ScaleTransform->Update();
  this->Internal->ScaleTransform->GetPrismRange(range);

}
  void vtkPrismRepresentation::SetScaleFactor(double* scale)
{
  this->Internal->Transform->Scale(scale);

  this->MarkModified();
}


int vtkPrismRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // cout << this << ":" << this->DebugString << ":RequestData" << endl;

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    this->Internal->ScaleTransform->SetInputConnection(
      this->GetInternalOutputPort());
    this->GeometryFilter->SetInputConnection(
      this->Internal->ScaleTransform->GetOutputPort());
  //  this->GeometryFilter->SetInputConnection(
  //    GetInternalOutputPort());
    this->CacheKeeper->Update();
    this->DeliveryFilter->SetInputConnection(
      this->CacheKeeper->GetOutputPort());
    this->LODDeliveryFilter->SetInputConnection(
      this->Decimator->GetOutputPort());
    }
  else
    {
    this->DeliveryFilter->RemoveAllInputs();
    this->LODDeliveryFilter->RemoveAllInputs();
    }

  return this->vtkPVDataRepresentation::RequestData(request, inputVector, outputVector);
}



//----------------------------------------------------------------------------
void vtkPrismRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
