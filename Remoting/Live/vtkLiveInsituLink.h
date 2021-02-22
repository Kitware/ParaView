/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @defgroup LiveInsitu Live Insitu
 *
 * The Live Insitu module visualizes and controls a remote simulation.
 *
 * It allows a user to visualize live data, control the visualization
 *  pipeline, and pause a running simulation that was linked with the
 *  Catalyst module.
 */

/**
 * @class   vtkLiveInsituLink
 * @brief   link for live-coprocessing.
 *
 * vtkLiveInsituLink manages the communication link between Insitu and
 * Live visualization servers. vtkLiveInsituLink is created on both
 * ends of the live-insitu channel i.e. in Insitu code (by
 * instantiating vtkLiveInsituLink directly) and in the Live ParaView
 * application (by using a proxy that instantiates the
 * vtkLiveInsituLink).
 * @ingroup LiveInsitu
*/

#ifndef vtkLiveInsituLink_h
#define vtkLiveInsituLink_h

#include "vtkRemotingLiveModule.h" //needed for exports

#include "vtkSMObject.h"
#include "vtkSmartPointer.h" // Needed for Smart pointer
#include "vtkWeakPointer.h"  // Needed for Weak pointer

class vtkMultiProcessController;
class vtkSMSessionProxyManager;
class vtkPVXMLElement;
class vtkPVSessionBase;
class vtkTrivialProducer;
class vtkExtractsDeliveryHelper;

