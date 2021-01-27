set(interfaces)
set(sources
  pqMyRenderView.cxx
  pqMyRenderView.h)

paraview_plugin_add_proxy(
  NAME MyProxies
  INTERFACES proxy_interfaces
  SOURCES proxy_sources
  PROXY_TYPE pqMyRenderView
    XML_GROUP views
    XML_NAME MyRenderView)

list(APPEND interfaces
  ${proxy_interfaces})
list(APPEND sources
  ${proxy_sources})

paraview_add_plugin(AddPQProxy
  VERSION "1.0"
  XML_DOCUMENTATION OFF
  SERVER_MANAGER_XML AddPQProxy.xml
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

target_link_libraries(AddPQProxy
    PRIVATE
      ParaView::RemotingViews)
