#include "Note.h"
#include <math.h>

namespace rparser
{

const char *kChartObjectTypesStr[] = {
  "NONE",
  "NOTE",
  "TOUCH",
  "KNOB",
  "BGM",
  "BGA",
  "TEMPO",
  "SPECIAL",
};

const char *kNoteTypesStr[] = {
  "NORMAL",
  "INVISIBLE",
  "LONGNOTE",
  "CHARGE",
  "HCHARGE",
  "MINE",
  "AUTO",
  "FAKE",
  "COMBO",
  "DRAG",
  "CHAIN",
  "REPEAT",
};

const char *kNoteTempoTypesStr[] = {
  "BPM",
  "STOP",
  "WARP",
  "SCROLL",
  "MEASURE",
  "TICK",
  "BMSBPM",
  "BMSSTOP",
};

const char *kNoteBgaTypesStr[] = {
  "MAINBGA",
  "MISSBGA",
  "LAYER1",
  "LAYER2",
  "LAYER3",
  "LAYER4",
  "LAYER5",
  "LAYER6",
  "LAYER7",
  "LAYER8",
  "LAYER9",
  "LAYER10",
};

const char *kNoteSpecialTypesStr[] = {
  "REST",
  "BMSKEYBIND",
  "BMSEXTCHR",
  "BMSTEXT",
  "BMSOPTION",
  "BMSARGBBASE",
  "BMSARGBMISS",
  "BGAARGBLAYER1",
  "BGAARGBLAYER2",
};

const char **pNoteSubtypeStr[] = {
  kNoteTypesStr,
  kNoteTempoTypesStr,
  kNoteBgaTypesStr,
  kNoteSpecialTypesStr,
};

const auto noteposcompfunc
  = [](const NotePos *a, const NotePos *b)->bool { return *a < *b; };


// ------------------------------ class NotePos

NotePos::NotePos()
  : type(NotePosTypes::NullPos), track_(0),
    time_msec(0), measure(.0), num(0), deno(0) {}

void NotePos::SetRowPos(int measure, RowPos deno, RowPos num)
{
  ASSERT(deno > 0);
  type = NotePosTypes::Beat;
  this->measure = measure + (double)num / deno;
  this->deno = deno;
  this->num = num;
}

/* @warn acually it uses measure value */
void NotePos::SetBeatPos(double beat)
{
  type = NotePosTypes::Beat;
  this->measure = beat / 4.0;
  // row pos is set in 16th note if no denominator is set.
  if (deno == 0) deno = 16;
  num = static_cast<uint16_t>((beat - measure * 4.0) * deno);
}

#if 0
void NotePos::SetTimePos(double time_msec)
{
  type = NotePosTypes::Time;
  this->time_msec = time_msec;
}
#endif

void NotePos::SetTime(double time_msec)
{
  this->time_msec = time_msec;
}

void NotePos::SetDenominator(uint32_t denominator)
{
  this->deno = denominator;
  num = static_cast<uint16_t>(fmod(measure, 1.0) * deno);
}

std::string NotePos::toString() const
{
  std::stringstream ss;
  int32_t num = 0;
  switch (type)
  {
  case NotePosTypes::NullPos:
    ss << "(No note pos set)" << std::endl;
    break;
  case NotePosTypes::Beat:
    ss << "Measure: " << measure << "(" << num << " / " << deno << ")" << std::endl;
    break;
  case NotePosTypes::Time:
    ss << "Time: " << time_msec << "(" << num << " / " << deno << ")" << std::endl;
    break;
  }
  return ss.str();
}

NotePosTypes NotePos::postype() const
{
  return type;
}

double NotePos::GetBeatPos() const
{
  return measure * 4.0;
}

double NotePos::GetTimePos() const
{
  return time_msec;
}

void NotePos::set_track(int track)
{
  track_ = track;
}

int NotePos::get_track() const
{
  return track_;
}

// used for chained note
NotePos& NotePos::endpos()
{
  return *this;
}

const NotePos& NotePos::endpos() const
{
  return const_cast<NotePos*>(this)->endpos();
}

bool NotePos::operator<(const NotePos &other) const noexcept
{
  return measure < other.measure;
}

bool NotePos::operator==(const NotePos &other) const noexcept
{
  return measure == other.measure;
}

NotePos* NotePos::clone() const
{
  return new NotePos(*this);
}

#if 0
bool NotePos::operator==(const NotePos &other) const noexcept
{
  if (type != other.type) return false;
  switch (type)
  {
  case NotePosTypes::Beat:
    return beat == other.beat;
  case NotePosTypes::Time:
    return time_msec == other.time_msec;
  default:
    ASSERT(0);
    return false;
  }
}
#endif


// ----------------------------------- NoteDesc

NoteDesc::NoteDesc()
  : x_(0), y_(0), z_(0) {}

void NoteDesc::set_pos(int x, int y) { x_ = x; y_ = y; }
void NoteDesc::set_pos(int x, int y, int z) { x_ = x; y_ = y; z_ = z; }
void NoteDesc::get_pos(int &x, int &y) const { x = x_; y = y_; }
void NoteDesc::get_pos(int &x, int &y, int &z) const { x = x_; y = y_; z = z_; }

NoteDesc* NoteDesc::clone() const
{
  return new NoteDesc(*this);
}

// ------------------------------ NoteSoundDesc

NoteSoundDesc::NoteSoundDesc()
  : channel_(0), length_(0), volume_(0) {}

void NoteSoundDesc::set_channel(Channel ch) { channel_ = ch; }
void NoteSoundDesc::set_length(uint32_t len_ms) { length_ = len_ms; }
void NoteSoundDesc::set_volume(float v) { volume_ = v; }
void NoteSoundDesc::set_key(int key) { key_ = key; }
Channel NoteSoundDesc::channel() const { return channel_; }
float NoteSoundDesc::volume() const { return volume_; }
uint32_t NoteSoundDesc::length() const { return length_; }
int NoteSoundDesc::key() const { return key_; }

NoteSoundDesc* NoteSoundDesc::clone() const
{
  return new NoteSoundDesc(*this);
}

// --------------------------------------- Note

Note::Note()
  : type_(0), player_(0)
{
  chains_.push_back(this);
}

Note::Note(const Note& note)
{
  type_ = note.type_;
  for (const auto *chain : note.chains_)
  {
    chains_.push_back(chain->clone());
  }
}

Note::~Note()
{
  RemoveAllChain();
}

NoteType Note::type() const
{
  return type_;
}

void Note::set_type(NoteType t)
{
  type_ = t;
}

int Note::player() const
{
  return player_;
}

void Note::set_player(int player)
{
  player_ = player;
}

int Note::get_player() const
{
  return player_;
}

std::vector<NoteDesc*>::iterator Note::begin() { return chains_.begin(); }
std::vector<NoteDesc*>::iterator Note::end() { return chains_.end(); }
std::vector<NoteDesc*>::const_iterator Note::begin() const { return chains_.begin(); }
std::vector<NoteDesc*>::const_iterator Note::end() const { return chains_.end(); }

NoteDesc* Note::NewChain()
{
  NoteDesc *d = new NoteDesc();
  chains_.push_back(d);
  return d;
}

void Note::RemoveAllChain()
{
  for (size_t i = 1; i < chains_.size(); ++i)
    delete chains_[i];
  chains_.clear();
  chains_.push_back(this);
}

size_t Note::chainsize() const
{
  return chains_.size();
}

int Note::get_tap_type() const
{
  // TODO
  return 1;
}

bool Note::is_focusout_allowed() const
{
  // TODO
  return false;
}

NotePos& Note::endpos() { return *chains_.back(); }

std::string Note::toString() const
{
  std::stringstream ss;
  std::string sType;
  sType = kNoteTypesStr[type_];
  ss << "[Object Note]\n";
  ss << "type: " << sType << std::endl;
  ss << NotePos::toString();
  return ss.str();
}

bool Note::operator==(const Note &other) const noexcept
{
  return NotePos::operator==(other) && type() == other.type();
}

Note* Note::clone() const
{
  return new Note(*this);
}


// ---------------------------------- BgmObject

BgmObject::BgmObject()
  : bgm_type_(0), column_(0) {}

void BgmObject::set_bgm_type(int bgm_type) { bgm_type_ = bgm_type; }
void BgmObject::set_column(int col) { column_ = col; }
int BgmObject::get_bgm_type() const { return bgm_type_; }
int BgmObject::get_column() const { return column_; }

BgmObject* BgmObject::clone() const
{
  return new BgmObject(*this);
}

// ---------------------------------- BgaObject

BgaObject::BgaObject()
  : channel_(0), layer_(0) {}

void BgaObject::set_layer(uint32_t layer) { layer_ = layer; }
void BgaObject::set_channel(Channel ch) { channel_ = ch; }
uint32_t BgaObject::layer() const { return layer_; }
Channel BgaObject::channel() const { return channel_; }

BgaObject* BgaObject::clone() const
{
  return new BgaObject(*this);
}


// ------------------------------- TimingObject

TimingObject::TimingObject()
{
  value.i = 0;
}

void TimingObject::set_type(int type)
{
  set_track(type);
}

int TimingObject::get_type() const
{
  return get_track();
}

void TimingObject::SetBpm(float bpm)
{
  set_type(TimingObjectTypes::kBpm);
  value.f = bpm;
}

void TimingObject::SetBmsBpm(Channel bms_channel)
{
  set_type(TimingObjectTypes::kBmsBpm);
  value.i = bms_channel;
}

void TimingObject::SetStop(float stop)
{
  set_type(TimingObjectTypes::kStop);
  value.f = stop;
}

void TimingObject::SetBmsStop(Channel bms_channel)
{
  set_type(TimingObjectTypes::kBmsStop);
  value.i = bms_channel;
}

void TimingObject::SetMeasure(float measure_length)
{
  set_type(TimingObjectTypes::kMeasure);
  value.f = measure_length;
}

void TimingObject::SetScroll(float scrollspeed)
{
  set_type(TimingObjectTypes::kScroll);
  value.f = scrollspeed;
}

void TimingObject::SetTick(int32_t tick)
{
  set_type(TimingObjectTypes::kTick);
  value.i = tick;
}

void TimingObject::SetWarp(float warp_to)
{
  set_type(TimingObjectTypes::kWarp);
  value.f = warp_to;
}

float TimingObject::GetFloatValue() const
{
  return value.f;
}

int32_t TimingObject::GetIntValue() const
{
  return value.i;
}

std::string TimingObject::getValueAsString() const
{
  std::stringstream ss;
  ss << "Value (float): " << value.f << std::endl;
  ss << "Value (int): " << value.i << std::endl;
  return ss.str();
}

bool TimingObject::operator==(const TimingObject &other) const noexcept
{
  return NotePos::operator==(other) &&
    value.i == other.value.i;
}

TimingObject* TimingObject::clone() const
{
  return new TimingObject(*this);
}


// ------------------------------- EffectObject

std::string EffectObject::getValueAsString() const
{
  std::stringstream ss;
  ss << "Command: " << command_;
  if (arg1_) ss << ", arg1: " << arg1_;
  if (arg2_) ss << ", arg2: " << arg2_;
  ss << std::endl;
  return ss.str();
}

EffectObject::EffectObject() : type_(0), command_(0), arg1_(0), arg2_(0)
{}

/* @depreciated */
void EffectObject::set_type(int type) { set_track(type); }
/* @depreciated */
int EffectObject::get_type() const { return get_track(); }

void EffectObject::SetMidiCommand(uint8_t command, uint8_t arg1, uint8_t arg2)
{
  set_type(EffectObjectTypes::kMIDI);
  command_ = command;
  arg1_ = arg1;
  arg2_ = arg2;
}

void EffectObject::SetBmsARGBCommand(BgaTypes bgatype, Channel channel)
{
  set_type(EffectObjectTypes::kBmsARGBLAYER);
  command_ = bgatype;
  arg1_ = static_cast<decltype(arg1_)>(channel);
  arg2_ = 0;
}

void EffectObject::GetMidiCommand(uint8_t &command, uint8_t &arg1, uint8_t &arg2) const
{
  ASSERT(get_type() == EffectObjectTypes::kMIDI);
  command = (uint8_t)command_;
  arg1 = (uint8_t)arg1_;
  arg2 = (uint8_t)arg2_;
}

void EffectObject::GetBga(int &bga_type, Channel &channel) const
{
  bga_type = command_;
  channel = arg1_;
}

bool EffectObject::operator==(const EffectObject &other) const noexcept
{
  return NotePos::operator==(other) &&
         command_ == other.command_ &&
         arg1_ == other.arg1_ &&
         arg2_ == other.arg2_;
}

EffectObject* EffectObject::clone() const
{
  return new EffectObject(*this);
}


// --------------------------------- ObjectData

Track::Track() {}

Track::~Track()
{
  ClearAll();
}

/* @brief Add object without object duplication. */
void Track::AddObject(NotePos* object)
{
  // if object is added sequentially, add it directly
  if (objects_.empty() || objects_.back()->endpos() < *object)
  {
    objects_.push_back(object);
    return;
  }

  // find smart position to append object
  bool already_inserted = false;
  auto it = std::lower_bound(objects_.begin(), objects_.end(), object,
    noteposcompfunc);
  if (it != objects_.begin())
  {
    // before insert, check previous note is overlapped.
    // if overlapped, then replace it.
    auto prev_it = std::prev(it);
    if (*object < (*prev_it)->endpos())
    {
      delete *prev_it;
      *prev_it = object;
      already_inserted = true;
    }
  }
  if (!already_inserted)
    objects_.insert(it, object);
  // check overlapping for next (long)notes.
  {
    auto it = std::upper_bound(objects_.begin(), objects_.end(), object,
      noteposcompfunc);
    auto it_end = it;
    while (it_end != objects_.end() && **it_end < object->endpos())
    {
      delete *it_end;
      ++it_end;
    }
    if (it != it_end)
      objects_.erase(it, it_end);
  }
}

/* @brief Add object with object duplication (not checking object pos collision) */
void Track::AddObjectDuplicated(NotePos* object)
{
  // if object is added sequentially, add it directly
  if (objects_.empty() || *objects_.back() < *object)
  {
    objects_.push_back(object);
    return;
  }

  // find smart position to append object
  auto it = std::lower_bound(objects_.begin(), objects_.end(), object,
    noteposcompfunc);
  objects_.insert(it, object);
}


void Track::RemoveObject(NotePos* object)
{
  auto it = std::find(objects_.begin(), objects_.end(), object);
  if (it == objects_.end())
    return;
  delete object;
  objects_.erase(it);
}

NotePos* Track::GetObjectByPos(int measure, int nu, int de)
{
  ASSERT(de > 0);
  return GetObjectByMeasure(measure + (double)nu / de);
}

NotePos* Track::GetObjectByMeasure(double measure)
{
  for (auto *obj : objects_)
  {
    if (obj->measure > measure)
      break;
    else if (obj->measure == measure)
      return obj;
  }
  return nullptr;
}

void Track::RemoveObjectByPos(int measure, int nu, int de)
{
  ASSERT(de > 0);
  return RemoveObjectByMeasure(measure + (double)nu / de);
}

void Track::RemoveObjectByMeasure(double measure)
{
  NotePos* o = GetObjectByMeasure(measure);
  if (o) RemoveObject(o);
}

bool Track::IsHoldNoteAt(double mesure) const
{
  if (objects_.empty()) return false;
  NotePos *n = objects_.front();
  NotePos p; /* temporary object to use lower_bound */
  p.measure = mesure;
  auto it = std::upper_bound(objects_.rbegin(), objects_.rend(), &p,
    noteposcompfunc);
  if (it != objects_.rend()) n = *it;
  return (n->endpos().measure >= mesure);
}

bool Track::IsRangeEmpty(double m_start, double m_end) const
{
  if (objects_.empty()) return false;
  NotePos p; /* temporary object to use lower_bound */
  p.measure = m_start;
  auto it = std::lower_bound(objects_.begin(), objects_.end(), &p,
    noteposcompfunc);
  if (it != objects_.begin()) it = std::prev(it);
  if ((*it)->endpos().measure >= m_start) return false;
  ++it;
  if (it != objects_.end())
  {
    if ((*it)->measure <= m_end) return false;
  }
  return true;
}

void Track::GetObjectByRange(double m_start, double m_end, std::vector<NotePos*> &out)
{
  for (size_t i = 0; i < objects_.size(); ++i)
  {
    if (objects_[i]->measure >= m_start)
    {
      while (i < objects_.size())
      {
        out.push_back(objects_[i]);
        if (objects_[i]->measure > m_end)
          return;
        ++i;
      }
      break;
    }
  }
}


void Track::ClearAll()
{
  for (auto *object : objects_)
    delete object;
  objects_.clear();
}

void Track::ClearRange(double m_begin, double m_end)
{
  for (size_t i = 0; i < objects_.size(); ++i)
  {
    if (objects_[i]->measure >= m_begin)
    {
      delete objects_[i];
      size_t si = i;
      size_t ei = i+1;
      while (ei < objects_.size())
      {
        if (objects_[ei]->measure > m_end)
          break;
        delete objects_[ei];
        ei++;
      }
      objects_.erase(objects_.begin() + si, objects_.begin() + ei);
      break;
    }
  }
}

void Track::CopyAll(const Track& from)
{
  for (size_t i = 0; i < from.objects_.size(); ++i)
  {
    AddObject(from.objects_[i]->clone());
  }
}

void Track::CopyRange(const Track& from, double m_begin, double m_end)
{
  for (size_t i = 0; i < from.objects_.size(); ++i)
  {
    if (from.objects_[i]->measure >= m_begin)
    {
      while (i < from.objects_.size())
      {
        if (from.objects_[i]->measure > m_end)
          break;
        AddObject(from.objects_[i]->clone());
        ++i;
      }
      break;
    }
  }
}

void Track::MoveRange(double m_delta, double m_begin, double m_end)
{
  ClearRange(m_begin + m_delta, m_end + m_delta);
  std::vector<NotePos*> objs;
  GetObjectByRange(m_begin, m_end, objs);
  for (auto *obj : objs)
    obj->SetBeatPos(obj->measure + m_delta);
}

void Track::InsertBlank(double m_begin, double m_delta)
{
  MoveRange(m_delta, m_begin, std::numeric_limits<double>::max());
}

void Track::MoveAll(double m_delta)
{
  for (auto *obj : objects_)
  {
    obj->SetBeatPos(obj->measure + m_delta);
  }
}

Track::iterator Track::begin() { return objects_.begin(); }
Track::iterator Track::end() { return objects_.end(); }
Track::const_iterator Track::begin() const { return objects_.begin(); }
Track::const_iterator Track::end() const { return objects_.end(); }
NotePos& Track::front() { return *objects_.front(); };
NotePos& Track::back() { return *objects_.back(); };

void Track::swap(Track &track)
{
  objects_.swap(track.objects_);
}

size_t Track::size() const
{
  return objects_.size();
}

bool Track::is_empty() const
{
  return objects_.empty();
}

void Track::clear()
{
  ClearAll();
}


// ----------------------------------- NoteData

TrackData::TrackData() : track_count_(1) {}

void TrackData::set_track_count(int track_count)
{
  track_count_ = track_count;
}

int TrackData::get_track_count() const
{
  return track_count_;
}

void TrackData::auto_set_track_count()
{
  for (size_t i = 0; i < kMaxTrackSize; ++i)
    if (!track_[i].is_empty()) track_count_ = (int)i;
}

void TrackData::AddObject(NotePos* object)
{
  // add object to proper track
  track_[object->get_track()].AddObject(object);
}

void TrackData::AddObjectDuplicated(NotePos* object)
{
  track_[object->get_track()].AddObjectDuplicated(object);
}

void TrackData::RemoveObject(NotePos* object)
{
  track_[object->get_track()].RemoveObject(object);
}

NotePos* TrackData::GetObjectByPos(size_t track, int measure, int nu, int de)
{
  return track_[track].GetObjectByPos(measure, nu, de);
}

NotePos* TrackData::GetObjectByMeasure(size_t track, double measure)
{
  return track_[track].GetObjectByMeasure(measure);
}

NotePos** TrackData::GetRowByPos(int measure, int nu, int de)
{
  for (int i = 0; i < track_count_; ++i)
    track_row_[i] = track_[i].GetObjectByPos(measure, nu, de);
  return track_row_;
}

NotePos** TrackData::GetRowByMeasure(double measure)
{
  for (int i = 0; i < track_count_; ++i)
    track_row_[i] = track_[i].GetObjectByMeasure(measure);
  return track_row_;
}

void TrackData::RemoveObjectByPos(int measure, int nu, int de)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].RemoveObjectByPos(measure, nu, de);
}

