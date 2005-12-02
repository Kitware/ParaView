#include "vtkProperty.h"
#include "vtkKWApplication.h"
#include "vtkKWSurfaceMaterialPropertyWidget.h"
#include "vtkKWWindow.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWSurfaceMaterialPropertyWidgetItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
};

KWWidgetsTourItem* vtkKWSurfaceMaterialPropertyWidgetEntryPoint(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create the surface property that will be modified by the widget

  vtkProperty *sprop1 = vtkProperty::New();

  // -----------------------------------------------------------------------

  // Create the material widget
  // Assign our surface property to the editor

  vtkKWSurfaceMaterialPropertyWidget *sprop1_widget = 
    vtkKWSurfaceMaterialPropertyWidget::New();
  sprop1_widget->SetParent(parent);
  sprop1_widget->Create();
  sprop1_widget->SetBalloonHelpString(
    "A surface material property widget.");

  sprop1_widget->SetProperty(sprop1);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    sprop1_widget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create another material widget, in popup mode
  // Assign our surface property to the editor

  vtkKWSurfaceMaterialPropertyWidget *sprop2_widget = 
    vtkKWSurfaceMaterialPropertyWidget::New();
  sprop2_widget->SetParent(parent);
  sprop2_widget->PopupModeOn();
  sprop2_widget->Create();
  sprop2_widget->SetBalloonHelpString(
    "A surface material property widget, created in popup mode. Note that "
    "it edits the same surface property object as the first widget.");

  sprop2_widget->SetProperty(sprop1);

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 15", 
    sprop2_widget->GetWidgetName());

  // Both editor are linked to the same surface prop, so they should notify
  // each other of any changes to refresh the preview nicely

  sprop2_widget->SetPropertyChangingCommand(sprop1_widget, "Update");
  sprop2_widget->SetPropertyChangedCommand(sprop1_widget, "Update");

  sprop1_widget->SetPropertyChangingCommand(sprop2_widget, "Update");
  sprop1_widget->SetPropertyChangedCommand(sprop2_widget, "Update");

  sprop1_widget->Delete();
  sprop2_widget->Delete();
  sprop1->Delete();

  return new vtkKWSurfaceMaterialPropertyWidgetItem;
}
