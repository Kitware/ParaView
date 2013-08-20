/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add control button to ParaViewWeb usage.
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
                session.call("vtk:exit");
                session.close();
                setTimeout("window.close()", 100);
            }

            me.click(disconnect);
        });
    };

}(window, jQuery));