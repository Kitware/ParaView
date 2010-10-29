

#include "pqIntRangeWidget.h"
#include "pqPropertyLinks.h"
#include "vtkCallbackCommand.h"
#include "vtkInitializationHelper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkRenderView.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <QApplication>
#include <QMainWindow>
#include "QVTKWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>

vtkSMProxy* addWavelet(vtkSMProxy* view, vtkSMProxy* wavelet=NULL)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!wavelet)
    {
    wavelet  = pxm->NewProxy("sources", "RTAnalyticSource");
    wavelet->SetConnectionID(view->GetConnectionID());
    wavelet->UpdateVTKObjects();

    vtkSMProxy* tetra = pxm->NewProxy("filters", "DataSetTriangleFilter");
    tetra->SetConnectionID(view->GetConnectionID());
    vtkSMPropertyHelper(tetra, "Input").Set(wavelet);
    tetra->UpdateVTKObjects();
    wavelet->Delete();

    wavelet = tetra;
    }
  else
    {
    wavelet->Register(NULL);
    }

  vtkSMProxy* lut = pxm->NewProxy("lookup_tables",
    "PVLookupTable");
  lut->SetConnectionID(view->GetConnectionID());
  double rgb_points[] = {0, 1, 0, 0, 512, 0, 0, 1 };
  vtkSMPropertyHelper(lut, "RGBPoints").Set(rgb_points, 8);
  lut->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  lut->UpdateVTKObjects();

  vtkSMProxy* opacity = pxm->NewProxy("piecewise_functions",
    "PiecewiseFunction");
  opacity->SetConnectionID(view->GetConnectionID());
  double opacity_points[] = {0.0, 0.0, 512, 1.0};
  vtkSMPropertyHelper(opacity, "Points").Set(opacity_points, 8);
  opacity->UpdateVTKObjects();

  vtkSMProxy* repr = pxm->NewProxy("representations",
    "UnstructuredGridVolumeRepresentation");
  repr->SetConnectionID(view->GetConnectionID());
  vtkSMPropertyHelper(repr, "Input").Set(wavelet);
  vtkSMPropertyHelper(repr, "LookupTable").Set(lut);
  vtkSMPropertyHelper(repr, "ScalarOpacityFunction").Set(opacity);
  vtkSMPropertyHelper(repr, "ColorAttributeType").Set(0);
  vtkSMPropertyHelper(repr, "ColorArrayName").Set("RTData");
//  vtkSMPropertyHelper(repr, "SelectMapper").Set("GPU");
  repr->UpdateVTKObjects();

  vtkSMPropertyHelper(view, "Representations").Add(repr);
  view->UpdateVTKObjects();

  wavelet->Delete();
  repr->Delete();
  lut->Delete();
  opacity->Delete();
  return wavelet;
}

// returns sphere proxy
vtkSMProxy* addSphere(vtkSMProxy* view, vtkSMProxy* sphere = NULL)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!sphere)
    {
    sphere  = pxm->NewProxy("sources", "SphereSource");
    sphere->SetConnectionID(view->GetConnectionID());
    vtkSMPropertyHelper(sphere, "Radius").Set(10);
    vtkSMPropertyHelper(sphere, "PhiResolution").Set(20);
    vtkSMPropertyHelper(sphere, "ThetaResolution").Set(20);
    sphere->UpdateVTKObjects();
    }
  else
    {
    sphere->Register(NULL);
    }

  vtkSMProxy* repr = pxm->NewProxy("representations",
    "GeometryRepresentation");
  repr->SetConnectionID(view->GetConnectionID());
  vtkSMPropertyHelper(repr, "Input").Set(sphere);
  //vtkSMPropertyHelper(repr, "Representation").Set(3);
  repr->UpdateVTKObjects();

  vtkSMPropertyHelper(view, "Representations").Add(repr);
  view->UpdateVTKObjects();

  sphere->Delete();
  repr->Delete();
  return sphere;
}

vtkSMProxy* createScalarBar(vtkSMProxy* view)
{
  return NULL;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* sb = pxm->NewProxy("representations",
    "ScalarBarWidgetRepresentation");
  sb->SetConnectionID(view->GetConnectionID());

  vtkSMProxy* lut = pxm->NewProxy("lookup_tables",
    "LookupTable");
  lut->SetConnectionID(view->GetConnectionID());
  lut->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  lut->UpdateVTKObjects();

  vtkSMPropertyHelper(sb, "LookupTable").Set(lut);
  vtkSMPropertyHelper(sb, "Enabled").Set(1);
  sb->UpdateVTKObjects();
  lut->Delete();

  vtkSMPropertyHelper(view, "Representations").Add(sb);
  sb->Delete();
  return sb;
}

