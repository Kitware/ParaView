/*=========================================================================

  Program:   ParaView
  Module:    ServersServerManagerPrintSelf.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVCaveRenderModule.h"
#include "vtkPVCompositePartDisplay.h"
#include "vtkPVCompositeRenderModule.h"
#include "vtkPVDisplay.h"
#include "vtkPVLODPartDisplay.h"
#include "vtkPVLODRenderModule.h"
#include "vtkPVMPIRenderModule.h"
#include "vtkPVMultiDisplayPartDisplay.h"
#include "vtkPVMultiDisplayRenderModule.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVPickDisplay.h"
#include "vtkPVPlotDisplay.h"
#include "vtkPVSimpleRenderModule.h"
#include "vtkRM3DWidget.h"
#include "vtkRMBoxWidget.h"
#include "vtkRMImplicitPlaneWidget.h"
#include "vtkRMLineWidget.h"
#include "vtkRMObject.h"
#include "vtkRMPointWidget.h"
#include "vtkRMScalarBarWidget.h"
#include "vtkRMSphereWidget.h"
#include "vtkSMApplication.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArrayRangeDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMArraySelectionInformationHelper.h"
#include "vtkSMBooleanDomain.h"
#include "vtkSMBoundsDomain.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMDisplayerProxy.h"
#include "vtkSMDisplayWindowProxy.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMExtentDomain.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMFixedTypeDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInformationHelper.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMNumberOfPartsDomain.h"
#include "vtkSMObject.h"
#include "vtkSMPart.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyGroupDomain.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSimpleIntInformationHelper.h"
#include "vtkSMSimpleStringInformationHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMSubPropertyIterator.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMXDMFInformationHelper.h"
#include "vtkSMXDMFPropertyDomain.h"
#include "vtkSMXMLParser.h"

#ifdef PARAVIEW_USE_ICE_T
#include "vtkPVDeskTopRenderModule.h"
#include "vtkPVIceTDisplayRenderModule.h"
#include "vtkPVIceTRenderModule.h"
#include "vtkSMProcessModule.h"
#include "vtkSMPropertyInternals.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyManagerInternals.h"
#include "vtkSMSimpleDoubleInformationHelper.h"
#endif  //PARAVIEW_USE_ICE_T

int main(int , char *[])
{
  vtkObject *c;
  c = vtkPVCaveRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVCompositePartDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVCompositeRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVLODPartDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVLODRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVMPIRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVMultiDisplayPartDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVMultiDisplayRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVPartDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVPickDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVPlotDisplay::New(); c->Print( cout ); c->Delete();
  c = vtkPVSimpleRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkRM3DWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMBoxWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMImplicitPlaneWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMLineWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMObject::New(); c->Print( cout ); c->Delete();
  c = vtkRMPointWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMScalarBarWidget::New(); c->Print( cout ); c->Delete();
  c = vtkRMSphereWidget::New(); c->Print( cout ); c->Delete();
  c = vtkSMApplication::New(); c->Print( cout ); c->Delete();
  c = vtkSMArrayListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArrayRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArraySelectionDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMArraySelectionInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMBooleanDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMBoundsDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDataTypeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDisplayerProxy::New(); c->Print( cout ); c->Delete();
//  c = vtkSMDisplayWindowProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDomainIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMDoubleRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMDoubleVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMEnumerationDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMExtentDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMFieldDataDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMFixedTypeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMIdTypeVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMInputArrayDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMInputProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMIntRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMIntVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMLookupTableProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMNumberOfPartsDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMObject::New(); c->Print( cout ); c->Delete();
  c = vtkSMPart::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyAdaptor::New(); c->Print( cout ); c->Delete();
  c = vtkSMProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyGroupDomain::New(); c->Print( cout ); c->Delete();
//  c = vtkSMProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyManager::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleIntInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleStringInformationHelper::New(); c->Print( cout ); c->Delete();
//  c = vtkSMSourceProxy::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringListDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringListRangeDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMStringVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMSubPropertyIterator::New(); c->Print( cout ); c->Delete();
  c = vtkSMVectorProperty::New(); c->Print( cout ); c->Delete();
  c = vtkSMXDMFInformationHelper::New(); c->Print( cout ); c->Delete();
  c = vtkSMXDMFPropertyDomain::New(); c->Print( cout ); c->Delete();
  c = vtkSMXMLParser::New(); c->Print( cout ); c->Delete();

#ifdef PARAVIEW_USE_ICE_T
  c = vtkPVDeskTopRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVIceTDisplayRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkPVIceTRenderModule::New(); c->Print( cout ); c->Delete();
  c = vtkSMProcessModule::New(); c->Print( cout ); c->Delete();
  c = vtkSMPropertyInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMProxyManagerInternals::New(); c->Print( cout ); c->Delete();
  c = vtkSMSimpleDoubleInformationHelper::New(); c->Print( cout ); c->Delete();
#endif  //PARAVIEW_USE_ICE_T

  return 0;
}
