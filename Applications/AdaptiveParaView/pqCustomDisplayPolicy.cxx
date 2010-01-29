#include "pqCustomDisplayPolicy.h"
#include <pqPipelineSource.h>
#include <vtkSMSourceProxy.h>
#include <pqOutputPort.h>
#include <vtkPVDataInformation.h>
#include <QString>
#include <vtkStructuredData.h>
#include <pqTwoDRenderView.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <QDebug>

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

  vtkPVDataInformation* datainfo =  opPort->getDataInformation();
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

//-----------------------------------------------------------------------------
pqDataRepresentation* pqCustomDisplayPolicy::setRepresentationVisibility(
  pqOutputPort* opPort, pqView* view, bool visible)  const
{
  if (!opPort)
    {
    // Cannot really repr a NULL source.
    return 0;
    }

  pqDataRepresentation* repr = opPort->getRepresentation(view);

  if (!repr && !visible)
    {
    // isn't visible already, nothing to change.
    return 0;
    }
  else if(!repr)
    {
    // FIXME:UDA -- can't we simply use createPreferredRepresentation?
    // No repr exists for this view.
    // First check if the view exists. If not, we will create a "suitable" view.
    if (!view)
      {
      view = this->getPreferredView(opPort, view);
      }
    if (view)
      {
      repr = pqApplicationCore::instance()->getObjectBuilder()->
        createDataRepresentation(opPort, view);
      }
    }
  if (!repr)
    {
    qDebug() << "Cannot show the data in the current view although"
      "the view reported that it can show the data.";
    return 0;
    }

  repr->setVisible(visible);

  // If this is the only source displayed in the view, reset the camera to make 
  // sure its visible. Only do so if a source is being turned ON. Otherwise when 
  // the next to last source is turned off, the camera would be reset to fit the 
  // last remaining one which would be unexpected to the user 
  // (hence the conditional on "visible")
  //
  // The static pointer makes it not reset the viewpoint when we hide and 
  // show the same thing.
  static pqView *lview = NULL;
  if(view->getNumberOfVisibleRepresentations()==1 && visible)
    {
    pqRenderViewBase* ren = qobject_cast<pqRenderViewBase*>(view);
    if (ren && lview != view)
      {
      ren->resetCamera();
      lview = view;
      }
    }

  return repr;
}
