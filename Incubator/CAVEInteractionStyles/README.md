# CAVE Interactor Styles

This document describes the functionality and naming of the existing C++
interactor style proxies, and gives some best practices for writing new
ones.

## Features and Naming

Many of the built-in C++ interactor styles can be grouped into two main
categories: those that control the entire scene, and those that control
a single source or view property. Interactor styles that control the
entire scene include `Travel` in the name, while interactor styles that
control a single property include `Control` in the name. Note, however,
there are several interactor styles that do not fall into either of these
categories, for example, `Debug`, `Track`, and `Python`.

When interactor styles provide a rotational component to the interaction,
this can happen in one of two ways. Either the scene/object can appear to
orbit around the viewer, which we have termed ego-centric rotation, or else
the scene/object can appear to rotate about the center of the world (turn
on "Center Axes" in ParaView's view properties to see where this point is).
We have termed this latter type of rotation exo-centric.  Built-in C++
interactor styles that provide a rotational component to their interaction
thus include either `Exo` or `Ego` in the name to indicate which type of
rotation is provided.

When an interactor style includes `Grab` in the name, the intent is that it
behaves by following the location and orientation of the controller, as if
you had grabbed the object or scene.  When an interactor style includes
`Split` in the name, it indicates that it provides both translational and
rotational components to the interaction, which are triggered by separate
buttons.

## CAVEInteraction Event Loop

To provide some background and motivation for the best practices section,
below, this section describes how the plugin works. The CAVEInteraction plugin
is multithreaded, when the user clicks the "Start" button in the panel, it
starts the background thread collecting events from the tracking system. In
the foreground thread, this begins the event loop running which consists of
the following:

1. Lock the event queue and dequeue any events that have come in since the
last iteration of the loop.
2. Process each event that was dequeued by passing it to every interactor
style, one at a time. This is where the `Handle...()` methods implemented by
the interactor styles are called.
3. Once the pile of events has been exhausted, call `Update()` once on each
interactor style
4. Perform a `StillRender()`

Typically, many new events will arrive during one iteration of the event
loop, mostly tracker events (or valuator events if those are used). Many of
those tracker events are essentially a replacement of the previous tracker
event associated to the same role, so it doesn't make sense to do a lot
of work that will just be repeated multiple times before the next `Update()`
and `StillRender()` portion of the event loop.

## Interactor Style Best Practices

Based on the event loop description above, it's clear that one best practice
(aimed at performance) is to minimize the work done in `HandleTracker()`.
Thus, all of the built-in C++ interactor styles follow the practice of simply
copying the tracker matrix contained in the event payload (and possibly
doing any work that needs to be done only once after the initial button
press). Then in the `Update()` method, the latest copy of the tracker matrix
is used to do any computations, and finally the `UpdateVTKObjects()` method
is called on the controlled proxy, if necessary. This practice maximizes the
effective fps that can be achieved by the plugin.

Controlling the entire scene can only be accomplished by manipulating the
`ModelTransformMatrix` property of the active `vtkSMRenderViewProxy`. This
proxy can be acquired by calling `GetActiveView()` in Python, or by calling
the `vtkSMVRInteractorStyleProxy::GetActiveViewProxy()` in C++. There is
also an alias (available in both Python and C++) for this behavior in the
`vtkSMVRInteractorStyleProxy` base class: `SetNavigationMatrix()`. This
alias was created to align with the terminology used for the coordinate
system property of ParaView representations, which allows them to be placed
in either `Navigable` (world) or `Fixed` (physical) coordinate systems.

When writing any new interactor style, recall from above that all travel
(control of the entire scene) can only be accomplished by manipulation of
the `ModelTransformMatrix`. Thus, all interactor styles must account for
likely non-identity `ModelTransformMatrix` to make sure they work well
with other interactor styles. When controlling a 3D point or vector, this
means first converting the 3D element into model coordinates, then
performing the desired transformation on the element, then converting back
to world coordinates. When controlling the `ModelTransformMatrix` itself,
this typically means translating the matrix back to the origin, operating
on it, then translating it back, before setting the property back on the
proxy again. All built-in C++ interactor styles behave this way, so it
should always work well to have several of them loaded at any given time.

Notice from the description of the event loop above that nothing prevents
a button press from being the final event in the pile of dequeued events.
If that happens when more than one interactor is being used to control the
same property, it is important not to proceed with `Update()`, or you may
notice distinct jumps in position or orientation of the target property.
To avoid this, all built-in interactor styles use a variable to indicate
whether at least one tracker event has been handled since the button was
pressed, and they avoid doing anything in `Update()` until that has
happened.
