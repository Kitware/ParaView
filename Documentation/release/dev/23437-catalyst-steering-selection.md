## Catalyst steering extractors follow pipeline selection

When `catalyst_execute` selects pipelines through `catalyst/state/pipelines`, `catalyst_results` now only returns the steering extractors of pipelines that executed in that call. Steering extractors of pipelines that were not selected, or whose execution failed, are left out of the results node and are no longer computed during `catalyst_results`. Previously, every steering extractor was serialized, so a deselected pipeline was still run in `catalyst_results` and deselecting it saved no time.

Without `catalyst/state/pipelines`, all pipelines execute and all steering extractors are returned, as before. Steerable parameters are not affected.