void TrackData::RemoveObjectByMeasure(double measure)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].RemoveObjectByMeasure(measure);
}

void TrackData::RemoveObjectByPos(size_t track, int measure, int nu, int de)
{
  track_[track].RemoveObjectByPos(measure, nu, de);
}

void TrackData::RemoveObjectByMeasure(size_t track, double measure)
{
  track_[track].RemoveObjectByMeasure(measure);
}

bool TrackData::IsHoldNoteAt(double beat) const
{
  for (size_t i = 0; i < track_count_; ++i)
    if (track_[i].IsHoldNoteAt(beat)) return true;
  return false;
}

bool TrackData::IsRangeEmpty(double beat_start, double beat_end) const
{
  for (size_t i = 0; i < track_count_; ++i)
    if (!track_[i].IsRangeEmpty(beat_start, beat_end)) return false;
  return true;
}

void TrackData::GetNoteByRange(double beat_start, double beat_end, std::vector<NotePos*> &out)
{

}

void TrackData::GetAllTrackNote(std::vector<NotePos*> &out)
{

}

void TrackData::ClearAll()
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].ClearAll();
}

void TrackData::ClearTrack(size_t track)
{
  track_[track].ClearAll();
}

void TrackData::ClearRange(double beat_begin, double beat_end)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].ClearRange(beat_begin, beat_end);
}

