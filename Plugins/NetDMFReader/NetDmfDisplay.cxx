
#include "NetDmfDisplay.h"

#include "GlyphRepresentation.h"
#include "NetDmfView.h"

#include "pqCubeAxesEditorDialog.h"
#include "pqDisplayProxyEditor.h"
#include "pqOutputPort.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"

#include "ui_NetDmfDisplayPanel.h"

#include "vtkPVDataInformation.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMGlyphRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMScatterPlotViewProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkScatterPlotMapper.h"
#include "vtkWeakPointer.h"

#include <QDebug>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

inline void vtkSMProxySetString(
  vtkSMProxy* proxy, const char* pname, const char* pval)
{
  vtkSMStringVectorProperty* ivp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, pval);
    proxy->UpdateProperty(pname);
    }
}

inline void vtkSMProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    proxy->UpdateProperty(pname);
    }
}


//-----------------------------------------------------------------------------
class NetDmfDisplay::pqInternal : public Ui::NetDmfDisplayPanel
{
public:
  pqInternal()
    {
      this->CompositeTreeAdaptor =0;
    }

  ~pqInternal()
    {
      delete this->CompositeTreeAdaptor;
    }

  pqPropertyLinks Links;
  vtkWeakPointer<vtkSMGlyphRepresentationProxy> GlyphRepresentationProxy;
  QPointer<GlyphRepresentation> Representation;

  pqSignalAdaptorCompositeTreeWidget* CompositeTreeAdaptor;
};


NetDmfDisplay::NetDmfDisplay(pqRepresentation* d, QWidget* p)
  : pqDisplayPanel(d,p)
{
  this->Internal = 0;

  pqPipelineRepresentation* pipelineRepresentation = 
    qobject_cast<pqPipelineRepresentation*>(d);

  if (!pipelineRepresentation)
    {
    return;
    }

  GlyphRepresentation* glyphRepresentation = 
    qobject_cast<GlyphRepresentation*>(d);

  vtkSMGlyphRepresentationProxy* proxy =
    vtkSMGlyphRepresentationProxy::SafeDownCast(d->getProxy());
  if (!proxy)
    {
    qWarning() << "NetDmfDisplay given a representation proxy "
                  "that is not a GlyphRepresentation.  Cannot edit.";
    return;
    }

  // Locate the representation-type domain.
  vtkSMProperty* prop = proxy->GetProperty("Representation");
  vtkSMEnumerationDomain* enumDomain = prop?
    vtkSMEnumerationDomain::SafeDownCast(prop->GetDomain("enum")): 0;
  int glyphRep = enumDomain->GetEntryValueForText("Glyph Representation");
  if (glyphRep != glyphRepresentation->getRepresentationType())
    {
    //QWidget* l = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(this);
    pqDisplayProxyEditor* w =  new pqDisplayProxyEditor(glyphRepresentation, this);
    l->addWidget(w);
    return;
    }

  this->Internal = new NetDmfDisplay::pqInternal();
  this->Internal->setupUi(this);
  
  QObject::connect(&this->Internal->Links, SIGNAL(smPropertyChanged()),
                   this, SLOT(updateAllViews()));

  this->Internal->Representation = glyphRepresentation;
  this->Internal->GlyphRepresentationProxy = proxy;

  
  // setup visibility
  this->Internal->Links.addPropertyLink(this->Internal->ViewData,
    "checked", SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("Visibility"));

  // setup zoomtodata
  QObject::connect(
    this->Internal->ViewZoomToData, SIGNAL(clicked(bool)), 
    this, SLOT(zoomToData()));

  // setup cube axes visibility.
  if ((prop = proxy->GetProperty("CubeAxesVisibility")) != 0)
    {
    this->Internal->ShowCubeAxes->setChecked( 
      pqSMAdaptor::getElementProperty(prop).toInt());
    QObject::connect(this->Internal->ShowCubeAxes, SIGNAL(toggled(bool)),
                     this, SLOT(cubeAxesVisibilityChanged()));
    this->Internal->AnnotationGroup->show();
    }
  else
    {
    this->Internal->AnnotationGroup->hide();
    }
  QObject::connect(this->Internal->EditCubeAxes, SIGNAL(clicked(bool)),
                   this, SLOT(editCubeAxes()));

  // setup for point size
  if ((prop = proxy->GetProperty("PointSize")) !=0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->StylePointSize,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("PointSize"));
    this->Internal->StylePointSize->setEnabled(true);
    }
  else
    {
    this->Internal->StylePointSize->setEnabled(false);
    }
  // setup for line width
  if ((prop = proxy->GetProperty("LineWidth")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->StyleLineWidth,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("LineWidth"));
    this->Internal->StyleLineWidth->setEnabled(true);
    }
  else
    {
    this->Internal->StyleLineWidth->setEnabled(false);
    }
  // setup for opacity  
  if ((prop = proxy->GetProperty("Opacity")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->Opacity,
      "value", SIGNAL(editingFinished()),
      proxy, proxy->GetProperty("Opacity"));
    this->Internal->Opacity->setEnabled(true);
    }
  else
    {
    this->Internal->Opacity->setEnabled(false);
    }

  // setup glyph
  vtkSMProperty* glyphModeProperty = proxy->GetProperty("GlyphMode");
  if (glyphModeProperty)
    {
    QObject::connect(this->Internal->GlyphGroupBox,
                     SIGNAL(toggled(bool)),
                     this, SLOT(updateGlyphMode()), Qt::QueuedConnection);

    int glyphMode = pqSMAdaptor::getElementProperty(glyphModeProperty).toInt();
    this->Internal->GlyphGroupBox->setChecked(
      glyphMode & vtkScatterPlotMapper::UseGlyph);
    }

  connect(this->Internal->GlyphInputComboBox, 
          SIGNAL(currentIndexChanged(const QString&)),
          glyphRepresentation, 
          SLOT( setGlyphInput(const QString&)));
  this->Internal->GlyphInputComboBox->addItems(
    glyphRepresentation->getGlyphSources());
  this->Internal->GlyphGroupBox->setEnabled(
    this->Internal->GlyphInputComboBox->count() > 0);

  // setup for scale factor
  if ((prop = proxy->GetProperty("ScaleFactor")) != 0)
    {
    this->Internal->Links.addPropertyLink(this->Internal->GlyphScaleFactorSpinBox,
                                          "value", SIGNAL(valueChanged(double)),
                                          proxy, proxy->GetProperty("ScaleFactor"));
    this->Internal->GlyphScaleFactorSpinBox->setEnabled(true);
    }
  else
    {
    this->Internal->GlyphScaleFactorSpinBox->setEnabled(false);
    }

  // setup for compositeTree
  if (proxy->GetProperty("ExtractedBlockIndex"))
    {
    this->Internal->CompositeTreeAdaptor = 
      new pqSignalAdaptorCompositeTreeWidget(
        this->Internal->compositeTree,
        this->Internal->Representation->getOutputPortFromInput()->getOutputPortProxy(),
        vtkSMCompositeTreeDomain::NONE,
        pqSignalAdaptorCompositeTreeWidget::INDEX_MODE_FLAT, false, true);
    this->Internal->compositeTree->hide();
    }
  else
    {
    this->Internal->compositeTree->hide();
    }

  // one has to force the rendering of the view so that 
  // the mappers have the correct data to compute the bounds 
  this->Internal->Representation->renderView(true);
  this->zoomToData();
  // the vtkPainters propagate the vtkInformation only when
  // rendering. It is a problem to compute the bounds.
  this->Internal->Representation->renderView(true);
  this->zoomToData();
}

