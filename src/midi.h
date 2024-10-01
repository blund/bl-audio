
typedef enum Midi_Message{
  MIDI_NOTE_ON        = 0x90,
  MIDI_NOTE_OFF       = 0x80,
  MIDI_CONTROL_CHANGE = 0xB0,
} Midi_Message;

typedef struct Midi_Note {
  Midi_Message message;
  char channel;
  union {
    char note;
    char controller;
  };
  union {
    char vel;
    char val;
  };
} Midi_Note;

int read_midi_note_from_buf(unsigned char* buffer, int* index, Midi_Note* note);
