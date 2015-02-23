(function (GLOBAL, $) {
    var session,
        viewport,
        pipeline,
        proxyEditor,
        settingsEditor,
        rvSettingsProxyId = null,
        saveOptionsEditor,
        defaultSaveFilenames = { 'data': 'server-data/savedData.vtk', 'state': 'server-state/savedState.pvsm', 'screen': 'server-images/savedScreen.png' },
        saveTypesMap = {
            'AMR Dataset (Deprecated)': 'vtm',
            'Composite Dataset': 'vtm',
            'Hierarchical DataSet (Deprecated)': 'vtm',
            'Image (Uniform Rectilinear Grid) with blanking': 'vti',
            'Image (Uniform Rectilinear Grid)': 'vti',
            'Multi-block Dataset': 'vtm',
            'Multi-group Dataset': 'vtm',
            'Multi-piece Dataset': 'vtm',
            'Non-Overlapping AMR Dataset': 'vtm',
            'Overlapping AMR Dataset': 'vtm',
            'Point Set': 'vts',
            'Polygonal Mesh': 'vtp',
            'Polygonal Mesh': 'vtp',
            'Rectilinear Grid': 'vtr',
            'Structured (Curvilinear) Grid': 'vts',
            'Structured Grid': 'vts',
            'Table': 'csv',
            'Unstructured Grid': 'vtu'
        },
        currentSaveType = 'state',
        infoManager,
        busyElement = $('.busy').hide(),
        notBusyElement = $('.not-busy').show(),
        busyCount = 0,
        paletteNameList = [],
        pipelineDataModel = { metadata: null, source: null, representation: null, view: null, sources: []},
        activeProxyId = 0,
        module = {},
        vcrPlayStatus = false,
        pipelineLoadedCallBack = null,
        error = function(e){workDone();console.log(e);};

    // ========================================================================
    // Main callback
    // ========================================================================

    function invalidatePipeline(arg) {
        if(arg && arg.hasOwnProperty('id')) {
            // Update active proxy in pipeline
            activeProxy(arg.id);
        }
        pipeline.trigger('pipeline-reload');
        viewport.resetViewId();
        viewport.invalidateScene();
        workDone();
    }

    // ========================================================================
    // Possible callback methods
    // ========================================================================

    function onProxyCreationDone() {
        // Handle parent visibility if need be
        if(pipelineDataModel.source.hasOwnProperty('hints') && pipelineDataModel.source.hints.hasOwnProperty('replaceInput')) {
            var replaceStrategie = pipelineDataModel.source.hints.replaceInput;
            if(replaceStrategie > 0) {
                // We are not handling the case with 2 but we could eventually
                // For 2 we need to check representation Opacity and Representation property state
                startWorking();
                session.call('pv.proxy.manager.update', [[{ 'name': 'Visibility', 'value': 0, 'id': getProxyMetaData(pipelineDataModel.metadata.parent).rep } ]]).then(invalidatePipeline, invalidatePipeline);
            }
        }
    }

    // ========================================================================
    // Busy feedback
    // ========================================================================

    function startWorking() {
        busyCount++;
        if(busyCount === 1) {
            notBusyElement.hide();
            busyElement.show();
        }
    }

    // ------------------------------------------------------------------------

    function workDone() {
        busyCount--;
        if(busyCount === 0) {
            busyElement.hide();
            notBusyElement.show();
        }
    }

    // ========================================================================
    // Helper methods
    // ========================================================================

    function updatePaletteNames() {
        session.call('pv.color.manager.list.preset', []).then(function(list){
            paletteNameList = list;
        });
    }

    // ------------------------------------------------------------------------

    function activeProxy(newActiveProxy) {
        if(newActiveProxy) {
            pipeline.data('pipeline').active_proxy = newActiveProxy;
        }
        return pipeline.data('pipeline').active_proxy;
    }

    // ------------------------------------------------------------------------

    function getProxyMetaData(id) {
        var list = pipelineDataModel.sources,
            count = list.length;

        while(count--) {
            if(list[count].id === id) {
                return list[count];
            }
        }
        return null;
    }

    // ------------------------------------------------------------------------

    function loadProxy(proxy_id, name) {
        if(session) {
            startWorking();
            session.call("pv.proxy.manager.get", [proxy_id]).then(function(proxy){
                workDone();
                pipelineDataModel[name] = proxy;
                onNewProxyLoaded();
            }, error);
        }
    }

    // ------------------------------------------------------------------------

    function activePipelineInspector() {
        $('.inspector-selector[data-type="pipeline"]').trigger('click');
        $('.inspector-container').scrollTop(0);
    }

    // ------------------------------------------------------------------------

    function addFixHeightBehavior() {
        $(window).resize(function() {
            var windowHeight = $(window).height();
            $('.fix-height').each(function(){
                var delta = this.parentNode.getBoundingClientRect().top,
                    me = $(this);

                me.css('height', (windowHeight - delta) + 'px');
                if(me.hasClass('shift-top')) {
                    me.css('top', delta + 'px');
                }
                updateView();
            });
        }).trigger('resize');
    }

    // ------------------------------------------------------------------------

    function addDefaultButtonsBehavior() {
        var inspectorContainer = $('.inspector-container'),
            inspectors = $('.inspector');

        // Toggle inspector visibility
        $('.toggle-inspector').click(function(){
            if(inspectorContainer.is(':visible')) {
                inspectorContainer.hide();
            } else {
                inspectorContainer.show();
            }
        });

        // Toggle inspector visibility
        $('.inspector-selector').click(function(){
            var me = $(this),
                activeType = me.attr('data-type');

            inspectors.hide();
            $('.inspector[data-type="TYPE"]'.replace(/TYPE/g, activeType)).show();
            inspectorContainer.show();

            if(activeType === 'pipeline') {
                inspectorContainer.scrollTop(0);
            }
        });

        // Reset camera
        $('.reset-camera').click(resetCamera);
    }

    // ------------------------------------------------------------------------

    function addScrollBehavior() {
        $('.inspector-container').scroll(function (event) {
            var y = $(this).scrollTop();
                reservedHeight = 0;
            if ($('.pv-pipeline').is(':visible') === true) {
                reservedHeight = $('.pv-pipeline').height();
            } else if ($('.pv-preferences').is(':visible') === true) {
                reservedHeight = $('.pv-preferences').height();
            }
            if (y >= reservedHeight) {
                $('.pv-editor-bar').addClass('pv-fixed').css('top', (y-reservedHeight)+'px');
            } else {
                $('.pv-editor-bar').removeClass('pv-fixed');
            }
        });
    }

    // ========================================================================
    // Preference Panel (Renderer Type...)
    // ========================================================================

    function addPreferencePanelBehavior() {
        $('.pv-update-viewport').bind('change', function(){
            var me = $(this),
                propName = me.attr('data-property-name'),
                value = me.hasClass('checkbox') ? (me.prop('checked') ? 1 : 0) : me.val();
            if(propName === 'ActiveRendererType') {
                if (value === 'image') {
                    $('.local-only').hide();
                } else {
                    $('.local-only').show();
                }
                viewport.setActiveRenderer(value);
                viewport.invalidateScene();
            } else if(propName === 'Stats') {
                viewport.showStatistics(value === 1);
            } else if(propName === 'CloseBehavior') {
                if(value === 1) {
                    reallyStop = true;
                } else {
                    reallyStop = false;
                }
            }
        });
    }

    // ========================================================================
    // Time / Animation feedback
    // ========================================================================

    function addTimeAnimationButtonsBehavior() {
        // Action management
        $('.vcr-play').click(onTimeAnimationPlay);
        $('.vcr-stop').hide().click(onTimeAnimationStop);
        $('.vcr-action').click(onTimeAnimationAction);
        $('.vcr-fetch-times').click(onRetrieveTimesteps);
        $('.vcr-clear-cache').click(onClearTimestepCache);

        // Handle visibility toggle
        $('.toggle-time-toolbar').click(function(){
            var toolbar = $('.vcr-control'),
                me = $(this);
            if(me.hasClass('pv-text-red')) {
                $(this).removeClass('pv-text-red');
                toolbar.hide();
            } else {
                toolbar.show();
                $(this).addClass('pv-text-red');
            }
        });

        // Update default state
        $('.vcr-control').hide();
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationAction() {
        startWorking()
        session.call('pv.vcr.action', [ $(this).attr('data-action')]).then(function(timeValue){
            $('.time-value').val(timeValue);
            updateView();
            workDone();
        }, error);
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationPlay() {
        $('.vcr-play').hide();
        $('.vcr-stop').show();
        vcrPlayStatus = true;
        runTimeAnimationLoop();
    }

    // ------------------------------------------------------------------------

    function onTimeAnimationStop() {
        $('.vcr-play').show();
        $('.vcr-stop').hide();
        vcrPlayStatus = false;
    }

    // ------------------------------------------------------------------------

    function runTimeAnimationLoop() {
        if(vcrPlayStatus) {
            session.call('pv.vcr.action', ['next']).then(function(timeValue){
                $('.time-value').val(timeValue);
                updateView();
                setTimeout(runTimeAnimationLoop, 50);
            });
        }
    }

    // ------------------------------------------------------------------------

    function onRetrieveTimesteps() {
        viewport.downloadTimestepData();
    }

    // ------------------------------------------------------------------------

    function onClearTimestepCache() {
        viewport.clearGeometryCache();
    }

    // ========================================================================
    // Proxy Creation management (sources + filters)
    // ========================================================================

    function createCreationView(selector, category) {
        var panelContainer = $(selector);

        function onDataCreation(action) {
            if(session) {
                startWorking()
                session.call('pv.proxy.manager.create', [action.value, activeProxy()]).then(function(arg){
                    pipelineLoadedCallBack = onProxyCreationDone;
                    invalidatePipeline(arg);
                    activePipelineInspector();
                }, error);
            }
        }

        startWorking();
        session.call('pv.proxy.manager.available',[category]).then(function(list){
            var actionList = [],
                size = list.length;

            list.sort();
            for(var idx = 0; idx < size; ++idx) {
                actionList.push({ id: list[idx], label: list[idx], value: list[idx]});
            }
            panelContainer.actionList({actions:actionList});
            workDone();
        }, error);

        panelContainer.bind('action', onDataCreation);
    }

    // ========================================================================
    // Data Information Panel
    // ========================================================================

    function createDataInformationPanel(dataInfoSelector) {
        infoManager = $(dataInfoSelector).empty();
    }

    // ------------------------------------------------------------------------

    function updateDataInformationPanel(data) {
        if(data) {
            $('.active-data-type-label').text(data.type);
            updateSaveDataFilename(data.type);
            infoManager.pvDataInformation(data);
        } else {
            infoManager.empty();
        }
    }

    // ========================================================================
    // ViewPort management (active + visibility)
    // ========================================================================

    function createViewportView(viewportSelector) {
        $(viewportSelector).empty()
                           .bind('captured-screenshot-ready', onScreenshotCaptured);
        viewport = vtkWeb.createViewport({session: session});
        viewport.bind(viewportSelector);
    }

    // ------------------------------------------------------------------------

    function updateView() {
        if(viewport) {
            viewport.invalidateScene();
        }
    }

    // ------------------------------------------------------------------------

    function resetCamera() {
        if(viewport) {
            viewport.resetCamera();
        }
    }


    // ========================================================================
    // Pipeline management (active + visibility)
    // ========================================================================

    function createPipelineManagerView(pipelineSelector) {
        pipeline = $(pipelineSelector);
        pipeline.pipelineSVG({session: session});
        pipeline.bind('pipeline-visibility-change', onProxyVisibilityChange);
        pipeline.bind('pipeline-data-change', onPipelineDataChange);
        pipeline.trigger('pipeline-reload');
    }

    // ------------------------------------------------------------------------

    function onProxyVisibilityChange(proxy) {
        updateView();
    }

   // ------------------------------------------------------------------------

    function onPipelineDataChange(event) {
        // { active: active_proxy_id, view: view_id, sources: proxy_list }
        activeProxyId = event.active;

        // Update data model
        pipelineDataModel.sources = event.sources;

        // Handle the new active proxy
        if(event.active === '0') {
            $('.need-input-source').hide();
            proxyEditor.empty();
            updateView();
        } else {
            $('.need-input-source').show();
            pipelineDataModel.metadata = getProxyMetaData(event.active);
            pipelineDataModel.source = null;
            pipelineDataModel.representation = null;
            if(pipelineDataModel.metadata) {
                loadProxy(pipelineDataModel.metadata.id, 'source');
                loadProxy(pipelineDataModel.metadata.rep, 'representation');
                if(pipelineDataModel.view === null) {
                    loadProxy(event.view, 'view');
                }
            }
        }
    }

    // ========================================================================
    // Global settings management
    // ========================================================================

    function reloadSettingsEditor() {
        startWorking();
        settingsEditor.empty();

        if (rvSettingsProxyId !== null) {
            session.call('pv.proxy.manager.get', [rvSettingsProxyId]).then(function(rvSettingsProps) {

                var props = [].concat(
                    "+RenderViewSettings", rvSettingsProps.properties, '_RenderViewSettings'
                    ),
                ui = [].concat(
                    "+RenderViewSettings", rvSettingsProps.ui, '_RenderViewSettings'
                    );

                settingsEditor.proxyEditor("Global Settings", false, 0, props, ui, [], [], {});

                workDone();
            }, function(rvSettingsErr) {
                console.log("Failed to get the RenderViewSettings proxy properties:");
                console.log(rvSettingsErr);
                workDone();
            });
        }
    }

    function createGlobalSettingsPanel(settingsSelector) {
        startWorking();
        settingsEditor = $(settingsSelector);

        // Get the proxy id of the RenderViewSettings proxy
        session.call('pv.proxy.manager.find.id', ['settings', 'RenderViewSettings']).then(function(proxyId) {
            rvSettingsProxyId = proxyId;
            workDone();
            reloadSettingsEditor();
        }, function(proxyIdErr) {
            console.log('Error getting RenderViewSettings proxy id:');
            console.log(proxyIdErr);
            workDone();
        });

        // To apply render view settings, first update the proxy, then reload it
        settingsEditor.bind('apply', function(evt) {
            startWorking();
            session.call('pv.proxy.manager.update', [evt.properties]).then(function(updateResult) {
                console.log("Successfully updated RenderViewSettings proxy:")
                console.log(updateResult);
                viewport.invalidateScene();
                reloadSettingsEditor();
                workDone();
            }, function(updateError) {
                console.log("Failed to update RenderViewSettings proxy:")
                console.log(updateError);
                workDone();
            });
        });
    }

    // ========================================================================
    // Proxy Editor management (update + delete)
    // ========================================================================

    function createProxyEditorView(proxyEditorSelector) {
        proxyEditor = $(proxyEditorSelector);
        proxyEditor.bind('delete-proxy', onProxyDelete);
        proxyEditor.bind('apply', onProxyApply);
        proxyEditor.bind('scalarbar-visibility', onScalarBarVisibility);
        proxyEditor.bind('rescale-transfer-function', onRescaleTransferFunction);
        proxyEditor.bind('update-scalar-opacity-function', onUpdateOpacityPoints);
        proxyEditor.bind('store-scalar-opacity-parameters', onStoreOpacityParameters);
        proxyEditor.bind('initialize-scalar-opacity-widget', onInitializeScalarOpacityWidget);
        proxyEditor.bind('request-scalar-range', onRequestScalarRange);
        proxyEditor.bind('push-new-surface-opacity', onSurfaceOpacityChanged);
        proxyEditor.bind('initialize-color-editor-widget', onInitializeColorEditorWidget);
        proxyEditor.bind('update-rgb-points', onUpdateRgbPoints);
    }

    // ------------------------------------------------------------------------

    function onProxyDelete(event) {
        startWorking();
        session.call('pv.proxy.manager.delete', [event.id]).then(function(result) {
            invalidatePipeline(result);
            // Make sure all old tooltips are cleaned up...
            $('.tooltip').remove();
        }, invalidatePipeline);
    }

    // ------------------------------------------------------------------------

    function onProxyApply(event) {
        startWorking();
        session.call('pv.proxy.manager.update', [event.properties]).then(invalidatePipeline, invalidatePipeline);

        // Args: representation, colorMode, arrayLocation='POINTS', arrayName='', vectorMode='Magnitude', vectorComponent = 0, rescale=False
        var args = [].concat(event.colorBy.representation, event.colorBy.mode, event.colorBy.array, event.colorBy.component);
        startWorking();
        session.call('pv.color.manager.color.by', args).then(invalidatePipeline, error);
        // Update palette ?
        if(event.colorBy.palette) {
            startWorking();
            session.call('pv.color.manager.select.preset', [ event.colorBy.representation, event.colorBy.palette ]).then(invalidatePipeline, error);
        }
    }

    // ------------------------------------------------------------------------

    function onScalarBarVisibility(event) {
        startWorking();
        var visibilityMap = {};
        visibilityMap[event.id] = event.visible;
        session.call('pv.color.manager.scalarbar.visibility.set', [visibilityMap]).then(invalidatePipeline, invalidatePipeline);
    }

    // ------------------------------------------------------------------------

    function onUpdateOpacityPoints(event) {
        var colorBy = event.colorBy;
        if (colorBy.array.length >= 2 && colorBy.array[1] !== '') {
            var args = [colorBy.array[1], event.points];
            startWorking();
            session.call('pv.color.manager.opacity.points.set', args).then(function(successResult) {
                viewport.invalidateScene();
                workDone();
            }, error);
        }
    }

    // ------------------------------------------------------------------------

    function onSurfaceOpacityChanged(event) {
        var colorBy = event.colorBy;
        if (colorBy.array.length >= 2 && colorBy.array[1] !== '') {
            var args = [colorBy.representation, (event.opacity === true ? 1 : 0)];
            startWorking();
            session.call('pv.color.manager.surface.opacity.set', args).then(function(successResult) {
                viewport.invalidateScene();
                workDone();
            }, error);
        }
    }

    // ------------------------------------------------------------------------

    function onStoreOpacityParameters(event) {
        var colorArray = event.colorBy.array;
        if (colorArray.length >= 2 && colorArray[1] !== '') {
            var storeKey = colorArray[1] + ":opacityParameters";
            var args = [storeKey, event.parameters];
            startWorking();
            session.call('pv.keyvaluepair.store', args).then(workDone, error);
        }
    }

    // ------------------------------------------------------------------------

    function onRequestScalarRange(event) {
        var proxyId = event.proxyId;
        startWorking();
        session.call('pv.color.manager.scalar.range.get', [proxyId]).then(function(curScalarRange) {
            proxyEditor.trigger({
                'type': 'update-scalar-range-values',
                'min': curScalarRange.min,
                'max': curScalarRange.max
            });
            workDone();
        }, error);
    }

    // ------------------------------------------------------------------------

    function onRescaleTransferFunction(event) {
        startWorking();
        var options = { proxyId: event.id, type: event.mode };
        if(event.mode === 'custom') {
            options.min = event.min;
            options.max = event.max;
        }
        session.call('pv.color.manager.rescale.transfer.function', [options]).then(function(successResult) {
            if (successResult['success'] === true) {
                viewport.invalidateScene();

                proxyEditor.trigger({
                    'type': 'update-scalar-range-values',
                    'min': successResult.range.min,
                    'max': successResult.range.max
                });

                session.call('pv.color.manager.rgb.points.get', [event.colorBy.array[1]]).then(function(result) {
                    proxyEditor.trigger({
                        'type': 'notify-new-rgb-points-received',
                        'rgbpoints': result
                    });
                    workDone();
                }, error);
            } else {
                workDone();
            }
        }, error);
    }

    // ------------------------------------------------------------------------

    function extractRepresentation(list) {
        var count = list.length;
        while(count--) {
            if(list[count].name === 'Representation') {
                return [list[count]];
            }
        }
        return [];
    }

    // ------------------------------------------------------------------------

    function onNewProxyLoaded() {
        if(pipelineDataModel.metadata && pipelineDataModel.source && pipelineDataModel.representation && pipelineDataModel.view) {
            var colorBy = pipelineDataModel.representation.colorBy,
                widgetKey = 'pv.proxy.editor.settings',
                widgetSettings = retrieveWidgetSettings(widgetKey),
                options = { 'widgetKey': widgetKey, 'widgetData': widgetSettings },
                srcVal = widgetSettings['Source'] || "+Source",
                repVal = widgetSettings['Representation'] || "-Representation",
                viewVal = widgetSettings['View'] || "-View",
                colMgmtVal = widgetSettings['Color Management'] || "+Color Management",
                props = [].concat(
                    srcVal, pipelineDataModel.source.properties, '_Source',
                    repVal, pipelineDataModel.representation.properties, '_Representation',
                    viewVal, pipelineDataModel.view.properties, "_View"
                    ),
                ui = [].concat(
                    srcVal, pipelineDataModel.source.ui, '_Source',
                    repVal, pipelineDataModel.representation.ui, '_Representation',
                    viewVal, pipelineDataModel.view.ui, "_View"
                    );

            if (!$.isEmptyObject(colorBy) && colorBy.hasOwnProperty('array')) {
                props = [].concat(colMgmtVal,
                                  extractRepresentation(pipelineDataModel.representation.properties),
                                  "ColorByPanel",
                                  "_Color Management",
                                  props);
                ui = [].concat(colMgmtVal,
                               extractRepresentation(pipelineDataModel.representation.ui),
                               "ColorByPanel",
                               "_Color Management",
                               ui);
            }

            try {
                proxyEditor.proxyEditor(pipelineDataModel.metadata.name,
                                        pipelineDataModel.metadata.leaf,
                                        pipelineDataModel.metadata.id,
                                        props,
                                        ui,
                                        pipelineDataModel.source.data.arrays,
                                        paletteNameList,
                                        colorBy,
                                        options);
                proxyEditor.bind('store-widget-settings', onStoreWidgetSettings);
                $('.inspector-container').scrollTop(0);
            } catch(err) {
                console.log(err);
            }
            // Update information section
            updateDataInformationPanel(pipelineDataModel.source.data);

            // Handle callback if any
            if(pipelineLoadedCallBack) {
                pipelineLoadedCallBack();
                pipelineLoadedCallBack = null;
            }

            // Handle automatic reset camera
            if(pipelineDataModel.sources.length === 1) {
                resetCamera();
            } else {
                updateView();
            }
        }
    }

    // ------------------------------------------------------------------------

    function onStoreWidgetSettings(event) {
        vtkWeb.storeApplicationDataObject(event.widgetKey, event.widgetData);
    }

    function retrieveWidgetSettings(widgetKey) {
        return vtkWeb.retrieveApplicationDataObject(widgetKey);
    }

    // ========================================================================
    // Color editor widget creation
    // ========================================================================

    function onInitializeColorEditorWidget(event) {
        var container = event.container,
            colorArray = event.colorBy.array,
            initOptions = {
                'topMargin': 10,
                'rightMargin': 15,
                'bottomMargin': 10,
                'leftMargin': 15,
                'widgetKey': 'pv.color.editor.settings'
            };

        if (colorArray.length >= 2 && colorArray[1] !== '') {
            startWorking();
            session.call('pv.color.manager.rgb.points.get', [colorArray[1]]).then(function(result) {
                initOptions['rgbInfo'] = result;
                initOptions['widgetData'] = retrieveWidgetSettings(initOptions['widgetKey']);
                container.colorEditor(initOptions);
                container.bind('store-widget-settings', onStoreWidgetSettings);
                workDone();
            }, error);
        } else {
            console.log("WARNING: Initializing the color editor while not coloring by an array.");
            container.colorEditor(initOptions);
        }
    }

    function onUpdateRgbPoints(event) {
        var arrayName = event.colorBy.array[1];
        var rgbInfo = event.rgbInfo;

        startWorking();
        session.call('pv.color.manager.rgb.points.set', [arrayName, rgbInfo]).then(function(result) {
            workDone();
            viewport.invalidateScene();
        }, error);
    }

    // ========================================================================
    // Opacity editor widget creation
    // ========================================================================
    function onInitializeScalarOpacityWidget(event) {
        var container = event.container,
            colorArray = event.colorBy.array,
            needParams = [ 'currentPointSet', 'surfaceOpacityEnabled' ],
            initOptions = {
                'buttonsPosition': 'top',
                'topMargin': 10,
                'rightMargin': 15,
                'bottomMargin': 10,
                'leftMargin': 15,
                'widgetKey': 'pv.opacity.editor.settings'
            };

        function gotInitParam(paramName) {
            needParams.splice(needParams.indexOf(paramName), 1);
            if (needParams.length === 0) {
                initOptions['widgetData'] = retrieveWidgetSettings(initOptions['widgetKey']);
                container.opacityEditor(initOptions);
                container.bind('store-widget-settings', onStoreWidgetSettings);
            }
        }

        if (colorArray.length >= 2 && colorArray[1] !== '') {

            var retrieveKey = colorArray[1] + ":opacityParameters";
            session.call('pv.keyvaluepair.retrieve', [retrieveKey]).then(function(result) {
                if (result !== null) {
                    initOptions.gaussiansList = result.gaussianPoints;
                    initOptions.linearPoints = result.linearPoints;
                    initOptions.gaussianMode = result.gaussianMode;
                }
                gotInitParam('currentPointSet');
                workDone();
            }, error);

            var representation = event.colorBy.representation;
            session.call('pv.color.manager.surface.opacity.get', [representation]).then(function(result) {
                if (result !== null) {
                    initOptions.surfaceOpacityEnabled = (result === 1 ? true : false);
                }
                gotInitParam('surfaceOpacityEnabled');
                workDone();
            }, error);

        } else {
            console.log("WARNING: Initializing the opacity editor while not coloring by an array.");
            container.opacityEditor(initOptions);
        }
    }

    // ========================================================================
    // Panel for saving data, state, and screenshots
    // ========================================================================

    function createSaveOptionsPanel(saveSelector) {
        saveOptionsEditor = $(saveSelector);

        $('.active-panel-btn').click(setActiveSaveProperties);
        $('.save-data-button').click(saveDataClicked);
        $('.screenshot-reset-size-btn').click(updateSaveScreenshotDimensions);
        $('.screenshot-grab-image-btn').click(viewport.captureScreenImage);

        $('.screenshot-pixel-width').val(500);
        $('.screenshot-pixel-height').val(500);

        // To begin with, we select the "Save State" panel
        $('.active-panel-btn[data-action=screen]').trigger('click');
    }

    function onScreenshotCaptured(event) {
        var imgElt = $('.captured-screenshot-image');
        imgElt.attr('src', event.imageData);
        imgElt.removeClass('hidden');
    }

    function updateSaveScreenshotDimensions() {
        var viewportElt = $('.pv-viewport');
        $('.screenshot-pixel-width').val(viewportElt.width());
        $('.screenshot-pixel-height').val(viewportElt.height());
    }

    function saveDataClicked(event) {
        var filename = $('.save-data-filename').val(),
            saveOptions = {};

        if (currentSaveType === 'state') {
            // No special options yet
        } else if (currentSaveType === 'data') {
            if (activeProxyId !== '0') {
                saveOptions['proxyId'] = activeProxyId;
            }
        } else if (currentSaveType === 'screen') {
            console.log("Going to save screenshot (" + filename + ")");
            saveOptions.size = [ $('.screenshot-pixel-width').val(), $('.screenshot-pixel-height').val() ];
        }

        defaultSaveFilenames[currentSaveType] = filename;

        startWorking()
        session.call('pv.data.save', [filename, saveOptions]).then(function(saveResult) {
            if (saveResult.success !== true) {
                alert(saveResult.message);
            }
            workDone();
        }, error);
    }

    function updateSaveDataFilename(activeType) {
        var xmlExt = saveTypesMap[activeType] || 'vtk';
        var replName = defaultSaveFilenames['data'].replace(/\.[^\.]+$/, '.' + xmlExt);
        defaultSaveFilenames['data'] = replName;
        if ($('.active[data-action=data]').length === 1) {
            // Additionally, if 'data' is the active save panel, update the text field itself
            $('.save-data-filename').val(replName);
        }
    }

    function setActiveSaveProperties(event) {
        var target_container = $(event.target)
            action = target_container.attr('data-action');

        //$('.active-panel-btn').toggleClass('active');
        $('.active-panel-btn').removeClass('active');
        target_container.addClass('active');

        if (action === 'screen') {
            $('.screenshot-save-only').show();
            $('.data-save-only').hide();
        } else if (action === 'data') {
            $('.data-save-only').show();
            $('.screenshot-save-only').hide();
        } else {
            $('.data-save-only').hide();
            $('.screenshot-save-only').hide();
        }

        $('.save-data-filename').val(defaultSaveFilenames[action]);
        currentSaveType = action;
    }

    // ========================================================================
    // File management (browse + open)
    // ========================================================================

    function createFileManagerView(fileSelector) {
        var fileManagerContainer = $(fileSelector);

        function onFileAction(event) {
            var action = event.action,
                basePath = event.path,
                files = event.files;

            if(action === 'directory' || action === 'path') {
                if(basePath.length === 0) {
                    startWorking();
                    session.call('file.server.directory.list', ['.']).then(function(dirContent){
                        fileManagerContainer.fileBrowser2(dirContent);
                        workDone();
                    }, error);
                } else {
                    var fullPath = [].concat(basePath.split('/'), files).join('/');
                    startWorking();
                    session.call('file.server.directory.list', [fullPath]).then(function(dirContent){
                        fileManagerContainer.fileBrowser2(dirContent);
                        workDone();
                    }, error);
                }
            } else if(action === 'file') {
                var fullpathFileList = [],
                    list = files.split(','),
                    count = list.length;
                for(var i = 0; i < count; ++i) {
                    fullpathFileList.push([].concat(basePath.split('/').slice(1), list[i]).join('/'));
                }
                startWorking();
                session.call("pv.proxy.manager.create.reader", [fullpathFileList]).then(invalidatePipeline, invalidatePipeline);
                activePipelineInspector();
            }
        }

        // Get initial content and handle events internally
        startWorking();
        session.call('file.server.directory.list', ['.']).then(function(dirContent) {
            fileManagerContainer.fileBrowser2(dirContent);
            fileManagerContainer.bind('file-action', onFileAction);
            workDone();
        }, error);
    }

    // ========================================================================
    // Main - Visualizer Setup
    // ========================================================================

    function initializeVisualizer(session_, viewportSelector, pipelineSelector, proxyEditorSelector, fileSelector,
                                  sourceSelector, filterSelector, dataInfoSelector, settingsSelector, saveOptsSelector) {
        session = session_;

        // Initialize data and DOM behavior
        updatePaletteNames();
        addScrollBehavior();
        addDefaultButtonsBehavior();
        addTimeAnimationButtonsBehavior();
        addPreferencePanelBehavior();

        // Create panels
        createFileManagerView(fileSelector);
        createCreationView(sourceSelector, 'sources');
        createCreationView(filterSelector, 'filters');
        createViewportView(viewportSelector);
        createPipelineManagerView(pipelineSelector);
        createProxyEditorView(proxyEditorSelector);
        createDataInformationPanel(dataInfoSelector);
        createGlobalSettingsPanel(settingsSelector);
        createSaveOptionsPanel(saveOptsSelector);

        // Set initial state
        $('.need-input-source').hide();
        proxyEditor.empty();
        activePipelineInspector();

        // Make sure everything is properly sized
        addFixHeightBehavior();
    };

    // ----------------------------------------------------------------------
    // Init vtkWeb module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("pv")) {
        module = GLOBAL.pv || {};
    } else {
        GLOBAL.pv = module;
    }

    // Expose some methods to pv namespace
    module.initializeVisualizer = initializeVisualizer;

}(window, jQuery));
