/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiplexerSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMMultiplexerSourceProxy
 * @brief a multiplexer proxy
 *
 * vtkSMMultiplexerSourceProxy is a multiplexer that can pick and act as
 * one of the internal proxies (called multiplexed proxies). Once a choice is
 * made, for all intents and purposes the multiplexer behaves like the chosen
 * proxy.
 *
 * For original motivation and design, refer to paraview/paraview#19936.
 *
 * @section DefineMultiplexer Defining a Multiplexer Proxy
 *
 * A multiplexer proxy can be defined in the ServerManager XML configuration
 * as follows:
 *
 * @code{xml}
 * <ProxyGroup name="filters">
 *  <MultiplexerSourceProxy name="ExtractBlock" label="Extract Block">
 *    <Documentation long_help="Extract blocks from composite datasets">
 *      This filter extract blocks.
 *    </Documentation>
 *
 *    <InputProperty name="Input">
 *      <Documentation>
 *        Specify the input to this filter.
 *      </Documentation>
 *      <MultiplexerInputDomain name="input" />
 *    </InputProperty>
 *  </MultiplexerSourceProxy>
 * @endcode
 *
 * It's similar to other proxy definitions, except a few key differences:
 *
 * * A `classname` attribute is generally not specified. The implementation
 *   internally creates a `vtkPassThroughFilter`.
 * * Only properties that help with the selection of the multiplexed proxy are
 *   defined.
 * * `InputProperty` must have `MultiplexerInputDomain` as the domain. This is
 *   essential to ensure that `IsInDomain` checks take into consideration input
 *   domains for all available multiplexed proxies.
 *
 * Defining proxies that should be included as the available
 * multiplexed proxies is done by adding the `<MultiplexerSourceProxy />` hint
 * as follows:
 *
 * @code{xml}
 * <SourceProxy class="vtkExtractBlock"
 *               name="ExtractBlockUsingIds">
 *    <Documentation long_help="This filter extracts a range of blocks from a multiblock dataset."
 *                   short_help="Extract block.">This filter extracts a range
 *                   of groups from a multiblock dataset</Documentation>
 *    <InputProperty name="Input" command="SetInputConnection">
 *      <DataTypeDomain name="input_type">
 *        <DataType value="vtkMultiBlockDataSet" />
 *      </DataTypeDomain>
 *    </InputProperty>
 *    ...
 *    <Hints>
 *      <MultiplexerSourceProxy proxygroup="filters" proxyname="ExtractBlock" >
 *        <LinkProperties>
 *          <Property name="Input" with_property="Input" />
 *        </LinkProperties>
 *      </MultiplexerSourceProxy>
 *    </Hints>
 * </SourceProxy>
 * @endcode
 *
 * The `proxygroup` and `proxyname` on the `MultiplexerSourceProxy` hint element
 * refer to the group and name of the multiplexed proxy. If the proxy can be
 * part of multiple multiplexers, then multiple `MultiplexerSourceProxy`
 * elements can be added to the hints.
 *
 * When a vtkSMMultiplexerSourceProxy is instantiated, it looks for all available
 * proxy definitions that have a matching `MultiplexerSourceProxy` hint. The
 * optional `LinkProperties` nested element defines how the properties on this
 * proxy map to properties on the multiplexer proxy. If not specified, they are
 * matched by name.
 *
 * In `vtkSMMultiplexerSourceProxy::CreateVTKObjects`, based on the values set
 * on the multiplexer's properties, one (or more) of the available multiplexed
 * proxies are chosen and rest are discarded. Selecting a proxy implies that
 * it will be added as subproxies, and its properties (those that are not linked
 * with properties on the multiplexer) exposed from the multiplexer so that they
 * are available for users to check and set.
 *
 * The check to see if one of the available multiplexed proxy can be chosen is
 * as follows: for each property on the multiplexed proxy that is linked with a
 * property on the multiplexer, we copy the current value from the multiplexer
 * property and then check if the value is in domain (vtkSMProperty::IsInDomains);
 * if it succeeds for all linked properties, then that multiplexed property is
 * chosen, else it's not and is discarded.
 *
 * Current implementation is intended for at most 1 proxy to be chosen.
 * If multiple proxies succeed at this test, current implementation
 * only selects the first one.
 *
 * @section Caveats Caveats and TODOs
 *
 * While the implementation has some initial plumbing to support multiple
 * multiplexed proxies, currently we only support exactly one. In future, the
 * plan is to add a user-settable property to choose between multiple proxies if
 * multiple of them match.
 *
 * When multiple proxies are chosen, we need to add a mechanism to share
 * properties with same names between the chosen proxies since we cannot expose
 * a property with same name multiple times.
 *
 * @section PythonSupport Python Support
 *
 * vtkSMMultiplexerSourceProxy currently suffers from lack of Python support.
 * ParaView's Python infrastructure is not capable of supporting different
 * instances of the same proxy with different set of properties. Consequently,
 * do not use this unless you don't need Python support.
 * See paraview/paraview#20187.
 *
 * @sa vtkSIMultiplexerSourceProxy, vtkSMMultiplexerInputDomain.
 */

#ifndef vtkSMMultiplexerSourceProxy_h
#define vtkSMMultiplexerSourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMSourceProxy.h"
#include <memory> // for std::unique_ptr

class vtkSMMultiplexerInputDomain;
class vtkSMInputProperty;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMMultiplexerSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMMultiplexerSourceProxy* New();
  vtkTypeMacro(vtkSMMultiplexerSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Overridden to add/load meta-data about the chosen subproxy to ensure that when
   * the state is loaded, correct proxy is chosen.
   */
  using Superclass::SaveXMLState;
  vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter) override;
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;
  //@}

protected:
  vtkSMMultiplexerSourceProxy();
  ~vtkSMMultiplexerSourceProxy();

  void CreateVTKObjects() override;

  int CreateSubProxiesAndProperties(
    vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  friend class vtkSMMultiplexerInputDomain;
  int IsInDomain(vtkSMInputProperty* property);

  void Select(vtkSMProxy*);

private:
  vtkSMMultiplexerSourceProxy(const vtkSMMultiplexerSourceProxy&) = delete;
  void operator=(const vtkSMMultiplexerSourceProxy&) = delete;

  void CurateApplicableProxies();
  void SetupSubProxies();

  void SetupSubProxy(vtkSMProxy* proxy);

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
