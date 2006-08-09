#include "vtkKWPushButton.h"
#include "vtkKWPushButtonWithLabel.h"
#include "vtkKWPushButtonWithMenu.h"
#include "vtkKWPushButtonSet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWMenu.h"
#include "vtkKWIcon.h"
#include "vtkMath.h"

#include <vtksys/stl/string>
#include "vtkKWWidgetsTourExample.h"

class vtkKWPushButtonItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *);
};

void vtkKWPushButtonItem::Create(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a push button

  vtkKWPushButton *pushbutton1 = vtkKWPushButton::New();
  pushbutton1->SetParent(parent);
  pushbutton1->Create();
  pushbutton1->SetText("A push button");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    pushbutton1->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another push button, use an icon

  vtkKWPushButton *pushbutton2 = vtkKWPushButton::New();
  pushbutton2->SetParent(parent);
  pushbutton2->Create();
  pushbutton2->SetImageToPredefinedIcon(vtkKWIcon::IconConnection);
  pushbutton2->SetBalloonHelpString(
    "Another pushbutton, using one of the predefined icons");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton2->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another push button, use both text and icon

  vtkKWPushButton *pushbutton2b = vtkKWPushButton::New();
  pushbutton2b->SetParent(parent);
  pushbutton2b->Create();
  pushbutton2b->SetText("A push button with an icon");
  pushbutton2b->SetImageToPredefinedIcon(vtkKWIcon::IconWarningMini);
  pushbutton2b->SetCompoundModeToLeft();
  pushbutton2b->SetBalloonHelpString(
    "Another pushbutton, using both a text and one of the predefined icons");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton2b->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another push button, with a label this time

  vtkKWPushButtonWithLabel *pushbutton3 = vtkKWPushButtonWithLabel::New();
  pushbutton3->SetParent(parent);
  pushbutton3->Create();
  pushbutton3->SetLabelText("Press this...");
  pushbutton3->GetWidget()->SetText("button");
  pushbutton3->SetBalloonHelpString(
    "This is a vtkKWPushButtonWithLabel, i.e. a pushbutton associated to a "
    "label that can be positioned around the pushbutton.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton3->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another push button, with a menu

  vtkKWPushButtonWithMenu *pushbutton4 = vtkKWPushButtonWithMenu::New();
  pushbutton4->SetParent(parent);
  pushbutton4->Create();
  pushbutton4->GetPushButton()->SetImageToPredefinedIcon(
    vtkKWIcon::IconTransportRewind);
  pushbutton4->SetBalloonHelpString(
    "This is a vtkKWPushButtonWithMenu, i.e. a pushbutton associated to a "
    "menu.");

  vtkKWMenu *menu = pushbutton4->GetMenu();
  menu->AddCommand("Microsoft Office");
  menu->AddCommand("Program Files");
  menu->AddCommand("C:");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton4->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a set of pushbutton
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWPushButtonSet *pushbutton_set = vtkKWPushButtonSet::New();
  pushbutton_set->SetParent(parent);
  pushbutton_set->Create();
  pushbutton_set->SetBorderWidth(2);
  pushbutton_set->SetReliefToGroove();
  pushbutton_set->SetWidgetsPadX(1);
  pushbutton_set->SetWidgetsPadY(1);
  pushbutton_set->SetPadX(1);
  pushbutton_set->SetPadY(1);
  pushbutton_set->ExpandWidgetsOn();
  pushbutton_set->SetMaximumNumberOfWidgetsInPackingDirection(3);

  char buffer[50];
  for (int id = 0; id < 9; id++)
    {
    sprintf(buffer, "Push button %d", id);
    vtkKWPushButton *pushbutton = pushbutton_set->AddWidget(id);
    pushbutton->SetText(buffer);
    pushbutton->SetBackgroundColor(
      vtkMath::HSVToRGB((double)id / 8.0, 0.3, 0.75));
    pushbutton->SetBalloonHelpString(
      "This pushbutton is part of a unique set (a vtkKWPushButtonSet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one. The widgets can be layout as a "
      "NxM grid. Each button is assigned a different color.");
    }

  pushbutton_set->GetWidget(0)->SetText("I'm the first button");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    pushbutton_set->GetWidgetName());

  pushbutton1->Delete();
  pushbutton2->Delete();
  pushbutton2b->Delete();
  pushbutton3->Delete();
  pushbutton4->Delete();
  pushbutton_set->Delete();

  // TODO: add callbacks
}

int vtkKWPushButtonItem::GetType()
{
  return KWWidgetsTourItem::TypeCore;
}

KWWidgetsTourItem* vtkKWPushButtonEntryPoint()
{
  return new vtkKWPushButtonItem();
}
