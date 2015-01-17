/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extends jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-opacity-editor'
 *
 * @class jQuery.paraview.ui.Opacity
 */
(function (GLOBAL, $) {

    var TEMPLATE_OPACITY_EDIT =
    '<div class="scalar-opacity-overflow-container">' +
    '<div class="scalar-opacity-editor-canvas-container">' +
    '<canvas class="scalar-opacity-editor-canvas-gaussian opacity-editor-canvas buttons-right"></canvas>' +
    '<canvas class="scalar-opacity-editor-canvas-linear opacity-editor-canvas buttons-right"></canvas>' +
    '<div class="opacity-editor-button-container buttons-right">' +
    '<div class="left-button-panel">' +
    '<div class="opacity-editor-toggle-mode vtk-icon-shuffle opacity-editor-button buttons-right shift-right shift-down" data-toggle="tooltip" data-placement="left" title="Toggle Gaussian/Linear Mode"></div>' +
    '<div class="opacity-editor-add-new vtk-icon-plus-circled opacity-editor-button buttons-right shift-right shift-down" data-toggle="tooltip" data-placement="left" title="Add New"></div>' +
    '<div class="opacity-editor-delete-selected vtk-icon-cancel-circled opacity-editor-button buttons-right shift-right shift-down" data-toggle="tooltip" data-placement="left" title="Delete Selected"></div>' +
    '<div class="opacity-editor-delete-all vtk-icon-trash opacity-editor-button buttons-right shift-right shift-down" data-toggle="tooltip" data-placement="left" title="Delete All"></div>' +
    '</div>' +
    '<div class="right-button-panel">' +
    '<div class="opacity-editor-toggle-surface-opacity vtk-icon-cancel-squared opacity-editor-button buttons-right shift-left shift-up" data-toggle="tooltip" data-placement="left" title="Toggle Opacity Mapping for Surfaces"></div>' +
    '<div class="opacity-editor-toggle-interactive vtk-icon-lock opacity-editor-button buttons-right shift-left shift-up" data-toggle="tooltip" data-placement="left" title="Toggle Interactive Mode"></div>' +
    '<div class="opacity-editor-apply vtk-icon-ok opacity-editor-button buttons-right shift-left shift-up" data-toggle="tooltip" data-placement="left" title="Apply"></div>' +
    '</div>' +
    '</div>' +
    '</div>' +
    '</div>',

    nearnessThreshold = 10,

    clearColor = "rgba(0, 0, 0, 0)",
    bgColor = "rgba(255, 255, 255, 255)",
    outlineWidth = "1",
    plotColor = "rgba(0, 0, 0, 255)",
    outlineColor = "rgba(85, 85, 85, 255)",
    activePlotColor = "rgba(0, 0, 255, 255)",
    inactivePointColor = 'pink',
    activePointColor = "rgba(48, 205, 64, 255)";


    /*********************************************************************
     * Given an array of "gaussian" objects, each containing the gaussian
     * parameters, calculate 255 opacity values.
     ********************************************************************/
    function calculateOpacities(gaussians) {
        var opacities = [];

        for (var i = 0; i < 256; ++i) {
            opacities[i] = 0.0;
        }

        for (var gnum = 0; gnum < gaussians.length; ++gnum) {
            var gaussian = gaussians[gnum];

            var position = gaussian.x;
            var height = gaussian.h;
            var width = gaussian.w;
            var xbias = gaussian.bx;
            var ybias = gaussian.by;

            for (var i = 0; i < 256; ++i) {
                var x = i / 255.0;
                // clamp non-zero values to pos +/- width
                if (x > (position + width) || x < (position - width)) {
                    //opacities[i] = opacities[i] > 0.0 ? opacities[i] : 0.0f;
                    if (opacities[i] < 0.0) {
                        opacities[i] = 0.0;
                    }
                    continue;
                }

                // non-zero width
                if (width === 0)
                    width = .00001;

                // translate the original x to a new x based on the xbias
                var x0;
                if (xbias === 0 || x === (position + xbias)) {
                    x0 = x;
                } else if (x > (position + xbias)) {
                    if (width === xbias)
                        x0 = position;
                    else
                        x0 = position + ((x - position - xbias) * (width / (width - xbias)));
                } else { // (x < pos+xbias)
                    if (-width === xbias)
                        x0 = position;
                    else
                        x0 = position - ((x - position - xbias) * (width / (width + xbias)));
                }

                // center around 0 and normalize to -1,1
                var x1 = (x0 - position) / width;

                // do a linear interpolation between:
                //    a gaussian and a parabola        if 0<ybias<1
                //    a parabola and a step function   if 1<ybias<2
                var h0a = Math.exp(-(4 * x1 * x1));
                var h0b = 1.0 - x1 * x1;
                var h0c = 1.0;
                var h1;
                if (ybias < 1) {
                    h1 = ybias * h0b + (1 - ybias) * h0a;
                } else {
                    h1 = (2 - ybias) * h0b + (ybias - 1) * h0c;
                }
                var h2 = height * h1;

                // perform the MAX over different gaussians, not the sum
                if (h2 > opacities[i]) {
                    opacities[i] = h2;
                }
            }
        }

        return opacities;
    }

    /*********************************************************************
     * Get an object containing the relative x,y coordinates of the click
     * point, given the click event and the container in which the click
     * occurred.
     ********************************************************************/
    function getRelativeClickCoords(event, canvas) {
        var x, y;

        canoffset = $(canvas).offset();
        x = event.clientX + document.body.scrollLeft + document.documentElement.scrollLeft - Math.floor(canoffset.left);
        y = event.clientY + document.body.scrollTop + document.documentElement.scrollTop - Math.floor(canoffset.top) + 1;

        return { 'x': x, 'y': y };
    }

    /*********************************************************************
     * Utility function to map from one known range of values into
     * another.
     ********************************************************************/
    function transform(inMin, value, inMax, outMin, outMax) {
        return (((value - inMin) / (inMax - inMin)) * (outMax - outMin)) + outMin;
    }

    /*********************************************************************
     * Utility function to clamp a value within the range of values given
     * by min and max.
     ********************************************************************/
    function clamp(min, value, max) {
        if (value < min) {
            return min;
        } else if (value > max) {
            return max;
        }
        return value;
    }

    /*********************************************************************
     * Convenience function to calculate distance between two 2D points.
     ********************************************************************/
    function distance(point1, point2) {
        return Math.sqrt(Math.pow((point1.x - point2.x), 2) + Math.pow((point1.y - point2.y), 2));
    }

    /*********************************************************************
     * Convenience function to identify the closest gaussian to the click
     * point, which could be none, if the click point was not close enough
     * to any gaussian.  The index of the closest gaussian is returned.
     ********************************************************************/
    function findClosestGaussian(coords, canvasElt, offset, size, gaussiansList) {
        var minDist = 100000000,
        idxOfNearest = -1;

        for (var i = 0; i < gaussiansList.length; ++i) {
            var g = gaussiansList[i];
            var gCanX = transform(0, g.x, 1.0, 0, size.w) + offset.x;
            var d = distance({'x': gCanX, 'y': 0}, {'x': coords.x, 'y': 0});
            if (d < minDist) {
                minDist = d;
                idxOfNearest = i;
            }
        }

        return idxOfNearest;
    }

    /*********************************************************************
     * Control points have keys 'center', 'width', 'height', and 'bias',
     * so return the key of the control point closest to the click point,
     * or return '' if no control point was close enough.
     ********************************************************************/
    function findClosestActiveControlPoint(coords, canvasElt, activeControlPoints) {
        var minDist = 100000000,
        keyOfNearest = '';

        // Find the closest one
        var activeProps = [ 'height', 'lwidth', 'rwidth', 'bias'];
        for (var idx in activeProps) {
            var key = activeProps[idx];
            if (activeControlPoints.hasOwnProperty(key)) {
                var cp = activeControlPoints[key];
                var d = distance(coords, cp);
                if (d < minDist && d < nearnessThreshold) {
                    minDist = d;
                    keyOfNearest = key;
                }
            }
        }

        return keyOfNearest;
    }

    /*********************************************************************
     * Calculate the screen coordinates of the control points for the
     * given gaussian.
     ********************************************************************/
    function calculateActiveControlPoints(gaussian, canvasElt, offset, size) {
        var centerX = transform(0.0, gaussian.x, 1.0, 0, size.w) + offset.x;
        var heightY = transform(0.0, gaussian.h, 1.0, 0, size.h);
        heightY = (size.h - heightY) + offset.y;
        var height = { 'x': centerX, 'y': heightY };
        var biasY = offset.y + heightY + ((size.h - heightY) / 2.0);
        var bias = { 'x': centerX, 'y': biasY };

        var lwidth = { 'y': size.h + offset.y,
                       'x': transform(0, (gaussian.x - gaussian.w), 1.0, 0, size.w) + offset.x };

        var rwidth = { 'y': size.h + offset.y,
                       'x': transform(0, (gaussian.x + gaussian.w), 1.0, 0, size.w) + offset.x };

        return { 'height': height, 'bias': bias, 'lwidth': lwidth, 'rwidth': rwidth };
    }

    /*********************************************************************
     * Clear the plot canvas and draw the plot-containing box
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
     * Plot the opacities in the scalar opacity canvas.
     ********************************************************************/
    function drawOpacities(canvasElt, offset, size, opacities, lineWidth, color) {
        var ctx = canvasElt[0].getContext("2d");

        ctx.beginPath();
        ctx.lineWidth = lineWidth;
        ctx.strokeStyle = color;

        // plot the opacities
        for (var i = 0; i < opacities.length; ++i) {
            var plotx = transform(0, i, (opacities.length - 1), 0, size.w) + offset.x;
            var ploty = transform(0, opacities[i], 1.0, 0, size.h);
            ploty = (size.h - ploty) + offset.y;
            if (i === 0) {
                ctx.moveTo(plotx, ploty);
            } else {
                ctx.lineTo(plotx, ploty);
            }
        }

        ctx.stroke();
    }

    /*********************************************************************
     * Plot linear opacities and their control points in the given canvas.
     ********************************************************************/
    function drawLinearOpacities(canvasElt, offset, size, linearPoints) {
        var ctx = canvasElt[0].getContext("2d");

        var startX = transform(0, linearPoints[0].x, 1.0, 0, size.w) + offset.x;
        var startY = (size.h - transform(0, linearPoints[0].y, 1.0, 0, size.h)) + offset.y;

        ctx.lineWidth = "2";
        ctx.strokeStyle = plotColor;

        ctx.beginPath();
        ctx.moveTo(startX, startY);

        for (var i = 1; i < linearPoints.length; ++i) {
            ctx.lineTo(transform(0, linearPoints[i].x, 1.0, 0, size.w) + offset.x,
                       (size.h - transform(0, linearPoints[i].y, 1.0, 0, size.h)) + offset.y);
        }

        ctx.stroke();
    }

    /*********************************************************************
     * Draw in all the control points of the active gaussian.
     ********************************************************************/
    function drawGaussianControlPoints(canvasElt, offset, size, activeGaussian, activeControlPoints) {
        var ctx = canvasElt[0].getContext("2d");

        // Draw the circular control points for the active gaussian
        var activeProps = [ 'height', 'lwidth', 'rwidth' ];
        for (var idx in activeProps) {
            var key = activeProps[idx];
            if (activeControlPoints.hasOwnProperty(key)) {
                var cp = activeControlPoints[key];
                if (cp.x >= offset.x && cp.x <= (offset.x + size.w)) {
                    ctx.lineWidth = "3";
                    ctx.strokeStyle = activePointColor;
                    ctx.beginPath();
                    ctx.arc(cp.x, cp.y, 6, 0, 2*Math.PI);
                    ctx.stroke();
                }
            }
        }

        // Draw the 'bias' control point a little differently
        if (activeControlPoints.hasOwnProperty('bias')) {
            var biasPoint = activeControlPoints.bias;
            var heightPoint = activeControlPoints.height;
            ctx.beginPath();
            ctx.moveTo(biasPoint.x - 7, biasPoint.y);
            ctx.lineTo(biasPoint.x, biasPoint.y - 7)
            ctx.lineTo(biasPoint.x + 7, biasPoint.y);
            ctx.lineTo(biasPoint.x, biasPoint.y + 7);
            // continue a little further left and up to completely close the diamond
            ctx.lineTo(biasPoint.x - 8, biasPoint.y - 1);
            ctx.stroke();
        }
    }

    /*********************************************************************
     * Draw in circles around all the linear control points
     ********************************************************************/
    function drawLinearControlPoints(canvasElt, offset, size, linearPoints, indexOfActive) {
        var ctx = canvasElt[0].getContext("2d");

        for (var i = 0; i < linearPoints.length; ++i) {
            var px = transform(0, linearPoints[i].x, 1.0, 0, size.w) + offset.x;
            var py = (size.h - transform(0, linearPoints[i].y, 1.0, 0, size.h)) + offset.y;
            ctx.lineWidth = "3";
            if (i === indexOfActive) {
                ctx.strokeStyle = activePointColor;
            } else {
                ctx.strokeStyle = inactivePointColor;
            }
            ctx.beginPath();
            ctx.arc(px, py, 6, 0, 2*Math.PI);
            ctx.stroke();
        }
    }

    /*********************************************************************
     * Find the right insert index and insert this point into the list
     ********************************************************************/
    function insertLinearPoint(point, linearPoints) {
        var idx = 1;
        while (idx < linearPoints.length) {
            if (point.x >= linearPoints[idx-1].x && point.x <= linearPoints[idx].x) {
                break;
            }
            idx += 1;
        }
        linearPoints.splice(idx, 0, point);
        return idx;
    }

    /*********************************************************************
     * Find the index of the nearest linear point, as long as its near
     * enough.  If no point is near enough, return -1.
     ********************************************************************/
    function findNearestLinearPoint(canvasElt, offset, size, point, linearPoints) {
        var ctx = canvasElt[0].getContext("2d"),
        idxOfNearest = -1,
        minDist = 100000000;

        for (var i = 0; i < linearPoints.length; ++i) {
            var px = transform(0, linearPoints[i].x, 1.0, 0, size.w) + offset.x;
            var py = (size.h - transform(0, linearPoints[i].y, 1.0, 0, size.h)) + offset.y;
            var d = distance({'x': px, 'y': py}, point);
            if (d < minDist && d < nearnessThreshold) {
                minDist = d;
                idxOfNearest = i;
            }
        }

        return idxOfNearest;
    }

    // ------------------------------------------------------------------------
    /**
     * Graphical component used to create a widget for editing the scalar
     * opacity function.
     *
     * @member jQuery.paraview.ui.Opacity
     * @method opacityEditor
     *
     * Usage:
     *
     *      $('.opacity-editor').opacityEditor( ... );
     *
     * Events:
     *
     *      {
     *        type: 'update-opacity-points',
     *        opacityPoints: [ x1, y1, x2, y2, ... ]
     *      }
     */
    $.fn.opacityEditor = function(options) {
        // Handle data with default values
        var opts = $.extend({}, $.fn.opacityEditor.defaults, options);

        // Widget creator function
        return this.each(function() {
            var me = $(this).empty().addClass('pv-opacity-editor'),
            buffer = [],
            addNewMode = false,
            gaussianMode = opts.gaussianMode,
            cpDragKey = '',
            dragging = false,
            lastCoords = {},
            indexOfActiveGaussian = -1,
            indexOfActiveLinear = -1,
            colors = [],
            opacities = [],
            activeControlPoints = {},
            disableClick = false,
            gaussiansList = $.extend(true, [], opts.gaussiansList),
            linearPoints = $.extend(true, [], opts.linearPoints),
            interactiveMode = opts.interactiveMode,
            surfaceOpacity = opts.surfaceOpacityEnabled,
            offset = {},
            size = {};

            buffer.push(TEMPLATE_OPACITY_EDIT);

            me[0].innerHTML = buffer.join('');

            var buttonsRight = true;   // otherwise, it's buttons along the top
            if (opts.buttonsPosition !== 'right') {
                buttonsRight = false;
            }

            var topMargin = opts.topMargin;
            var rightMargin = opts.rightMargin;
            var bottomMargin = opts.bottomMargin;
            var leftMargin = opts.leftMargin;

            if (opts.hasOwnProperty('margin') === true) {
                topMargin = opts.margin;
                rightMargin = opts.margin;
                bottomMargin = opts.margin;
                leftMargin = opts.margin;
            }

            var buttonTrayWidth = $('.opacity-editor-toggle-mode', me).width();

            // Initialize both canvases
            var pwfCanvas = $('.scalar-opacity-editor-canvas-gaussian', me);
            var linearPwfCanvas = $('.scalar-opacity-editor-canvas-linear', me);

            // Calculate canvas height and width based on whether we want buttons
            // on top or on right
            var canvasHeight = pwfCanvas.parent().height();
            var canvasWidth = pwfCanvas.parent().width() - buttonTrayWidth;

            if (buttonsRight === false) {
                canvasHeight = pwfCanvas.parent().height() - buttonTrayWidth;
                canvasWidth = pwfCanvas.parent().width();
                $('.buttons-right', me).toggleClass('buttons-right').toggleClass('buttons-top').attr('data-placement', 'bottom');
            }

            // Size the canvases
            pwfCanvas.attr('height', canvasHeight);
            pwfCanvas.attr('width', canvasWidth);
            linearPwfCanvas.attr('height', canvasHeight);
            linearPwfCanvas.attr('width', canvasWidth);

            // Set up the drawing area for the canvases
            offset = {'x': leftMargin, 'y': topMargin};
            size = {'w': (canvasWidth - (leftMargin + rightMargin)), 'h': (canvasHeight - (topMargin + bottomMargin))};

            /*
             * Internal convenience function to trigger the
             */
            function fireNewPoints() {
                var newPointsEvent = {
                    'type': 'update-opacity-points',
                };

                var points = [];

                if (gaussianMode === true) {
                    var opacities = calculateOpacities(gaussiansList);
                    for (var i = 0; i < opacities.length; ++i) {
                        var xVal = transform(0, i, (opacities.length - 1), 0.0, 1.0);
                        points.push(xVal);
                        points.push(opacities[i]);
                        points.push(0.0);
                        points.push(0.5);
                    }
                } else {
                    for (var j = 0; j < linearPoints.length; ++j) {
                        points.push(linearPoints[j].x);
                        points.push(linearPoints[j].y);
                        points.push(0.0);
                        points.push(0.5);
                    }
                }

                newPointsEvent.opacityPoints = points;
                newPointsEvent.gaussianPoints = gaussiansList;
                newPointsEvent.linearPoints = linearPoints;
                newPointsEvent.gaussianMode = gaussianMode;
                newPointsEvent.interactiveMode = interactiveMode;
                me.trigger(newPointsEvent);
            }

            /*
             * Internal convenience function to do all canvas drawing depending on mode
             */
            function render() {
                if (gaussianMode === true) {
                    renderGaussian();
                } else {
                    renderLinear();
                }
            }

            /*
             * Aggregates the draw calls needed when working in multi-gaussian mode
             */
            function renderGaussian() {
                clearCanvas(pwfCanvas, offset, size);
                var opacities = calculateOpacities(gaussiansList);
                drawOpacities(pwfCanvas, offset, size, opacities, "2", "black");
                if (indexOfActiveGaussian >= 0) {
                    var activeOpacities = calculateOpacities([ gaussiansList[indexOfActiveGaussian] ]);
                    drawOpacities(pwfCanvas, offset, size, activeOpacities, "3", activePlotColor);
                }
                drawGaussianControlPoints(pwfCanvas, offset, size, gaussiansList[indexOfActiveGaussian], activeControlPoints);
            }

            /*
             * Aggregates the draw calls needed when working in piecewise-linear mode
             */
            function renderLinear() {
                clearCanvas(linearPwfCanvas, offset, size);
                drawLinearOpacities(linearPwfCanvas, offset, size, linearPoints);
                drawLinearControlPoints(linearPwfCanvas, offset, size, linearPoints, indexOfActiveLinear);
            }

            /*
             * Internal convenience function to show/hide appropriate canvas for mode
             */
            function exposeCanvasForMode() {
                if (gaussianMode === true) {
                    pwfCanvas.show();
                    linearPwfCanvas.hide();
                } else {
                    pwfCanvas.hide();
                    linearPwfCanvas.show();
                }
            }

            /*
             * Internal convenience function to handle change in interactive mode
             */
            function updateInteractiveMode(element) {
                if (interactiveMode === true) {
                    $('.opacity-editor-apply', me).css('opacity', 0.3);
                    element.removeClass('vtk-icon-lock-open').addClass('vtk-icon-lock');
                } else {
                    $('.opacity-editor-apply', me).css('opacity', 1.0);
                    element.removeClass('vtk-icon-lock').addClass('vtk-icon-lock-open');
                }
            }

            /*
             * Internal convenience function to handle change in surface opacity mode
             */
            function updateSurfaceOpacityMode(element) {
                if (surfaceOpacity === true) {
                    element.removeClass('vtk-icon-cancel-squared').addClass('vtk-icon-ok-squared');
                } else {
                    element.removeClass('vtk-icon-ok-squared').addClass('vtk-icon-cancel-squared');
                }
                me.trigger({
                    type: 'update-surface-opacity',
                    enabled: surfaceOpacity
                });
            }

            // Single-clicking the center point of a gaussian makes it the "active"
            // one, which results in all of its control points getting drawn.
            pwfCanvas.click(function(evt) {
                if (disableClick === true) {
                    disableClick = false;
                    return;
                }
                var coords = getRelativeClickCoords(evt, $(this));
                var gIdx = findClosestGaussian(coords, pwfCanvas, offset, size, gaussiansList);
                if (gIdx >= 0) {
                    activeControlPoints = calculateActiveControlPoints(gaussiansList[gIdx], pwfCanvas, offset, size);
                    indexOfActiveGaussian = gIdx;
                    render();
                } else {
                    activeControlPoints = {};
                    indexOfActiveGaussian = -1;
                    render();
                }
            });

            // Mouse-down events might begin manipulation of existing control points
            pwfCanvas.mousedown(function(evt) {
                disableClick = false;
                if (indexOfActiveGaussian >= 0) {
                    var coords = getRelativeClickCoords(evt, $(this));
                    var cpKey = findClosestActiveControlPoint(coords, pwfCanvas, activeControlPoints);
                    if (cpKey !== '') {
                        lastCoords = coords;
                        dragging = true;
                        cpDragKey = cpKey;
                    }
                }
            });

            // Depending on which type of gaussian control point is selected, ('height',
            // 'bias', or 'width') manipulate the currently active gaussian accordingly.
            pwfCanvas.mousemove(function(evt) {
                var coords = getRelativeClickCoords(evt, $(this));
                var newX = clamp(0.0, transform(0, (coords.x - offset.x), size.w, 0.0, 1.0), 1.0);
                var newY = clamp(0.0, transform(0, (size.h - (coords.y - offset.y)), size.h, 0.0, 1.0), 1.0);

                if (addNewMode === true) {
                    // In this case, we must have just clicked the '+' button, so as
                    // soon as the mouse enters the canvas, we want to create a new
                    // gaussian right at the entry point.
                    addNewMode = false;

                    gaussiansList.push({'x': newX,  'w': 0.25, 'h': newY, 'bx': 0.0, 'by': 0.0 });
                    indexOfActiveGaussian = gaussiansList.length - 1;
                    activeControlPoints = calculateActiveControlPoints(gaussiansList[indexOfActiveGaussian], pwfCanvas, offset, size);
                    cpDragKey = 'height';
                    dragging = true;
                    render();
                } else if (dragging === true) {
                    // In this case, the user must have clicked on one of the active
                    // gaussian control points, so we should allow them to manipulate
                    // the parameter(s) associated with it.
                    disableClick = true;

                    // Switch logic depending on which control point of the gaussian we
                    // are actually manipulating
                    if (cpDragKey === 'height') {
                        gaussiansList[indexOfActiveGaussian].x = newX;
                        gaussiansList[indexOfActiveGaussian].h = newY;
                        activeControlPoints = calculateActiveControlPoints(gaussiansList[indexOfActiveGaussian], pwfCanvas, offset, size);
                        render();
                    } else if (cpDragKey.indexOf('width') >= 0) {
                        var gaussian = gaussiansList[indexOfActiveGaussian];
                        var centerScreenX = activeControlPoints.height.x;
                        var screenDelta = coords.x - centerScreenX;
                        if (cpDragKey === 'lwidth') {
                            screenDelta = centerScreenX - coords.x;
                        }
                        var newWidth = clamp(0.0, transform(0, Math.abs(screenDelta), size.w, 0.0, 1.0), 2.0);
                        gaussian.w = newWidth;
                        activeControlPoints = calculateActiveControlPoints(gaussian, pwfCanvas, offset, size);
                        render();
                    } else if (cpDragKey === 'bias') {
                        var gaussian = gaussiansList[indexOfActiveGaussian];
                        var screenDeltaX = coords.x - lastCoords.x;
                        var realDeltaX = transform(0, Math.abs(screenDeltaX), size.w, 0.0, 1.0);
                        realDeltaX *= Math.sign(screenDeltaX);
                        var screenDeltaY = lastCoords.y - coords.y;
                        var realDeltaY = transform(0, Math.abs(screenDeltaY), Math.abs(activeControlPoints.lwidth.y - activeControlPoints.height.y) / 2.0, 0.0, 2.0);
                        realDeltaY *= Math.sign(screenDeltaY);
                        gaussian.bx += realDeltaX;
                        var newBy = clamp(0.0, (gaussian.by + realDeltaY), 2.0);
                        gaussian.by = newBy;
                        activeControlPoints = calculateActiveControlPoints(gaussian, pwfCanvas, offset, size);
                        render();
                    }

                    lastCoords = coords;
                }
            });

            // On mouseup is where we will want to trigger the event indicating that
            // a new set of points is available.  For Gaussian mode, it will be an
            // array of the 256 opacities.
            pwfCanvas.mouseup(function(evt) {
                dragging = false;
                cpDragKey = '';
                if (interactiveMode === true) {
                    fireNewPoints();
                }
            });

            // Handle clicks on the piecewise-linear mode canvas.  We're either adding a
            // new point or else making an existing one "active".
            linearPwfCanvas.click(function(evt) {
                if (disableClick === true) {
                    disableClick = false;
                    return;
                }
                var coords = getRelativeClickCoords(evt, $(this));

                var idxOfNearest = findNearestLinearPoint(linearPwfCanvas, offset, size, coords, linearPoints);
                if (idxOfNearest < 0) {
                    var nx = clamp(0.0, transform(0, (coords.x - offset.x), size.w, 0.0, 1.0), 1.0);
                    var ny = clamp(0.0, transform(0, size.h - (coords.y - offset.y), size.h, 0.0, 1.0), 1.0);
                    var insertIdx = insertLinearPoint({'x': nx, 'y': ny}, linearPoints);
                    indexOfActiveLinear = insertIdx;
                    render();
                    if (interactiveMode === true) {
                        fireNewPoints();
                    }
                } else {
                    indexOfActiveLinear = idxOfNearest;
                    render();
                }
            });

            // If the mouse goes down on the piecewise-linear mode canvas, and its
            // near enough to an existing control point, then get ready to manipulate
            // that point
            linearPwfCanvas.mousedown(function(evt) {
                var coords = getRelativeClickCoords(evt, $(this));
                var idxOfNearest = findNearestLinearPoint(linearPwfCanvas, offset, size, coords, linearPoints);
                if (idxOfNearest >= 0) {
                    dragging = true;
                    indexOfActiveLinear = idxOfNearest;
                    render();
                }
            });

            // either manipulate an existing linear point, or else manipulate a new one
            // we're about to drop
            linearPwfCanvas.mousemove(function(evt) {
                var coords = getRelativeClickCoords(evt, $(this));
                var nx = clamp(0.0, transform(0, (coords.x - offset.x), size.w, 0.0, 1.0), 1.0);
                var ny = clamp(0.0, transform(0, size.h - (coords.y - offset.y), size.h, 0.0, 1.0), 1.0);

                if (addNewMode === true) {
                    // Create a new linear point as soon as mouse enters canvas,
                    // at that location
                    addNewMode = false;
                    var insertIdx = insertLinearPoint({'x': nx, 'y': ny}, linearPoints);
                    indexOfActiveLinear = insertIdx;
                    dragging = true;
                    render();
                } else if (dragging === true) {
                    disableClick = true;
                    var curIdx = indexOfActiveLinear;
                    if (curIdx == 0 || curIdx == (linearPoints.length - 1)) {
                        linearPoints[curIdx].y = ny
                    } else {
                        linearPoints[curIdx].x = nx;
                        linearPoints[curIdx].y = ny;
                        if (nx > linearPoints[curIdx + 1].x) {
                            // swap with right neighbor
                            var tmp = linearPoints[curIdx + 1];
                            linearPoints[curIdx + 1] = linearPoints[curIdx];
                            linearPoints[curIdx] = tmp;
                            indexOfActiveLinear = curIdx + 1;
                        } else if (nx < linearPoints[curIdx - 1].x) {
                            // swap with left neighbor
                            var tmp = linearPoints[curIdx - 1];
                            linearPoints[curIdx - 1] = linearPoints[curIdx];
                            linearPoints[curIdx] = tmp;
                            indexOfActiveLinear = curIdx - 1;
                        }
                    }
                    render();
                }
            });

            // End manipulation of a piecewise-linear mode control point
            linearPwfCanvas.mouseup(function(evt) {
                dragging = false;
                if (interactiveMode === true) {
                    fireNewPoints();
                }
            });

            $('.opacity-editor-toggle-mode', me).click(function(evt) {
                gaussianMode = !gaussianMode;
                addNewMode = false;
                dragging = false;
                exposeCanvasForMode();
                render();
                if (interactiveMode === true) {
                    fireNewPoints();
                }
            });

            $('.opacity-editor-add-new', me).click(function(evt) {
                addNewMode = true;
            });

            $('.opacity-editor-delete-selected', me).click(function(evt) {
                dragging = false;
                if (gaussianMode === true && indexOfActiveGaussian >= 0) {
                    gaussiansList.splice(indexOfActiveGaussian, 1);
                    indexOfActiveGaussian = -1;
                    activeControlPoints = {};
                    render();
                    if (interactiveMode === true) {
                       fireNewPoints();
                    }
                } else if (gaussianMode === false) {
                    if (indexOfActiveLinear > 0 && indexOfActiveLinear < (linearPoints.length - 1)) {
                        linearPoints.splice(indexOfActiveLinear, 1);
                        indexOfActiveLinear = -1;
                        render();
                        if (interactiveMode === true) {
                            fireNewPoints();
                        }
                    }
                }
            });

            $('.opacity-editor-delete-all', me).click(function(evt) {
                dragging = false;
                if (gaussianMode === true) {
                    indexOfActiveGaussian = -1;
                    activeControlPoints = {};
                    gaussiansList = [];
                } else {
                    var oldPoints = linearPoints;
                    var newPoints = [ { 'x': oldPoints[0].x, 'y': oldPoints[0].y },
                                      { 'x': oldPoints[oldPoints.length-1].x, 'y': oldPoints[oldPoints.length-1].y } ];
                    linearPoints = newPoints;
                    indexOfActiveLinear = -1;
                }
                render();
                if (interactiveMode === true) {
                    fireNewPoints();
                }
            });

            $('.opacity-editor-toggle-interactive', me).click(function(evt) {
                interactiveMode = !interactiveMode;
                updateInteractiveMode($(this));
            });

            $('.opacity-editor-apply', me).click(function(evt) {
                dragging = false;
                if (gaussianMode === true) {
                    indexOfActiveGaussian = -1;
                    activeControlPoints = {};
                } else {
                    indexOfActiveLinear = -1;
                }
                render();
                fireNewPoints();
            });

            $('.opacity-editor-toggle-surface-opacity', me).click(function(evt) {
                surfaceOpacity = !surfaceOpacity;
                updateSurfaceOpacityMode($(this));
            });

            $('[data-toggle="tooltip"]').tooltip({container: 'body'})

            // Finally draw the initial view
            updateInteractiveMode($('.opacity-editor-toggle-interactive', me));
            updateSurfaceOpacityMode($('.opacity-editor-toggle-surface-opacity', me));
            exposeCanvasForMode();
            render();
        });
    };

    $.fn.opacityEditor.defaults = {
        buttonsPosition: 'right',
        surfaceOpacityEnabled: false,
        gaussianMode: false,
        interactiveMode: true,
        topMargin: 10,
        rightMargin: 10,
        bottomMargin: 10,
        leftMargin: 10,
        gaussiansList: [],
        linearPoints: [ {'x': 0.0, 'y': 0.0}, {'x': 1.0, 'y': 1.0} ]
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-opacity-editor');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
