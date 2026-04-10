### Intro
Context: Custom retro game-engine.

The parser is able to deal with any .tga format (colormapping, RLE-Compression, ...).
While pre-existent libraries could've been used instead, writing a custom .tga parser was the right step to deepen my knowledge of image formats while maintaining full understanding, independence and control of my game-engine.
As retro-styled games are the overarching goal, the maximum valid image size was kept very low (1MB) - which can, of course, be reconfigured.


### Usage
`#include "P_TGAFile.h"`

Either load a .tga file...
...from disk by path: `P_TGAFile file("dir/to/my/image.tga");`
...or from RAM by buffer: `P_TGAFile file(tgaFileBytesPtr, tgaFileByteCount);`

The parser will then work to convert the file to usable RGBA32 (1 byte per channel, assuming 1 byte = 8 bits) data. 

You can get the (non null-terminated) pixel-buffer directly with: `file.GetRawPixels_RGBA();`
And the resolution with: 'file.GetRez()' where .first = width and .second = height


### Known Shortcomings
- 1MB File-Size is an imprecise metric for retro-graphics and does not accurately reflect legacy memory-constraints. A high-resolution .tga image could easily bypass this limit via compression. A max. valid resolution would've been closer to actual retro-graphics limitations.

- Newer .tga features (e.G. footer) haven't been implemented~ the parser only handles the pixel data.

> These issues haven't been fixed due to their low priority.


### Reference
- https://www.fileformat.info/format/tga/egff.htm#TGA-DMYID.3