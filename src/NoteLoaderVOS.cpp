/*
 * VOS has quite weird format; it contains MIDI file,
 * and it just make user available to play, following MIDI's beat.
 * So, All tempo change/beat/instrument information is contained at MIDI file.
 * VOS only contains key & timing data; NO BEAT DATA.
 * That means we need to generate beat data from time data,
 * So we have to crawl BPM(Tempo) data from MIDI. That may be a nasty thing. 
 */