void TrackData::CopyRange(const TrackData& from, double beat_begin, double beat_end)
{
  ClearAll();
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].CopyRange(from.track_[i], beat_begin, beat_end);
}

void TrackData::CopyAll(const TrackData& from)
{
  ClearAll();
  track_count_ = from.track_count_;
  track_count_shuffle_ = from.track_count_shuffle_;
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].CopyAll(from.track_[i]);
}

void TrackData::MoveRange(double beat_delta, double beat_begin, double beat_end)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].MoveRange(beat_delta, beat_begin, beat_end);
}

void TrackData::MoveAll(double beat_delta)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].MoveAll(beat_delta);
}

void TrackData::InsertBlank(double beat_begin, double beat_delta)
{
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].InsertBlank(beat_begin, beat_delta);
}

void TrackData::RemapTracks(size_t *track_map)
{
  Track track_backup_[kMaxTrackSize];
  for (size_t i = 0; i < track_count_; ++i)
  {
    track_backup_[i].swap(track_[i]);
  }
  for (size_t i = 0; i < track_count_; ++i)
  {
    track_backup_[i].swap(track_[track_map[i]]);
  }
}

void TrackData::UpdateTracks()
{
  auto alliter = GetAllTrackIterator();
  for (size_t i = 0; i < track_count_; ++i)
    track_[i].clear();
  while (!alliter.is_end())
  {
    AddObjectDuplicated(&*alliter);
    ++alliter;
  }
}