class VTKREMOTINGLIVE_EXPORT vtkLiveInsituLink : public vtkSMObject
{
public:
  static vtkLiveInsituLink* New();
  vtkTypeMacro(vtkLiveInsituLink, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the port number. This is the port on which the root data-server node
   * will open a server-socket to accept connections from VTK InSitu Library.
   */
  vtkSetMacro(InsituPort, int);
  vtkGetMacro(InsituPort, int);
  //@}

  //@{
  /**
   * Set the host name.
   */
  vtkSetStringMacro(Hostname);
  vtkGetStringMacro(Hostname);
  //@}

  //@{
  /**
   * Set/Get the link type i.e. whether the current process is the visualization
   * process or the insitu process.
   */
  enum
  {
    LIVE = 0,
    INSITU = 1
  };
  vtkSetClampMacro(ProcessType, int, LIVE, INSITU);
  vtkGetMacro(ProcessType, int);
  //@}

  //@{
  /**
   * When instantiated on the ParaView visualization server side using a
   * vtkSMProxy, ProxyId is used to identify the proxy corresponding to this
   * instance. That helps us construct notification messages that the
   * visualization server can send to the client.
   */
  vtkSetMacro(ProxyId, unsigned int);
  vtkGetMacro(ProxyId, unsigned int);
  //@}

  //@{
  /**
   * 'SimulationPaused' is set/reset on Paraview Live and sent to Insitu
   * every time step.
   */
  vtkGetMacro(SimulationPaused, int);
  void SetSimulationPaused(int paused);
  //@}

  /**
   * Initializes the link. For in situ this returns true it there is a
   * connection and false otherwise. For live it always returns true.
   */
  bool Initialize() { return this->Initialize(nullptr); }
  bool Initialize(vtkSMSessionProxyManager*);

  // **************************************************************************
  //      *** API to be used from the insitu library ***

  /**
   * Every time Insitu is ready to communicate with ParaView visualization
   * engine call this method. The goal of this call is too get the latest
   * updates from ParaView including changes to state for the co-processing
   * pipeline or changes in what extract the visualization engine is expecting.
   * This method's primary goal is to obtain information from ParaView vis
   * engine. If no active connection to ParaView visualization engine exists,
   * this will make an attempt to connect to ParaView.
   */
  void InsituUpdate(double time, vtkIdType timeStep);

  /**
   * Every time Insitu is ready to push extracts to ParaView visualization
   * engine, call this method. If no active ParaView visualization engine
   * connection exists (or the connection dies), then this method does nothing
   * (besides some bookkeeping).  Otherwise, this will push any extracts
   * requested to the ParaView visualization engine.
   */
  void InsituPostProcess(double time, vtkIdType timeStep);

  //@{
  /**
   * is called on the catalyst side. Insitu stops until the pipeline
   * is edited, an extract is added or removed or the user continues
   * the simulation. Returns != 0 if the visualization side disconnected,
   * 0 otherwise
   */
  int WaitForLiveChange();
  /// Description: Called on INSITU side when LIVE has changed
  void OnLiveChanged();
  //@}

  // **************************************************************************

  // **************************************************************************
  // API to be used from the LIVE side.
  // Register/unregister a producer for an extract.
  void RegisterExtract(
    vtkTrivialProducer* producer, const char* groupname, const char* proxyname, int portnumber);
  void UnRegisterExtract(vtkTrivialProducer* producer);

  void OnInsituUpdate(double time, vtkIdType timeStep);
  void OnInsituPostProcess(double time, vtkIdType timeStep);
  /**
   * Signal a change on the ParaView Live side and transmit it to the Insitu
   * side. This is called when the state or extracts are changed or when
   * the simulation is continued.
   */
  void LiveChanged();
  // **************************************************************************

  enum NotificationTags
  {
    CONNECTED = 1200,
    NEXT_TIMESTEP_AVAILABLE = 1201,
    DISCONNECTED = 1202
  };

  void UpdateInsituXMLState(const char* txt);

  /**
   * This method will remove references to proxy that shouldn't be shared with ParaView
   * Return true if something has been removed
   */
  static bool FilterXMLState(vtkPVXMLElement* xmlState);

  // ***************************************************************
  // Internal methods, public for callbacks.
  void InsituConnect(vtkMultiProcessController* proc0NodesController);

  /**
   * Called to drop the connection between Insitu and ParaView Live.
   */
  void DropLiveInsituConnection();

protected:
  vtkLiveInsituLink();
  ~vtkLiveInsituLink() override;

  enum RMITags
  {
    UPDATE_RMI_TAG = 8800,
    POSTPROCESS_RMI_TAG = 8801,
    INITIALIZE_CONNECTION = 8802,
    DROP_CAT2PV_CONNECTION = 8803,
    // Message from LIVE, sent when simulation is paused,
    // signalling a change.  INSITU wakes up and checks for new
    // simulation state, changed extracts or if it should continue the
    // simulation
    LIVE_CHANGED = 8804
  };

  /**
   * Called by Initialize() to initialize on a ParaView Live process.
   */
  void InitializeLive();

  /**
   * Called by Initialize() to initialize on a Insitu process. Returns
   * true if a connection is made.
   */
  bool InitializeInsitu();

  /**
   * Callback on Visualization process when a simulation connects to it.
   */
  void OnConnectionCreatedEvent();

  /**
   * Callback on Visualization process when a connection dies during
   * vtkNetworkAccessManager::ProcessEvents().
   */
  void OnConnectionClosedEvent(vtkObject*, unsigned long eventid, void* calldata);

  char* Hostname;
  int InsituPort;
  int ProcessType;
  unsigned int ProxyId;

  bool InsituXMLStateChanged;
  bool ExtractsChanged;
  int SimulationPaused;

  char* InsituXMLState;
  vtkWeakPointer<vtkPVSessionBase> LiveSession;
  /**
   * The controller that communicates between the INSITU and the
   * LIVE process 0 nodes.
   */
  vtkSmartPointer<vtkMultiProcessController> Proc0NodesController;
  vtkSmartPointer<vtkExtractsDeliveryHelper> ExtractsDeliveryHelper;

private:
  vtkLiveInsituLink(const vtkLiveInsituLink&) = delete;
  void operator=(const vtkLiveInsituLink&) = delete;

  vtkWeakPointer<vtkSMSessionProxyManager> InsituProxyManager;

  vtkSetStringMacro(URL);
  char* URL;

  vtkSetStringMacro(InsituXMLState);

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
