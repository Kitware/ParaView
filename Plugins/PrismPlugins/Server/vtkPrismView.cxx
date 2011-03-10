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
#include "vtkPrismView.h"
#include "vtkObjectFactory.h"
#include "vtkPrismRepresentation.h"
#include "vtkPVCompositeRepresentation.h"
vtkStandardNewMacro(vtkPrismView);
//----------------------------------------------------------------------------
vtkPrismView::vtkPrismView()
{
      //this->Transform= vtkSmartPointer<vtkTransform>::New();
      //this->Transform->Scale(1,1,1);
}

//----------------------------------------------------------------------------
vtkPrismView::~vtkPrismView()
{
}

 /*  void vtkPrismView::AddRepresentationInternal(vtkDataRepresentation* rep)
{
    vtkCompositeRepresentation *compositeRep = vtkCompositeRepresentation::SafeDownCast(rep);
    if(compositeRep)
    {
      vtkPVDataRepresentation*pvDataRep= compositeRep->GetActiveRepresentation();
      if(pvDataRep)
      {
        vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(pvDataRep);

        if(prismRep)
        {
          prismRep->SetTransform(this->Transform);
        }
      }
    }

}
   void vtkPrismView::RemoveRepresentationInternal(vtkDataRepresentation* rep)
{
    vtkCompositeRepresentation *compositeRep = vtkCompositeRepresentation::SafeDownCast(rep);
    if(compositeRep)
    {
      vtkPVDataRepresentation*pvDataRep= compositeRep->GetActiveRepresentation();
      if(pvDataRep)
      {
        vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(pvDataRep);

        if(prismRep)
        {
          prismRep->SetTransform(NULL);
        }
      }
    }

}*/

void vtkPrismView::Update()
{
 this->Superclass::Update();
  double ranges[3];
  ranges[0]=0;
  ranges[1]=0;
  ranges[2]=0;

  double scale[3];
  scale[0]=1.0;
  scale[1]=1.0;
  scale[2]=1.0;

  int numberRanges=0;

  int numReps=this->GetNumberOfRepresentations();
  for(int i=0;i<numReps;i++)
  {
    vtkDataRepresentation* rep=this->GetRepresentation(i);
    vtkPVCompositeRepresentation *compositeRep = vtkPVCompositeRepresentation::SafeDownCast(rep);
    if(compositeRep && compositeRep->GetVisibility())
    {
      vtkPVDataRepresentation*pvDataRep= compositeRep->GetActiveRepresentation();
      if(pvDataRep)
      {
        vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(pvDataRep);

        if(prismRep && prismRep->GetVisibility())
        {
          //what to do here.
          //We are calculating the scale values shared by all of prism representations in this view.
          //I guess we sill do an average.
          double range[3];
          prismRep->GetPrismRange(range);

          if(range[0]>0)
          {
            //If the range is negative then the range isn't valid.
            ranges[0]+=range[0];
            ranges[1]+=range[1];
            ranges[2]+=range[2];
            numberRanges++;
          }
        }
      }
    }
  }

  if(numberRanges)
  {
      ranges[0]/=numberRanges;
      ranges[1]/=numberRanges;
      ranges[2]/=numberRanges;

      if(ranges[0]<=1e-6)
      {
        ranges[0]=100;
      }
      if(ranges[1]<=1e-6)
      {
        ranges[1]=100;
      }
      if(ranges[2]<=1e-6)
      {
        ranges[2]=100;
      }

      scale[0]=100/ranges[0];
      scale[1]=100/ranges[1];
      scale[2]=100/ranges[2];

      int numReps=this->GetNumberOfRepresentations();
      for(int i=0;i<numReps;i++)
      {
        vtkDataRepresentation* rep=this->GetRepresentation(i);
        vtkPVCompositeRepresentation *compositeRep = vtkPVCompositeRepresentation::SafeDownCast(rep);
        if(compositeRep && compositeRep->GetVisibility())
        {
          vtkPVDataRepresentation*pvDataRep= compositeRep->GetActiveRepresentation();
          if(pvDataRep)
          {
            vtkPrismRepresentation *prismRep = vtkPrismRepresentation::SafeDownCast(pvDataRep);

            if(prismRep && prismRep->GetVisibility())
            {
              double range[3];
              prismRep->GetPrismRange(range);

              if(range[0]>0)
              {
                prismRep->SetScaleFactor(scale);
              }

            }
          }
        }
      }



  }

      this->Superclass::Update();

}


//----------------------------------------------------------------------------
void vtkPrismView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