TrackData::iterator TrackData::begin(size_t track)
{
  return track_[track].begin();
}

TrackData::iterator TrackData::end(size_t track)
{
  return track_[track].end();
}

TrackData::const_iterator TrackData::begin(size_t track) const
{
  return track_[track].begin();
}

TrackData::const_iterator TrackData::end(size_t track) const
{
  return track_[track].end();
}

Track& TrackData::get_track(size_t track) { return track_[track]; }
const Track& TrackData::get_track(size_t track) const { return track_[track]; }

NotePos* TrackData::all_track_iterator::p() { return *iter_; }
void TrackData::all_track_iterator::next() { iter_++; }
bool TrackData::all_track_iterator::is_end() const { return notes_.end() == iter_; }
TrackData::all_track_iterator &TrackData::all_track_iterator::operator++() { next(); return *this; }
NotePos& TrackData::all_track_iterator::operator*() { return **iter_; }
const NotePos& TrackData::all_track_iterator::operator*() const { return **iter_; }

NotePos* TrackData::front()
{
  NotePos* n = nullptr;
  for (size_t i = 0; i < track_count_; ++i)
  {
    if (!track_[i].is_empty() && (!n || track_[i].front() < *n))
      n = &track_[i].front();
  }
  return n;
}

NotePos* TrackData::back()
{
  NotePos* n = nullptr;
  for (size_t i = 0; i < track_count_; ++i)
  {
    if (!track_[i].is_empty() && (!n || *n < track_[i].back()))
      n = &track_[i].back();
  }
  return n;
}

