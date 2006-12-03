/*=========================================================================

  Module:    vtkKWLogWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLogWidget.h"

#include "vtkKWEvent.h"
#include "vtkKWIcon.h"
#include "vtkKWMessage.h"
#include "vtkObjectFactory.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"
#include "vtkKWInternationalization.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWToolbar.h"
#include "vtkKWPushButton.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWTextWithScrollbars.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWText.h"

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/string>
#include <vtksys/stl/list>
#include <time.h>

#define MAX_NUMBER_OF_RECORDS 5000
#define MIN_NUMBER_OF_RECORDS 1

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLogWidget );
vtkCxxRevisionMacro(vtkKWLogWidget, "1.5");

vtkIdType vtkKWLogWidget::IdCounter = 1;

//----------------------------------------------------------------------------
class vtkKWLogWidgetInternals
{
public:
  
  // A struct to store an record info

  struct Record
  {
    vtkIdType             Id;
    int                   Type;
    unsigned int          Time;
    vtksys_stl::string    Description;
  };
  
  typedef vtksys_stl::list<Record> RecordContainerType;
  typedef vtksys_stl::list<Record>::iterator RecordContainerIterator;

  RecordContainerType RecordContainer;
  
  vtksys_stl::string ErrorImage;
  vtksys_stl::string WarningImage; 
  vtksys_stl::string InformationImage; 
  vtksys_stl::string DebugImage; 
};

//----------------------------------------------------------------------------
vtkKWLogWidget::vtkKWLogWidget()
{  
  this->Internals              = new vtkKWLogWidgetInternals;

  this->Toolbar                = NULL;
  this->RecordList             = NULL;
  this->SaveButton             = NULL;
  this->RemoveSelectedButton   = NULL;
  this->RemoveAllButton        = NULL;
  this->DescriptionFrame       = NULL;
  this->DescriptionText        = NULL;
  
  this->MaximumNumberOfRecords = 200;
}

//----------------------------------------------------------------------------
vtkKWLogWidget::~vtkKWLogWidget()
{
  if (this->Internals)
    {
    delete this->Internals;
    }
  if (this->RecordList)
    { 
    this->RecordList->Delete();
    }
  if (this->SaveButton)
    {
    this->SaveButton->Delete();
    }
  if (this->RemoveSelectedButton)
    {
    this->RemoveSelectedButton->Delete();
    }
  if (this->RemoveAllButton)
    {
    this->RemoveAllButton->Delete();
    }
  if (this->DescriptionFrame)
    {
    this->DescriptionFrame->Delete();
    }
  if (this->DescriptionText)
    {
    this->DescriptionText->Delete();
    }
  if (this->Toolbar)
    {
    this->Toolbar->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  
  this->CreateRecordList();

  vtkKWIcon *icon = vtkKWIcon::New();
  
  this->Internals->WarningImage = 
    this->RecordList->GetWidget()->GetWidgetName();
  this->Internals->WarningImage.append("_warning");
  icon->SetImage(vtkKWIcon::IconWarningMini);
  if (!vtkKWTkUtilities::UpdatePhoto(
        this->GetApplication(),
        this->Internals->WarningImage.c_str(),
        icon->GetData(), 
        icon->GetWidth(), 
        icon->GetHeight(), 
        icon->GetPixelSize()))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " 
      << this->Internals->WarningImage.c_str());
    }

  this->Internals->ErrorImage = 
    this->RecordList->GetWidget()->GetWidgetName();
  this->Internals->ErrorImage.append("_error");
  icon->SetImage(vtkKWIcon::IconErrorRedMini);
  if (!vtkKWTkUtilities::UpdatePhoto(
        this->GetApplication(),
        this->Internals->ErrorImage.c_str(),
        icon->GetData(), 
        icon->GetWidth(), 
        icon->GetHeight(), 
        icon->GetPixelSize()))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " 
      << this->Internals->ErrorImage.c_str());
    }

  this->Internals->InformationImage = 
    this->RecordList->GetWidget()->GetWidgetName();
  this->Internals->InformationImage.append("_info");
  icon->SetImage(vtkKWIcon::IconInfoMini);
  if (!vtkKWTkUtilities::UpdatePhoto(
        this->GetApplication(),
        this->Internals->InformationImage.c_str(),
        icon->GetData(), 
        icon->GetWidth(), 
        icon->GetHeight(), 
        icon->GetPixelSize()))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " 
      << this->Internals->InformationImage.c_str());
    }

  this->Internals->DebugImage = 
    this->RecordList->GetWidget()->GetWidgetName();
  this->Internals->DebugImage.append("_debug");
  icon->SetImage(vtkKWIcon::IconBugMini);
  if (!vtkKWTkUtilities::UpdatePhoto(
        this->GetApplication(),
        this->Internals->DebugImage.c_str(),
        icon->GetData(), 
        icon->GetWidth(), 
        icon->GetHeight(), 
        icon->GetPixelSize()))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " 
      << this->Internals->DebugImage.c_str());
    }
  
  icon->Delete();
  
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::CreateRecordList()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Setup toolbar

  if (!this->Toolbar)
    {
    this->Toolbar = vtkKWToolbar::New();
    }
  this->Toolbar->SetParent(this);
  this->Toolbar->Create();
  this->Toolbar->SetToolbarAspectToFlat();
  this->Toolbar->SetWidgetsAspectToFlat();

  // Save Button

  if (!this->SaveButton)
    {
    this->SaveButton = vtkKWLoadSaveButton::New();
    }
  this->SaveButton->SetParent(this->Toolbar->GetFrame());
  this->SaveButton->Create();
  this->SaveButton->SetImageToPredefinedIcon(vtkKWIcon::IconFloppy);
  this->SaveButton->SetBalloonHelpString("Write records to a text file");
  this->SaveButton->SetCommand(this, "WriteRecordsToFileCallback");
  this->SaveButton->GetLoadSaveDialog()->SaveDialogOn();
  this->SaveButton->GetLoadSaveDialog()->SetFileTypes("{ {Text} {*.*} }");
  this->SaveButton->GetLoadSaveDialog()->RetrieveLastPathFromRegistry(
    "OpenPath");
  this->Toolbar->AddWidget(this->SaveButton);

  // Remove Selected Button

  if (!this->RemoveSelectedButton)
    {
    this->RemoveSelectedButton = vtkKWPushButton::New();
    }
  this->RemoveSelectedButton->SetParent(this->Toolbar->GetFrame());
  this->RemoveSelectedButton->Create();
  this->RemoveSelectedButton->SetImageToPredefinedIcon(
    vtkKWIcon::IconFileDelete);
  this->RemoveSelectedButton->SetBalloonHelpString("Remove selected records");
  this->RemoveSelectedButton->SetCommand(
    this, "RemoveSelectedRecordsCallback");
  this->Toolbar->AddWidget(this->RemoveSelectedButton);

  // Remove All Button

  if (!this->RemoveAllButton)
    {
    this->RemoveAllButton = vtkKWPushButton::New();
    }
  this->RemoveAllButton->SetParent(this->Toolbar->GetFrame());
  this->RemoveAllButton->Create();
  this->RemoveAllButton->SetImageToPredefinedIcon(vtkKWIcon::IconTrashcan);
  this->RemoveAllButton->SetBalloonHelpString("Clear all records");
  this->RemoveAllButton->SetCommand(this, "RemoveAllRecordsCallback");
  this->Toolbar->AddWidget(this->RemoveAllButton);

  this->Script("pack %s -side top -anchor nw -padx 0 -pady 0",
               this->Toolbar->GetWidgetName());
  
  // Setup multicolumn list

  if (!this->RecordList)
    {
    this->RecordList = vtkKWMultiColumnListWithScrollbars::New();
    }
  this->RecordList->SetParent(this);
  this->RecordList->Create();

  vtkKWMultiColumnList *tablelist = this->RecordList->GetWidget();
  tablelist->MovableColumnsOn();
  tablelist->SetWidth(0);
  tablelist->SetHeight(13);
  tablelist->ExportSelectionOff();
  tablelist->SetColumnSeparatorsVisibility(0);
  tablelist->SetStripeHeight(0);
  tablelist->ColorSortedColumnOn();
  tablelist->ClearStripeBackgroundColor();
  tablelist->SetRowSpacing(0);
  tablelist->SetSelectionModeToExtended();
  tablelist->SetKeyPressDeleteCommand(this, "RemoveSelectedRecordsCallback");
  tablelist->SetSelectionChangedCommand(this, "SelectionChangedCallback");

  int col_index;

  col_index = tablelist->AddColumn("ID");
  tablelist->SetColumnVisibility(col_index, 0);

  col_index = tablelist->AddColumn("Type");

  col_index = tablelist->AddColumn("Time");
  tablelist->SetColumnAlignmentToRight(col_index);
  tablelist->SetColumnFormatCommand(
    col_index, this, "GetFormatTimeStringCallback");
  tablelist->SetColumnSortModeToInteger(col_index);

  col_index = tablelist->AddColumn("Description");

  for (int i = 0; i < tablelist->GetNumberOfColumns(); i++)
    {
    tablelist->SetColumnEditable(i, 0);
    }

  this->Script(
    "pack %s -side top -fill both -expand true -padx 0 -pady 2",
    this->RecordList->GetWidgetName());
  
  // Description Text Frame

  if (!this->DescriptionFrame)
    {
    this->DescriptionFrame = vtkKWFrameWithLabel::New();
    }
  this->DescriptionFrame->SetParent(this);
  this->DescriptionFrame->Create();
  this->DescriptionFrame->SetLabelText("Selected Record Description");

  this->Script("pack %s -side top -anchor nw  -fill x  -padx 0 -pady 0",
               this->DescriptionFrame->GetWidgetName());
  
  // Description Text Box

  if (!this->DescriptionText)
    {
    this->DescriptionText = vtkKWTextWithScrollbars::New();
    }
  this->DescriptionText->SetParent(this->DescriptionFrame->GetFrame());
  this->DescriptionText->Create();
  this->DescriptionText->GetWidget()->SetHeight(8);
  this->DescriptionText->GetWidget()->SetReadOnly(1);

  this->Script("pack %s -side top -fill both -expand true",
               this->DescriptionText->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::AddErrorRecord(const char* description)
{
  return this->AddRecord(description, vtkKWLogWidget::ErrorType);
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::AddWarningRecord(const char* description)
{
  return this->AddRecord(description, vtkKWLogWidget::WarningType);
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::AddInformationRecord(const char* description)
{
  return this->AddRecord(description, vtkKWLogWidget::InformationType);
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::AddDebugRecord(const char* description)
{
  return this->AddRecord(description, vtkKWLogWidget::DebugType);   
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::GetNumberOfRecords()
{
  if (this->Internals)
    {
    return this->Internals->RecordContainer.size();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::RemoveAllRecords()
{
  int nb_records = this->GetNumberOfRecords();

  if (this->Internals)
    {
    this->Internals->RecordContainer.clear();
    }
    
  if (this->RecordList && this->RecordList->IsCreated())
    {
    this->RecordList->GetWidget()->DeleteAllRows();
    }
    
  if (this->DescriptionText && this->DescriptionText->IsCreated())
    {
    this->DescriptionText->GetWidget()->SetText("");
    }
    
  if (nb_records && !this->GetNumberOfRecords())
    {
    this->InvokeEvent(vtkKWLogWidget::RecordsClearedEvent, NULL);
    }

  this->Update();   
}

//----------------------------------------------------------------------------
unsigned int vtkKWLogWidget::GetCurrentTimeInSeconds()
{
  time_t rawtime;
  time(&rawtime );
  return rawtime;
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::WriteRecordsToFileCallback()
{
  if (this->SaveButton && this->SaveButton->IsCreated())
    {
    this->SaveButton->SetText(NULL);
    if (this->SaveButton->GetLoadSaveDialog()->GetStatus() ==
        vtkKWDialog::StatusOK)
      {
      this->SaveButton->GetLoadSaveDialog()->SaveLastPathToRegistry(
        "OpenPath");
      this->WriteRecordsToFile(this->SaveButton->GetFileName());
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::WriteRecordsToFile(const char* filename)
{
  if (!this->RecordList || 
      !this->RecordList->IsCreated())
    {
    return 0;
    }

  // Make sure the user specified a filename

  if (!filename || !(*filename))
    {
    vtkErrorMacro(<< "Please specify a valid file name!");
    return 0;
    }
    
  // Open text file

  ofstream fout(filename, ios::out | ios::trunc);
  if (fout.fail())
    {
    vtkWarningMacro(<< "Unable to open file to write: " << filename);
    return 0;
    }
 
  // Write all records

  vtkKWMultiColumnList *record_list = this->RecordList->GetWidget();
  int numrows = record_list->GetNumberOfRows();

  for (int i = 0; i < numrows; i++)
    {
    fout << "Type: " << record_list->GetCellText(i, 1) << endl;
    fout << "Time: " << this->GetFormatTimeStringCallback(
      record_list->GetCellText(i, 2)) << endl;
    fout << "Description: " << this->GetRecordDescription(
      record_list->GetCellTextAsInt(i, 0)) << endl << endl;
    }
  
  fout.close();
    
  return 1;         
}
//----------------------------------------------------------------------------
// Some  buggy versions of gcc complain about the use of %c: warning: `%c'
// yields only last 2 digits of year in some locales.  Of course  program-
// mers  are  encouraged  to  use %c, it gives the preferred date and time
// representation. One meets all kinds of strange obfuscations to  circum-
// vent this gcc problem. A relatively clean one is to add an intermediate
// function. This is described as bug #3190 in gcc bugzilla:
// [-Wformat-y2k doesn't belong to -Wall - it's hard to avoid]
inline size_t
my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
  return strftime(s, max, fmt, tm);
}

//----------------------------------------------------------------------------
char* vtkKWLogWidget::GetFormatTimeStringCallback(const char* celltext)
{
  if (celltext && *celltext)
    {
    time_t t = 0;
    t = atol(celltext); //    sscanf(celltext, "%lu", &t);
    struct tm *new_time = localtime(&t);
    static char buffer[256];
    my_strftime(buffer, 256, "%c", new_time); 
    return buffer;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::RemoveAllRecordsCallback()
{
  if (this->IsCreated() && 
      this->RecordList->GetWidget()->GetNumberOfRows()>0)
    {
    if (vtkKWMessageDialog::PopupYesNo( 
          this->GetApplication(), 
          this, 
          ks_("Record Widget|Title|Delete All Records?"),
          k_("Are you sure you want to delete all records?"), 
          vtkKWMessageDialog::WarningIcon))
      {
      this->RemoveAllRecords();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::RemoveSelectedRecordsCallback()
{
  if (this->IsCreated() && 
      this->RecordList->GetWidget()->GetNumberOfSelectedRows()>0)
    {
    if (vtkKWMessageDialog::PopupYesNo( 
          this->GetApplication(), 
          this, 
          "RemoveSelectedLogRecords",
          ks_("Record Widget|Title|Delete Selected Records?"),
          k_("Are you sure you want to delete the selected records?"), 
          vtkKWMessageDialog::WarningIcon | vtkKWMessageDialog::RememberYes))
      {
      vtkKWMultiColumnList *record_list = this->RecordList->GetWidget();
      int nb_selected_rows = record_list->GetNumberOfSelectedRows();
      int *indices = new int [nb_selected_rows];
      nb_selected_rows = record_list->GetSelectedRows(indices);
      for (int i = nb_selected_rows - 1; i >= 0; i--)
        {
        this->RemoveInternalRecord(
          record_list->GetCellTextAsInt(indices[i], 0));
        record_list->DeleteRow(indices[i]);
        }
      int nb_rows = record_list->GetNumberOfRows();
      record_list->SelectSingleRow(
        indices[0] >= nb_rows ? nb_rows - 1 : indices[0]);
      delete [] indices;
      this->Update();
      this->DescriptionText->GetWidget()->SetText("");
      if (!this->GetNumberOfRecords())
        {
        this->InvokeEvent(vtkKWLogWidget::RecordsClearedEvent, NULL);
        }
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkKWLogWidget::GetRecordDescription(int record_id)
{
  if (this->Internals->RecordContainer.size()>0)
    {
    vtkKWLogWidgetInternals::RecordContainerIterator it = 
      this->Internals->RecordContainer.begin();
    vtkKWLogWidgetInternals::RecordContainerIterator end = 
      this->Internals->RecordContainer.end();
    for (; it!=end; it++)
      {
      if ((*it).Id == record_id)
        {
        return (*it).Description.c_str();
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::RemoveInternalRecord(int record_id)
{
  if (this->Internals->RecordContainer.size() > 0)
    {
    vtkKWLogWidgetInternals::RecordContainerIterator it = 
      this->Internals->RecordContainer.begin();
    vtkKWLogWidgetInternals::RecordContainerIterator end = 
      this->Internals->RecordContainer.end();
    for (; it!=end; it++)
      {
      if ((*it).Id == record_id)
        {
        this->Internals->RecordContainer.erase(it);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::AddRecord(
  const char* description, int type)
{
  struct vtkKWLogWidgetInternals::Record record;
  record.Id = vtkKWLogWidget::IdCounter++;
  record.Description = description;
  record.Time = this->GetCurrentTimeInSeconds();
  record.Type = type;

  vtkKWMultiColumnList *tablelist = this->RecordList->GetWidget();
  tablelist->InsertRow(0);
  tablelist->SeeRow(0);

  if (record.Type == vtkKWLogWidget::ErrorType)
    {
    tablelist->SetCellText(0, 1, "Error");
    tablelist->SetCellImage(0, 1,  
      this->Internals->ErrorImage.c_str());
    }
  else if (record.Type == vtkKWLogWidget::WarningType)
    {
    tablelist->SetCellText(0, 1, "Warning");
    tablelist->SetCellImage(0, 1,  
      this->Internals->WarningImage.c_str());
    }
   else if (record.Type == vtkKWLogWidget::InformationType)
    {
    tablelist->SetCellText(0, 1, "Information");
    tablelist->SetCellImage(0, 1,  
      this->Internals->InformationImage.c_str());
    }
  else if (record.Type == vtkKWLogWidget::DebugType)
    {
    tablelist->SetCellText(0, 1, "Debug");
    tablelist->SetCellImage(0, 1,  
      this->Internals->DebugImage.c_str());
    }
   
  tablelist->SetCellTextAsInt(0, 2, record.Time);

  // if the description has multiple lines, display only the first
  // line and add (...) at the end of the first line to indicate
  // there is more. The whole description will be displayed 
  // in the description box when this record is selected.

  int newline_pos = record.Description.find_first_of('\n');
  if (newline_pos > 0 && newline_pos < (int)record.Description.length())
    {
    vtksys_stl::string celltext = record.Description.substr(
      0, newline_pos).append("...");
    tablelist->SetCellText(0, 3, celltext.c_str());
    }
  else
    {
    tablelist->SetCellText(0, 3, record.Description.c_str());
    }
    
  tablelist->SetCellTextAsInt(0, 0, record.Id);
  this->Internals->RecordContainer.push_front(record);
  this->PruneRecords();
  
  // If the table is sorted, we need to resort it in the same way.

  int row_index = 0;
  int sorted_col = tablelist->GetLastSortedColumn();
  if (sorted_col >= 1)
    {
    tablelist->SortByColumn(
      sorted_col, tablelist->GetLastSortedOrder());
    row_index = this->GetIndexOfRowWithRecordId(record.Id);
    }
  
  tablelist->SeeRow(row_index);

  this->Update();
  
  return record.Id;
}

//----------------------------------------------------------------------------
int vtkKWLogWidget::GetIndexOfRowWithRecordId(int record_id)
{
  if (this->RecordList)
    {
    vtkKWMultiColumnList *record_list = this->RecordList->GetWidget();
    int nb_rows = record_list->GetNumberOfRows();
    for (int i = nb_rows - 1; i >= 0; i--)
      {
      if (record_list->GetCellTextAsInt(i, 0) == record_id)
        {
        return i;
        }
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::RemoveRowWithRecordId(int record_id)
{
  int row_index = this->GetIndexOfRowWithRecordId(record_id);
  if (row_index >= 0)
    {
    this->RecordList->GetWidget()->DeleteRow(row_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::PruneRecords()
{
  while ((int)this->Internals->RecordContainer.size() > 
         this->GetMaximumNumberOfRecords())
    {
    this->RemoveRowWithRecordId(this->Internals->RecordContainer.back().Id);
    this->Internals->RecordContainer.pop_back();
    }
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::Update()
{
  this->UpdateEnableState();
  
  if (this->RecordList)
    {
    if (this->RecordList->GetWidget()->GetNumberOfRows() == 0)
      {
      this->SaveButton->SetEnabled(0);
      this->RemoveAllButton->SetEnabled(0);
      }
    if (this->RecordList->GetWidget()->
        GetNumberOfSelectedRows() == 0)
      {
      this->RemoveSelectedButton->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::SelectionChangedCallback()
{
  // Update the description text

  if (this->RecordList->GetWidget()->GetNumberOfSelectedRows() > 0)
    {
    this->DescriptionText->GetWidget()->SetText("");
    
    vtkKWMultiColumnList *record_list = this->RecordList->GetWidget();
    int nb_selected_rows = record_list->GetNumberOfSelectedRows();
    int *indices = new int [nb_selected_rows];
    record_list->GetSelectedRows(indices);
    
    if (this->Internals->RecordContainer.size() > 0)
      {
      ostrstream text;
      if (nb_selected_rows > 1)
        {
        for (int i=0; i<nb_selected_rows; i++)
          {
          text << "Type: " << record_list->GetCellText(indices[i], 1) << endl;
          text << "Time: " << this->GetFormatTimeStringCallback(
            record_list->GetCellText(indices[i], 2)) << endl;
          text << "Description: " << this->GetRecordDescription(
            record_list->GetCellTextAsInt(indices[i], 0)) << endl << endl;
          }
        }
      else
        {
        text << this->GetRecordDescription(
          record_list->GetCellTextAsInt(indices[0], 0)) << endl << endl;
        } 
      text << ends;
      this->DescriptionText->GetWidget()->SetText(text.str());
      text.rdbuf()->freeze(0);
      }
    delete [] indices;
    }
    
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::SetMaximumNumberOfRecords(int maxnum)
{
  if (maxnum < MIN_NUMBER_OF_RECORDS)
    {
    maxnum = MIN_NUMBER_OF_RECORDS;
    }
  else if (maxnum > MAX_NUMBER_OF_RECORDS)
    {
    maxnum = MAX_NUMBER_OF_RECORDS;
    }
  if (this->MaximumNumberOfRecords == maxnum)
  {
  return;
  }
  
  this->MaximumNumberOfRecords = maxnum;
  this->PruneRecords();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->RecordList);
  this->PropagateEnableState(this->Toolbar);
  this->PropagateEnableState(this->SaveButton);
  this->PropagateEnableState(this->RemoveSelectedButton);
  this->PropagateEnableState(this->RemoveAllButton);
}

//----------------------------------------------------------------------------
void vtkKWLogWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MaximumNumberOfRecords: " 
     << this->MaximumNumberOfRecords << endl;
}
