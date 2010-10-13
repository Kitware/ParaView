/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismCubeAxesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPrismCubeAxesRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"
#include "pqSMAdaptor.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMPrismCubeAxesRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::vtkSMPrismCubeAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPrismCubeAxesRepresentationProxy::~vtkSMPrismCubeAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
// Don't think this is the best place to do this. Ideally
// vtkPrismCubeAxesRepresentation should handle it on its own. But I don't
// know the code well to make that call. So simply keeping the old
// implementation.
void vtkSMPrismCubeAxesRepresentationProxy::RepresentationUpdated()
{
  this->Superclass::RepresentationUpdated();
    if (vtkSMPropertyHelper(this, "Visibility").GetAsInt() != 0)
    {
        // Get bounds and set on the actor.
        //  vtkSMSourceProxy* output = this->Strategy->GetOutput();
        // this->Strategy->UpdateVTKObjects();
        vtkSMSourceProxy* output =
          vtkSMSourceProxy::SafeDownCast(vtkSMPropertyHelper(this,
              "Input").GetAsProxy(0));

        vtkPVDataInformation* info = output->GetDataInformation(0);
        if (info)
        {
            vtkPVDataSetAttributesInformation* fieldInfo=info->GetFieldDataInformation();
            if(fieldInfo)
            {
                double labelRanges[6];
                vtkPVArrayInformation* xRangeArrayInfo=fieldInfo->GetArrayInformation("XRange");
                if(xRangeArrayInfo)
                {
                    double* range=xRangeArrayInfo->GetComponentRange(0);
                    labelRanges[0]=range[0];
                    labelRanges[1]=range[1];
                }
                vtkPVArrayInformation* yRangeArrayInfo=fieldInfo->GetArrayInformation("YRange");
                if(yRangeArrayInfo)
                {
                    double* range=yRangeArrayInfo->GetComponentRange(0);
                    labelRanges[2]=range[0];
                    labelRanges[3]=range[1];
                }
                vtkPVArrayInformation* zRangeArrayInfo=fieldInfo->GetArrayInformation("ZRange");
                if(zRangeArrayInfo)
                {
                    double* range=zRangeArrayInfo->GetComponentRange(0);
                    labelRanges[4]=range[0];
                    labelRanges[5]=range[1];
                }


                vtkstd::string name=output->GetXMLName();
                if(name=="PrismSurfaceReader")
                {
                    vtkSMProperty* xVariableProperty = output->GetProperty("XAxisVariableName");
                    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("XTitle"),
                        str);

                    vtkSMProperty* yVariableProperty = output->GetProperty("YAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("YTitle"),
                        str);

                    vtkSMProperty* zVariableProperty = output->GetProperty("ZAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("ZTitle"),
                        str);
                }
                else if(name=="PrismFilter")
                {
                    vtkSMProperty* xVariableProperty = output->GetProperty("SESAMEXAxisVariableName");
                    QVariant str = pqSMAdaptor::getEnumerationProperty(xVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("XTitle"),
                        str);

                    vtkSMProperty* yVariableProperty = output->GetProperty("SESAMEYAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(yVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("YTitle"),
                        str);

                    vtkSMProperty* zVariableProperty = output->GetProperty("SESAMEZAxisVariableName");
                    str = pqSMAdaptor::getEnumerationProperty(zVariableProperty);

                    pqSMAdaptor::setElementProperty(
                        this->GetProperty("ZTitle"),
                        str);
                }


                vtkSMDoubleVectorProperty* rvp = vtkSMDoubleVectorProperty::SafeDownCast(
                    this->GetProperty("LabelRanges"));
                rvp->SetElements(labelRanges);
            }
            this->UpdateVTKObjects();
        }
    }
}

//----------------------------------------------------------------------------
void vtkSMPrismCubeAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


