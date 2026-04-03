### Intro
Context: Custom retro game-engine.

The parser is able to deal with any .tga format (colormapping, RLE-Compression, ...).
While pre-existent libraries could've been used instead, thorough understanding and control are of priority.
As retro-styled games are the overarching goal, the maximum valid image size was kept very low (1MB) - which can reconfigured.


### Usage
Either load a .tga file...
...from disk by path: `P_TGAFile file("dir/to/my/image.tga");`
...or from RAM by buffer: `P_TGAFile file(tgaFileBytesPtr, tgaFileByteCount);`

The parser will then work to convert the file to usable RGBA32 (1 byte per channel, assuming 1 byte = 8 bits) data. 

You can get the (non null-terminated) pixel-buffer directly with: `file.GetRawPixels_RGBA();`
And the resolution with: 'file.GetRez()' where .first = width and .second = height


### Reference
- https://www.fileformat.info/format/tga/egff.htm#TGA-DMYID.3