### Description
This excerpt from my engines build-toolchain shows how an asset-archive, .apk/.aab and .ico are being built.

Most steps of the toolchain involve:
- Initialize and validate inputs
- Validate operation (compare modification time of input x output)
- Run operation
- Return / Validate out 

It's built on the idea that each folder represents a single step~ with the output being at its root, sub-steps being sub-folders.
Because the output is the sum of its steps, each output folder has a set of sub-folders (steps) and its output at root-level (e.G. an .exe or .apk).
An example is provided in the 'Example use-case' folder.


### Alternatives
"Why not just use gradle or cmake?"
> Excellent question!

It comes down to a matter of long-term strategy. While the tools mentioned above are surely great for many use-cases~ they don't suit my needs of a solo-developer. In my experience: Simplicity, flexibility and readability of a project structure are all aspects required for long-term maintainability- qualities, that I thus value and need in my projects, all adjusted for being used by a single person. The games I ought to make require a simple, clean and unified toolchain that abstracts away the heavy lifting for quick iteration cycles (just as any modern game engine does), but still allows a simple project structure and keeps the room open for further platforms. Additional flexibility is mainly achieved by a modular architecture~ all steps of the toolchain are individual elements, coupled as loosely as feasible.


### Known shortcomings
- The cpp linking may yield errors when deleting .cpp files from the project. A potential solution would be to validate present .cpp and .obj files before linking.

- Naming conventions could create hidden folders on UNIX-Systems. While this could be favorable, its inconsistent in comparison to the folder rendering on Windows.

- Log-messages have some inconsistencies.

> Due to the toolchain's overall stability and more pressing issues, these shortcomings have to wait for a fix.