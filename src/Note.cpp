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
  : time_msec_(0), measure_(.0), num_(0), deno_(0), chain_status_(NoteChainStatus::Tap)
{
  memset(&point_, 0, sizeof(point_));
  memset(&v_, 0, sizeof(v_));
}

void NoteElement::SetRowPos(int measure, RowPos deno, RowPos num)
{
  ASSERT(deno > 0);
  this->measure_ = measure + (double)num / deno;
  this->deno_ = deno;
  this->num_ = num;
}

/* @warn acually it uses measure value */
void NoteElement::set_measure(double measure)
{
  this->measure_ = measure;
  // row pos is set in 16th note if no denominator is set.
  if (deno_ == 0) deno_ = 16;
  num_ = static_cast<uint16_t>((this->measure_ - measure) * deno_);
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
  this->deno_ = denominator;
  num_ = static_cast<uint16_t>(fmod(measure_, 1.0) * deno_);
}

std::string NoteElement::toString() const
{
  std::stringstream ss;
  ss << "Measure: " << measure_ << " / Time: " << time_msec_ << " (" << num_ << " / " << deno_ << ")" << std::endl;
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

void NoteElement::set_value(NoteSoundProperty &sprop)
{
  v_.sprop = sprop;
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

const NoteSoundProperty& NoteElement::get_value_sprop() const
{
  return v_.sprop;
}

NoteSoundProperty& NoteElement::get_value_sprop()
{
  return v_.sprop;
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


void Track::AddNoteElement(const NoteElement& object)
{
  // for fast-append logic
  if (!notes_.empty() && notes_.back().measure() >= object.measure())
  {
    // if object is not duplicatable
    if (!is_object_duplicable_ && notes_.back().measure() == object.measure())
    {
      // if end of longnote, pop invalid notes
      if (object.chain_status() == NoteChainStatus::End)
      {
        while (notes_.back().chain_status() == NoteChainStatus::Tap && notes_.size() > 1)
          notes_.pop_back();
      }
      notes_.back() = object;
      return;
    }
    notes_.push_back(object);
    return;
  }
  // search position to insert note
  auto it = std::upper_bound(notes_.begin(), notes_.end(), object);
  // if end of longnote, pop all invalid tapnotes
  if (object.chain_status() == NoteChainStatus::End && it != notes_.begin())
  {
    auto it_end = std::prev(it);
    auto it_begin = it_end;
    while (it_begin != notes_.begin())
    {
      if (it_begin->chain_status() != NoteChainStatus::Tap)
        break;
      it_begin = std::prev(it_begin);
    }
    notes_.erase(it_begin, it_end);
  }
  notes_.insert(it, object);
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

void Track::GetNoteElementsByRange(double m_start, double m_end, std::vector<NoteElement*> &out)
{
  for (size_t i = 0; i < notes_.size(); ++i)
  {
    if (notes_[i].measure() >= m_start)
    {
      while (i < notes_.size())
      {
        // TODO: check for longnote
        out.push_back(&notes_[i]);
        if (notes_[i].measure() > m_end)
          return;
        ++i;
      }
      break;
    }
  }
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

Track::iterator Track::begin() { return notes_.begin(); }
Track::iterator Track::end() { return notes_.end(); }
Track::const_iterator Track::begin() const { return notes_.begin(); }
Track::const_iterator Track::end() const { return notes_.end(); }
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

TrackData::TrackData() {}

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

TrackData::all_track_iterator::all_track_iterator(TrackData *td) : idx_(0)
{
  std::vector<NoteElement*> nv;
  std::vector<NoteElementsWithTrackNumber> ntv;
  for (size_t i = 0; i < td->get_track_count(); ++i)
  {
    td->get_track(i).GetAllNoteElements(nv);
    for (auto *ne : nv)
      ntv.emplace_back(NoteElementsWithTrackNumber{ ne, i });
    nv.clear();
  }

  /* Use stable_sort to keep track order.
   * This result sorting in first position, then track. */
  std::stable_sort(ntv.begin(), ntv.end(), noteposcompfuncwithtn);

  for (auto &nt : ntv)
  {
    all_notes_.push_back(nt.nelem);
    track_numbers_.push_back(nt.track_idx);
  }
}

TrackData::all_track_iterator::all_track_iterator(TrackData *td, double m_start, double m_end) : idx_(0)
{
  std::vector<NoteElement*> nv;
  std::vector<NoteElementsWithTrackNumber> ntv;
  for (size_t i = 0; i < td->get_track_count(); ++i)
  {
    td->get_track(i).GetNoteElementsByRange(m_start, m_end, nv);
    for (auto *ne : nv)
      ntv.emplace_back(NoteElementsWithTrackNumber{ ne, i });
    nv.clear();
  }

  /* Use stable_sort to keep track order.
   * This result sorting in first position, then track. */
  std::stable_sort(ntv.begin(), ntv.end(), noteposcompfuncwithtn);

  for (auto &nt : ntv)
  {
    all_notes_.push_back(nt.nelem);
    track_numbers_.push_back(nt.track_idx);
  }

#if 0
  for (size_t i = 0; i < td->get_track_count(); ++i)
    td->get_track(i).GetNoteElementsByRange(m_start, m_end, all_notes_);
  /* Use stable_sort to keep track order.
   * This result sorting in first position, then track. */
  std::stable_sort(all_notes_.begin(), all_notes_.end(), noteposcompfunc);
#endif
}

bool TrackData::all_track_iterator::is_end() const { return all_notes_.size() == idx_; }
TrackData::all_track_iterator &TrackData::all_track_iterator::operator++() { next(); return *this; }
NoteElement& TrackData::all_track_iterator::operator*() { ASSERT(idx_ < all_notes_.size()); return *all_notes_[idx_]; }
const NoteElement& TrackData::all_track_iterator::operator*() const { ASSERT(idx_ < all_notes_.size()); return *all_notes_[idx_]; }
double TrackData::all_track_iterator::get_measure() const { return (**this).measure(); }
size_t TrackData::all_track_iterator::get_track() const { return track_numbers_[idx_]; }
NoteElement* TrackData::all_track_iterator::get() { return &**this; }

void TrackData::all_track_iterator::next()
{
  if (idx_ < all_notes_.size()) idx_++;
}

TrackData::all_track_iterator TrackData::GetAllTrackIterator()
{
  return all_track_iterator(this);
}

TrackData::all_track_iterator TrackData::GetAllTrackIterator(double m_start, double m_end)
{
  return all_track_iterator(this, m_start, m_end);
}

TrackData::row_iterator::row_iterator(TrackData *td)
  : all_track_iterator(td), is_row_iter_end(false), measure(0)
{
  track_count = td->get_track_count();
  memset(curr_row_note_elem, 0, sizeof(curr_row_note_elem));
  memset(curr_row_longnote, 0, sizeof(curr_row_longnote));
  // set first track info by calling next() method
  next();
}

TrackData::row_iterator::row_iterator(TrackData *td, double m_start, double m_end)
  : all_track_iterator(td, m_start, m_end), is_row_iter_end(false), measure(0)
{
  track_count = td->get_track_count();
  memset(curr_row_note_elem, 0, sizeof(curr_row_note_elem));
  memset(curr_row_longnote, 0, sizeof(curr_row_longnote));
  // set first track info by calling next() method
  next();
}

double TrackData::row_iterator::get_measure() const { return measure; }

void TrackData::row_iterator::next()
{
  NoteElement *curr_nelem;
  memset(curr_row_note_elem, 0, sizeof(curr_row_note_elem));
  if (all_track_iterator::is_end())
  {
    is_row_iter_end = true;
    return;
  }

  // update current measure to new one
  measure = all_track_iterator::get_measure();

  while (!all_track_iterator::is_end() && measure == all_track_iterator::get_measure())
  {
    curr_nelem = all_track_iterator::get();
    curr_row_note_elem[get_track()] = curr_nelem;
    if (curr_nelem->chain_status() == NoteChainStatus::Start)
      curr_row_longnote[get_track()] = true;
    else if (curr_nelem->chain_status() == NoteChainStatus::End)
      curr_row_longnote[get_track()] = false;
    all_track_iterator::next();
  }
}

bool TrackData::row_iterator::is_end() const { return is_row_iter_end; }

bool TrackData::row_iterator::is_longnote() const
{
  for (size_t i = 0; i < track_count; ++i)
    if (curr_row_longnote[i]) return true;
  return false;
}

TrackData::row_iterator &TrackData::row_iterator::operator++() { next(); return *this; }
NoteElement& TrackData::row_iterator::operator[](size_t i) { return *curr_row_note_elem[i]; }
const NoteElement& TrackData::row_iterator::operator[](size_t i) const { return *curr_row_note_elem[i]; }
double TrackData::row_iterator::operator*() const { return get_measure(); }

NoteElement* TrackData::row_iterator::get(size_t idx)
{
  return curr_row_note_elem[idx];
}

TrackData::row_iterator TrackData::GetRowIterator()
{
  return row_iterator(this);
}

TrackData::row_iterator TrackData::GetRowIterator(double beat_start, double beat_end)
{
  return row_iterator(this, beat_start, beat_end);
}


void TrackData::swap(TrackData &data)
{
  tracks_.swap(data.tracks_);
  std::swap(name_, data.name_);
  std::swap(track_datatype_default_, data.track_datatype_default_);
}

size_t TrackData::size() const {
  size_t cnt = 0;
  for (auto &track : tracks_)
    cnt += track.size();
  return cnt;
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

}
