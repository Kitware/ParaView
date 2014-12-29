Property Hints    {#PropertyHints}
==============

This page documents *Property Hints*, which are xml tags accepted under *Hints*
for a *Property* element in Server-Manager configuration XMLs.

[TOC]

NoDefault
----------
Skip setting run-time defaults during initalization.

Any property with a domain that is dynamic i.e. depends on run-time
values such as data information or the information-only properties, often
resets the property value during initialization of the newly created proxy
(vtkSMParaViewPipelineController::PostInitializeProxy()). This, however, may
be undesirable and one may want the property to simply use the XML default.
In that case, use this property hint as follows.

    <IntVectorProperty ...>
      ...
      <Hints>
        <NoDefault />
      </Hints>
    </IntVectorProperty>

PlaceholderText
---------------
Put a PlaceHolder text in the Text entry widget.

For StringVectorProperty elements, one can suggest a place-holder text to use
in the UI using the `<PlaceholderText/>` tag.

    <StringVectorProperty...>
      ...
      <Hints>
        <PlaceholderText>Enter label</PlaceholderText>
      </Hints>
    </StringVectorProperty>

PropertyLink
------------
Link properties to existing proxies at initialization time.

PropertyLink hints is typically used to link a non-UI property to a
property on some other proxy known to exist when this proxy is created.
Consider this, we have a (settings, RenderViewSettings) proxy that created
when a new session is initialized in vtkSMParaViewPipelineController::InitializeSession().
Now, when a new (views, RenderView) proxy is created, we'd like a few properties
from the RenderView proxy to be linked to session wide instance of RenderViewSettings proxy.
That way, when the properties on RenderViewSettings proxy are changed, those on all
the RenderView proxy instances also change. We use `<PropertyLink/>` hint for such a case.
For example, consider the following:

    <IntVectorProperty command="SetUseOffscreenRenderingForScreenshots"
                        name="UseOffscreenRenderingForScreenshots"
                        number_of_elements="1"
                        default_values="0"
                        panel_visibility="never">
      <Hints>
        <PropertyLink group="settings"
                      proxy="RenderViewSettings"
                      property="UseOffscreenRenderingForScreenshots"/>
      </Hints>
    </IntVectorProperty>

Here, we want the UseOffscreenRenderingForScreenshots property linked with the corresponding
property on the RenderViewSettings proxy. When this tag is encountered by
vtkSMParaViewPipelineController::PostInitializeProxy(), it will try to locate an
existing proxy of registered as (settings, RenderViewSettings).
vtkSMParaViewPipelineController::InitializeSession() ensures that all proxies encountered in
\c settings group are created and registered using the same group. Thus, when the
vtkSMParaViewPipelineController::PostInitializeProxy() looks for (settings, RenderViewSettings)
proxy, it will find one and then be able to setup a property link to ensure that the property
on the RenderView proxy is kept in sync with the property on the RenderViewSettings proxy.

SelectionInput
--------------
Mark a input as a one that accepts a vtkSelection.

Sometimes an input to an algorithm is a vtkSelection, not any other dataset.
In ParaView, vtkSelection inputs are treated specially to allow the user
to copy active selection from the application. Also, it keeps the application
from prompting the user to pick the second input when the filter is created.
To indicate to the UI that this input is a selection input, use the
`<SelectionInput/>` hint.

    <InputProperty ...>
      ...
      <Hints>
        <SelectionInput />
      </Hints>
    </InputProperty>
