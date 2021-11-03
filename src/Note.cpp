#include "Note.h"
#include "common.h"
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


// ------------------------------ class NotePos

NoteElement::NoteElement()
  : measure_(.0), time_msec_(0), rpos_({ 0, 0 }), chain_status_(NoteChainStatus::Tap)
{
  memset(&point_, 0, sizeof(point_));
  memset(&v_, 0, sizeof(v_));
  memset(&prop_, 0, sizeof(prop_));
}

void NoteElement::SetRowPos(int measure, const RowPos& rpos)
{
  ASSERT(rpos.deno > 0);
  this->measure_ = measure + (double)rpos.num / rpos.deno;
  this->rpos_ = rpos;
}

const RowPos& NoteElement::GetRowPos() const { return rpos_; }

/* @warn acually it uses measure value */
void NoteElement::set_measure(double measure)
{
  this->measure_ = measure;
  // row pos is set in 16th note if no denominator is set.
  if (rpos_.deno == 0) rpos_.deno = 16;
  rpos_.num = static_cast<unsigned>((measure - (int)this->measure_) * rpos_.deno);
}

void NoteElement::set_time(double time_msec)
{
  this->time_msec_ = time_msec;
}

void NoteElement::set_chain_status(NoteChainStatus cstat)
{
  chain_status_ = cstat;
}

void NoteElement::SetDenominator(uint32_t denominator)
{
  this->rpos_.deno = denominator;
  rpos_.num = static_cast<uint16_t>(fmod(measure_, 1.0) * rpos_.deno);
}

std::string NoteElement::toString() const
{
  std::stringstream ss;
  ss << "Measure: " << measure_ << " / Time: " << time_msec_
     << " (" << rpos_.num << " / " << rpos_.deno << ")" << std::endl;
  return ss.str();
}

double NoteElement::measure() const
{
  return measure_;
}

double NoteElement::time() const
{
  return time_msec_;
}

NoteChainStatus NoteElement::chain_status() const
{
  return chain_status_;
}

bool NoteElement::operator<(const NoteElement &other) const noexcept
{
  return measure_ < other.measure_;
}

bool NoteElement::operator==(const NoteElement &other) const noexcept
{
  return measure_ == other.measure_;
}

void NoteElement::set_value(int v)
{
  v_.i[0] = v;
}

void NoteElement::set_value(unsigned v)
{
  v_.u[0] = v;
}

void NoteElement::set_point(int x)
{
  point_.x = x;
  point_.y = 0;
  point_.z = 0;
}

void NoteElement::set_point(int x, int y)
{
  point_.x = x;
  point_.y = y;
  point_.z = 0;
}

void NoteElement::set_point(int x, int y, int z)
{
  point_.x = x;
  point_.y = y;
  point_.z = z;
}

void NoteElement::get_point(int &x)
{
  x = point_.x;
}

void NoteElement::get_point(int &x, int &y)
{
  x = point_.x;
  y = point_.y;
}

void NoteElement::get_point(int &x, int &y, int &z)
{
  x = point_.x;
  y = point_.y;
  z = point_.z;
}

void NoteElement::set_value(double v)
{
  v_.f = v;
}

void NoteElement::get_value(int &v)
{
  v = v_.i[0];
}

int NoteElement::get_value_i() const
{
  return v_.i[0];
}

unsigned int NoteElement::get_value_u() const
{
  return v_.u[0];
}

double NoteElement::get_value_f() const
{
  return v_.f;
}

void NoteElement::set_property(NoteSoundProperty &sprop)
{
  prop_.sound = sprop;
}

const NoteSoundProperty& NoteElement::get_property_sound() const
{
  return prop_.sound;
}

NoteSoundProperty& NoteElement::get_property_sound()
{
  return prop_.sound;
}

// --------------------------------------- Note

Note::Note(Track* track) : track_(track), index_(0), size_(track_->size()) {}

size_t Note::size() const
{
  return size_;
}

bool Note::next()
{
  if (index_ + 1 >= size_) return false;
  index_++;
  return true;
}

bool Note::prev()
{
  if (index_ == 0) return false;
  index_--;
  return true;
}

void Note::reset()
{
  index_ = 0;
  size_ = track_->size();
}