const NotePos* TrackData::front() const { return const_cast<TrackData*>(this)->front(); }
const NotePos* TrackData::back() const { return const_cast<TrackData*>(this)->back(); }

TrackData::all_track_iterator TrackData::GetAllTrackIterator() const
{
  all_track_iterator iter;
  for (int i = 0; i < track_count_; ++i)
    for (auto *obj : track_[i])
      iter.notes_.push_back(obj);

  /* Use stable_sort to keep track order.
   * This result sorting in first position, then track. */
  std::stable_sort(iter.notes_.begin(), iter.notes_.end(), noteposcompfunc);

  iter.iter_ = iter.notes_.begin();
  return iter;
}

TrackData::all_track_iterator TrackData::GetAllTrackIterator(double m_start, double m_end) const
{
  all_track_iterator iter;
  for (int i = 0; i < track_count_; ++i)
  {
    for (auto *obj : track_[i])
    {
      if (obj->measure < m_start) continue;
      if (obj->measure > m_end) break;
      iter.notes_.push_back(obj);
    }
  }

  /* Use stable_sort to keep track order.
   * This result sorting in first position, then track. */
  std::stable_sort(iter.notes_.begin(), iter.notes_.end(), noteposcompfunc);

  iter.iter_ = iter.notes_.begin();
  return iter;
}

