.. _textures_fileformats_supported_file_formats:

Supported file formats
======================

The Cafu engine currently supports 4 different image file formats:
**.jpg, .png, .tga and .bmp**. This documentation shows you the
advantages/disadvantages of those file formats.

--------------

**.png file format:** The .png file format is the optimal file format
for Cafu! It can feature transparent parts that allows them to be used
for textures that have invisible parts (like windows). Advantage: The
transparent parts can be created very easily with image editing
software. There is NO loss of quality at all with this compression -
optimal for high detailed textures. Disadvantage: They are bigger in the
size than jpg on the hard drive.

**.jpg file format:** The .jpg image format is very small in size on the
hard drive because of it's compression. jpg-files can be used for
diffuse maps - but with care - dont compress them too much! The
disadvantages are that they can't handle transparent parts, which makes
them unusable for some tasks and the compression leads to a loss of
quality. Don't use it for high frequency textures!

**.tga file format:** The .tga file format is like the .png format - the
only difference is the size - tga files are bigger in size on the hard
drive than png!

**.bmp file format:** This file format is very large in size and can't
feature transparent parts. It's advantage is it's quality - there is no
loss of informations when saving into this format!

--------------

Summary: Use png files whenever it's possible. The alternative for
textures without alpha channles is bmp, for those with it's tga. Try to
avoid jpg for high frequency textures and normal-maps!