void Note::SeekByBeat(double beat)
{
  // TODO: fast binary search
  if (size_ == 0) return;
}

void Note::SeekByTime(double time)
{
  // TODO: fast binary search
  if (size_ == 0) return;
}

bool Note::is_end() const
{
  return index_ + 1 == size_;
}

NoteElement* Note::get()
{
  return track_->get(index_);
}

const NoteElement* Note::get() const
{
  return const_cast<Note*>(this)->get();
}


// -------------------------------------- Track

Track::Track() : is_object_duplicable_(true) {}

Track::~Track() {}


void Track::set_name(const std::string& name) { name_ = name; }

const std::string& Track::name() const { return name_; }

void Track::AddNoteElement(const NoteElement& object)
{
  // for fast-append logic
  bool is_last_note_duplicated = !notes_.empty() && notes_.back().measure() == object.measure();
  if (notes_.empty() || is_last_note_duplicated)
  {
    // if object is not duplicatable
    if (!is_object_duplicable_ && is_last_note_duplicated)
    {
      if (object.chain_status() == NoteChainStatus::End)
      {
        // if end of longnote, pop other tapnotes till beginning of the longnote.
        while (notes_.back().chain_status() == NoteChainStatus::Tap && notes_.size() > 1)
          notes_.pop_back();
      }
      // don't check tapnote in case of fast-append logic,
      // as there'll be no longnote at last position.
      notes_.back() = object;
      return;
    }
    notes_.push_back(object);
    return;
  }
  // search position to insert note
  auto it = std::upper_bound(notes_.begin(), notes_.end(), object);
  auto end = notes_.insert(it, object);
  if (!is_object_duplicable_)
  {
    if (object.chain_status() == NoteChainStatus::End)
    {
      // if end of longnote, pop all invalid tapnotes
      int remove_count = 0;
      while (end - remove_count != notes_.begin()
        && (end - remove_count - 1)->chain_status() == NoteChainStatus::Tap)
      {
        remove_count++;
      }
      notes_.erase(end - remove_count, end);
    }
    else {
      // if tapnote / LN start, then check overlapping with other longnotes.
      unsigned l = 1;
      while (end + l != notes_.end() && (end + l)->chain_status() != NoteChainStatus::Tap)
        l++;
      notes_.erase(end + 1, end + l);
      l = 0;
      while (end - l != notes_.begin() && (end - l - 1)->chain_status() != NoteChainStatus::Tap)
        l++;
      notes_.erase(end - l, end);
    }
  }
}

void Track::RemoveNoteElement(const NoteElement& object)
{
  if (notes_.empty()) return;
  NoteElement *nf = &notes_.front();
  NoteElement *ne = &notes_.back();

  // TODO: throw exception when object is not included in track?
  if (&object < nf || &object > ne) return;

  notes_.erase(std::remove(notes_.begin(), notes_.end(), object), notes_.end());
}

NoteElement* Track::GetNoteElementByPos(int measure, int nu, int de)
{
  return GetNoteElementByMeasure(measure + de / (double)nu);
}

NoteElement* Track::GetNoteElementByMeasure(double measure)
{
  // @brief Find note element that is equal or bigger than given measure.
  // TODO: use faster binary search without creating NoteElement
  NoteElement n;
  n.set_measure(measure);
  auto it = std::lower_bound(notes_.begin(), notes_.end(), n);
  if (it == notes_.end())
    return nullptr;
  return &*it;
}

NoteElement* Track::get(size_t index) { return index < notes_.size() ? &notes_[index] : nullptr; }

void Track::RemoveNoteByPos(int measure, int nu, int de)
{
  RemoveNoteByMeasure(measure + nu / (double)de);
}

void Track::RemoveNoteByMeasure(double measure)
{
  // @brief Remove note element that is equal or bigger than given measure.
  if (notes_.empty()) return;
  NoteElement n;
  n.set_measure(measure);
  auto it = std::lower_bound(notes_.begin(), notes_.end(), n);
  notes_.erase(it);
}

void Track::SetObjectDupliable(bool duplicable) { is_object_duplicable_ = duplicable; }

