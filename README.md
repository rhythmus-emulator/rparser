# RParser

Notedata parser for BMS/BMSON/OSZ,OSU/VOS/DTXMania(dtx)/SM/OJM files.

Not suitable to use, currently under development.

Part of codes may be refered from Stepmania/Bmsbel libraries.

May require: zlip/libarchive/iconv library.

MIT License.

### Advantage
- Support Reading/writing to most series file format
- Support placing object without loss of resolution
- Support Archive-based file format
- Support various edit/modifying functions(utilities).

### Concept
- Objects are separated in two: Timing-based object and placable objects(notes).
  * Conversion rule: Row <-> Beat <-> Time / Measure <-> Beat
  * To calculate object's timing, beat position must be calculated, and Timing-based object must be Lookup-ed.
- Object position is described in two ways: Beat and Row
  * Row is resolution-based position, which is used while editing.
  * Beat is floating value position, which is used while playing.
  * Both of them are automatically filled when loading file.
  * You can only modify Row, and beat value will be calculated using UpdateBeatData().
- Archive(Directory) file supported.
  * You can only read charts, or read all files from archive.
  * To modify chart save whole archive, you should read all files and keep it in memory.
  * You can use resources(raw data) for playing, also.