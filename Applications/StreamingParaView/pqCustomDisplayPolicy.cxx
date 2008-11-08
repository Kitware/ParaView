#include "pqCustomDisplayPolicy.h"
#include <pqPipelineSource.h>
#include <vtkSMSourceProxy.h>
#include <pqOutputPort.h>
#include <vtkPVDataInformation.h>
#include <QString>
#include <vtkStructuredData.h>
#include <pqTwoDRenderView.h>

pqCustomDisplayPolicy::pqCustomDisplayPolicy(QObject *o)
  : pqDisplayPolicy(o)
{
}

pqCustomDisplayPolicy::~pqCustomDisplayPolicy()
{
}

//-----------------------------------------------------------------------------
QString pqCustomDisplayPolicy::getPreferredViewType(pqOutputPort* opPort,
  bool update_pipeline) const
{
  pqPipelineSource* source = opPort->getSource();
  if (update_pipeline)
    {
    source->updatePipeline();
    }
  
  QString view_type = QString::null;

  // HACK: for now, when update_pipeline is false, we don't do any gather
  // information as that can result in progress events which may case Qt paint
  // issues.
  vtkSMSourceProxy* spProxy = vtkSMSourceProxy::SafeDownCast(
    source->getProxy());
  if (!spProxy || (!update_pipeline && !spProxy->GetOutputPortsCreated()))
    {
    // If parts aren't created, don't update the information at all.
    // Typically means that the filter hasn't been "Applied" even once and
    // updating information on it may raise errors.
    return view_type;
    }

  vtkPVDataInformation* datainfo = update_pipeline?
    opPort->getDataInformation(true) : opPort->getCachedDataInformation();
  QString className = datainfo?  datainfo->GetDataClassName() : QString();

  // * Check if we should create the 2D view.
  if ((className == "vtkImageData" || className == "vtkUniformGrid") && 
    datainfo->GetCompositeDataClassName()==0)
    {
    int extent[6];
    datainfo->GetExtent(extent);
    int temp[6]={0, 0, 0, 0, 0, 0};
    int dimensionality = vtkStructuredData::GetDataDimension(
      vtkStructuredData::SetExtent(extent, temp));
    if (dimensionality == 2)
      {
      return pqTwoDRenderView::twoDRenderViewType();
      }
    }

  return view_type;
}