bool Track::IsRangeEmpty(double measure) const
{
  // TODO: use faster binary search
  bool is_ln = false;
  for (unsigned i = 0; i < notes_.size(); ++i)
  {
    if (notes_[i].chain_status() == NoteChainStatus::Start)
      is_ln = true;
    else if (notes_[i].chain_status() == NoteChainStatus::End)
      is_ln = false;
    if (notes_[i].measure() == measure)
      return false;
    if (notes_[i].measure() > measure)
      break;
  }
  return is_ln;
}

bool Track::IsRangeEmpty(double m_start, double m_end) const
{
  // XXX: should use temporary NoteElement?
  NoteElement n;
  n.set_measure(m_start);
  auto it = std::lower_bound(notes_.begin(), notes_.end(), n);
  return (it == notes_.end() || it->measure() > m_end);
}

bool Track::IsHoldNoteAt(double measure) const
{
  NoteElement n;
  n.set_measure(measure);
  auto it = std::lower_bound(notes_.begin(), notes_.end(), n);
  return (n.chain_status() == NoteChainStatus::Body || n.chain_status() == NoteChainStatus::End);
}

bool Track::HasLongnote() const
{
  for (auto &n : notes_)
    if (n.chain_status() == NoteChainStatus::Start) return true;
  return false;
}

void Track::GetNoteElementsByRange(double m_start, double m_end, std::vector<const NoteElement*> &out) const
{
  for (auto iter = begin(m_start); iter != end(m_end); ++iter)
    out.push_back(&*iter);
}

void Track::GetAllNoteElements(std::vector<const NoteElement*> &out) const
{
  for (size_t i = 0; i < notes_.size(); ++i)
    out.push_back(&notes_[i]);
}

void Track::GetNoteElementsByRange(double m_start, double m_end, std::vector<NoteElement*> &out)
{
  for (auto iter = begin(m_start); iter != end(m_end); ++iter)
    out.push_back(&*iter);
}

void Track::GetAllNoteElements(std::vector<NoteElement*> &out)
{
  for (size_t i = 0; i < notes_.size(); ++i)
    out.push_back(&notes_[i]);
}


void Track::ClearAll()
{
  notes_.clear();
}

void Track::ClearRange(double m_begin, double m_end)
{
  for (size_t i = 0; i < notes_.size(); ++i)
  {
    if (notes_[i].measure() >= m_begin)
    {
      size_t si = i;
      size_t ei = i+1;
      while (ei < notes_.size())
      {
        if (notes_[ei].measure() > m_end)
          break;
        ei++;
      }
      notes_.erase(notes_.begin() + si, notes_.begin() + ei);
      break;
    }
  }
}

void Track::CopyAll(const Track& from)
{
  // TODO: check track type before copy element from other track
  for (size_t i = 0; i < from.notes_.size(); ++i)
  {
    AddNoteElement(from.notes_[i]);
  }
}

void Track::CopyRange(const Track& from, double m_begin, double m_end)
{
  // TODO: check track type before copy element from other track
  for (size_t i = 0; i < from.notes_.size(); ++i)
  {
    if (from.notes_[i].measure() >= m_begin)
    {
      while (i < from.notes_.size())
      {
        if (from.notes_[i].measure() > m_end)
          break;
        AddNoteElement(from.notes_[i]);
        ++i;
      }
      break;
    }
  }
}

void Track::MoveRange(double m_delta, double m_begin, double m_end)
{
  ClearRange(m_begin + m_delta, m_end + m_delta);
  std::vector<NoteElement*> objs;
  GetNoteElementsByRange(m_begin, m_end, objs);
  for (auto *obj : objs)
    obj->set_measure(obj->measure() + m_delta);
}

void Track::MoveAll(double m_delta)
{
  for (auto &obj : notes_)
  {
    obj.set_measure(obj.measure() + m_delta);
  }
}

void Track::InsertBlank(double m_begin, double m_delta)
{
  MoveRange(m_delta, m_begin, std::numeric_limits<double>::max());
}

static bool __comp_note_pos(const NoteElement& a, const NoteElement& b) {
  return a.measure() < b.measure();
}

Track::iterator Track::begin() { return notes_.begin(); }
Track::iterator Track::end() { return notes_.end(); }
Track::const_iterator Track::begin() const { return notes_.begin(); }
Track::const_iterator Track::end() const { return notes_.end(); }

