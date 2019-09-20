FlipBook plugin for ParaView
============================

by M.Migliore & J.Pouderoux, Kitware SAS 2018-2019

This work was supported by Total SA.

Description
-----------

This plugin creates a new toolbar which allows a flip book type
rendering mode: when the flip book mode is enabled every visible representations
of an active Render view will be flipped at each flip iteration step.
This mode allows to use persistence of vision to perceive differences between
several representations. Note that the SPACE key can be used to flip visibility manually.

How to test
-----------

* Build and load the plugin in ParaView, a new toolbar is displayed.
* Add at least two objects in the pipeline browser.
* Toggle _flip book_ button and press _SPACE_ to switch visibility.
* Automatic switch can be configured using the second button (the spinbox configures the time delay between a switch and the next one).
