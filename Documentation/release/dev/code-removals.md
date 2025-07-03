## Removals in ParaView 6.1

* In `pqAddToFavoritesReaction`, the constructor `pqAddToFavoritesReaction(QAction* parent, QVector<QString>& filters)` has been removed. Favorites are now integrated into Categories, and should be initialized from a `pqProxyCategory` instead.
* In `pqCategoryToolbarsBehavior`, the slot `void prepareForTest()` was removed as it was unused.
* In `pqColorMapEditor`, the slot `void setDataRepresentation(pqDataRepresentation* repr, bool forceUpdate = false)` was removed. Use `setRepresentation()` instead.
* In `pqDataAssemblyPropertyWidget`, the properties `selectorColors`, `compositeIndexColors`, `selectorOpacities`, and `compositeIndexOpacities` and their associated accessors and signals have been removed.
* In `pqLiveSourceBehavior`, the static member functions `void pause()`, `void resume()`, and `bool isPaused()` have been removed. Use functions of the same name in `pqLiveSourceManager` instead.
* Class `pqManageFavoritesReaction` has been removed. Favorites should be replaced by Categories configuration. See pqConfigureCategories instead.
* In `pqProxyGroupMenuManager`, `vtkSMProxy* getPrototype(QAction* action) const` has been removed in favor of `static vtkSMProxy* GetProxyPrototype(QAction*)`, `bool hideForTests(const QString&) const` and `void setEnabled(bool)` have been removed because they were mostly unused, `void populateFavoritesMenu()`, `void loadFavoritesItems()`, and `QAction* getAddToFavoritesAction(const QString& path)` have been removed because favorites are now a category configurable like other categories, and the protected `Enabled` member variable has been removed because it is unused.
* Class `pqFavoritesDialog` and its associated Qt `.ui` file have been deleted.
* Class `pqQuickLaunchDialog` has been removed. Please use `pqQuickLaunchDialogExtended` instead.
* Class `pqWaitCursor` has been removed. Please use `pqScopedOverrideCursor(Qt::WaitCursor)` instead.
* In `vtkSMAnimationScene`, member function `void SetDuration(int)` has been removed. Use `SetStride(int)` instead.
* In `vtkInitializationHelper`,
  * Member function `static bool InitializeMiscellaneous(int)` has been removed. Use `InitializeSettings()` and `InitializeOthers()` instead.
  * Member function `static std::string GetUserSettingsDirectory()` has been removed. Use `vtkPVStandardPaths::GetUserSettingsDirectory()` instead.
  * Member function `static std::string GetUserSettingsFilePath()` has has been removed. Use `vtkPVStandardPaths::GetUserSettingsFilePath()` instead.
* In `vtkGeometryRepresentation`, member function `virtual void SetFlipTextures(bool)` has been removed. Use `SetTextureTransform()` instead to flip a texture.
* In `vtkPolarAxesRepresentation`, accessors for `EnableCustomRadius` have been removed. Use accessors for `EnableCustomMinRadius` instead.
* In `vtkSMColorMapEditorHelper`, member function `static vtkSMProxy* GetLUTProxy(vtkSMProxy*, vtkSMProxy*)` has been removed and replaced with `GetLookupTable(vtkSMProxy*, vtkSMProxy*)`.
* In `vtkSMPVRepresentationProxy`,
  * Member function `void SetLastLUTProxy(vtkSMProxy*)` has been removed and replaced with `SetLastLookupTable(vtkSMProxy*)`.
  * Member function `vtkSMProxy* GetLastLUTProxy()` has been removed and replaced with `vtkSMProxy* GetLastLookupTable()`.
  * Member function `vtkSMProxy* GetLUTProxy(vtkSMProxy*)` has been removed and replaced with `vtkSMProxy* GetLookupTable()` instead.
* In `vtkPVGeometryFilter`, the following information keys have been removed because they were not used:
  * `static vtkInformationIntegerVectorKey* POINT_OFFSETS(`)
  * `static vtkInformationIntegerVectorKey* VERTS_OFFSETS(`)
  * `static vtkInformationIntegerVectorKey* LINES_OFFSETS(`)
  * `static vtkInformationIntegerVectorKey* POLYS_OFFSETS(`)
  * `static vtkInformationIntegerVectorKey* STRIPS_OFFSETS()`.

  Also in `vtkPVGeometryFilter`, member function `void CleanupOutputData(vtkPolyData* , int)` has been replaced with `void CleanupOutputData(vtkPolyData*)`, and member function `void ExecuteCellNormals(vtkPolyData* output, int doCommunicate)` has been replaced with `void ExecuteNormalsComputation(vtkPolyData*)`.

A number of filter proxies have been removed and replaced:
* **AppendArcLength**, replace with **PolyLineLength**.
* **GenerateIds**, replace with **PointAndCellIds**.
* **PolyDataTanges**, replace with **SurfaceTangents**.
* **AppendLocationAttributes**, replace with **Coordinates**.
* **BlockIdScalars**, replace with **BlockIds**.
* **OverlappingLevelId Scalars**, replace with **OverlappingAMRLevelIds**.
* **GenerateSpatioTemporalHarmonics**, replace with **SpatioTemporalHarmonics**.
* **PolyDataNormals**, replace with **SurfaceNormals**.
* **ComputeConnectedSurfaceProperties**, repalce with **ConnectedSurfaceProperties**.
* **GenerateProcessIds**, replace with **ProcessIds**.
* **GhostCellsGenerator**, replace with **GhostCells**.
* **GenerateGlobalIds**, replace with **GlobalPointAndCellIds**.
* **AddFieldArrays**, replace with **FieldArrayFromFile**.
