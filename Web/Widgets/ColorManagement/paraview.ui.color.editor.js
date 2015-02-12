/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extends jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-color-editor'
 *
 * @class jQuery.paraview.ui.ColorMap
 */
(function (GLOBAL, $) {

    var TEMPLATE_COLOR_EDIT =
    '<div class="row"> <div class="col-xs-12">' +
    '<div class="color-editor-button-container">' +
    '<div class="left-button-panel">' +
    '<div class="color-editor-add-new vtk-icon-plus-circled color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Add New Point"></div>' +
    '<div class="color-editor-toggle-list vtk-icon-list-1 color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Toggle Control Point List"></div>' +
    '<div class="color-editor-rescale-points vtk-icon-resize-vertical-1 color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Evenly Space Control Points"></div>' +
    '<div class="color-editor-toggle-categorical vtk-icon-font color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Interpret Values as Categories"></div>' +
    '<div class="color-editor-toggle-edit-mode vtk-icon-tags color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Click to edit annotations"></div>' +
    '</div>' +
    '<div class="right-button-panel">' +
    '<div class="color-editor-toggle-interactive vtk-icon-lock color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Toggle Interactive Mode"></div>' +
    '<div class="color-editor-apply vtk-icon-ok color-editor-button" data-toggle="tooltip" data-placement="bottom" title="Apply"></div>' +
    '</div></div></div></div>' +
    '<div class="row" style="display: none;"> <div class="col-xs-12">' +
    '<div class="color-editor-overflow-container">' +
    '<div class="color-editor-canvas-container">' +
    '<canvas class="color-editor-canvas"></canvas>' +
    '</div></div></div></div>' +
    '<div class="color-editor-list-container">RGBVALUES</div>',
    TEMPLATE_RGB_VALUE = '<div class="row property" data-index="LISTINDEX">' +
    '<div class="col-xs-12" style="position: relative;">' +
    '<div class="float-left" style="width: 25px; height: 19px;"><div class="color-select-button float-left" data-picker="hidden" style="background-color: BGCOLOR;"></div></div>' +
    '<div style="position: absolute; right: 35px; left: 41px;"><input class="form-control pv-form-height control-point-value-edit" type="text" value="VALUE" EDITTOOLTIP/></div>' +
    '<div class="text-right" style="float: right; height: 19px; width: 25px;"><span class="delete-control-point-btn vtk-icon-trash DELETE"></span></div>' +
    '</div></div>',
    TEMPLATE_SWATCHES_CONTAINER = '<div class="row property" data-forindex="">' +
    '<div class="col-xs-12"><div class="row"><div class="col-xs-8" style="padding-right: 0;"><img class="color-swatch-table" src=""></img></div>' +
    '<div class="rgb-edit-fields col-xs-4">' +
    '<div><label class="control-label">R</label><input class="pv-form-height rgb-input-field red" type="text" value="" data-toggle="tooltip" data-placement="bottom" title="range: [0, 255]"/></div>' +
    '<div><label class="control-label">G</label><input class="pv-form-height rgb-input-field green" type="text" value="" data-toggle="tooltip" data-placement="bottom" title="range: [0, 255]"/></div>' +
    '<div><label class="control-label">B</label><input class="pv-form-height rgb-input-field blue" type="text" value="" data-toggle="tooltip" data-placement="bottom" title="range: [0, 255]"/></div>' +
    '</div></div></div></div>',

    clearColor = "rgba(0, 0, 0, 0)",
    bgColor = "rgba(255, 255, 255, 255)",
    outlineWidth = "1",
    outlineColor = "rgba(85, 85, 85, 255)";


    /*********************************************************************
     * Clear the canvas and draw an outline around the drawing area
     ********************************************************************/
    function clearCanvas(canvasElt, offset, size) {
        var ctx = canvasElt[0].getContext("2d"),
        canWidth = canvasElt.attr('width'),
        canHeight = canvasElt.attr('height');

        ctx.clearRect(0, 0, canWidth, canHeight);
        ctx.lineJoin = "round";
        ctx.fillStyle = clearColor;
        ctx.fillRect(0, 0, canWidth, canHeight);

        ctx.fillStyle = bgColor;
        ctx.fillRect(offset.x, offset.y, size.w, size.h);

        ctx.lineWidth = outlineWidth;
        ctx.strokeStyle = outlineColor;
        ctx.beginPath();
        ctx.strokeRect(offset.x, offset.y, size.w, size.h);

        ctx.stroke();
    }

    /*********************************************************************
     * Get an object containing the relative x,y coordinates of the click
     * point, given the click event and the container in which the click
     * occurred.
     ********************************************************************/
    function getRelativeClickCoords(event, container) {
        var x, y;

        offset = $(container).offset();
        x = event.clientX + document.body.scrollLeft + document.documentElement.scrollLeft - Math.floor(offset.left);
        y = event.clientY + document.body.scrollTop + document.documentElement.scrollTop - Math.floor(offset.top) + 1;

        return { 'x': x, 'y': y };
    }

    /*********************************************************************
     * Perform affine transformation
     ********************************************************************/
    function affine(inMin, value, inMax, outMin, outMax) {
        return (((value - inMin) / (inMax - inMin)) * (outMax - outMin)) + outMin;
    }

    /*********************************************************************
     * Given the control points list, generate the html string containing
     * the edit fields.
     ********************************************************************/
    function inflateRgbEditList(rgbInfo, editMode) {
        var colorMode = rgbInfo['mode'],
            colorList = rgbInfo[colorMode]['colors'],
            scalars = rgbInfo[colorMode]['scalars'],
            annotations = rgbInfo[colorMode]['annotations'],
            showAnnotations = (annotations && editMode === 'annotations' ? true : false),
            numControlPoints = scalars.length,
            cpBuffer = [],
            canDelete = numControlPoints > 2 ? 'can-delete-cp' : 'cannot-delete-cp';

        // Inflate the list of control points ui from the control points
        for (var i = 0; i < numControlPoints; i+=1) {
            var cpx = (showAnnotations ? annotations[i] : scalars[i]),
                cpr = colorList[i][0] * 255,
                cpg = colorList[i][1] * 255,
                cpb = colorList[i][2] * 255,
                ttText = '';
            if (colorMode === 'categorical') {
                var note = annotations[i];
                ttText = 'data-toggle="tooltip" data-placement="bottom" title=' +
                         (showAnnotations ? '"Value: ' + scalars[i] + '"' : ((note && note !== '') ?  '"Text: ' + note + '"' : '"No Annotation"'));
            }
            cpBuffer.push(TEMPLATE_RGB_VALUE
                .replace(/VALUE/, cpx)
                .replace(/BGCOLOR/, "rgb(" + cpr.toFixed(0) +"," + cpg.toFixed(0) + "," + cpb.toFixed(0) + ")")
                .replace(/LISTINDEX/, i)
                .replace(/DELETE/, canDelete)
                .replace(/EDITTOOLTIP/, ttText));
        }

        return cpBuffer.join('');
    }

    /*********************************************************************
     * Sorts the control points in the argument array
     ********************************************************************/
    function sortControlPoints(rgbInfo) {
        var rMap = {},
            colorMode = rgbInfo['mode'],
            scalars = rgbInfo[colorMode]['scalars'],
            colors = rgbInfo[colorMode]['colors'],
            annotations = rgbInfo[colorMode]['annotations'],
            numCps = scalars.length;

        // First, remember which rgb colors were associated with each value
        for (var i = 0; i < numCps; i+=1) {
            rMap[scalars[i]] = {
                'a': (annotations && annotations[i] ? annotations[i] : null),
                'r': colors[i][0],
                'g': colors[i][1],
                'b': colors[i][2]
            };
        }

        // Now use the built-in sort function to sort the scalar values
        scalars.sort(function(a,b) { return a-b; });

        // Now rebuild the colors and annotations lists in the proper order
        for (var j = 0; j < numCps; j+=1) {
            colors[j][0] = rMap[scalars[j]].r;
            colors[j][1] = rMap[scalars[j]].g;
            colors[j][2] = rMap[scalars[j]].b;
            if (rMap[scalars[j]].a !== null) {
                annotations[j] = rMap[scalars[j]].a;
            }
        }

        return rgbInfo;
    }

    /*********************************************************************
     * Clamp a value between 0 and 255
     ********************************************************************/
    function rgbClamp(val) {
        if (val < 0) {
            return 0;
        } else if (val > 255) {
            return 255;
        }
        return val;
    }


    // ------------------------------------------------------------------------
    /**
     * Graphical component used to create a widget for editing the color map.
     *
     * @member jQuery.paraview.ui.ColorMap
     * @method colorEditor
     *
     * Usage:
     *
     *      $('.color-editor').colorEditor( ... );
     *
     */
    $.fn.colorEditor = function(options) {
        // Handle data with default values
        var opts = $.extend({}, $.fn.colorEditor.defaults, options);

        // Widget creator function
        return this.each(function() {
            var me = $(this).empty().addClass('pv-color-editor'),
            rgbInfo = $.extend(true, {}, opts.rgbInfo),
            topMargin = opts.topMargin,
            rightMargin = opts.rightMargin,
            bottomMargin = opts.bottomMargin,
            leftMargin = opts.leftMargin,
            interactiveMode = opts.interactiveMode,
            editMode = 'scalars',
            offset = {},
            size = {},
            canvas = document.createElement('canvas'),
            context = canvas.getContext('2d'),
            swatchesWidth = 0,
            swatchesHeight = 0,
            myEvents = ['control-point-update-color', 'control-point-edited', 'new-rgb-points-received'],
            widgetKey = opts.widgetKey,
            widgetData = $.extend(true, {}, opts.widgetData);

            /*
             * Internal convenience function to handle change in interactive mode
             */
            function updateInteractiveMode(element) {
                if (interactiveMode === true) {
                    $('.color-editor-apply', me).css('opacity', 0.3);
                    element.removeClass('vtk-icon-lock-open').addClass('vtk-icon-lock');
                } else {
                    $('.color-editor-apply', me).css('opacity', 1.0);
                    element.removeClass('vtk-icon-lock').addClass('vtk-icon-lock-open');
                }
            }

            /*
             * Internal convenience function to handle change categorical vs. continuous
             */
            function updateCategoricalMode(element) {
                if (rgbInfo.mode === 'categorical') {
                    //element.removeClass('vtk-icon-cancel-squared').addClass('vtk-icon-ok-squared');
                    $('.color-editor-toggle-edit-mode', me).css('opacity', 1.0);
                    element.addClass('icon-toggled-on');
                } else {
                    //element.removeClass('vtk-icon-ok-squared').addClass('vtk-icon-cancel-squared');
                    $('.color-editor-toggle-edit-mode', me).css('opacity', 0.3);
                    element.removeClass('icon-toggled-on');
                }
            }

            /*
             * Convenience function to change the scalar edit fields to allow editing
             * annotations if we are in 'annotations' edit mode.  Also updates the
             * tooltips to show either the scalars or the annotations on the edit fields,
             * whichever is not currently being displayed in the field.
             */
            function updateValueFieldsForEditMode() {
                if (rgbInfo.mode !== 'categorical') {
                    // we really should not have arrived here in this case
                    return;
                }
                var valueList = rgbInfo['categorical'].scalars,
                    tooltipsList = rgbInfo['categorical'].annotations,
                    tooltipPrefix = 'Text: ';
                if (editMode === 'annotations') {
                    valueList = rgbInfo['categorical'].annotations;
                    tooltipsList = rgbInfo['categorical'].scalars;
                    tooltipPrefix = 'Value: ';
                }
                for (var i = 0; i < valueList.length; i += 1) {
                    var ithValue = (valueList[i] === undefined ? "No Value" : valueList[i]),
                        ithTooltip = (tooltipsList[i] !== undefined && tooltipsList[i] !== "" ? tooltipPrefix + tooltipsList[i] : "No Annotation"),
                        ithEditField = $('div[data-index=' + i + '] > div > div > input');
                    ithEditField.val(ithValue);
                    fixTooltipTitle(ithEditField, ithTooltip);
                }
            }

            /*
             * Dynamically update the tooltip content on an element
             */
            function fixTooltipTitle(element, newTitle) {
                element.tooltip('hide')
                       .attr('title', newTitle)
                       .tooltip('fixTitle');
            }

            /*
             * Update icon button state and button tooltip appropriately for current edit mode
             */
            function updateEditMode(element) {
                if (editMode === 'scalars') {
                    // element.removeClass('vtk-icon-comment').addClass('vtk-icon-list-numbered');
                    element.removeClass('icon-toggled-on');
                    fixTooltipTitle(element, 'Click to edit annotations');
                } else {  // case: 'annotations'
                    //element.removeClass('vtk-icon-list-numbered').addClass('vtk-icon-comment');
                    element.addClass('icon-toggled-on');
                    fixTooltipTitle(element, 'Click to edit scalars');
                }

                updateValueFieldsForEditMode();
            }

            /*
             * Update the application data object and store it
             */
            function storeWidgetSettings(keyvals) {
                for (var key in keyvals) {
                    if (keyvals.hasOwnProperty(key)) {
                        widgetData[key] = keyvals[key];
                    }
                }
                me.trigger({
                    type: 'store-widget-settings',
                    widgetKey: widgetKey,
                    widgetData: widgetData
                });
            }

            /*
             * Convenience function to send event up to outside listeners
             */
            function fireNewRgbPoints() {
                //if (rgbInfo.mode === 'categorical') {
                //    // Work-around because paraview will not show scalar color bar when interpreting
                //    // values as categories unless at least one annotation string is not empty
                //    var allEmpty = true;
                //
                //    for (var i = 0; i < rgbInfo.annotations.length; i+=1) {
                //        if (rgbInfo.annotations[i] !== '') {
                //            allEmpty = false;
                //            break;
                //        }
                //    }
                //
                //    if (allEmpty === true) {
                //        rgbInfo.annotations[0] = " ";
                //    }
                //}

                me.trigger({
                    type: 'color-editor-cp-update',
                    rgbInfo: rgbInfo
                });
            }

            /*
             * Clean up and rebuild
             */
            function rebuildWidget() {
                $('.tooltip').remove();
                for (var evtIdx = 0; evtIdx < myEvents.length; evtIdx+=1) {
                    me.unbind(myEvents[evtIdx]);
                }
                me.empty();
                buildUi();
            }

            // We want to rebuild the whole ui any time the control values change,
            // or a new control point is inserted, or a control point is deleted.
            function buildUi() {
                var swatchesContainer = $(TEMPLATE_SWATCHES_CONTAINER),
                    html = TEMPLATE_COLOR_EDIT;

                // This canvas is used to be able to read the color at the
                // swatches image click point
                $(canvas).attr('width', 1);
                $(canvas).attr('height', 1);

                // When the swatches image loads, we need to get its native size
                // so when we click on it after it has been scaled by css, we know
                // how to scale the click coords
                var swatchImage = $('.color-swatch-table', swatchesContainer);
                swatchImage.unbind().on('load', function(imgLoadEvt) {
                    swatchesWidth = imgLoadEvt.target.width;
                    swatchesHeight = imgLoadEvt.target.height;
                });
                swatchImage.attr('src', '../../lib/img/defaultSwatches.png');

                // Generate the html containing the edit controls for the rgb points
                html = html.replace(/RGBVALUES/, inflateRgbEditList(rgbInfo, editMode));
                me[0].innerHTML = html;

                // Initialize the color canvas where we will draw the current LUT
                var colorCanvas = $('.color-editor-canvas', me);

                // Set canvas width and height according to its container
                var canvasHeight = colorCanvas.parent().height();
                var canvasWidth = colorCanvas.parent().width();
                colorCanvas.attr('height', canvasHeight);
                colorCanvas.attr('width', canvasWidth);

                // These margins are used to leave some space inside the canvas
                // that is not part of the drawing area.
                if (opts.hasOwnProperty('margin') === true) {
                    topMargin = opts.margin;
                    rightMargin = opts.margin;
                    bottomMargin = opts.margin;
                    leftMargin = opts.margin;
                }

                // Set up the drawing area for the canvases
                offset = {'x': leftMargin, 'y': topMargin};
                size = {'w': (canvasWidth - (leftMargin + rightMargin)), 'h': (canvasHeight - (topMargin + bottomMargin))};

                // Handle internal event generated when user clicks a swatch to change
                // color of current control point
                me.bind('control-point-update-color', function(event) {
                    var colorMode = rgbInfo.mode,
                        colorDiv = $('.color-select-button', $('[data-index=' + event.index + ']'));
                    colorDiv.css('background-color', event.newColorStr);
                    var r = event.newColorArr[0] / 255;
                    var g = event.newColorArr[1] / 255;
                    var b = event.newColorArr[2] / 255;
                    rgbInfo[colorMode].colors[event.index] = [r, g, b];
                    if (interactiveMode === true) {
                        fireNewRgbPoints();
                    }
                });

                // Handle internal event indicating the value of one of the control points changed
                me.bind('control-point-edited', function(event) {
                    // Sort the control points just in case
                    sortControlPoints(rgbInfo);

                    // Trigger the server-side recoloring
                    if (interactiveMode === true) {
                        fireNewRgbPoints();
                    }

                    // Now rebuild the edit list ui
                    rebuildWidget();
                });

                me.bind('new-rgb-points-received', function(event) {
                    rgbInfo = event.rgbpoints;
                    rebuildWidget();
                });

                // show or hide the list of control point edit fields
                $('.color-editor-toggle-list', me).click(function(evt) {
                    var listContainer = $('.color-editor-list-container', me);
                    if (listContainer.is(':visible')) {
                        listContainer.hide();
                    } else {
                        listContainer.show();
                    }
                });

                // Handle click on the button to add new control point to the list
                $('.color-editor-add-new', me).click(function(evt) {
                    var colorMode = rgbInfo.mode,
                        scalars = rgbInfo[colorMode].scalars,
                        newValue = scalars.length,
                        newColor = [newValue, newValue, newValue];
                    if (scalars.length >= 2) {
                        newValue = scalars[0] + (0.5 * (scalars[1] - scalars[0]));
                        newColor = [1.0, 1.0, 1.0];
                    }
                    scalars.push(newValue);
                    rgbInfo[colorMode].colors.push(newColor);
                    if (colorMode === 'categorical') {
                        rgbInfo[colorMode].annotations.push('');
                    }

                    me.trigger({
                        type: 'control-point-edited'
                    });
                });

                // Rescale the points so they're evenly spaced between the current min and max
                $('.color-editor-rescale-points', me).click(function(evt) {
                    var colorMode = rgbInfo.mode,
                        scalars = rgbInfo[colorMode].scalars,
                        min = scalars[0],
                        numCps = scalars.length,
                        max = scalars[numCps - 1];

                    for (var i = 0; i < numCps; i+=1) {
                        scalars[i] = affine(0, i, numCps-1, min, max);
                    }

                    me.trigger({
                        type: 'control-point-edited'
                    });
                });

                // Handle changing back and forth between 'continuous' and 'categorical' colors mode
                $('.color-editor-toggle-categorical', me).click(function(evt) {
                    var elt = $(this);

                    if (rgbInfo.mode === 'categorical') {
                        rgbInfo.mode = 'continuous';
                    } else {
                        rgbInfo.mode = 'categorical';
                    }

                    elt.toggleClass('icon-toggled-on');

                    // updateCategoricalMode(elt);
                    if (interactiveMode === true) {
                        fireNewRgbPoints();
                    }

                    rebuildWidget();
                });

                // Handle click to change between editing scalars and editing annotations.
                $('.color-editor-toggle-edit-mode', me).click(function(evt) {
                    if (rgbInfo.mode === 'continuous') {
                        return;
                    }
                    editMode = (editMode === 'scalars' ? 'annotations' : 'scalars');
                    updateEditMode($(this));
                });

                // Toggle whether or not rgb points updates are sent to server immediately
                $('.color-editor-toggle-interactive', me).click(function(evt) {
                    interactiveMode = !interactiveMode;
                    updateInteractiveMode($(this));
                    storeWidgetSettings({ 'interactiveMode': interactiveMode });
                });

                // Directly send current rbg points to the server
                $('.color-editor-apply', me).click(function(evt) {
                    fireNewRgbPoints();
                });

                // Handle when a trash can icon is clicked to delete a control point
                $('.delete-control-point-btn.can-delete-cp', me).click(function(evt) {
                    var colorMode = rgbInfo.mode,
                        idx = $(this).parent().parent().parent().attr('data-index');
                    rgbInfo[colorMode].colors.splice(idx, 1);
                    rgbInfo[colorMode].scalars.splice(idx, 1);
                    if (colorMode === 'categorical') {
                        rgbInfo[colorMode].annotations.splice(idx, 1);
                    }
                    me.trigger({
                        type: 'control-point-edited'
                    });
                });

                // Handle typing in the value field for a control point
                $('.control-point-value-edit', me).on('blur keyup', function(evt) {
                    if (evt.type === 'blur' || evt.keyCode == '13') {
                        var colorMode = rgbInfo.mode,
                            field = $(this),
                            greatGrandParent = field.parent().parent().parent(),
                            idx = greatGrandParent.attr('data-index');
                        if (colorMode === 'continuous' || editMode === 'scalars') {
                            rgbInfo[colorMode].scalars[idx] = parseFloat(field.val());
                        } else {   // case: 'annotations'
                            rgbInfo[colorMode].annotations[idx] = field.val();
                        }
                        me.trigger({
                            type: 'control-point-edited'
                        });
                    }
                });

                // When we click on the color patch to the left of the value, we want to
                // insert a row which will allow us to choose or edit the color associated
                // with the value.
                $('.color-select-button', me).click(function(evt) {
                    var btn = $(this),
                        colorMode = rgbInfo.mode,
                        colors = rgbInfo[colorMode].colors,
                        greatGrandParent = btn.parent().parent().parent(),
                        idx = greatGrandParent.attr('data-index'),
                        red = (colors[idx][0] * 255).toFixed(0),
                        green = (colors[idx][1] * 255).toFixed(0),
                        blue = (colors[idx][2] * 255).toFixed(0);

                    // Either show or hide the widgets for picking a color
                    if (btn.attr('data-picker') === 'hidden') {           // The color-picker is not there, add it
                        // First remove the swatches container if it is already in use somewhere
                        swatchesContainer.remove();
                        $('.color-select-button', me).attr('data-picker', 'hidden');

                        // Now insert it after the row that just requested it
                        swatchesContainer.insertAfter(greatGrandParent);
                        btn.attr('data-picker', 'visible');

                        $('.rgb-input-field.red', swatchesContainer).val(red);
                        $('.rgb-input-field.green', swatchesContainer).val(green);
                        $('.rgb-input-field.blue', swatchesContainer).val(blue);

                        // bind click on the swatch table image
                        $('.color-swatch-table', swatchesContainer).click(function(swatchEvt) {
                            var img = $(this);
                            var clickCoords = getRelativeClickCoords(swatchEvt, img);
                            var scale = swatchesWidth / img.width();
                            context.drawImage(img[0], scale * clickCoords.x, scale * (clickCoords.y-3), 1, 1, 0, 0, 1, 1);
                            var myData = context.getImageData(0, 0, 1, 1);
                            var rgb = [ myData.data[0], myData.data[1], myData.data[2] ];
                            $('.rgb-input-field.red', swatchesContainer).val(rgb[0]);
                            $('.rgb-input-field.green', swatchesContainer).val(rgb[1]);
                            $('.rgb-input-field.blue', swatchesContainer).val(rgb[2]);
                            me.trigger({
                                type: 'control-point-update-color',
                                index: idx,
                                newColorStr: 'rgb(' + rgb[0] + ',' + rgb[1] + ',' + rgb[2] + ')',
                                newColorArr: rgb
                            });
                        });

                        // bind focus change events for text input fields
                        $('.rgb-input-field', swatchesContainer).on('blur keyup', function(evt) {
                            if (evt.type == 'blur' || evt.keyCode == '13') {
                                var r = $('.rgb-input-field.red', swatchesContainer).val(),
                                    g = $('.rgb-input-field.green', swatchesContainer).val(),
                                    b = $('.rgb-input-field.blue', swatchesContainer).val();
                                me.trigger({
                                    type: 'control-point-update-color',
                                    index: idx,
                                    newColorStr: 'rgb(' + rgbClamp(r) + ',' + rgbClamp(g) + ',' + rgbClamp(b) + ')',
                                    newColorArr: [ r, g, b ]
                                });
                            }
                        });
                    } else {                                             // The color picker is there, remove it
                        swatchesContainer.remove();
                        btn.attr('data-picker', 'hidden');
                    }
                });

                $('[data-toggle="tooltip"]').tooltip({container: 'body'});

                if (widgetData.hasOwnProperty('interactiveMode')) {
                    interactiveMode = widgetData.interactiveMode;
                }
                updateInteractiveMode($('.color-editor-toggle-interactive', me));
                updateCategoricalMode($('.color-editor-toggle-categorical', me));
                updateEditMode($('.color-editor-toggle-edit-mode', me));

                // Do initial canvas drawing
                clearCanvas(colorCanvas, offset, size);
            }

            buildUi();
        });
    };

    $.fn.colorEditor.defaults = {
        topMargin: 10,
        rightMargin: 10,
        bottomMargin: 10,
        leftMargin: 10,
        interactiveMode: true,
        widgetKey: 'color.editor',
        widgetData: {},
        rgbInfo: {}
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-color-editor');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
