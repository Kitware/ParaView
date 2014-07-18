/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-data'
 *
 * @class jQuery.paraview.ui.Pipeline
 */
(function (GLOBAL, $) {

    var TEMPLATE_EXTENT = "<div class='pv-callout pv-callout-info'><h4>Extents</h4><dl class='dl-horizontal'><dt>X</dt><dd>x1 to x2 (dimension x3)</dd><dt>Y</dt><dd>y1 to y2 (dimension y3)</dd><dt>Z</dt><dd>z1 to z2 (dimension z3)</dd></dl></div>",
        TEMPLATE_ARRAY_LINE = "<tr><td title='TYPE[SIZE]'>ICON NAME</td><td><ul class='list-unstyled'>RANGES</ul></td></tr>",
        TEMPLATE_TIME_LINE = "<tr><td>INDEX</td><td>VALUE</td></tr>",
        TEMPLATE_RANGE_LINE = "<li title='NAME'>[MIN, MAX]</li>"
        TEMPLATE_DATA_INFORMATION = "<div class='pv-callout pv-callout-info'><h4>Statistics</h4><dl class='dl-horizontal'><dt>Type</dt><dd>TYPE</dd><dt>Cells</dt><dd>NB_CELLS</dd><dt>Points</dt><dd>NB_POINTS</dd><dt>Memory</dt><dd>MEMORY</dd></dl></div><div class='pv-callout pv-callout-info'><h4>Data Arrays</h4><table class='table table-hover table-condensed'><thead><tr><th>Name</th><th>Ranges</th></tr></thead><tbody>ARRAY_LIST</tbody></table></div>EXTENT_CONTENT<div class='pv-callout pv-callout-info'><h4>Bounds</h4><dl class='dl-horizontal'><dt>X</dt><dd>[X_RANGE]</dd><dt>Y</dt><dd>[Y_RANGE]</dd><dt>Z</dt><dd>[Z_RANGE]</dd></dl></div><div class='pv-callout pv-callout-info'><h4>Time</h4><table class='table table-hover table-condensed'><thead><tr><th>Index</th><th>Value</th></tr></thead><tbody>TIME_LIST</tbody></table></div>",
        TEMPLATE_ICON = {
            'POINTS' : '  : ',
            'CELLS' : ' | ',
            'FIELDS' : 'O ',
        };


    // TYPE, NB_CELLS, NB_POINTS, MEMORY
    // ARRAY_LIST => ICON, NAME, TYPE, SIZE, RANGES
    // EXTENT_CONTENT => TEMPLATE_EXTENT => [x-z][1-3]
    // X_RANGE, Y_RANGE, Z_RANGE
    // TIME_LIST => INDEX, VALUE
    /**
     * JQuery Helper that will fill the provided DOM element with
     * some formatted content that you can find in the 'data' section
     * of the ParaViewWebProxyManager protocol when you retreive a Source Proxy.
     *
     * @member jQuery.paraview.ui.Pipeline
     * @method pvDataInformation
     * @param {Object} data object describing the data part of the Source Proxy
     *
     * Usage:
     *
     *      $('.info-content').pvDataInformation( proxy.data );
     */
    $.fn.pvDataInformation = function(data) {
        // Handle data with default values
        return this.each(function() {
            var me = $(this).empty().addClass('pv-data-info'),
                arrayListBuffer = [],
                timeListBuffer = [],
                extentContent = '',
                count;

            // Fill array lists
            count = data.arrays.length;
            while(count--) {
                var item = data.arrays[count],
                    nbRanges = item.hasOwnProperty('range') ? item.range.length : 0,
                    ranges = [];

                if(nbRanges > 4) {
                    nbRanges = 1;
                }

                for(var i = 0; i < nbRanges; ++i) {
                    ranges.push(TEMPLATE_RANGE_LINE
                        .replace(/NAME/g, item.range[i].name)
                        .replace(/MIN/g, item.range[i].min)
                        .replace(/MAX/g, item.range[i].max));
                }
                if(nbRanges == 1 && item.range.length > 4) {
                    ranges.push('<li>...</li>')
                }

                arrayListBuffer.push(TEMPLATE_ARRAY_LINE
                    .replace(/ICON/g, '') // TEMPLATE_ICON[item.type]) // FIXME replace with location
                    .replace(/NAME/g, item.name)
                    .replace(/TYPE/g, item.type)
                    .replace(/SIZE/g, item.size)
                    .replace(/RANGES/g, ranges.join('')));
            }
            arrayListBuffer.sort();

            // Fill time lists
            count = data.time.length;
            for(var i = 0; i < count; ++i) {
                timeListBuffer.push(TEMPLATE_TIME_LINE
                    .replace(/INDEX/g, i)
                    .replace(/VALUE/g, data.time[i]));
            }

            // Fill extent
            if(data.hasOwnProperty('extent')) {
                extentContent = TEMPLATE_EXTENT;
                var keys = 'xyz';
                for(var keyIdx = 0; keyIdx < 3; ++keyIdx) {
                    for(var comp = 0; comp < 2; ++comp) {
                        extentContent = extentContent.replace(keys[keyIdx] + (1+comp), data.extent[keyIdx*2 + comp]);
                    }
                    extentContent = extentContent.replace(keys[keyIdx] + '3', data.extent[keyIdx*2+1] - data.extent[keyIdx*2]);
                }
            }

            // Update HTML
            me[0].innerHTML = TEMPLATE_DATA_INFORMATION
                .replace(/TYPE/g, data.type)
                .replace(/NB_CELLS/g, data.cells)
                .replace(/NB_POINTS/g, data.points)
                .replace(/MEMORY/g, data.memory)
                .replace(/ARRAY_LIST/g, arrayListBuffer.join(''))
                .replace(/EXTENT_CONTENT/g, extentContent)
                .replace(/X_RANGE/g, data.bounds.slice(0,2).join(', '))
                .replace(/Y_RANGE/g, data.bounds.slice(2,4).join(', '))
                .replace(/Z_RANGE/g, data.bounds.slice(4,6).join(', '))
                .replace(/TIME_LIST/g, timeListBuffer.join(''));
        });
    };

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-data');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }
}(window, jQuery));