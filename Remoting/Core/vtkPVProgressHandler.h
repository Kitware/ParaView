// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
 * This also handles abort by sending an abort flag back on each progress event.
 *
 * Progress events are currently not supported in multi-clients mode.
 * Abort is current only supported in built-in and non-distributed client-server mode.
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
 *
 * Starting ParaView 5.5, vtkCommand::MessageEvent is no longer fired.
 */

#ifndef vtkPVProgressHandler_h
#define vtkPVProgressHandler_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

#include <set>

class vtkMultiProcessController;
class vtkPVSession;

class VTKREMOTINGCORE_EXPORT vtkPVProgressHandler : public vtkObject
{
public:
  static vtkPVProgressHandler* New();
  vtkTypeMacro(vtkPVProgressHandler, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the session. This is not reference-counted to avoid cycles.
   */
  void SetSession(vtkPVSession* conn);
  vtkGetObjectMacro(Session, vtkPVSession);
  ///@}

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
   * This method add wrong tag event handlers and rmi call back
   * for progress related mathods
   */
  void AddHandlers();

  /**
   * Get whether or not progress is currently enable and if
   * this progress handler is ready to receive progress events
   */
  bool GetEnableProgress();

  /**
   * This method collects all outstanding progress messages. All progress
   * events after this call are ignored.
   */
  void CleanupPendingProgress();

  /**
   * Local cleanup of progress flags
   */
  void LocalCleanupPendingProgress();

  ///@{
  /**
   * Get/Set the progress interval in seconds. Progress events
   * occurring more frequently than this interval are skipped.
   * Default is 0.1 seconds on client and 1 second on server and batch processes.
   */
  vtkSetClampMacro(ProgressInterval, double, 0.01, 30.0);
  vtkGetMacro(ProgressInterval, double);
  ///@}

  ///@{
  /**
   * These are only valid in handler for the vtkCommand::ProgressEvent.
   */
  vtkGetStringMacro(LastProgressText);
  vtkGetMacro(LastProgress, int);
  vtkGetMacro(LastProgressId, vtkTypeUInt32);
  ///@}

  /**
   * Abort the object linked with provided object id.
   * This method only store the information which will only
   * be used on the next progress id of this object.
   * This class is only able to SetAbortExecute flag.
   * Use vtkSMProxy/vtkSIProxy ClearAbortFlags to remove them.
   * @warning this does not work with a distributed server.
   */
  void Abort(vtkTypeUInt32 objectId);

  /**
   * Recover the set of all aborted object ids
   */
  std::set<int> GetAbortedObjectIds();

  /**
   * Clear the set of all aborted object ids
   */
  void ClearAbortedObjectIds();

  /**
   * Enable abort checking for a specific object
   */
  void EnableAbortCheck(vtkTypeUInt32 objectId);

protected:
  vtkPVProgressHandler();
  ~vtkPVProgressHandler() override;

  enum TAGS
  {
    ABORT_TAG = 188968,
    CLEANUP_TAG = 188969,
    PROGRESS_EVENT_TAG = 188970,
    MESSAGE_EVENT_TAG = 188971
  };

  enum RMI_TAGS
  {
    CLEANUP_TAG_RMI = 188972,
    MESSAGE_EVENT_TAG_RMI = 188973
  };

  /**
   * Update the last progress and progress text and invokes a progress event
   */
  void RefreshProgress(const char* progress_text, double progress, vtkTypeUInt32 progress_id);

  vtkPVSession* Session;
  double ProgressInterval;

private:
  vtkPVProgressHandler(const vtkPVProgressHandler&) = delete;
  void operator=(const vtkPVProgressHandler&) = delete;

  /**
   * Callback called when vtkCommand::ProgressEvent is received.
   */
  void OnProgressEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
   * Callback called when events from vtkOutputWindow singleton are received.
   * This is also called when vtkCommand::MessageEvent is received from any
   * vtkObject we're observing progress from.
   */
  void OnMessageEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
   * Callback called when WrongTagEvent is fired by the controllers.
   */
  bool OnWrongTagEvent(vtkObject* caller, unsigned long eventid, void* calldata);

  /**
   * Update the last message and invokes a message event
   */
  void RefreshMessage(const char* message_text, int eventid, bool is_local);

  /**
   * Check if provided progressId was aborted and set the abort execute flag server side if needed
   * Server side, caller is exepected to be non-null.
   * Client side, communicator is expected to be non-null.
   * @warning this does not work with a distributed server.
   */
  void CheckAbort(vtkTypeUInt32 progressId, vtkObject* caller, vtkObject* communicator);

  bool AddedHandlers;
  class vtkInternals;
  vtkInternals* Internals;

  vtkSetStringMacro(LastProgressText);
  int LastProgress;
  char* LastProgressText;
  vtkTypeUInt32 LastProgressId;

  class RMICallback;
  friend class RMICallback;
  ;
};

#endif
