## vtkProjectSpectrumMagnitude filter can use octave bands

You can now specify octave bands for automatic frequency range computation for the `vtkProjectSpectrumMagnitude` filter. If
you select *Frequencies From Octave*, three new options will appear.

* *Base Two Octave*: allows you to specify whether to compute frequencies using base-two or base-ten power. It's an
  **advanced** parameter.
* *Octave*: allows you to choose which octave-band you want to project, in audible spectrum.
* *Octave Subdivision*: allows you to choose which part of the octave you're interested in (e.g. a full octave or a
  third-octave).

### Examples

* Selecting the *500 Hz* octave with the *Full* octave subdivision and *Base Two* On will result in a frequency range of
  (353.553, 707.107).
* Selecting the *500 Hz* octave with the *Second Third* octave subdivision and *Base Two* Off will result in a frequency
  range of (446.684, 562.341).
