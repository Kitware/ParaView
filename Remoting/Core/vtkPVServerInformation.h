/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVServerInformation
 * @brief   Gets features of the server.
 *
 * This objects is used by the client to get the features
 * supported by the server.
 * At the moment, server information is only on the root.
 */

#ifndef vtkPVServerInformation_h
#define vtkPVServerInformation_h

#include <string>

#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkClientServerStream;

class VTKREMOTINGCORE_EXPORT vtkPVServerInformation : public vtkPVInformation
{
public:
  static vtkPVServerInformation* New();
  vtkTypeMacro(vtkPVServerInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * This flag indicates whether the server can render remotely.
   * If it is off, all rendering has to be on the client.
   * This is only off when the user starts the server with
   * the --disable-composite command line option.
   */
  vtkSetMacro(RemoteRendering, int);
  vtkGetMacro(RemoteRendering, int);
  //@}

  /**
   * Returns true if server is in tile-display mode.
   */
  vtkGetMacro(IsInTileDisplay, bool);

  /**
   * Returns true if server is in CAVE mode.
   */
  vtkGetMacro(IsInCave, bool);

  void DeepCopy(vtkPVServerInformation* info);

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Returns 1 if IceT is available.
   */
  vtkSetMacro(UseIceT, int);
  vtkGetMacro(UseIceT, int);
  //@}

  //@{
  /**
   * if the server supports compressing images via NVPipe
   */
  //@}
  vtkSetMacro(NVPipeSupport, bool);
  vtkGetMacro(NVPipeSupport, bool);

  //@{
  /**
   * Get/Set the time after which the server timesout.
   */
  vtkSetMacro(Timeout, int);
  vtkGetMacro(Timeout, int);
  //@}

  //@{
  /**
   * Get the timeout command used by the server to retrieve remaining time.
   */
  const std::string& GetTimeoutCommand() const { return this->TimeoutCommand; }
  //@}

  /**
   * When in tile display mode, returns the tile dimensions.
   */
  vtkGetVector2Macro(TileDimensions, int);

  //@{
  /**
   * Get the number of processes.
   */
  vtkGetMacro(NumberOfProcesses, int);
  //@}

  /**
   * Return whether MPI is initialized or not.
   */
  virtual bool IsMPIInitialized() const;

  //@{
  /**
   * Return true if the server allow server client to connect to itself
   */
  vtkGetMacro(MultiClientsEnable, int);
  //@}

  //@{
  /**
   * Get the id that correspond to the current client
   */
  vtkGetMacro(ClientId, int);
  //@}

  //@{
  /**
   * Set/Get vtkIdType size, which can be 32 or 64
   */
  vtkSetMacro(IdTypeSize, int);
  vtkGetMacro(IdTypeSize, int);
  //@}

  //@{
  /**
   * Get the SMP Tools backend name of the server.
   */
  vtkGetMacro(SMPBackendName, std::string);
  //@}

  //@{
  /**
   * Get the max number of threads of the server.
   */
  vtkGetMacro(SMPMaxNumberOfThreads, int);
  //@}

  //@{
  /**
   * Return true if VTK-m accelerated filters override is enabled in this build.
   */
  vtkGetMacro(AcceleratedFiltersOverrideAvailable, int);
  //@}

protected:
  vtkPVServerInformation();
  ~vtkPVServerInformation() override;

  int NumberOfProcesses;
  bool MPIInitialized;
  int OGVSupport;
  int AVISupport;
  bool NVPipeSupport;
  int RemoteRendering;
  int Timeout;
  std::string TimeoutCommand;
  int UseIceT;
  int MultiClientsEnable;
  int ClientId;
  int IdTypeSize;
  bool IsInTileDisplay;
  bool IsInCave;
  int TileDimensions[2];
  std::string SMPBackendName;
  int SMPMaxNumberOfThreads;
  bool AcceleratedFiltersOverrideAvailable;

private:
  vtkPVServerInformation(const vtkPVServerInformation&) = delete;
  void operator=(const vtkPVServerInformation&) = delete;
};

#endif
