#include "midiJukebox.h"

bool checkRootDir(File& aFile) 
{
  if(!aFile) {
    Serial.println("Failed to open root dir!");
    return false;
  }
  if(!aFile.isDirectory()) {
    Serial.println("Not a directory!");
    return false;
  }

  return true;
}

String getNextMidiFileNameFromDir(Dir& dir)
{
  String midiFileName;

  if(dir.next())
    midiFileName = dir.fileName();
  else
    midiFileName = "NoMoreMidiFileInDir";

  return midiFileName;
}

String getNextMidiFileNameFromDir(File& aFile)
{
  File midiFile = aFile.openNextFile();

  if(!midiFile)
    return "NoMidiFile";
    
  Serial.print("MIDI file: ");
  String midiFileName = midiFile.name();
  Serial.println(midiFileName);
  midiFileName = "//" + midiFileName;  
  return midiFileName;
}
