/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to ParaViewWeb usage.
 *
 * This module registers itself as: 'paraview-ui-pipeline'
 *
 * @class jQuery.paraview.ui.PipelineBrowser
 */
(function (GLOBAL, $) {

    // =======================================================================
    // ==== Defaults constant values =========================================
    // =======================================================================

    //PIPELINE_REPRESENTATION_NAMES PIPELINE_REPRESENTATION_NAMES_TO_LABELS

    var PIPELINE_REPRESENTATION_NAMES = 'hide outline wireframe surface surface_edge volume',
    PIPELINE_REPRESENTATION_NAMES_TO_LABELS = {
        'hide': 'Hide',
        'outline': 'Outline',
        'wireframe': 'Wireframe',
        'surface': 'Surface',
        'surface_edge': 'Surface With Edges',
        'volume': 'Volume'
    },
    PIPELINE_REPRESENTATION_LABELS_TO_NAMES = {
        'Hide': 'hide',
        'Outline': 'outline',
        'Wireframe': 'wireframe',
        'Surface': 'surface',
        'Surface With Edges': 'surface_edge',
        'Volume': 'volume'
    },
    PIPELINE_COLOR_BY_TYPES = 'color points cells',
    PIPELINE_VIEW_TYPES = 'view-pipeline view-files view-sources',
    DEFAULT_PIPELINE = {
        name: "kitware.com",
        type: 'server',
        children: [{
            proxy_id: 234,
            name: "Cow.vtu",
            icon: 'dataset',
            representation: 'Outline',
            showScalarBar: false,
            pointData: [
            {
                name: 'a',
                range: [0,1],
                size: 1
            },

            {
                name: 'b',
                range: [5,10],
                size: 1
            },

            {
                name: 'c',
                range: [2,4],
                size: 3
            }],
            cellData: [
            {
                name: 'aa',
                range: [0,1],
                size: 1
            },

            {
                name: 'bb',
                range: [5,10],
                size: 1
            },

            {
                name: 'cc',
                range: [2,4],
                size: 3
            }],
            activeData: 'p:a',
            children: [{
                proxy_id: 235,
                name: "Iso",
                icon: 'contour',
                representation: 'Surface',
                showScalarBar: false,
                pointData: [
                {
                    name: 'a',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'b',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'c',
                    range: [2,4],
                    size: 3
                }],
                cellData: [
                {
                    name: 'aa',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'bb',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'cc',
                    range: [2,4],
                    size: 3
                }],
                activeData: 'p:a',
                children: [{
                    proxy_id: 236,
                    name: "Clip",
                    type: 'clip',
                    representation: 'Surface'
                }]
            }, {
                proxy_id: 237,
                name: "Slice",
                icon: 'slice',
                representation: 'Surface',
                showScalarBar: true,
                pointData: [
                {
                    name: 'a',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'b',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'c',
                    range: [2,4],
                    size: 3
                }],
                cellData: [
                {
                    name: 'aa',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'bb',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'cc',
                    range: [2,4],
                    size: 3
                }],
                activeData: 'p:a'
            }, {
                proxy_id: 238,
                name: "Stream Tracer",
                icon: 'stream',
                representation: 'Wireframe',
                showScalarBar: false,
                pointData: [
                {
                    name: 'a',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'b',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'c',
                    range: [2,4],
                    size: 3
                }],
                cellData: [
                {
                    name: 'aa',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'bb',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'cc',
                    range: [2,4],
                    size: 3
                }],
                activeData: 'p:a'
            }, {
                proxy_id: 239,
                name: "Threshold",
                icon: 'threshold',
                representation: 'Surface Width Edge',
                showScalarBar: false,
                pointData: [
                {
                    name: 'a',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'b',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'c',
                    range: [2,4],
                    size: 3
                }],
                cellData: [
                {
                    name: 'aa',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'bb',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'cc',
                    range: [2,4],
                    size: 3
                }],
                activeData: 'p:a'
            }, {
                proxy_id: 240,
                name: "Random filter",
                icon: 'filter',
                representation: 'Volume',
                showScalarBar: false,
                pointData: [
                {
                    name: 'a',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'b',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 'c',
                    range: [2,4],
                    size: 3
                }],
                cellData: [
                {
                    name: 'aa',
                    range: [0,1],
                    size: 1
                },

                {
                    name: 'bb',
                    range: [5,10],
                    size: 1
                },

                {
                    name: 't',
                    range: [2,4],
                    size: 3
                }],
                activeData: 'c:t'
            }]
        }, {
            proxy_id: 245,
            name: "Random source",
            icon: 'dataset',
            representation: 'Surface',
            showScalarBar: false,
            pointData: [
            {
                name: 'z',
                range: [0,1],
                size: 1
            },

            {
                name: 'b',
                range: [5,10],
                size: 1
            },

            {
                name: 'c',
                range: [2,4],
                size: 3
            }],
            cellData: [
            {
                name: 'aa',
                range: [0,1],
                size: 1
            },

            {
                name: 'bb',
                range: [5,10],
                size: 1
            },

            {
                name: 'cc',
                range: [2,4],
                size: 3
            }],
            activeData: 'p:z'
        }]
    },
    DEFAULT_SOURCES = [{
        name: 'Cone',
        icon: 'dataset',
        category: 'source'
    },{
        name: 'Wavelet',
        icon: 'dataset',
        category: 'source'
    },{
        name: 'Clip',
        icon: 'clip',
        category: 'filter'
    },{
        name: 'Slice',
        icon: 'slice',
        category: 'filter'
    },{
        name: 'Contour',
        icon: 'contour',
        category: 'filter'
    },{
        name: 'Threshold',
        icon: 'threshold',
        category: 'filter'
    },{
        name: 'Stream Tracer',
        icon: 'stream',
        category: 'filter'
    },{
        name: 'Warp',
        icon: 'filter',
        category: 'filter'
    }],
    busyStatus = 0,
    buffer = null;


    // =======================================================================
    // ==== jQuery based methods =============================================
    // =======================================================================

    /**
     * Graphical component use to show and interact with the ParaViewWeb
     * pipeline.
     *
     * @member jQuery.paraview.ui.PipelineBrowser
     * @method pipelineBrowser
     * @param {Object} options
     *
     * Usage:
     *
     *      $('.pipeline-container-div').pipelineBrowser({
     *          session: sessionObj,
     *          pipeline: pipeline,
     *          sources: sourceList,
     *          files: fileList,
     *          title: 'Kitware',
     *          cacheFiles: false
     *      });
     */
    $.fn.pipelineBrowser = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.pipelineBrowser.defaults, options);

        return this.each(function() {
            var me = $(this).empty().addClass('pipelineBrowser view-pipeline'),
            session = opts.session;

            // Initialize global html buffer
            if (buffer === null) {
                buffer = createBuffer();
            }
            buffer.clear();

            // Update DOM
            me.data('pipeline', opts);
            updateIndexMap(me, 0, opts.pipeline.children);

            // Fill buffer with pipeline HTML
            addPipelineToBuffer(opts.title, opts);
            me[0].innerHTML = buffer.toString();

            // Initialize file section
            $('.pipeline-files').fileBrowser({session: session, cacheFiles: opts.cacheFiles}).bind('file-click file-group-click', function(e){
                pipeline = getPipeline(me), toggleButton = $('.files.active', pipeline);
                pipeline.removeClass(PIPELINE_VIEW_TYPES).addClass('view-pipeline');
                toggleButton.removeClass('active');

                fireBusy(pipeline, true);
                session.call("pv.pipeline.manager.file.ropen", [e.relativePathList]).then(function(newFile){
                    dataChanged(me);
                    addProxy(me, 0, newFile);
                    fireAddSource(pipeline, newFile, 0);
                    fireBusy(pipeline, false);
                });
            });

            // Initialize pipelineBrowser (Visibility + listeners)
            initializeListener(me);

            // Attach RPC method if possible
            attachSessionController(me);
        });
    };

    $.fn.pipelineBrowser.defaults = {
        session: null,
        pipeline: DEFAULT_PIPELINE,
        sources: DEFAULT_SOURCES,
        title: 'Kitware',
        cacheFiles: true
    };

    /**
     * Method used to retreive a proxy from the Pipeline browser.
     * If the proxyId is null/undefined the selected proxy will be returned.
     *
     * @member jQuery.paraview.ui.PipelineBrowser
     * @method getProxy
     * @param {Number|undefined|null} proxyId
     * @return {pv.Proxy} proxy that have the given id or null if not found.
     *
     * Usage:
     *      var selectedProxy = $('.pipeline-container-div').getProxy();
     *      var proxy = $('.pipeline-container-div').getProxy(1234);
     */

    $.fn.getProxy = function(proxyId) {
        var me = $(this);
        if(proxyId === null || proxyId === undefined) {
            proxyId = me.data('active_proxy_id');
        }
        return getProxy(me, proxyId);
    };

    // =======================================================================
    // ==== Events triggered on the pipelineBrowser ==========================
    // =======================================================================

    /**
     * Event triggered when a Proxy has changed.
     *
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event proxyModified
     * @param {Number} id
     * Proxy ID that was changed.
     * @param {String} origin
     * Origin on which the change set apply. Such as 'property' or 'representation' or 'scalarbar'.
     * ['property', 'representation', 'scalarbar', 'colorBy']
     * @param {Object} changeSet
     * Object that contain a set of key/value pair that correspond to field name and field value.
     */
    function fireProxyChange(uiWidget, origin, changeSet) {
        getPipeline(uiWidget).trigger({
            type: 'proxyModified',
            proxy_id: getProxyId(uiWidget),
            origin: origin,
            changeSet: changeSet
        });
        dataChanged(uiWidget);
    }

    /**
     * Event triggered when selection change in the Pipeline browser.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event proxySelected
     * @param {Number} id
     * Id of the proxy that get selected in the Pipeline. Can be 0 if no selection.
     */
    function fireProxySelected(uiWidget) {
        var proxyWidget = getProxyWidget(uiWidget),
        activeProxyId = getProxyId(uiWidget),
        pipelineBrowser = getPipeline(uiWidget),
        proxy = getProxy(pipelineBrowser, activeProxyId);

        // Save selected proxy
        setActiveProxyId(uiWidget, activeProxyId);

        // Update property panel
        updateProxyProperties(pipelineBrowser, proxy);

        $('.proxy-control', pipelineBrowser).removeClass('selected');
        $('.proxy-control:eq(0)', proxyWidget).addClass('selected');

        // Allow delete ?
        if(activeProxyId === 0 || $('ul', proxyWidget).children().length > 0) {
            $('.pipeline-control .delete-proxy', getPipeline(uiWidget)).addClass('disabled');
        } else {
            $('.pipeline-control .delete-proxy', getPipeline(uiWidget)).removeClass('disabled');
            getPipeline(uiWidget).addClass('view-filters');
        }

        // Send event
        pipelineBrowser.trigger({
            type: 'proxySelected',
            proxy_id: activeProxyId
        });
    }

    /**
     * Event that get triggered when the user wants to invalidate the full pipeline
     * and get a new version from the server.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event reloadPipeline
     */
    function fireReloadPipeline(uiWidget) {
        getPipeline(uiWidget).trigger({
            type: 'reloadPipeline'
        });
    }

    /**
     * Event that get triggered when a source or a filter is getting added.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event addSource
     * @param {String} name
     * Name of the SourceProxy to be created.
     * @param {Number} parent
     * Global Id of the parent Proxy if any, null otherwise.
     */
    function fireAddSource(container, algoName, parent) {
        container.trigger({
            type: 'addSource',
            name: algoName,
            parent_id: parent
        });
        dataChanged(container);
    }

    /**
     * Event that get triggered when a Proxy get deleted.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event deleteProxy
     * @param {pv.Proxy} proxy
     * Proxy that is getting deleted.
     */
    function fireDeleteProxy(uiWidget) {
        getPipeline(uiWidget).trigger({
            type: 'deleteProxy',
            proxy_id: getActiveProxyId(uiWidget)
        });
        dataChanged(uiWidget);
    }

    /**
     * Event that get triggered when the apply button is cliked to push proxy
     * properties to the server.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event apply
     */
    function fireApply(uiWidget) {
        getPipeline(uiWidget).trigger('apply');
        dataChanged(uiWidget);
    }

    /**
     * Event that get triggered when the reset button is cliked to update
     * ui widget with previous values.
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event reset
     */
    function fireReset(uiWidget) {
        getPipeline(uiWidget).trigger('reset');
    }

    /**
     * Event triggered when anything in the pipeline has changed and therefore
     * a viewport update should occurs.
     *
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event dataChanged
     */

    function dataChanged(uiWidget) {
        getPipeline(uiWidget).trigger('dataChanged');
    }

    /**
     * Event triggered when the pipeline browser changes busy states
     *
     * @member jQuery.paraview.ui.PipelineBrowser
     * @event busy
     * @param {Boolean} status
     * True when pipeline is busy, false otherwise
     */

    function fireBusy(uiWidget, isBusy) {
        busyStatus += (isBusy ? 1 : -1);
        getPipeline(uiWidget).trigger({
            type: 'busy',
            status: (busyStatus > 0)
        });
    }

    // =======================================================================
    // ==== Data Model Access ================================================
    // =======================================================================

    function updateIndexMap(pipelineBrowser, parentId, childrenList) {
        var indexObject = pipelineBrowser.data('proxyIndexMaps'), proxy, idx;
        if(indexObject === null || indexObject === undefined) {
            indexObject = {
                'ProxyIdToProxy': {},
                'ProxyIdToParentId': {}
            };
            pipelineBrowser.data('proxyIndexMaps', indexObject);
        }

        // Create fake root node that has the children
        if(parentId === 0) {
            indexObject.ProxyIdToProxy['0'] = {
                'children': childrenList
            };
        }

        // Fill maps
        for(idx in childrenList) {
            proxy = childrenList[idx];
            indexObject.ProxyIdToProxy[proxy.proxy_id.toString()] = proxy;
            indexObject.ProxyIdToParentId[proxy.proxy_id.toString()] = parentId;
            if(proxy.hasOwnProperty("children")) {
                updateIndexMap(pipelineBrowser, proxy.proxy_id, proxy.children);
            }
        }
    }

    // =======================================================================

    function generateDisabledLut(dataArray) {
        var lutArray = [];
        if(dataArray != null) {
            for(var idx in dataArray) {
                lutArray.push( {
                    id: (dataArray[idx].name + '_' + dataArray[idx].size),
                    name: dataArray[idx].name,
                    size: dataArray[idx].size,
                    enabled: 0
                });
            }
        }
        return lutArray;
    }

    // =======================================================================

    function getActiveLookupTable(proxy) {
        var activeData = proxy.activeData, name, arrays = null;
        if(activeData.indexOf('POINT_DATA:') === 0 ) {
            arrays = proxy.pointData;
        } else if(activeData.indexOf('CELL_DATA:') === 0) {
            arrays = proxy.cellData;
        }

        if(arrays != null) {
            name = activeData.split(':')[1];
            for(var idx in arrays) {
                if(arrays[idx].name === name) {
                    return {
                        id: (name + '_' + arrays[idx].size),
                        name: name,
                        size: arrays[idx].size,
                        enabled: 0
                    };
                }
            }
        }

        return null; // Color
    }

    // =======================================================================

    function generateScalarbarStatus(pipelineBrowser, changedProxy) {
        var indexObject = pipelineBrowser.data('proxyIndexMaps'), proxy, lut,
        state = {}, fullLutArray = [], lutsToEnable = {}, lutToDisable = null;

        if(changedProxy != null && !changedProxy.showScalarBar) {
            // Need to force active lut to 0
            lutToDisable = getActiveLookupTable(changedProxy);
        }

        if(indexObject != null && indexObject != undefined) {
            for(var key in indexObject.ProxyIdToProxy) {
                proxy =  indexObject.ProxyIdToProxy[key];
                if(proxy.hasOwnProperty('showScalarBar')) {
                    // Generate lut info for each data array
                    fullLutArray = fullLutArray.concat(generateDisabledLut(proxy.pointData));
                    fullLutArray = fullLutArray.concat(generateDisabledLut(proxy.cellData));

                    // Valid proxy
                    lut = getActiveLookupTable(proxy);
                    if(lut != null && proxy.showScalarBar) {
                        lutsToEnable[lut.id] = 1;
                    }
                }
            }
        }

        // Build state
        for(var idx in fullLutArray) {
            lut = fullLutArray[idx];
            state[lut.id] = lut;
            if(lutsToEnable.hasOwnProperty(lut.id)) {
                state[lut.id].enabled = 1;
            }
        }

        // Update LUT from changed proxy if need be
        if(lutToDisable != null) {
            state[lutToDisable.id].enabled = 0;
        }

        return state;
    }

    // =======================================================================

    function getProxy(anyInnerPipelineWidget, proxyId) {
        var indexObject = getPipeline(anyInnerPipelineWidget).data('proxyIndexMaps');
        return indexObject.ProxyIdToProxy[proxyId.toString()];
    }

    // =======================================================================

    function getParentProxyId(anyInnerPipelineWidget, proxyId) {
        var indexObject = getPipeline(anyInnerPipelineWidget).data('proxyIndexMaps');
        return indexObject.ProxyIdToParentId[proxyId.toString()];
    }

    // =======================================================================

    function getActiveProxyId(anyInnerPipelineWidget) {
        return getPipeline(anyInnerPipelineWidget).data('active_proxy_id');
    }

    // =======================================================================

    function setActiveProxyId(anyInnerPipelineWidget, proxyId) {
        getPipeline(anyInnerPipelineWidget).data('active_proxy_id', proxyId);
    }

    // =======================================================================

    function updateProxy(pipelineBrowser, proxy) {
        var proxyInModel, pipelineLineAfter, pipelineLineBefore;

        // Handle data model part
        proxyInModel = getProxy(pipelineBrowser, proxy.proxy_id);
        mergeProxy(proxyInModel, proxy);

        // Handle UI part
        // Update subtree

        // Generate html
        buffer.clear();
        addProxyToBuffer(proxy);

        // Update HTML
        pipelineLineBefore = $('.proxy[proxy_id=' + proxy.proxy_id + '] > .pipeline-line', pipelineBrowser);
        pipelineLineAfter = $(buffer.toString()).find('.pipeline-line');

        pipelineLineBefore.empty()
        pipelineLineBefore[0].innerHTML = pipelineLineAfter[0].innerHTML;

        // Attach listeners
        initializeListener(pipelineLineBefore);

        // Update proxy editor
        updateProxyProperties(pipelineBrowser, proxy);
    }

    // =======================================================================

    function addProxy(pipelineBrowser, parentId, newNode) {
        var container, parentProxy, newProxyContainer;

        // Handle data model part
        parentProxy = getProxy(pipelineBrowser, parentId);
        parentProxy.children.push(newNode);
        updateIndexMap(pipelineBrowser, parentId, parentProxy.children);

        // Handle UI part
        if(parentId === 0) {
            container = $('li.server > ul', pipelineBrowser);
        } else {
            proxyWidget = $('.proxy[proxy_id=' + parentId + '] > .proxy-control > .representation', pipelineBrowser);
            container = $('.proxy[proxy_id=' + parentId + '] > ul', pipelineBrowser);

            // Update data model for new representation
            parentProxy.representation = 'Hide';
            // Update UI to hide proxy
            proxyWidget.removeClass(PIPELINE_REPRESENTATION_NAMES).addClass('hide');
            // Hide parent proxy
            fireProxyChange(container, 'representation', { 'representation': 'Hide' })
        }

        // Generate html
        buffer.clear();
        addProxyToBuffer(newNode);

        // Append to children list
        container.append($(buffer.toString()));

        // Attach listeners
        initializeListener(container);

        // Set that proxy to be active
        newProxyContainer = $('.proxy[proxy_id=' + newNode['proxy_id'] + '] .proxy-control', container);
        fireProxySelected(newProxyContainer);
    }

    // =======================================================================

    function removeProxy(pipelineBrowser, proxyId) {
        // Handle data model part
        var parentProxyId, parentProxy, proxy, idxToDelete, indexObject;

        proxy = getProxy(pipelineBrowser, proxyId);
        parentProxyId = getParentProxyId(pipelineBrowser, proxyId);
        parentProxy = getProxy(pipelineBrowser, parentProxyId);

        idxToDelete = parentProxy.children.indexOf(proxy);
        parentProxy.children.splice(idxToDelete,1);

        // Remove Proxy entry in map connectivity
        indexObject = pipelineBrowser.data('proxyIndexMaps');
        delete indexObject.ProxyIdToProxy[proxyId.toString()];
        delete indexObject.ProxyIdToParentId[proxyId.toString()];

        // Handle UI part
        $('.proxy[proxy_id=' + proxyId + ']', pipelineBrowser).remove();
        updateUIPipeline(pipelineBrowser);
        $('.delete-proxy', pipelineBrowser).addClass('disabled');
        $('.property', pipelineBrowser).remove();

        setActiveProxyId(pipelineBrowser, 0);
    }

    // =======================================================================
    // ==== HTML code generators =============================================
    // =======================================================================

    function VTK2ColorRGB(rgb) {
        var tmpBuffer = createBuffer(), hexCode = '0123456789ABCDEF', value;
        tmpBuffer.append("#");
        for(var i in rgb) {
            value = rgb[i] * 255;
            tmpBuffer.append(hexCode[Math.floor(value/16)]);
            tmpBuffer.append(hexCode[Math.floor(value%16)]);
        }
        return tmpBuffer.toString();
    }

    // =======================================================================


    function addPipelineToBuffer(title, data) {
        // Build pipeline header
        buffer.append("<div class='pipeline-tree'><ul><li class='server'><div class='pipeline-line server'><div class='head-icon server'></div><div class='label'>");
        buffer.append(title);
        buffer.append("</div><div class='pipeline-control'><div class='action edit'><div class='icon' alt='Toggle visibility of the Proxy property editor' title='Toggle visibility of the Proxy property editor'></div></div><div class='action files'><div class='icon' alt='Show the list of files that can be open on the server' title='Show the list of files that can be open on the server'></div></div><div class='action add'><div class='icon' alt='Add a source or a filter to the currently selected source' title='Add a source or a filter to the currently selected source'></div></div><div class='action delete-proxy disabled'><div class='icon' alt='Delete the selected source' title='Delete the selected source'></div></div></div></div>");

        addProxiesToBuffer(data.pipeline.children);

        // Close li.server / ul
        buffer.append("</li></ul>");

        // Add floating selector
        buffer.append("<div class='representation-selector option-selector' ><div class='representation all'></div><div class='representation-overlay-selector'></div><div class='representation-overlay-label'></div></div><div class='tooltip option-selector'></div><div class='array-selector option-selector'><ul><li class='color'>Solid Color</li><li class='points'>RTData</li><li class='points'>Normal</li><li class='cells'>ElementID</li></ul><div class='colorPicker'><input class='color {slider:false, pickerPosition:'bottom'}' value='#FFFFFF'/></div></div>");

        // pipeline-tree
        buffer.append("</div>\n");


        // Build file selector
        buffer.append("<div class='pipeline-files'></div>");

        // Build source/filter selector
        buffer.append("<div class='pipeline-sources'>");
        addSourcesToBuffer(data.sources);
        buffer.append("</div>");

        // Build pipeline-editor
        buffer.append("<div class='pipeline-editor'>");
        buffer.append("<div class='pipeline-editor-header'><div class='label'>Property panel</div><div class='pipeline-control'><div class='action reset'><div class='icon' alt='Reset to default values' title='Reset to default values'></div></div><div class='action apply'><div class='icon' alt='Apply changes' title='Apply changes'></div></div></div></div>");
        buffer.append("<div class='pipeline-editor-content'></div>");
    }

    // =======================================================================

    function addProxiesToBuffer(proxyList) {
        if(proxyList === undefined || proxyList === null) {
            return;
        }
        buffer.append("<ul>");
        for(var idx in proxyList) {
            addProxyToBuffer(proxyList[idx]);
        }
        buffer.append("</ul>");
    }

    // =======================================================================

    function addProxyToBuffer(proxy) {
        if(proxy === undefined || proxy === null) {
            return;
        }
        var colorBy = '#FFFFFF', selected = '' + proxy.activeData, lut = getActiveLookupTable(proxy);

        // Handle pipeline topology
        buffer.append("<li class='proxy' proxy_id='");
        buffer.append(proxy.proxy_id);
        buffer.append("'>");

        // Add header line
        buffer.append("<div class='pipeline-line proxy-control'><div class='head-icon representation ");
        buffer.append(PIPELINE_REPRESENTATION_LABELS_TO_NAMES[proxy.representation]); // Convert PV to CSS
        buffer.append("'></div><div class='label'>");
        buffer.append(proxy.name);
        buffer.append("</div><div class='color-control'><div class='colorBy ");
        if(selected.indexOf('POINT_DATA:') === 0 && selected != 'POINT_DATA:') {
            buffer.append('points');
            colorBy = selected.split(':')[1];
        } else if (selected.indexOf('CELL_DATA:') === 0 && selected != 'CELL_DATA:') {
            buffer.append('cells');
            colorBy = selected.split(':')[1];
        } else {
            buffer.append('color');
        }
        buffer.append("' active-data='");
        buffer.append(colorBy);

        if(lut != null) {
            buffer.append("' name='");
            buffer.append(lut.name);
            buffer.append("' size='");
            buffer.append(lut.size);
        }
        buffer.append("'></div><div class='scalarbar ");
        // disable / off : scalar bar
        buffer.append((proxy.showScalarBar) ? "" : "off");
        buffer.append("'></div></div></div>");

        // Handle children if any
        if(proxy.hasOwnProperty("children")) {
            addProxiesToBuffer(proxy.children);
        }
        buffer.append("</li>");
    }

    // =======================================================================

    function addSourcesToBuffer(sourceList) {
        if(sourceList === undefined || sourceList === null) {
            return;
        }

        // Build proxy line
        var i, item;

        buffer.append("<ul>");
        for(i in sourceList) {
            item = sourceList[i];
            buffer.append("<li class='action ");
            buffer.append(item.icon);
            buffer.append("' category='");
            buffer.append(item.category);
            buffer.append("'><div class='icon'></div>");
            buffer.append(item.name);
            buffer.append("</li>");
        }
        buffer.append("</ul>");
    }

    // =======================================================================

    function updateArrayList(proxyId, container) {
        var pipeline = getPipeline(container),
        proxy = getProxy(pipeline, proxyId),
        list = $('ul', container),
        idx = 0;

        // Generate HTML
        buffer.clear();
        buffer.append("<li class='color'>Solid Color</li>\n");
        for(idx in proxy.pointData) {
            buffer.append("<li class='points' name='");
            buffer.append(proxy.pointData[idx].name);
            buffer.append("' size='");
            buffer.append(proxy.pointData[idx].size);
            buffer.append("'>");
            buffer.append(proxy.pointData[idx].name);
            buffer.append(" [");
            buffer.append(proxy.pointData[idx].size);
            buffer.append("]</li>\n");
        }
        for(idx in proxy.cellData) {
            buffer.append("<li class='cells' name='");
            buffer.append(proxy.cellData[idx].name);
            buffer.append("' size='");
            buffer.append(proxy.cellData[idx].size);
            buffer.append("'>");
            buffer.append(proxy.cellData[idx].name);
            buffer.append(" [");
            buffer.append(proxy.cellData[idx].size);
            buffer.append("]</li>\n");
        }

        list.empty()[0].innerHTML = buffer.toString();

        initializeListener(container);

        return container;
    }

    // =======================================================================
    // ==== Helper internal functions ========================================
    // =======================================================================

    function createBuffer() {
        var idx = -1, buffer = [];
        return {
            clear: function(){
                idx = -1;
                buffer = [];
            },
            append: function(str) {
                buffer[++idx] = str;
                return this;
            },
            toString: function() {
                return buffer.join('');
            }
        };
    }

    // =======================================================================

    function getProxyId(anyInnerProxyWidget) {
        var value = Number(getProxyWidget(anyInnerProxyWidget).attr('proxy_id'));
        return value ? value : 0;
    }

    // =======================================================================

    function getProxyWidget(anyInnerProxyWidget) {
        return anyInnerProxyWidget.closest('.proxy');
    }

    // =======================================================================

    function getPipeline(anyInnerPipelineWidget) {
        return anyInnerPipelineWidget.closest('.pipelineBrowser');
    }

    // =======================================================================

    function mergeProxy(proxyToUpdate, newProxy) {
        proxyToUpdate.state = newProxy.state;
        proxyToUpdate.pointData = newProxy.pointData;
        proxyToUpdate.cellData = newProxy.cellData;
        proxyToUpdate.activeData = newProxy.activeData;
        proxyToUpdate.name = newProxy.name;
        proxyToUpdate.diffuseColor = newProxy.diffuseColor;
    }

    // =======================================================================

    function getProxyPropertyPanelState(anyInnerProxyWidget, onRepresentation) {
        var pipelineBrowser = getPipeline(anyInnerProxyWidget), state = {};
        state.proxy = getActiveProxyId(pipelineBrowser);
        $('.property' + (onRepresentation?".on-representation":""), pipelineBrowser).each(function(){
            var property = $(this),
            values = [],
            value = 0,
            isNumber = false,
            type = property.attr('data-type');
            if (type != undefined) {
                isNumber = domainIsNumber(type);
            }

            // Extract property values
            $('input[type=text]', property).each(function(){
                value = isNumber ? Number($(this).val()) : $(this).val();
                values.push(value);
            });
            $('input[type=checkbox]', property).each(function(){
                values.push($(this).is(':checked') ? 1 : 0);
            });
            $('select[type=array]', property).each(function(){
                value = $(this).val();
                if(value != null) {
                    values = $(this).val().split(';');
                } else {
                    values = null;
                }
            });
            $('select[type=enum]', property).each(function(){
                value = Number($(this).val());
                values.push(value);
            });
            $('select[type=list]', property).each(function(){
                value = $(this).val();
                values.push(value);
            });

            // Build property info
            if(!state.hasOwnProperty(property.attr('proxy'))) {
                state[property.attr('proxy')] = {};
            }
            if(values != null) {
                state[property.attr('proxy')][property.attr('label')] = values;
            }
        });
        return state;
    }

    // =======================================================================
    // ==== Controller methods (event > actions) =============================
    // =======================================================================

    function attachSessionController(pipelineBrowser) {
        // Is it a pipelineBrowser
        if(!pipelineBrowser.data('pipeline').hasOwnProperty('session')) {
            // No job for us...
            return;
        }

        // Get session
        var session = pipelineBrowser.data('pipeline').session;
        if(session === null || session === undefined) {
            // No job for us...
            return;
        }

        function idle() {
            fireBusy(pipelineBrowser, false);
        }

        // Attach filter creation
        pipelineBrowser.bind('addSource', function(e) {
            var parentId = (e.parent_id ? e.parent_id : 0);
            fireBusy(pipelineBrowser, true);
            session.call('pv.pipeline.manager.proxy.add', [e.name, parentId]).then(function(newNode) {
                fireBusy(pipelineBrowser, false);
                addProxy(pipelineBrowser, parentId, newNode);
            }, idle);
        });

        // Attach pipeline reload
        pipelineBrowser.bind('reloadPipeline', function(e) {
            fireBusy(pipelineBrowser, true);
            session.call('pv.pipeline.manager.reload').then(function(rootNode) {
                fireBusy(pipelineBrowser, false);
                var pipelineLineAfter, pipelineLineBefore;

                // Update data model part
                data = pipelineBrowser.data('pipeline');
                data.pipeline = rootNode;
                updateIndexMap(pipelineBrowser, 0, rootNode.children);

                // Handle UI part
                // Update subtree

                // Generate html
                buffer.clear();
                addProxiesToBuffer(rootNode['children']);

                // Update HTML
                pipelineLineBefore = $('li.server > ul', pipelineBrowser);
                pipelineLineAfter = $(buffer.toString());

                pipelineLineBefore.empty()
                pipelineLineBefore[0].innerHTML = pipelineLineAfter[0].innerHTML;

                // Attach listeners
                initializeListener(pipelineLineBefore);

                // Update proxy editor
                setActiveProxyId(pipelineBrowser, 0);
            }, idle);
        });

        // Attach representation change
        pipelineBrowser.bind('proxyModified', function(e) {
            var changeSet = e.changeSet,
            options = {
                'proxy_id': e.proxy_id,
                'action' : e.origin
            };

            if(e.origin === 'representation') {
                if(changeSet.representation === 'Hide') {
                    options['Visibility'] = 0;
                } else {
                    options['Visibility'] = 1;
                    options['Representation'] = changeSet.representation;
                }
                session.call('pv.pipeline.manager.proxy.representation.update', [options]);
            } else if(e.origin === 'colorBy') {
                if(changeSet.hasOwnProperty('color')) {
                    // Solid color
                    var colorArray = changeSet.color.split(',');
                    for(var i in colorArray) {
                        colorArray[i] = Number(colorArray[i]);
                    }
                    options['DiffuseColor'] = colorArray;
                    options['ColorArrayName'] = '';
                } else {
                    // Data array
                    options['ColorAttributeType'] = (changeSet.array_type === 'points') ? 'POINT_DATA' : 'CELL_DATA';
                    options['ColorArrayName'] = changeSet.array;
                }

                // Update server
                fireBusy(pipelineBrowser, true);
                session.call('pv.pipeline.manager.proxy.representation.update', [options]).then(function(){
                    session.call('pv.pipeline.manager.scalarbar.visibility.update').then(function(status){
                        pipelineBrowser.data('scalarbars', status);
                        updateUIPipeline(pipelineBrowser);
                        updateScalarBarUI(pipelineBrowser, status);
                        fireBusy(pipelineBrowser, false);
                    }, idle);
                }, idle);
            } else if(e.origin === 'scalarbar') {
                fireBusy(pipelineBrowser, true);
                session.call('pv.pipeline.manager.scalarbar.visibility.update', [changeSet]).then(function(status){
                    fireBusy(pipelineBrowser, false);
                    pipelineBrowser.data('scalarbars', status);
                    updateScalarBarUI(pipelineBrowser, status);
                }, idle);
            } else if(e.origin === 'property') {
                for(var key in changeSet) {
                    options[key] = changeSet[key];
                }
                fireBusy(pipelineBrowser, true);
                session.call('pv.pipeline.manager.proxy.update', [options]).then(function(newState){
                    fireBusy(pipelineBrowser, false);
                    updateProxy(pipelineBrowser, newState);
                }, idle);
            } else if(e.origin === 'representation-property') {
                for(var key in changeSet) {
                    options[key] = changeSet[key];
                }
                session.call('pv.pipeline.manager.proxy.representation.update', [options]);
            }
        });

        // Attach delete action
        pipelineBrowser.bind('deleteProxy', function(e) {
            fireBusy(pipelineBrowser, true);
            session.call('pv.pipeline.manager.proxy.delete', [e.proxy_id]).then(function(){
                removeProxy(pipelineBrowser, e.proxy_id);

                var fullLutStatus = pipelineBrowser.data('scalarbars'),
                localStatus = generateScalarbarStatus(pipelineBrowser, null),
                lutToDelete = {}, needToUpdateServer = false;
                for(var key in fullLutStatus) {
                    if(!localStatus.hasOwnProperty(key)) {
                        lutToDelete[key] =  fullLutStatus[key];
                        lutToDelete[key].enabled = 0;
                        needToUpdateServer = true;
                    } else if(fullLutStatus[key].enabled != 0 && localStatus[key].enabled == 0){
                        lutToDelete[key] = fullLutStatus[key];
                        lutToDelete[key].enabled = 0;
                        needToUpdateServer = true;
                    }
                }
                fireBusy(pipelineBrowser, false);
                if(needToUpdateServer) {
                    fireBusy(pipelineBrowser, true);
                    session.call('pv.pipeline.manager.scalarbar.visibility.update', [lutToDelete]).then(function(status){
                        updateScalarBarUI(pipelineBrowser, status);
                        fireBusy(pipelineBrowser, false);
                    }, idle);
                }
            }, idle);

        });

        // Attach property editing
        pipelineBrowser.bind('apply', function() {
            var proxyState = getProxyPropertyPanelState(pipelineBrowser, false),
            repState = getProxyPropertyPanelState(pipelineBrowser, true),
            repState_OK = {};

            // Convert representation props
            repState_OK['proxy_id'] = repState['proxy'];
            for(var key in repState[repState['proxy']]) {
                repState_OK[key] = repState[repState['proxy']][key];
            }
            getProxy(pipelineBrowser, repState_OK['proxy_id']).opacity = repState_OK['Opacity'][0];

            fireBusy(pipelineBrowser, true);
            session.call('pv.pipeline.manager.proxy.update', [proxyState]).then(function(newState){
                fireBusy(pipelineBrowser, false);
                session.call('pv.pipeline.manager.proxy.representation.update', [repState_OK]).then(function(){
                    newState.opacity = repState_OK['Opacity'][0];
                    updateProxy(pipelineBrowser, newState);
                });
            }, idle);
        });

        pipelineBrowser.bind('reset', function() {
            var activeProxyId = getActiveProxyId(pipelineBrowser),
            activeProxy = getProxy(pipelineBrowser, activeProxyId);
            updateProxyProperties(pipelineBrowser, activeProxy);
        });
    }

    // =======================================================================
    // ==== UI Listeners =====================================================
    // =======================================================================

    function updateUIPipeline(pipelineBrowser) {
        // Update Pipeline UI tree look
        $('li.proxy', pipelineBrowser).removeClass('lastChild open');
        $('li.proxy:last-child', pipelineBrowser).addClass('lastChild');
        $('li.proxy > ul > li.proxy', pipelineBrowser).parent().parent().addClass('open');

        // Handle delete button status
        var selectedProxy = $('.proxy > div.selected', pipelineBrowser);
        if(selectedProxy.length > 0) {
            $('.server .delete-proxy', pipelineBrowser).removeClass('disabled');
            selectedProxy.parent().children('ul').children('li').closest('.server').find('.delete-proxy').addClass('disabled');
        } else {
            $('.server .delete-proxy', pipelineBrowser).addClass('disabled');
        }

    }

    // =======================================================================

    function updateScalarBarUI(pipelineBrowser, state) {
        // Update scalarbar visibility
        $('.scalarbar', pipelineBrowser).each(function(){
            var me = $(this), lutId, colorBy = me.siblings();

            if(colorBy.attr('name') === undefined) {
                me.addClass('disable');
            } else {
                lutId = colorBy.attr('name') + '_' + colorBy.attr('size');
                if(state.hasOwnProperty(lutId) && state[lutId].enabled != 0) {
                    me.removeClass('disable off');
                } else {
                    me.removeClass('disable').addClass('off');
                }
            }
        });
    }

    // =======================================================================

    function initializeListener(container) {
        var pipelineBrowser = getPipeline(container),
        selectedProxy = getActiveProxyId(pipelineBrowser),
        colorPicker = pipelineBrowser.data('colorPicker');

        updateUIPipeline(pipelineBrowser);
        updateScalarBarUI(pipelineBrowser,generateScalarbarStatus(pipelineBrowser, null));

        // ============ Update UI to show selected proxy ===========

        if(selectedProxy === 0) {
            $('.proxy > div', pipelineBrowser).removeClass('selected');
        } else {
            $('.proxy[proxy_id=' + selectedProxy +'] > div', pipelineBrowser).addClass('selected');
        }

        // ============ Representations ============

        $(".representation:not(.all)", pipelineBrowser).unbind().click(function(e){
            var me = $(this), selector = $(".representation-selector", pipelineBrowser);
            if(me.hasClass('active') && selector.is(':visible')) {
                selector.hide();
                $(".representation", pipelineBrowser).removeClass("active");
            } else {
                $(".representation", pipelineBrowser).removeClass("active");
                me.addClass("active");
                selector.css("left", me.offset().left /*- 4*/ + 5 + me.width() ).css("top", me.offset().top - pipelineBrowser.offset().top - 5).show();
            }
        });

        // ============ Representations Selection ============

        $(".representation-selector", pipelineBrowser).unbind().click(function(e){
            var me = $(this),
            pos = Math.floor((e.pageX - me.offset().left)/34),
            representation = PIPELINE_REPRESENTATION_NAMES.split(' ')[pos],
            proxyWidget = $('.representation.active', pipelineBrowser);

            // Update UI
            proxyWidget.removeClass(PIPELINE_REPRESENTATION_NAMES).addClass(representation);
            me.hide();

            // Trigger event
            fireProxyChange(proxyWidget, 'representation', {
                'representation': PIPELINE_REPRESENTATION_NAMES_TO_LABELS[representation]
            } );
        }).mousemove(function(e){
            var me = $(this),
            pos = Math.floor((e.pageX - me.offset().left)/34),
            label = $('.representation-overlay-label', pipelineBrowser),
            selector = $(".representation-overlay-selector", pipelineBrowser);
            if(pos < PIPELINE_REPRESENTATION_NAMES.split(' ').length) {
                label.empty().html(PIPELINE_REPRESENTATION_NAMES_TO_LABELS[PIPELINE_REPRESENTATION_NAMES.split(' ')[pos]].replace(/ /g,'&nbsp;')).css("left", me.width() + 10).show();
                selector.css("left", 34*pos - 1).show();
            }
        });

        // ============ Color By / Array Selection ============

        $(".colorBy.color", pipelineBrowser).each(function(){
            // UpdateColor
            var me = $(this),
            proxyId = getProxyId(me),
            proxy = getProxy(me, proxyId),
            colorArray = eval(proxy.diffuseColor),
            color = VTK2ColorRGB(colorArray);

            me.css("background", color);
        });

        $(".colorBy", pipelineBrowser).unbind().hover(function(){
            // in
            var me = $(this), tooltip = $('.tooltip', pipelineBrowser);
            if(me.hasClass("color")) {
            // Nothing to do
            } else if(me.attr('active-data')) {
                tooltip.empty().html(me.attr('active-data'));
                tooltip.css("left", me.offset().left - 15 - tooltip.width()).css("top", me.offset().top - pipelineBrowser.offset().top - 3).show();
            }
        }, function() {
            // out
            $('.tooltip', pipelineBrowser).empty().hide();
        }).click(function(){
            var me = $(this),
            selector = $(".array-selector", pipelineBrowser),
            delta = 0,
            colorBy = $(".colorBy", pipelineBrowser);

            if(me.hasClass('active') && ((jscolor.picker && $(jscolor.picker.boxB).is(':visible')) || selector.is(':visible'))) {
                colorPicker = pipelineBrowser.data('colorPicker');
                colorPicker.hidePicker();
                selector.hide();

                colorBy.removeClass("active");
                pipelineBrowser.data('colorPicker').hidePicker();

                // Trigger event for color
                if(me.hasClass('color')) {
                    fireProxyChange(me, 'colorBy', {
                        'color': me.attr('active-data')
                    });
                }
            } else {
                colorBy.removeClass("active");
                me.addClass("active");

                $('.array-selector', pipelineBrowser).show();

                if(me.hasClass('color')) {
                    delta = 6;
                }
                selector.css("left", me.offset().left + me.width() + delta - selector.width()).css("top", me.offset().top + me.height() + delta - pipelineBrowser.offset().top + 5);
                updateArrayList(getProxyId(me),selector).show();
            }
        });

        // ============ Scalar bar ============

        $(".scalarbar", pipelineBrowser).unbind().click(function(){
            var me = $(this), state = {},
            proxyId = getProxyId(me), proxy = getProxy(me, proxyId);
            if(!me.hasClass("disable")) {
                proxy.showScalarBar = !proxy.showScalarBar;
                state = generateScalarbarStatus(pipelineBrowser, proxy);

                fireProxyChange(me, 'scalarbar', state);
                updateScalarBarUI(pipelineBrowser, state);
            }
        });

        // ============ Array Selection ============

        $(".array-selector", pipelineBrowser).unbind().hover(function(){}, function(){
            if($('ul', this).is(':visible')) {
                $(this).hide();
            }
        });

        $(".array-selector li", pipelineBrowser).unbind().click(function(){
            // Select array action
            var me = $(this), activeColorBy = $(".colorBy.active", pipelineBrowser),
            colorPicker, colorPickerWidget,
            proxyId = getProxyId(activeColorBy),
            proxy = getProxy(activeColorBy, proxyId);
            activeColorBy.attr('active-data', me.html()).removeClass(PIPELINE_COLOR_BY_TYPES).addClass(me.attr('class'));
            if(me.hasClass('color')) {
                // We are not done, we need to pick a color
                colorPicker = pipelineBrowser.data('colorPicker');
                colorPicker.showPicker();
                colorPickerWidget = $(jscolor.picker.boxB);
                colorPickerWidget.css('top', activeColorBy.offset().top - 2).css('left', activeColorBy.offset().left - colorPickerWidget.width() - 10);
                $('.array-selector', pipelineBrowser).hide();
                activeColorBy.removeAttr('name').removeAttr('size');
                proxy.activeData = '#';
                proxy.showScalarBar = false;
            } else {
                $(".array-selector", pipelineBrowser).hide();
                activeColorBy.attr('name', me.attr('name')).attr('size', me.attr('size'));
                proxy.activeData = (me.attr('class') === 'points' ? 'POINT_DATA:' : 'CELL_DATA:') + me.attr('name');

                // Trigger event
                fireProxyChange(activeColorBy, 'colorBy', {
                    'array': me.attr('name'),
                    'number_of_components': me.attr('size'),
                    'array_type': me.attr('class')
                });
            }

            // Update scalar bar visibility
            var scalarBarState = generateScalarbarStatus(pipelineBrowser);
            fireProxyChange(pipelineBrowser, 'scalarbar', scalarBarState);
            updateScalarBarUI(pipelineBrowser,scalarBarState);
        });

        // ============ Color picker ============

        try {
            if(colorPicker === null || colorPicker === undefined) {
                colorPicker = new jscolor.color($('input.color')[0], {
                    slider: false,
                    onImmediateChange: updateColorCallback
                });
                pipelineBrowser.data('colorPicker', colorPicker);
            }
        } catch(error) {
            console.log("No color picker library");
            console.log(error);
        }

        function updateColorCallback() {
            var colorPicker = pipelineBrowser.data('colorPicker'),
            proxyWidget = $(".colorBy.active", pipelineBrowser),
            color = "#" + colorPicker.toString();
            proxyWidget.attr('active-data', colorPicker.rgb).css("background", color);
        }

        // ============ Proxy Selection ============

        $('.proxy-control', pipelineBrowser).unbind().click(function(){
            var me = $(this), selector = $('.option-selector:visible', pipelineBrowser);
            if(getProxyId($('.proxy-control.selected', pipelineBrowser)) === getProxyId(me)) {
                return;
            }

            $('.proxy-control', pipelineBrowser).removeClass('selected');
            me.addClass('selected');

            if(selector && getProxyId(me) != getProxyId($('.active', pipelineBrowser))) {
                selector.hide();
            }

            // Trigger new selected proxy
            fireProxySelected(me);
        });

        // ============= Toggle Panel visibility ===========

        $('.pipeline-control .files', pipelineBrowser).unbind().click(function(){
            var me = $(this), pipeline = getPipeline(me), activeView = 'view-files';
            if(pipeline.hasClass('view-files')) {
                activeView = 'view-pipeline';
            }

            pipeline.removeClass(PIPELINE_VIEW_TYPES).addClass(activeView);
            me.toggleClass('active');
            if(me.hasClass('active')) {
                $('.pipeline-control .add', pipelineBrowser).removeClass('active');
            }
        });

        $('.pipeline-control .add', pipelineBrowser).unbind().click(function(){
            var me = $(this), pipeline = getPipeline(me), activeView = 'view-sources';
            if(pipeline.hasClass('view-sources')) {
                activeView = 'view-pipeline';
            }

            pipeline.removeClass(PIPELINE_VIEW_TYPES).addClass(activeView);
            me.toggleClass('active');
            if(me.hasClass('active')) {
                $('.pipeline-control .files', pipelineBrowser).removeClass('active');
            }
        });

        $('.pipeline-control .edit', pipelineBrowser).unbind().click(function(){
            var me = $(this), pipeline = getPipeline(me);
            pipeline.toggleClass('view-pipeline-editor');
            me.toggleClass('active');
        });

        // ============= Source/Filter browsing ===========

        $(".pipeline-sources .action", pipelineBrowser).unbind().click(function(){
            var me = $(this),
            pipeline = getPipeline(me),
            toggleButton = $('.add.active', pipeline);

            pipeline.removeClass(PIPELINE_VIEW_TYPES).addClass('view-pipeline');
            toggleButton.removeClass('active');


            var parentId = (me.attr('category') === 'filter') ? getActiveProxyId(pipelineBrowser) : 0;
            fireAddSource(pipeline, me.text(), parentId);
        });

        // ============= Delete Selected Proxy ===========

        $('.delete-proxy', pipelineBrowser).unbind().click(function(){
            var me = $(this);
            if(!me.hasClass('disabled')) {
                fireDeleteProxy(me);
            }
        });

        // ============= Apply / Reset ===========

        $('.apply', pipelineBrowser).unbind().click(function(){
            var me = $(this);
            me.removeClass('modified');
            fireApply(me);
        });

        $('.reset', pipelineBrowser).unbind().click(function(){
            fireReset($(this));
        });

        // ============= Invalidate / Reload Pipeline ===========

        $('.head-icon.server',pipelineBrowser).unbind().click(function(){
            fireReloadPipeline($(this));
        });

        // ============= Representation properties ===========

        $('.pipeline-editor-representation input[type="range"]').change(function(){
            var me = $(this),
            name = me.attr('name'),
            value = Number(me.val()) / 100,
            changeSet = {};
            changeSet[name] = value;
            fireProxyChange(me, 'representation-property', changeSet)
        });
    }

    // =======================================================================
    // ==== Property Panel - HTML code generators ============================
    // =======================================================================

    function domainIsNumber(domainName){
        return (domainName.indexOf('Int') != -1) || (domainName.indexOf('Double') != -1);
    }

    // =======================================================================

    function domainHasRange(domainName){
        return (domainName.indexOf('Range') != -1);
    }

    // =======================================================================

    function isInputArrayDomain(domainList) {
        for(var idx in domainList) {
            if(domainList[idx].type === 'ArrayList') {
                return true;
            }
        }
        return false;
    }

    // =======================================================================

    function isEnumDomain(domainList) {
        for(var idx in domainList) {
            if(domainList[idx].type === 'Enumeration') {
                return true;
            }
        }
        return false;
    }

    // =======================================================================

    function isProxyListDomain(domainList) {
        for(var idx in domainList) {
            if(domainList[idx].type === 'ProxyList') {
                return true;
            }
        }
        return false;
    }

    // =======================================================================

    function getInputArrayNumberOfComponents(domainList) {
        for(var idx in domainList) {
            if(domainList[idx].type === 'ArrayList') {
                return domainList[idx].nb_components;
            }
        }
        return -1;
    }

    // =======================================================================

    function updateProxyProperties(pipelineBrowser, proxy) {
        var me = $(".pipeline-editor-content",pipelineBrowser).empty(), key, value,
        opacityValue = proxy.opacity;
        if(opacityValue === undefined) {
            opacityValue = 1;
        }

        buffer.clear();
        buffer.append("<table>");

        if(proxy && proxy.state) {
            // Add Opacity property

            addPropertyToBuffer(proxy.state['proxy_id'], 'Opacity', opacityValue, {
                "default_values": "1",
                "domains": [{ "max":"1", "type":"DoubleRange", "min":"0"} ],
                "name": "Opacity",
                "order": 100,
                "size": "1",
                "type": "Double"
            }, true);

            // Other properties
            for(key in proxy.state.properties) {
                if(proxy.state.domains.hasOwnProperty(key)) {
                    addPropertyToBuffer(proxy.state['proxy_id'], key, proxy.state.properties[key], proxy.state.domains[key], false);
                } else {
                    addPropertyToBuffer(proxy.state['proxy_id'], key, proxy.state.properties[key], null, false);
                }
            }
        }
        buffer.append("</table>");
        me[0].innerHTML = buffer.toString();

        // Reorder the properties
        $(".property", pipelineBrowser).each(function(){
            updatePropertyPosition($(this));
        });

        generateWidget(me);

        $('.apply', pipelineBrowser).removeClass('modified');
    }

    // =======================================================================

    function markProxyModified(anyInnerPipelineWidget) {
        var pipelineBrowser = getPipeline(anyInnerPipelineWidget);
        $('.apply', pipelineBrowser).addClass('modified');
    }

    // =======================================================================

    function addPropertyToBuffer(proxyId, key, value, domain, onRepresentation) {
        var idx, filterList = ['proxy_id', 'type', 'domains'], nbComponents;

        for(idx in filterList) {
            if(key === filterList[idx]) {
                return;
            }
        }
        if(domain === null) {
            if(value.hasOwnProperty('proxy_id')) {
                buffer.append("<tr class='sub-proxy'><table>");
                for(var key2 in value.properties) {
                    addPropertyToBuffer(value['proxy_id'], key2, value.properties[key2], value.domains[key2], onRepresentation);
                }
                buffer.append("</table></tr>");
            }
        } else {
            buffer.append("<tr class='property REP' name='".replace(/REP/g, onRepresentation ? "on-representation" : ""));
            buffer.append(key);
            buffer.append("' proxy='");
            buffer.append(proxyId);
            buffer.append("' data-value='");
            buffer.append(value);
            buffer.append("' order='");
            buffer.append(domain['order']);
            buffer.append("' label='");
            if(domain.hasOwnProperty('label') && domain['label'] && domain['label'].length > 0) {
                buffer.append(domain['label']);
            } else {
                buffer.append(key);
            }
            buffer.append("' widget-type='");

            if(domain['domains'].length == 1 && domain['domains'][0]['type'] == 'Boolean') {
                buffer.append("boolean'");
            } else if (domain['domains'].length == 1 && domainHasRange(domain['domains'][0]['type']) && domain['domains'][0].hasOwnProperty('min') && domain['domains'][0].hasOwnProperty('max')) {
                buffer.append('range');
                buffer.append("' min='");
                buffer.append(domain['domains'][0]['min']);
                buffer.append("' max='");
                buffer.append(domain['domains'][0]['max']);
                buffer.append("' data-type='");
                buffer.append(domain['type']);
                buffer.append("'");
            } else if (isInputArrayDomain(domain['domains'])) {
                // Handle input array
                nbComponents = getInputArrayNumberOfComponents(domain['domains']);
                buffer.append("array' nb_comp='");
                buffer.append(nbComponents);
                buffer.append("' proxy='");
                buffer.append(proxyId);
                buffer.append("' selected_array_type='");
                buffer.append(value[0]);
                buffer.append("' selected_array='");
                buffer.append(value[1]);
                buffer.append("'");
            } else if (isEnumDomain(domain['domains'])) {
                buffer.append("enum' key='");
                buffer.append(key);
                buffer.append("'");
            } else if (isProxyListDomain(domain['domains'])) {
                buffer.append("list' key='");
                buffer.append(key);
                buffer.append("'");
            } else if (domain.hasOwnProperty('size')) {
                if(domain['size'] === '0') {
                    buffer.append('multi-value');
                } else {
                    buffer.append('text');
                }
                buffer.append("' size='");
                buffer.append(domain['size']);
                buffer.append("' data-type='");
                buffer.append(domain['type']);
                buffer.append("'");
            } else {
                buffer.append("?'");
            }

            buffer.append("></tr>");
        }
    }

    // =======================================================================

    function generateWidget(container) {
        $('.property', container).each(function(){
            var property = $(this).empty();
            var propertyName = property.attr('label');

            var propertyValue = property.attr('data-value');
            createWidget(property, propertyName, propertyValue);
        });
    }

    // =======================================================================

    function updatePropertyPosition(propertyElement) {
        var other = propertyElement.next();
        while(other.length == 1) {
            if(Number(propertyElement.attr('order')) > Number(other.attr('order'))) {
                other.after(propertyElement);
            }
            other = other.next();
        }
    }

    // =======================================================================

    function createWidget(container, propertyName, value) {
        var widgetType = container.attr('widget-type');
        if(widgetType === 'boolean') {
            createCheckbox(container, propertyName, value);
        } else if (widgetType === 'text') {
            createTextField(container, propertyName, container.attr('size'), container.attr('data-type'), value);
        } else if (widgetType === 'enum') {
            createEnumeration(container, container.attr('proxy'), propertyName, container.attr('key'), value);
        } else if (widgetType === 'list') {
            createList(container, container.attr('proxy'), propertyName, container.attr('key'), value);
        } else if (widgetType === 'range') {
            createSlider(container, propertyName, container.attr('min'), container.attr('max'), container.attr('data-type'), value)
        } else if (widgetType === 'array') {
            createArraySelector(container, propertyName, container.attr('proxy'), Number(container.attr('nb_comp')), container.attr('selected_array_type'), container.attr('selected_array'));
        } else if (widgetType === 'multi-value') {
            var array = value.length === 0 ? [] : value.split(',');
            createMultiValue(container, propertyName, array);

        }
    }

    // =======================================================================

    function createSliderOLD(container, propertyName, min, max, type, propertyValue) {
        tmpBuffer = createBuffer();
        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td class='pv-widget text-1'><input type='text' value='");
        tmpBuffer.append(propertyValue);
        tmpBuffer.append("'><div class='pv-widget-slider'></div></td>");
        container[0].innerHTML = tmpBuffer.toString();
        var updateValue = function(event, ui) {
            $('input', container).val(ui.value);
            markProxyModified(container);
        }
        $('input', container).change(function(){
            $('div.pv-widget-slider', container).slider('value', Number($(this).val()));
        });
        var options = {
            min: Number(min),
            max: Number(max),
            value: Number(propertyValue),
            slide: updateValue,
            change: updateValue
        };
        if(type === 'Int') {
            options['step'] = 1;
        }
        $('div.pv-widget-slider', container).slider(options);
    }

    // =======================================================================

    function createSlider(container, propertyName, min, max, type, propertyValue) {
        var minValue = Number(min),
        maxValue = Number(max),
        deltaValue = maxValue - minValue;

        function sliderValueToPropertyValue(v) {
            var sv = deltaValue * Number(v) / 100.0 + minValue;
            if(type === 'Int') {
                return Math.floor(sv);
            }
            return sv;
        }

        function propertyValueToSliderValue(v) {
            return 100 * (Number(v) - minValue) / deltaValue ;
        }

        container[0].innerHTML =
            "<td class='title'>NAME</td><td class='pv-widget text-1'><input type='text' value='VALUE'><div class='pv-widget-slider'><input type='range' min='0' max='100' value='SLIDE' data-min='MIN' data-max='MAX' data-value='VALUE'/></div></td>"
            .replace(/VALUE/g, propertyValue)
            .replace(/NAME/g, propertyName)
            .replace(/MIN/g, min)
            .replace(/MAX/g, max)
            .replace(/VALUE/g, propertyValue)
            .replace(/SLIDE/g, propertyValueToSliderValue(propertyValue));

        var textProperty = $('input[type="text"]', container),
        sliderProperty = $('input[type="range"]', container);


        function invalidateProperty(newValue) {
            textProperty.val(newValue);
            sliderProperty.attr('data-value', newValue).val(propertyValueToSliderValue(newValue));
            markProxyModified(container);
        }

        $('input[type="text"]', container).change(function() {
            var me = $(this),
            value = me.val();
            invalidateProperty(value);
        });

        $('input[type="range"]', container).bind('change keyup', function() {
            var me = $(this),
            value = sliderValueToPropertyValue(me.val());
            invalidateProperty(value);
        });
    }

    // =======================================================================

    function createTextField(container, propertyName, nbFields, type, propertyValue) {
        var values = [], tmpBuffer = createBuffer();
        if(nbFields === 1) {
            values = [ propertyValue ]
        } else {
            values =  propertyValue.split(',');
        }

        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td class='pv-widget text-");
        tmpBuffer.append(nbFields);
        tmpBuffer.append("'>");
        for(var i = 0; i < nbFields; ++i) {
            tmpBuffer.append("<input type='text' value='");
            tmpBuffer.append(values[i]);
            tmpBuffer.append("'>");
        }
        tmpBuffer.append("</td>");

        container[0].innerHTML = tmpBuffer.toString();
        $('input', container).change(function(){
            markProxyModified(container);
        });
    }

    // =======================================================================

    function createCheckbox(container, propertyName, propertyValue) {
        var tmpBuffer = createBuffer();
        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td class='pv-widget'>");
        tmpBuffer.append("<input type='checkbox' ");
        if(propertyValue == 1) {
            tmpBuffer.append(" checked");
        }
        tmpBuffer.append("></td>");

        container[0].innerHTML = tmpBuffer.toString();
        $('input', container).change(function(){
            markProxyModified(container);
        });
    }

    // =======================================================================

    function createEnumeration(container, proxyId, propertyLabel, propertyName, propertyValue) {
        var tmpBuffer = createBuffer(), proxy = getProxy(container, proxyId),
        list = proxy.state.domains[propertyName].domains[0]['enum'];
        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td class='pv-widget'><select type='enum'>");
        for(var i in list) {
            tmpBuffer.append("<option value='");
            tmpBuffer.append(list[i]['value']);
            tmpBuffer.append("'");
            if(propertyValue === list[i]['text']) {
                tmpBuffer.append(" SELECTED");
            }
            tmpBuffer.append(">");
            tmpBuffer.append(list[i]['text']);
            tmpBuffer.append("</option>");
        }
        tmpBuffer.append("</select></td>");

        container[0].innerHTML = tmpBuffer.toString();
        $('select', container).change(function(){
            markProxyModified(container);
        });
    }

    // =======================================================================

    function createList(container, proxyId, propertyLabel, propertyName, propertyValue)
    {
        var tmpBuffer = createBuffer(), proxy = getProxy(container, proxyId),
        list = [], domains = proxy.state.domains[propertyName].domains, idx;

        // Search the list domain
        for(idx in domains) {
            if(domains[idx].hasOwnProperty('list')) {
                list = domains[idx].list
            }
        }

        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyLabel);
        tmpBuffer.append("</td><td class='pv-widget'><select type='list'>");
        for(var i in list) {
            tmpBuffer.append("<option value='");
            tmpBuffer.append(list[i]);
            tmpBuffer.append("'");
            if(propertyValue === list[i]) {
                tmpBuffer.append(" SELECTED");
            }
            tmpBuffer.append(">");
            tmpBuffer.append(list[i]);
            tmpBuffer.append("</option>");
        }
        tmpBuffer.append("</select></td>");

        container[0].innerHTML = tmpBuffer.toString();
        $('select', container).change(function(){
            markProxyModified(container);
        });
    }

    // =======================================================================

    function createArraySelector(container, propertyName, proxyId, nbComponents, arrayType, arrayName) {
        var tmpBuffer = createBuffer(), list, isArrayInList,
        parentProxy = getProxy(container, getParentProxyId(container, proxyId));

        tmpBuffer.append("<td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td class='pv-widget'><select type='array'>");

        // Point data
        list = parentProxy.pointData;
        isArrayInList = (arrayType === 'POINTS');
        for(var i in list) {
            if(list[i]['size'] != nbComponents) {
                continue;
            }

            tmpBuffer.append("<option value='POINTS;");
            tmpBuffer.append(list[i]['name']);
            tmpBuffer.append("'");
            if(isArrayInList && arrayName === list[i]['name']) {
                tmpBuffer.append(" SELECTED");
            }
            tmpBuffer.append(">");
            tmpBuffer.append(list[i]['name']);
            tmpBuffer.append("</option>");
        }

        // Cell data
        list = parentProxy.cellData;
        isArrayInList = (arrayType === 'CELLS');
        for(var i in list) {
            if(list[i]['size'] != nbComponents) {
                continue;
            }

            tmpBuffer.append("<option value='CELLS;");
            tmpBuffer.append(list[i]['name']);
            tmpBuffer.append("'");
            if(isArrayInList && arrayName === list[i]['name']) {
                tmpBuffer.append(" SELECTED");
            }
            tmpBuffer.append(">");
            tmpBuffer.append(list[i]['name']);
            tmpBuffer.append("</option>");
        }

        tmpBuffer.append("</select></td>");

        container[0].innerHTML = tmpBuffer.toString();
        $('input', container).change(function(){
            markProxyModified(container);
        });
    }

    // =======================================================================

    function createMultiValue(container, propertyName, propertyValue) {
        var tmpBuffer = createBuffer(), emptyLineHTML = "<tr><td></td><td><input class='multi-value' type='text' value='0.0'></td><td class='delete-value'></td></tr>";

        function addEntry() {
            attachListener($(emptyLineHTML).appendTo($('table', container)));
            markProxyModified(container);
        }

        function attachListener(parent) {
            $('input', parent).change(function(){
                markProxyModified(container);
            });
            $('.add', parent).click(addEntry);
            $('.delete-value', parent).click(function(){
                $(this).parent().remove();
                markProxyModified(container);
            });
        }

        // First line with title
        tmpBuffer.append("<td colspan='2'><table class='multi-value' style='width: 100%;'><tr><td class='title'>");
        tmpBuffer.append(propertyName);
        tmpBuffer.append("</td><td><input class='multi-value' type='text'></td><td class='add'></td></tr>");
        tmpBuffer.append("</table></td>");

        container[0].innerHTML = tmpBuffer.toString();
        attachListener(container);

        // Update values
        for(var idx = 1; idx < propertyValue.length; idx++) {
            addEntry();
        }
        for(idx in propertyValue) {
            $('input.multi-value', container).eq(idx).val(propertyValue[idx]);
        }
    }

    // ----------------------------------------------------------------------
    // Local module registration
    // ----------------------------------------------------------------------
    try {
      // Tests for presence of jQuery, then registers this module
      if ($ !== undefined) {
        vtkWeb.registerModule('paraview-ui-pipeline');
      } else {
        console.error('Module failed to register, jQuery is missing: ' + err.message);
      }
    } catch(err) {
      console.error('Caught exception while registering module: ' + err.message);
    }

}(window, jQuery));
