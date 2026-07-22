# Third-party notices

Potato uses Qt dynamically. Qt licensing notices and the exact redistributed Qt
framework licenses are copied into the application bundle during packaging.
Qt 6.8.8 is a commercial LTS patch; producing or distributing that build
requires the packager to hold and comply with an appropriate Qt commercial
license. Development builds may use public Qt 6.8.4 or another compatible Qt 6
version under its applicable license, but are not the specified 6.8.8 artifact.

Pinned dependencies and bundled authoring content must retain their upstream
licenses in their respective project directories and in the final application's
license resources.

- miniz 3.1.2, MIT license: `third_party/miniz/LICENSE`
- Hatch Pet, Apache License 2.0: `third_party/hatch-pet/LICENSE.txt`

Hatch Pet is redistributed as an unmodified, separable Codex skill. Potato does
not claim its name, scripts, tests, or documentation as original Potato code.
