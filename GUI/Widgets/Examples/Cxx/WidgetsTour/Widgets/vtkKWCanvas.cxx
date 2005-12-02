#include "vtkKWCanvas.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWCanvasItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWCanvasEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // Create a canvas

  vtkKWCanvas *canvas1 = vtkKWCanvas::New();
  canvas1->SetParent(parent);
  canvas1->Create();
  canvas1->SetWidth(400);
  canvas1->SetHeight(200);
  canvas1->SetBorderWidth(2);
  canvas1->SetReliefToGroove();
  canvas1->SetBackgroundColor(0.4, 0.6, 0.9);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    canvas1->GetWidgetName());

  // There is no C++ API at the moment to access Tk's Canvas functions,
  // so let's just use Tk in the example:

  const char *wname = canvas1->GetWidgetName();

  app->Script(
    "%s create arc 10 10 90 90 -start 20 -extent 120 -width 1", wname);

  app->Script("%s create line 45 55 140 150 -width 1 -fill #223344", wname);
  app->Script("%s create line 41 59 120 160 -width 2 -fill #445566", wname);
  app->Script("%s create line 37 63 100 170 -width 3 -fill #667788", wname);
  app->Script("%s create line 33 67 80  180 -width 4 -fill #8899AA", wname);
  app->Script("%s create line 29 71 60  190 -width 3 -fill #AABBCC", wname);
  app->Script("%s create line 25 75 40  200 -width 2 -fill #CCDDEE", wname);
  app->Script("%s create line 21 79 20  190 -width 1 -fill #EEFFFF", wname);

  app->Script(
    "%s create oval 160 10 340 100 -outline #22BB33 -fill red -width 2", wname);

  app->Script(
    "%s create polygon 360 80 380 100 350 130 390 160 -outline green", wname);

  app->Script(
    "%s create rectangle 150 80 320 180 -outline #666666 -width 3", wname);

  // TODO: add a canvas with scrollbars

  canvas1->Delete();

  return new vtkKWCanvasItem;
}