// TODO: use custom lower/upper bound function to remove unnecessary struct creation
Track::iterator Track::begin(double mpos)
{ NoteElement e; e.set_measure(mpos); return std::lower_bound(notes_.begin(), notes_.end(), e, __comp_note_pos); }
Track::iterator Track::end(double mpos)
{ NoteElement e; e.set_measure(mpos); return std::upper_bound(notes_.begin(), notes_.end(), e, __comp_note_pos); }
Track::const_iterator Track::begin(double mpos) const
{ NoteElement e; e.set_measure(mpos); return std::lower_bound(notes_.begin(), notes_.end(), e, __comp_note_pos); }
Track::const_iterator Track::end(double mpos) const
{ NoteElement e; e.set_measure(mpos); return std::upper_bound(notes_.begin(), notes_.end(), e, __comp_note_pos); }

NoteElement& Track::front() { return notes_.front(); };
NoteElement& Track::back() { return notes_.back(); };

void Track::swap(Track &track)
{
  notes_.swap(track.notes_);
}

size_t Track::size() const
{
  return notes_.size();
}

bool Track::is_empty() const
{
  return notes_.empty();
}

void Track::clear()
{
  ClearAll();
}


// ----------------------------------- NoteData

TrackData::TrackData() : is_object_duplicable_(false) {}

void TrackData::set_track_count(size_t track_count)
{
  tracks_.resize(track_count);
}

size_t TrackData::get_track_count() const
{
  return tracks_.size();
}

Track& TrackData::get_track(size_t track) { return tracks_[track]; }
const Track& TrackData::get_track(size_t track) const { return tracks_[track]; }
Track& TrackData::operator[](size_t track) { return tracks_[track]; }
const Track& TrackData::operator[](size_t track) const { return tracks_[track]; }

NoteElement* TrackData::GetObjectByPos(int measure, int nu, int de)
{
  NoteElement *n = nullptr;
  for (auto &track : tracks_)
  {
    if (n = track.GetNoteElementByPos(measure, nu, de))
      break;
  }
  return n;
}

NoteElement* TrackData::GetObjectByMeasure(double measure)
{
  NoteElement *n = nullptr;
  for (auto &track : tracks_)
  {
    if (n = track.GetNoteElementByMeasure(measure))
      break;
  }
  return n;
}

void TrackData::RemoveObjectByPos(int measure, int nu, int de)
{
  for (auto &track : tracks_)
  {
    track.RemoveNoteByPos(measure, nu, de);
  }
}

void TrackData::RemoveObjectByMeasure(double measure)
{
  for (auto &track : tracks_)
  {
    track.RemoveNoteByMeasure(measure);
  }
}

bool TrackData::IsHoldNoteAt(double measure) const
{
  for (auto &track : tracks_)
    if (!track.IsHoldNoteAt(measure)) return false;
  return false;
}

bool TrackData::IsRangeEmpty(double m_start, double m_end) const
{
  for (auto &track : tracks_)
    if (!track.IsRangeEmpty(m_start, m_end)) return false;
  return true;
}

bool TrackData::HasLongnote() const
{
  for (auto &track : tracks_)
    if (track.HasLongnote()) return true;
  return false;
}

void TrackData::clear()
{
  for (auto &track : tracks_)
    track.clear();
}

void TrackData::ClearAll() { clear(); /* alias to clear() */ }

void TrackData::ClearRange(double m_begin, double m_end)
{
  for (auto &track : tracks_)
    track.ClearRange(m_begin, m_end);
}

void TrackData::SetObjectDupliable(bool duplicable)
{
  is_object_duplicable_ = duplicable;
  for (auto &track : tracks_)
    track.SetObjectDupliable(duplicable);
}

void TrackData::CopyRange(const TrackData& from, double m_begin, double m_end)
{
  ClearAll();
  for (size_t i = 0; i < tracks_.size(); ++i)
    tracks_[i].CopyRange(from.tracks_[i], m_begin, m_end);
}

void TrackData::CopyAll(const TrackData& from)
{
  ClearAll();
  set_track_count(from.get_track_count());
  for (size_t i = 0; i < tracks_.size(); ++i)
    tracks_[i].CopyAll(from.tracks_[i]);
}

