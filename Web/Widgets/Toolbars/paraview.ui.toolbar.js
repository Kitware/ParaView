/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add control button to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-toolbar'
 *
 * @class jQuery.paraview.ui.toolbar
 */
(function (GLOBAL, $) {

    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.toolbar
     * @method disconnectButton
     * @param {pv.Session} session
     * ParaViewWeb session object.
     *
     * Usage:
     *      $('.disconnect').disconnectButton(session);
     */
    $.fn.disconnectButton = function(session) {
        // Handle data with default values
        return this.each(function() {
            var me = $(this);

            function disconnect() {
                session.call("application.exit");
                session.close();
                setTimeout("window.close()", 100);
            }

            me.click(disconnect);
        });
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-toolbar-connect');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
