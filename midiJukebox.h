#ifndef MIDIJUKEBOX_H
#define MIDIJUKEBOX_H

#include <LittleFS.h>

bool checkRootDir(File& aFile);
String getNextMidiFileNameFromDir(File& aFile);
String getNextMidiFileNameFromDir(Dir& dir);

#endif