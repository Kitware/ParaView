/*=========================================================================

  Module:    vtkKWLogWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLogWidget - a log widget.
// .SECTION Description
// This widget can be used to display various types of records/events in the
// form of a multicolumn log. Each record is timestamped automatically, and 
/// the interface allow the user to sort the list by time, type, or 
// description.
// This widget can be inserted in any widget hierarchy, or used a standalone
// dialog through the vtkKWLogDialog class.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWLogDialog

#ifndef __vtkKWLogWidget_h
#define __vtkKWLogWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWMultiColumnListWithScrollbars;
class vtkKWLogWidgetInternals;
class vtkKWToolbar;
class vtkKWLoadSaveButton;
class vtkKWPushButton;
class vtkKWTextWithScrollbars;
class vtkKWFrameWithLabel;

class KWWidgets_EXPORT vtkKWLogWidget : public vtkKWCompositeWidget
{
public:
  static vtkKWLogWidget* New();
  vtkTypeRevisionMacro(vtkKWLogWidget,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Add a record.
  // Return a unique record ID, which can be used to retrieve later on.
  virtual int AddErrorRecord(const char *description);
  virtual int AddWarningRecord(const char *description);
  virtual int AddInformationRecord(const char *description);
  virtual int AddDebugRecord(const char *description);
  
  // Description:
  // Get number of records.
  virtual int GetNumberOfRecords();

  // Description:
  // Remove all records.
  virtual void RemoveAllRecords();

  // Description:
  // Set/Get the max number of records that will be stored. 
  virtual void SetMaximumNumberOfRecords(int);
  vtkGetMacro(MaximumNumberOfRecords, int);
  
  // Description:
  // Write all record and to a text file.
  // Return 1 on success, 0 otherwise
  virtual int WriteRecordsToFile(const char *filename);
  
  // Description:
  // Callback, do NOT use. 
  // When the time column is displayed, convert 
  // the cell text (time value in seconds) to ctime format.
  // Returns the formatted string that will be displayed.
  virtual char *GetFormatTimeStringCallback(const char *celltext);
  
  // Description:
  // Callback, do NOT use. 
  // Write all records to a text file
  virtual void WriteRecordsToFileCallback();
  
  // Description:
  // Callback, do NOT use. 
  // Remove selected or all records from the table.
  virtual void RemoveSelectedRecordsCallback();
  virtual void RemoveAllRecordsCallback();

  // Description:
  // Callback, do NOT use. 
  // Invoked when selection is changed in the table.
  virtual void SelectionChangedCallback();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object 
  // is updated and propagated to its internal parts/subwidgets. 
  // This will, for example, enable/disable parts of the widget UI, 
  // enable/disable the visibility of 3D widgets, etc.
  virtual void UpdateEnableState();
  virtual void Update();

  // Description:
  // Events. 
  // RecordsClearedEvent is called when all records have been cleared using
  // the "delete selected" or "delete all" button..
  //BTX
  enum
  {
    RecordsClearedEvent = 15000
  };
  //ETX

protected:
  vtkKWLogWidget();
  ~vtkKWLogWidget();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  // Description:
  // Define the record types with enumeration
  //BTX
  enum
  {
    ErrorType = 0,
    WarningType,
    InformationType,
    DebugType
  };
  //ETX
  
  // Description:
  // Create the log list.
  virtual void CreateRecordList();
  
  // Description:
  // Get current time in seconds.
  virtual unsigned int GetCurrentTimeInSeconds();
  
  // Description:
  // Remove the record from internal stored list.
  virtual void RemoveInternalRecord(int record_id);
  
  // Description:
  // Get the record description.
  virtual const char* GetRecordDescription(int record_id);
  
  // Description:
  // Add a record to the table and the stored list using the description 
  // and type.
  // Return a unique record ID.  
  virtual int AddRecord(const char *description, int type);
  
  // Description:
  // Check if the stored internal records number is greater than
  // MaximumNumberOfRecords. If yes, remove the oldest records
  // and corresponsing rows.  
  virtual void PruneRecords();
  
  // Description:
  // Remove the corresponding row from the record table given the 
  // record ID. 
  virtual void RemoveRowWithRecordId(int record_id);

  // Description:
  // Return the index of the row for the given record ID
  virtual int GetIndexOfRowWithRecordId(int record_id);
  
  // Description:
  // Member variables
  vtkKWLogWidgetInternals *Internals;
  vtkKWMultiColumnListWithScrollbars *RecordList;
  vtkKWToolbar *Toolbar;
  vtkKWPushButton *RemoveSelectedButton;
  vtkKWPushButton *RemoveAllButton;
  vtkKWLoadSaveButton *SaveButton;
  vtkKWFrameWithLabel *DescriptionFrame;
  vtkKWTextWithScrollbars *DescriptionText;

  int MaximumNumberOfRecords;
  
private:

  static vtkIdType IdCounter;

  vtkKWLogWidget(const vtkKWLogWidget&); // Not implemented
  void operator=(const vtkKWLogWidget&); // Not implemented
};
#endif
