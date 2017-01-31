/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProgressHandler.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVProgressHandler
 * @brief   progress handler.
 *
 * vtkPVProgressHandler handles the progress messages. It handles progress in
 * all configurations single process, client-server. It must be noted that when
 * running in parallel, progress updates are fetched from the root node. Due to
 * performance reasons, we no longer collect progress events (or messages) from
 * satellites, only root-node events are reported back to the client. While this
 * may not faithfully report the progress, this avoid nasty MPI issues that can
 * be painful to debug and diagnose.
 *
 * Progress events are currently not supported in multi-clients mode.
 *
 * @par Events:
 * vtkCommand::StartEvent
 * \li fired to indicate beginning of progress handling
 * \li \c calldata: vtkPVProgressHandler*
 * vtkCommand::ProgressEvent
 * \li fired to indicate a progress event.
 * \li \c calldata: vtkPVProgressHandler*
 * vtkCommand::EndEvent
 * \li fired to indicate end of progress handling
 * \li \c calldata: vtkPVProgressHandler*
*/

#ifndef vtkPVProgressHandler_h
#define vtkPVProgressHandler_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkMultiProcessController;
class vtkPVSession;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVProgressHandler : public vtkObject
{
public:
  static vtkPVProgressHandler* New();
  vtkTypeMacro(vtkPVProgressHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the session. This is not reference-counted to avoid cycles.
   */
  void SetSession(vtkPVSession* conn);
  vtkGetObjectMacro(Session, vtkPVSession);
  //@}

  /**
   * Listen to progress events from the object.
   */
  void RegisterProgressEvent(vtkObject* object, int id);

  /**
   * This method resets all the progress counters and prepares progress
   * reporting. All progress events before this call are ignored.
   */
  void PrepareProgress();

  /**
   * This method collects all outstanding progress messages. All progress
   * events after this call are ignored.
   */
  void CleanupPendingProgress();

  //@{
  /**
   * Get/Set the progress frequency in seconds. Default is 0.5 seconds.
   */
  vtkSetClampMacro(ProgressFrequency, double, 0.01, 30.0);
  vtkGetMacro(ProgressFrequency, double);
  //@}

  //@{
  /**
   * These are only valid in handler for the vtkCommand::ProgressEvent.
   */
  vtkGetStringMacro(LastProgressText);
  vtkGetMacro(LastProgress, int);
  //@}

  //@{
  /**
   * Temporary storage for most recent message text.
   */
  vtkGetStringMacro(LastMessage);
  //@}

protected:
  vtkPVProgressHandler();
  ~vtkPVProgressHandler();

  enum eTAGS
  {
    CLEANUP_TAG = 188969,
    PROGRESS_EVENT_TAG = 188970,
    MESSAGE_EVENT_TAG = 188971
  };

  //@{
  void RefreshProgress(const char* progress_text, double progress);
  void RefreshMessage(const char* message_text);
  //@}

  vtkPVSession* Session;
  double ProgressFrequency;

private:
  vtkPVProgressHandler(const vtkPVProgressHandler&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVProgressHandler&) VTK_DELETE_FUNCTION;

  /**
   * Callback called when vtkCommand::ProgressEvent is received.
   */
  void OnProgressEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
   * Callback called when vtkCommand::MessageEvent is received.
   */
  void OnMessageEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
   * Callback called when WrongTagEvent is fired by the controllers.
   */
  bool OnWrongTagEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  bool AddedHandlers;
  class vtkInternals;
  vtkInternals* Internals;

  vtkSetStringMacro(LastProgressText);
  int LastProgress;
  char* LastProgressText;

  vtkSetStringMacro(LastMessage);
  char* LastMessage;
};

#endif