TrackData::row_iterator::row_iterator()
  : m_start(0), m_end(std::numeric_limits<double>::max()), measure(0)
{
  memset(curr_row_notes, 0, sizeof(curr_row_notes));
}

TrackData::row_iterator::row_iterator(double m_start, double m_end)
  : m_start(m_start), m_end(m_end), measure(m_start)
{
  memset(curr_row_notes, 0, sizeof(curr_row_notes));
}

double TrackData::row_iterator::get_measure() const { return measure; }

void TrackData::row_iterator::next()
{
  if (all_track_iter_.is_end()) return;

  memset(curr_row_notes, 0, sizeof(curr_row_notes));
  measure = all_track_iter_.p()->measure;
  NotePos *cur_note;
  while (1)
  {
    all_track_iter_.next();
    if (all_track_iter_.is_end())
      break;
    cur_note = all_track_iter_.p();
    if (measure != cur_note->measure)
      break;
    curr_row_notes[cur_note->get_track()] = cur_note;
  }
}

TrackData::row_iterator &TrackData::row_iterator::operator++() { next(); return *this; }
NotePos& TrackData::row_iterator::operator[](size_t i) { return *curr_row_notes[i]; }
const NotePos& TrackData::row_iterator::operator[](size_t i) const { return *curr_row_notes[i]; }
double TrackData::row_iterator::operator*() const { return get_measure(); }