void TrackData::MoveRange(double m_delta, double m_begin, double m_end)
{
  for (auto &track : tracks_)
    track.MoveRange(m_delta, m_begin, m_end);
}

void TrackData::MoveAll(double m_delta)
{
  for (auto &track : tracks_)
    track.MoveAll(m_delta);
}

void TrackData::InsertBlank(double m_begin, double m_delta)
{
  for (auto &track : tracks_)
    track.InsertBlank(m_begin, m_delta);
}

void TrackData::RemapTracks(size_t *track_map)
{
  Track track_backup_[kMaxTrackSize];
  for (size_t i = 0; i < tracks_.size(); ++i)
  {
    track_backup_[i].swap(get_track(i));
  }
  for (size_t i = 0; i < tracks_.size(); ++i)
  {
    track_backup_[i].swap(get_track(track_map[i]));
  }
}

unsigned TrackData::GetNoteElementCount() const
{
  unsigned l = 0;
  for (auto &track : tracks_) l += track.size();
  return l;
}

unsigned TrackData::GetNoteCount() const
{
  unsigned l = 0;
  for (auto &track : tracks_)
  {
    for (auto &n : track)
    {
      if (n.chain_status() == NoteChainStatus::Tap ||
          n.chain_status() == NoteChainStatus::Start)
        l++;
    }
  }
  return l;
}

NoteElement* TrackData::front()
{
  NoteElement *n = nullptr;
  for (auto &track : tracks_) if (!track.is_empty())
    if (!n || n->measure() > track.front().measure())
      n = &track.front();
  return n;
}

NoteElement* TrackData::back()
{
  NoteElement *n = nullptr;
  for (auto &track : tracks_) if (!track.is_empty())
    if (!n || n->measure() < track.back().measure())
      n = &track.back();
  return n;
}

const NoteElement* TrackData::front() const { return const_cast<TrackData*>(this)->front(); }
const NoteElement* TrackData::back() const { return const_cast<TrackData*>(this)->back(); }


// @brief temporary struct to store note elements with track number
struct NoteElementsWithTrackNumber
{
  NoteElement *nelem;
  size_t track_idx;
};

const auto noteposcompfunc
= [](const NoteElement *a, const NoteElement *b)
  -> bool { return *a < *b; };

const auto noteposcompfuncwithtn
= [](const NoteElementsWithTrackNumber &a, const NoteElementsWithTrackNumber &b)
  -> bool { return a.nelem->measure() < b.nelem->measure(); };

template <typename TD, typename T, typename IT>
TrackData::all_track_iterator<TD, T, IT>::all_track_iterator()
  : track_(-1), pos_(.0) {}

template <typename TD, typename T, typename IT>
TrackData::all_track_iterator<TD, T, IT>::all_track_iterator(TD& td)
  : track_(-1), pos_(.0)
{
  for (size_t i = 0; i < td.get_track_count(); ++i) {
    begin_iters_.push_back(td.get_track(i).begin());
    curr_iters_.push_back(begin_iters_.back());
    end_iters_.push_back(td.get_track(i).end());
  }
}

template <typename TD, typename T, typename IT>
TrackData::all_track_iterator<TD, T, IT>::all_track_iterator(TD& td, double m_start, double m_end)
  : track_(-1), pos_(.0)
{
  for (size_t i = 0; i < td.get_track_count(); ++i) {
    begin_iters_.push_back(td.get_track(i).begin(m_start));
    curr_iters_.push_back(begin_iters_.back());
    end_iters_.push_back(td.get_track(i).end(m_end));
  }
}

template <typename TD, typename T, typename IT>
bool TrackData::all_track_iterator<TD, T, IT>::is_end() const { return track_ == -1; }

template <typename TD, typename T, typename IT>
TrackData::all_track_iterator<TD, T, IT> &TrackData::all_track_iterator<TD, T, IT>::operator++()
{ next(); return *this; }

template <typename TD, typename T, typename IT>
bool TrackData::all_track_iterator<TD, T, IT>::operator==(all_track_iterator& it) const
{
  if (track_ == -1 && it.track_ == -1) return true;
  else if (curr_iters_.size() == it.curr_iters_.size()) {
    for (unsigned i = 0; i < curr_iters_.size(); ++i) {
      if (curr_iters_[i] != it.curr_iters_[i])
        return false;
    }
    return true;
  }
  return false;
}