NetDmfDisplay::~NetDmfDisplay()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void NetDmfDisplay::zoomToData()
{
  double bounds[6];
  this->Internal->GlyphRepresentationProxy->GetBounds(bounds);
    
  if (bounds[0] <= bounds[1] && 
      bounds[2] <= bounds[3] && 
      bounds[4] <= bounds[5])
    {
    pqRenderView* renModule = qobject_cast<pqRenderView*>(
      this->Internal->Representation->getView());
    if (renModule)
      {
      vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
      rm->ResetCamera(bounds);
      renModule->render();
      return;
      }
    NetDmfView* NetDmfModule = qobject_cast<NetDmfView*>(
      this->Internal->Representation->getView());
    if (NetDmfModule)
      {
      vtkSMScatterPlotViewProxy* rm = 
        vtkSMScatterPlotViewProxy::SafeDownCast(NetDmfModule->getViewProxy());
      rm->GetRenderView()->ResetCamera(bounds);
      NetDmfModule->render();
      }
    }
}

//-----------------------------------------------------------------------------
void NetDmfDisplay::cubeAxesVisibilityChanged()
{
  vtkSMProxySetInt(this->Internal->GlyphRepresentationProxy, "CubeAxesVisibility", 
                   this->Internal->ShowCubeAxes->isChecked());
  this->updateAllViews();
}

//-----------------------------------------------------------------------------
void NetDmfDisplay::editCubeAxes()
{
  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->Internal->GlyphRepresentationProxy);
  dialog.exec();
}
//-----------------------------------------------------------------------------
void NetDmfDisplay::updateGlyphMode()
{  
  int glyphMode = 0;
  if(this->Internal->GlyphGroupBox->isChecked())
    {
    glyphMode |= vtkScatterPlotMapper::UseGlyph;
    }
  glyphMode |= vtkScatterPlotMapper::OrientedGlyph;

  vtkSMProxySetInt(this->Internal->GlyphRepresentationProxy, "GlyphMode", glyphMode);
  this->updateAllViews();
}
