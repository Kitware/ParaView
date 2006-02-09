#include "vtkKWMultiColumnList.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWIcon.h"

#include "vtkKWWidgetsTourExample.h"

class vtkKWMultiColumnListItem : public KWWidgetsTourItem
{
public:
  virtual int GetType();
  virtual void Create(vtkKWWidget *parent, vtkKWWindow *win);
};

void vtkKWMultiColumnListItem::Create(vtkKWWidget *parent, vtkKWWindow *win)
{
  vtkKWApplication *app = parent->GetApplication();

  size_t i;

  typedef struct
  {
    const char *Name;
    const char *Version;
    const char *Maintainer;
    int TeamSize;
    int Lock;
    const char *Color;
    double Completion;
    const char *TotalFunding;
    const char *AmountSpent;
    const char *FundingTerminationDate;
  } ProjectEntry;

  ProjectEntry projects[] =
    {
      {"KWWidgets", "1.0", "Sebastien Barre", 1, 0, "1.0 0.5 1.0", 75, "Total Funding: $10000", "Spent 75 cents", "Due on: Oct 5, 2005"},
      {"ParaView",  "2.3", "Ken Martin",      5, 1, "1.0 0.0 0.0", 34, "Total Funding: $3000", "Spent $1000", "Due on: Oct 5, 2006"},
      {"VolView",   "3.0", "Rick Avila",      4, 1, "0.0 1.0 0.0", 55, "Total Funding: $10000", "Spent $3000", "Due on: Dec 5, 2005"},
      {"CMake",     "3.0", "Bill Hoffman",    3, 0, "0.0 0.0 1.0", 85, "Total Funding: $100000", "Spent 2 cents", "Due on: Feb 29, 2500"}
    };

  // -----------------------------------------------------------------------

  // Create a multi-column list

  vtkKWMultiColumnList *mcl1 = vtkKWMultiColumnList::New();
  mcl1->SetParent(parent);
  mcl1->Create();
  mcl1->SetBalloonHelpString(
    "A simple multicolumn list. Columns can be resized, moved, and sorted. "
    "Double-click on some entries to edit them.");
  mcl1->MovableColumnsOn();
  mcl1->SetWidth(0);
  mcl1->SetPotentialCellColorsChangedCommand(
    mcl1, "RefreshColorsOfAllCellsWithWindowCommand");
  mcl1->SetColumnSortedCommand(
    mcl1, "RefreshColorsOfAllCellsWithWindowCommand");

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

  col_index = mcl1->AddColumn(NULL);
  mcl1->SetColumnFormatCommandToEmptyOutput(col_index);
  mcl1->SetColumnLabelImageToPredefinedIcon(col_index, vtkKWIcon::IconLock);

  col_index = mcl1->AddColumn("Color");
  mcl1->ColumnEditableOn(col_index);
  mcl1->SetColumnFormatCommandToEmptyOutput(col_index);

  // The completion command is special. Instead of displaying the value,
  // we will display a frame which length will represent the % of completion
  // In order to do so, we have to hide the text, and later on set a 
  // a callback on each cell that will create that internal frame

  col_index = mcl1->AddColumn("Completion");
  mcl1->SetColumnLabelImageToPredefinedIcon(col_index,vtkKWIcon::IconInfoMini);
  mcl1->SetColumnWidth(col_index, -75);
  mcl1->ColumnResizableOff(col_index);
  mcl1->ColumnStretchableOff(col_index);
  mcl1->SetColumnFormatCommandToEmptyOutput(col_index);

  /*
  col_index = mcl1->AddColumn("Finances");
  mcl1->SetColumnWidth(col_index, 20);
  mcl1->ColumnResizableOff(col_index);
  mcl1->ColumnStretchableOff(col_index);
  */
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

    mcl1->InsertCellTextAsInt(i, 4, project.Lock);
    mcl1->SetCellWindowCommandToCheckButton(i, 4);

    mcl1->InsertCellText(i, 5, project.Color);
    mcl1->SetCellWindowCommandToColorButton(i, 5);

    mcl1->InsertCellTextAsDouble(i, 6, project.Completion);
    mcl1->SetCellWindowCommand(i, 6, NULL, "CreateCompletionCellCallback");

    /*    
    mcl1->SetCellWindowCommandToReadOnlyComboBox( i, 7 );
    mcl1->SetNthEntryInReadOnlyComboBox( 0, project.TotalFunding, i, 7 );
    mcl1->SetNthEntryInReadOnlyComboBox( 1, project.AmountSpent, i, 7 );
    mcl1->SetNthEntryInReadOnlyComboBox( 2, project.FundingTerminationDate, i, 7 );
    */
    }

  app->Script(
    "pack %s -side top -anchor nw -expand n -padx 2 -pady 2", 
    mcl1->GetWidgetName());

  mcl1->Delete();
}

int vtkKWMultiColumnListItem::GetType()
{
  return KWWidgetsTourItem::TypeCore;
}

KWWidgetsTourItem* vtkKWMultiColumnListEntryPoint()
{
  return new vtkKWMultiColumnListItem();
}