template <typename TD, typename T, typename IT>
bool TrackData::all_track_iterator<TD, T, IT>::operator!=(all_track_iterator& it) const
{ return !operator==(it); }

template <typename TD, typename T, typename IT>
int TrackData::all_track_iterator<TD, T, IT>::track() const
{ return track_; }

template <typename TD, typename T, typename IT>
std::pair<unsigned, T*> TrackData::all_track_iterator<TD, T, IT>::operator*()
{ ASSERT(track_ >= 0); return std::make_pair((unsigned)track_, &*curr_iters_[track_]); }

template <typename TD, typename T, typename IT>
T* TrackData::all_track_iterator<TD, T, IT>::get()
{ return track_ >= 0 ? &*curr_iters_[track_] : nullptr; }

template <typename TD, typename T, typename IT>
void TrackData::all_track_iterator<TD, T, IT>::next()
{
  track_ = -1;
  pos_ = .0;
  for (unsigned i = 0; i < curr_iters_.size(); ++i) {
    if (curr_iters_[i] != end_iters_[i] && curr_iters_[i]->measure() < pos_) {
      track_ = (int)i;
      pos_ = curr_iters_[i]->measure();
    }
  }
}

// Explicit instantiation
template class TrackData::iterator;
template class TrackData::const_iterator;

TrackData::iterator TrackData::begin()
{
  return iterator(*this);
}

TrackData::iterator TrackData::begin(double m_start, double m_end)
{
  return iterator(*this, m_start, m_end);
}

TrackData::iterator TrackData::end()
{
  return iterator();
}

TrackData::const_iterator TrackData::begin() const
{
  return const_iterator(*this);
}

TrackData::const_iterator TrackData::begin(double m_start, double m_end) const
{
  return const_iterator(*this, m_start, m_end);
}

TrackData::const_iterator TrackData::end() const
{
  return const_iterator();
}

TrackData::iterator TrackData::GetAllTrackIterator()
{
  return begin();
}

TrackData::iterator TrackData::GetAllTrackIterator(double m_start, double m_end)
{
  return begin(m_start, m_end);
}

void TrackData::swap(TrackData &data)
{
  tracks_.swap(data.tracks_);
  std::swap(name_, data.name_);
  std::swap(track_datatype_default_, data.track_datatype_default_);
  std::swap(is_object_duplicable_, data.is_object_duplicable_);
}

size_t TrackData::size() const {
  /* redirect */
  return GetNoteElementCount();
}

bool TrackData::is_empty() const {
  for (auto &track : tracks_)
    if (!track.is_empty()) return false;
  return true;
}

std::string TrackData::toString() const
{
  std::stringstream ss;
  ss << "Total track count: " << tracks_.size() << std::endl;
  ss << "Total note count: " << size() << std::endl;
  return ss.str();
}

std::string TrackData::Serialize() const
{
  /*
   * For fast serialization:
   * 1. for each track,
   * 2. If measure is not set,
   * then get denominator of note and set it as base denominator of the measure.
   * (fill all row as '00')
   * 3. Fill note key data at row.
   * 4. Merge all columns.
   *
   * XXX: if stepmania,
   * then denominator should be fixed & output need to be transposed.
   */
  std::stringstream ss;
  std::map<unsigned, std::map<unsigned, std::string> > rows;
  char tmp[16];

  for (unsigned i = 0; i < tracks_.size(); ++i) {
    const auto& track = tracks_[i];
    for (const auto& note : track) {
      const auto &rpos = note.GetRowPos();
      unsigned measure = static_cast<unsigned>(note.measure());
      if (rows[measure][i].empty()) {
        rows[measure][i] = std::string(rpos.deno * 2, '0');
      }
      sprintf(tmp, "%02X", note.get_value_u());
      rows[measure][i][rpos.num * 2] = tmp[0];
      rows[measure][i][rpos.num * 2 + 1] = tmp[1];
    }
  }

  for (const auto& s_row : rows) {
    ss << "#R" << s_row.first << "\n";
    for (const auto& s_col : s_row.second) {
      sprintf(tmp, "%03u", s_col.first);
      ss << "#" << tmp << ":" << s_col.second << "\n";
    }
  }

  return ss.str();
}

}
