### Description
Context: Custom game-engine.

This excerpt from my engines build-toolchain shows how an asset-archive, .apk/.aab and .ico are being built.
Please be aware that this is only an excerpt and the full toolchain includes more steps.

Most steps of the toolchain involve:
- Initialize and Validate Inputs
- Validate Operation (compare modification time of input x output)
- Execute step-command
- Return / Validate out 

Its build over the idea that each folder represents a single step~ with the output being at its root, sub-steps being sub-folders.
Because the output is the sum of its steps, each output folder has a set of sub-folders (steps) and its output (e.G. an .exe or .apk)


### Alternatives
"Why not just use gradle or cmake?"
> Excellent question!

It comes down to a matter of long term strategy~ While the tools mentioned above are great for many usecases (such as larger teams i'd suspect)~ they don't suit my needs of a solo-developer. In my experience: Simplicity, flexibility and readability of a project structure are all aspects required for long term maintainability- qualities, that i thus value and need in my projects. The Games i ought to make require a simple, clean and unified toolchain that abstracts away the heavy lifting for quick iteration cycles (just as any modern game engine does), but still allows a simple project structure and keeps the room open for further platforms. Additional flexibility is mainly achieved by a modular architecture~ all steps of the toolchain are individual elements, coupled as loosely as feasible.


### Known shortcomings
- The cpp linkage may yield errors when deleting .cpp files. A potential solution would be to validate present .cpp and .obj files before linking.

- Step-name convention could create hidden folders on UNIX-Systems ~ while this could be favorable, its inconsistent in comparison to building on
windows.

- Specific phrasing of log-messages is sometimes inconsistent.

Due to the toolchains overall stability and more pressing issues, these shortcomings have to wait.