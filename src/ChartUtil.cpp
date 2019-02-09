#include "ChartUtil.h"



namespace rparser
{

// ------ class NoteSelection ------

void NoteSelection::SelectNote(Note* n)
{
	auto it = std::lower_bound(m_vNotes.begin(), m_vNotes.end(), n, nptr_less_row());
	m_vNotes.insert(it, n);
}
void NoteSelection::UnSelectNote(const Note* n)
{
	for (int i = 0; i < m_vNotes.size(); i++)
	{
		if (*m_vNotes[i] == *n)
		{
			m_vNotes.erase(i + m_vNotes.begin());
			break;
		}
	}
}
std::vector<Note*>& NoteSelection::GetSelection()
{
	return m_vNotes;
}
std::vector<Note*>::iterator NoteSelection::begin()
{
	return m_vNotes.begin();
}
std::vector<Note*>::iterator NoteSelection::end()
{
	return m_vNotes.end();
}
void NoteSelection::Clear()
{
	m_vNotes.clear();
}

}