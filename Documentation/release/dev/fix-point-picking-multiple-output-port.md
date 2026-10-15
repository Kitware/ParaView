## Fix point picking on sources with multiple output ports

`vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface` now correctly intersects the picked output port,
instead of always using the first one, when the source has multiple output ports.
