# RParser

Notedata parser for BMS/BMSON/OSZ,OSU/VOS/DTXMania(dtx)/SM/OJM files.

Not suitable to use, currently under development.

May require: zlip, zip, openssl library.

MIT License.

### Feature
- Support reading/writing to most series file format
- Support placing object without loss of resolution
- Support archived file format
- Support various edit/modifying functions(utilities).

### Concept
- Two types of objects: Timing-based object(mixing) and placable objects(notes).
  * Note object is more oriented to editing, while Mixing object is more optimized for mixing and playing by grouping and sorting object in time-sequence.
  * Only one-way conversion is allowed: Note --> Mixing object, by using `Digest()` function.
  * To calculate object's timing, beat position must be calculated, and Timing-based object must be Lookup-ed.
- Object position is described in two ways: Beat and Timing.
  * Beat is floating value position, which is used while editing.
    * To describe beat position more precisely, we use Row notation, which is resolution-based position.
	* Editing Row/Beat position will directly effects to each other.
	* Both(Beat, Row) are affected by BPM.
  * Timing object is generated when requested, as generating cost is expensive.
    * Beat / Timing object can refer to each other to get more detailed info.
	* Beat / Timing object may not 1:1 corresponding.
- Reading song data
  * Song data is read by `Resource class`.
    * It reads total song data, including image/sound files unless `fast_read` option isn't activated.
  * Song data could be in three types: folder, archive, an binary file.
    * Folder type is most used in bms file type.
	* Archive type is most used in ojm, osu file type, similarily treated like folder type.
	* Binary file is in special form like *.vos file type. it's read in bulk form in Resource class.
  * You may can edit 'Chart', or something other metadata, which is included in 'Song' data.
    * Editing Chart directly isn't basically supported by this library, and there is no reason to do so.