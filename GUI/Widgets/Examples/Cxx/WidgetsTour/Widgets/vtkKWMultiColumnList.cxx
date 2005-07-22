#include "vtkKWMultiColumnList.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWIcon.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWMultiColumnListItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeCore; };
};

KWWidgetsTourItem* vtkKWMultiColumnListEntryPoint(vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  size_t i;

  typedef struct
  {
    const char *Name;
    const char *Version;
    const char *Maintainer;
    int TeamSize;
    double Completion;
  } ProjectEntry;

  ProjectEntry projects[] =
    {
      {"KWWidgets", "1.0", "Sebastien Barre", 1, 75},
      {"ParaView",  "2.3", "Ken Martin",      5, 34},
      {"VolView",   "3.0", "Rick Avila",      4, 55},
      {"CMake",     "3.0", "Bill Hoffman",    3, 85}
    };

  // -----------------------------------------------------------------------

  // Create a multi-column list

  vtkKWMultiColumnList *mcl1 = vtkKWMultiColumnList::New();
  mcl1->SetParent(parent);
  mcl1->Create(app);
  mcl1->SetBalloonHelpString(
    "A simple multicolumn list. Columns can be resized, moved, and sorted. "
    "Double-click on some entries to edit them.");
  mcl1->MovableColumnsOn();

  int col_index;

  // Add the columns (make some of them editable)

  col_index = mcl1->AddColumn("Project");

  col_index = mcl1->AddColumn("Version");
  mcl1->SetColumnAlignmentToCenter(col_index);

  col_index = mcl1->AddColumn("Maintainer");
  mcl1->ColumnEditableOn(col_index);
  
  col_index = mcl1->AddColumn("Team Size");
  mcl1->ColumnEditableOn(col_index);
  mcl1->SetColumnAlignmentToCenter(col_index);

  // The completion command is special. Instead of displaying the value,
  // we will display a frame which length will represent the % of completion
  // In order to do so, we have to hide the text, and later on set a 
  // a callback on each cell that will create that internal frame

  col_index = mcl1->AddColumn("Completion");
  mcl1->SetColumnLabelImageToPredefinedIcon(
    col_index, vtkKWIcon::IconInfoMini);
  mcl1->SetColumnWidth(col_index, -75);
  mcl1->ColumnResizableOff(col_index);
  mcl1->ColumnStretchableOff(col_index);
  mcl1->SetColumnFormatCommandToEmptyOutput(col_index);

  // The callback that is invoked for each cell in the completion column. 
  // This is rather ugly to do in C++. In a real application, you will
  // want to use a real C++ callback, and create C++ KWWidgets inside that
  // cell. We can't do it here because this example is not wrapped into Tcl.

  app->Script(
    "proc CreateCompletionCellCallback {tw row col w} { "
    "  frame $w -bg #882233 -relief groove -bd 2 -height 10 -width [expr [%s GetCellTextAsDouble $row $col] * 0.01 * 70] ;"
    "  %s AddBindingsToWidgetName $w "
    "}", mcl1->GetTclName(), mcl1->GetTclName());

  // Insert each project entry

  for (i = 0; i < sizeof(projects) / sizeof(projects[0]); i++)
    {
    ProjectEntry project = projects[i];
    mcl1->InsertCellText(i, 0, project.Name);
    mcl1->InsertCellText(i, 1, project.Version);
    mcl1->InsertCellText(i, 2, project.Maintainer);
    mcl1->InsertCellTextAsInt(i, 3, project.TeamSize);
    mcl1->InsertCellTextAsDouble(i, 4, project.Completion);
    mcl1->SetCellWindowCommand(i, 4, NULL, "CreateCompletionCellCallback");
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    mcl1->GetWidgetName());

  mcl1->Delete();

  return new vtkKWMultiColumnListItem;
}