#define SECOND_WINDOW
#define REMOTE_CONNECTION_CS
////#define REMOTE_CONNECTION_CRS

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  QMainWindow mainWindow;
  mainWindow.resize(400, 400);
  mainWindow.show();
  QApplication::processEvents();

  vtkPVOptions* options = vtkPVOptions::New();
  vtkInitializationHelper::Initialize(argc, argv, options);
  options->Delete();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
#ifdef REMOTE_CONNECTION_CS
  vtkIdType connectionID = pm->ConnectToRemote("localhost", 11111);
#else
# ifdef REMOTE_CONNECTION_CRS
  vtkIdType connectionID = pm->ConnectToRemote("localhost", 11111,
    "localhost", 22221);
# else
  vtkIdType connectionID = pm->ConnectToSelf();
# endif
#endif

  vtkSMProxy* viewProxy = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView");
  viewProxy->SetConnectionID(connectionID);
  vtkSMPropertyHelper(viewProxy,"Size").Set(0, 400);
  vtkSMPropertyHelper(viewProxy,"Size").Set(1, 400);
  viewProxy->UpdateVTKObjects();
  viewProxy->UpdateVTKObjects();

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(viewProxy->GetClientSideObject());
  QVTKWidget* qwidget = new QVTKWidget(&mainWindow);
  qwidget->SetRenderWindow(rv->GetRenderWindow());
  vtkSMProxy* sphere = addSphere(viewProxy);
  //addWavelet(viewProxy);
  createScalarBar(viewProxy);

  QWidget *centralWidget = new QWidget(&mainWindow);
  QVBoxLayout* vbox = new QVBoxLayout(centralWidget);
  QHBoxLayout* hbox = new QHBoxLayout();
  vbox->addLayout(hbox);
  mainWindow.setCentralWidget(centralWidget);
  hbox->addWidget(qwidget);

  viewProxy->InvokeCommand("StillRender");
  viewProxy->InvokeCommand("ResetCamera");

  pqIntRangeWidget* slider = new pqIntRangeWidget(&mainWindow);
  slider->setMinimum(3);
  slider->setMaximum(1024);
  slider->setStrictRange(true);

  pqPropertyLinks links;
  links.addPropertyLink(slider, "value", SIGNAL(valueChanged(int)),
    sphere, sphere->GetProperty("PhiResolution"));
  links.addPropertyLink(slider, "value", SIGNAL(valueChanged(int)),
    sphere, sphere->GetProperty("ThetaResolution"));
  links.setAutoUpdateVTKObjects(true);
  links.setUseUncheckedProperties(false);
  vbox->addWidget(slider);

  pqIntRangeWidget* slider2 = new pqIntRangeWidget(&mainWindow);
  slider2->setMinimum(0);
  slider2->setMaximum(1000);
  slider2->setStrictRange(true);

 links.addPropertyLink(slider2, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("RemoteRenderThreshold"));
 links.addPropertyLink(slider2, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("LODThreshold"));
  vbox->addWidget(slider2);

#ifdef SECOND_WINDOW

  vtkSMProxy* view2Proxy = vtkSMProxyManager::GetProxyManager()->NewProxy("views",
    "RenderView");
  view2Proxy->SetConnectionID(connectionID);
  vtkSMPropertyHelper(view2Proxy, "Size").Set(0, 400);
  vtkSMPropertyHelper(view2Proxy, "Size").Set(1, 400);
  vtkSMPropertyHelper(view2Proxy, "Position").Set(0, 400);
  view2Proxy->UpdateVTKObjects();

  vtkPVRenderView* rv2 = vtkPVRenderView::SafeDownCast(view2Proxy->GetClientSideObject());
  qwidget = new QVTKWidget(&mainWindow);
  qwidget->SetRenderWindow(rv2->GetRenderWindow());

  addSphere(view2Proxy, sphere);
  createScalarBar(view2Proxy);

  hbox->addWidget(qwidget);
  view2Proxy->InvokeCommand("StillRender");
  view2Proxy->InvokeCommand("ResetCamera");

 links.addPropertyLink(slider2, "value", SIGNAL(valueChanged(int)),
    view2Proxy, view2Proxy->GetProperty("RemoteRenderThreshold"));
 links.addPropertyLink(slider2, "value", SIGNAL(valueChanged(int)),
    view2Proxy, view2Proxy->GetProperty("LODThreshold"));
  vbox->addWidget(slider2);
#endif
  links.reset();

  int ret = app.exec();
#ifdef SECOND_WINDOW
  view2Proxy->Delete();
#endif
  viewProxy->Delete();

  vtkInitializationHelper::Finalize();
  return ret;
}