NotePos* TrackData::row_iterator::col(size_t idx)
{
  return curr_row_notes[idx];
}

bool TrackData::row_iterator::is_end() const {
  /*return beat <= beat_end;*/
  return all_track_iter_.is_end();
}

TrackData::row_iterator TrackData::GetRowIterator() const
{
  row_iterator iter;
  iter.all_track_iter_ = GetAllTrackIterator();
  return iter;
}

TrackData::row_iterator TrackData::GetRowIterator(double beat_start, double beat_end) const
{
  row_iterator iter(beat_start, beat_end);
  iter.all_track_iter_ = GetAllTrackIterator(beat_start, beat_end);
  return iter;
}

void TrackData::swap(TrackData &data)
{
  for (size_t i = 0; i < kMaxTrackSize; ++i)
  {
    track_[i].swap(data.track_[i]);
    std::swap(track_row_[i], data.track_row_[i]); // XXX: need to swap, or clear?
  }
  std::swap(track_count_, data.track_count_);
  std::swap(track_count_shuffle_, data.track_count_shuffle_);
}

size_t TrackData::size() const {
  size_t cnt = 0;
  for (int i = 0; i < track_count_; ++i)
    cnt += track_[i].size();
  return cnt;
}

bool TrackData::is_empty() const {
  for (int i = 0; i < track_count_; ++i)
    if (!track_[i].is_empty()) return false;
  return true;
}

void TrackData::clear()
{
  ClearAll();
}

std::string TrackData::toString() const
{
  std::stringstream ss;
  ss << "Total track count: " << track_count_ << std::endl;
  ss << "Total note count: " << size() << std::endl;
  return ss.str();
}

}
