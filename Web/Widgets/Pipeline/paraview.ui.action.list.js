/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-action-list'
 *
 * @class jQuery.paraview.ui.Pipeline
 */
(function (GLOBAL, $) {

    var TEMPLATE_ITEM = "<li class='clickable hover-higlight pv-side-padding' data-action-id='KEY'>LABEL</li>"

    function getAction(list, id) {
        var count = list.length;
        while(count--) {
            if(list[count].id === id) {
                return list[count];
            }
        }
        return null;
    }

    /**
     * Graphical component use to create a list of clickable action which will trigger
     * an action event on the component on which it is bind.
     *
     * @member jQuery.paraview.ui.Pipeline
     * @method actionList
     * @param {Object} options
     *
     * Usage:
     *
     *      $('.plist-div').actionList({
     *          actions: [
     *              {    id: 'UniqueKey',
     *                label: 'Text on which you click',
     *                value: 'Content that you want to get access but should not be seen in HTML'},
     *              ...  ]
     *      });
     *
     * Event triggered when an item is clicked:
     *       {
     *           type: 'action',
     *           id: action.id,
     *           label: action.label,
     *           value: action.value
     *       }
     */
    $.fn.actionList = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.actionList.defaults, options);

        return this.each(function() {
            var me = $(this).empty().addClass('pvActionList'),
                buffer = ['<ul class="list-unstyled" style="width: 100%;">'],
                count = opts.actions.length;

            // Update DOM
            me.data('actions', opts.actions);
            for(var idx = 0; idx < count; ++idx) {
                buffer.push(TEMPLATE_ITEM.replace(/KEY/g, opts.actions[idx].id).replace(/LABEL/g, opts.actions[idx].label));
            }
            buffer.push('</ul>');
            me[0].innerHTML = buffer.join('');

            me.on('click', function(e){
                var itemId, action;
                if(itemId = ($(e.target).attr('data-action-id'))) {
                    if(action = getAction(me.data('actions'), itemId)) {
                        me.trigger({
                            type: 'action',
                            id: action.id,
                            label: action.label,
                            value: action.value
                        });
                    }
                }
            });
        });
    };

    $.fn.actionList.defaults = {
        actions: []
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-action-list');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));