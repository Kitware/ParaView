#include "vtkKWLabel.h"
#include "vtkKWLabelLabeled.h"
#include "vtkKWLabelSet.h"
#include "vtkKWLabelLabeledSet.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWIcon.h"

#include <vtksys/stl/string>
#include "KWWidgetsTourExampleTypes.h"

class vtkKWLabelItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWLabelEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a label

  vtkKWLabel *label1 = vtkKWLabel::New();
  label1->SetParent(parent);
  label1->Create(app);
  label1->SetText("A label");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    label1->GetWidgetName());

  label1->Delete();

  // -----------------------------------------------------------------------

  // Create another label, right justify it

  vtkKWLabel *label2 = vtkKWLabel::New();
  label2->SetParent(parent);
  label2->Create(app);
  label2->SetText("Another label");
  label2->SetJustificationToRight();
  label2->SetWidth(30);
  label2->SetBackgroundColor(0.5, 0.5, 0.95);
  label2->SetBorderWidth(2);
  label2->SetReliefToGroove();
  label2->SetBalloonHelpString(
    "Another label, explicit width, right-justified");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    label2->GetWidgetName());

  label2->Delete();

  // -----------------------------------------------------------------------

  // Create another label, with a label this time (!)

  vtkKWLabelLabeled *label3 = vtkKWLabelLabeled::New();
  label3->SetParent(parent);
  label3->Create(app);
  label3->GetLabel()->SetText("Name:");
  label3->GetLabel()->SetBackgroundColor(0.7, 0.7, 0.7);
  label3->GetWidget()->SetText("Sebastien Barre");
  label3->SetBalloonHelpString(
    "This is a vtkKWLabelLabeled, i.e. a label associated to a "
    "label that can be positioned around the label. This can be used for "
    "example to label a value without having to construct a single "
    "label out of two separate elements, one of them likely not to change.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    label3->GetWidgetName());

  label3->Delete();

  // -----------------------------------------------------------------------

  // Create another label, with a label this time (!)

  vtkKWLabelLabeled *label4 = vtkKWLabelLabeled::New();
  label4->SetParent(parent);
  label4->Create(app);
  label4->GetLabel()->SetImageOption(vtkKWIcon::IconInfoMini);
  label4->GetWidget()->SetText("Another use of a labeled label !");
  label4->SetBalloonHelpString(
    "This is a vtkKWLabelLabeled, i.e. a label associated to a "
    "label that can be positioned around the label. This can be used for "
    "example to prefix a label with a small icon to emphasize its meaning. "
    "Predefined icons include warning, info, error, etc.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    label4->GetWidgetName());

  label4->Delete();

  // -----------------------------------------------------------------------

  // Create a set of label
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWLabelSet *label_set = vtkKWLabelSet::New();
  label_set->SetParent(parent);
  label_set->Create(app);
  label_set->SetBorderWidth(2);
  label_set->SetReliefToGroove();

  for (int i = 0; i < 3; i++)
    {
    vtkKWLabel *label = label_set->AddWidget(i);
    label->SetBalloonHelpString(
      "This label is part of a unique set (a vtkKWLabelSet), "
      "which provides an easy way to create a bunch of related widgets "
      "without allocating them one by one.");
    }

  label_set->GetWidget(0)->SetText("Label 0");
  label_set->GetWidget(1)->SetText("Label 1");
  label_set->GetWidget(2)->SetText("Label 3");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    label_set->GetWidgetName());

  label_set->Delete();

  // -----------------------------------------------------------------------

  // Even trickier: create a set of labeled label !
  // An easy way to create a bunch of related widgets without allocating
  // them one by one

  vtkKWLabelLabeledSet *label_set2 = vtkKWLabelLabeledSet::New();
  label_set2->SetParent(parent);
  label_set2->Create(app);
  label_set2->SetBorderWidth(2);
  label_set2->SetReliefToGroove();
  label_set2->SetWidgetsPadX(2);
  label_set2->SetWidgetsPadY(1);
  label_set2->SetPadY(1);

  for (int i = 0; i < 3; i++)
    {
    vtkKWLabelLabeled *label = label_set2->AddWidget(i);
    label->SetLabelWidth(15);
    label->GetLabel()->SetBackgroundColor(0.7, 0.7, 0.7);
    label->SetBalloonHelpString(
      "This labeled label (!) is part of a unique set "
      "(a vtkKWLabeledLabelSet).");
    }

  label_set2->GetWidget(0)->SetLabelText("First Name:");
  label_set2->GetWidget(0)->GetWidget()->SetText("Sebastien");
  label_set2->GetWidget(1)->SetLabelText("Name:");
  label_set2->GetWidget(1)->GetWidget()->SetText("Barre");
  label_set2->GetWidget(2)->SetLabelText("Company:");
  label_set2->GetWidget(2)->GetWidget()->SetText("Kitware, Inc.");

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 6", 
    label_set2->GetWidgetName());

  label_set2->Delete();

  return new vtkKWLabelItem;
}
