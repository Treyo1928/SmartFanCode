#!/usr/bin/env python3
import mido
from collections import defaultdict

def midi_to_frequency(note):
    """Convert MIDI note number to frequency in Hz with validation"""
    freq = 440.0 * (2.0 ** ((note - 69) / 12.0))
    return int(freq) if 31 < freq < 5000 else 0  # Filter unrealistic frequencies

def process_midi_file(filename, track_num=0, tempo=500000, quantization=50, max_notes=200):
    """
    Process a MIDI file into monophonic Arduino tone arrays
    Args:
        filename: Path to MIDI file
        track_num: Which track to process (default 0)
        tempo: Microseconds per quarter note (default 500000 = 120 BPM)
        quantization: Round note durations to nearest X ms (default 50)
        max_notes: Maximum number of notes to process (default 200)
    Returns:
        Tuple of (melody_array, duration_array, size) as strings
    """
    try:
        mid = mido.MidiFile(filename)
    except Exception as e:
        print(f"Error loading MIDI file: {e}")
        return None, None, 0

    # Convert MIDI ticks to time in seconds
    ticks_per_beat = mid.ticks_per_beat
    
    # Process events into absolute time
    events = []
    current_time = 0  # in ticks
    
    for msg in mid.tracks[track_num]:
        current_time += msg.time
        events.append((current_time, msg))
    
    # Process events to get monophonic melody
    melody = []
    durations = []
    last_note_time = 0
    active_notes = []
    
    for tick, msg in events:
        time_ms = mido.tick2second(tick, ticks_per_beat, tempo) * 1000
        
        if msg.type == 'note_on' and msg.velocity > 0:
            active_notes.append((msg.note, time_ms))
        
        elif (msg.type == 'note_off' or 
              (msg.type == 'note_on' and msg.velocity == 0)):
            # Find the note to turn off
            for i, (note, start_time) in enumerate(active_notes):
                if note == msg.note:
                    duration_ms = time_ms - start_time
                    quantized_duration = max(quantization, round(duration_ms / quantization) * quantization)
                    
                    # Add the note if it's valid
                    freq = midi_to_frequency(note)
                    if freq > 0:
                        # Add rest if needed
                        if start_time > last_note_time:
                            rest_duration = start_time - last_note_time
                            quantized_rest = max(quantization, round(rest_duration / quantization) * quantization)
                            melody.append(0)
                            durations.append(quantized_rest)
                        
                        melody.append(freq)
                        durations.append(quantized_duration)
                        last_note_time = time_ms
                    
                    active_notes.pop(i)
                    break
    
    # Trim to max_notes
    melody = melody[:max_notes]
    durations = durations[:max_notes]
    
    # Format as Arduino arrays
    melody_str = "const int melody[] = {\n  " + ",\n  ".join(
        [", ".join(map(str, melody[i:i+8])) for i in range(0, len(melody), 8)]
    ) + "\n};"
    
    duration_str = "const unsigned int duration[] = {\n  " + ",\n  ".join(
        [", ".join(map(str, durations[i:i+8])) for i in range(0, len(durations), 8)]
    ) + "\n};"
    
    return melody_str, duration_str, len(melody)

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python midi_to_arduino.py <midi_file> [track_num] [tempo] [quantization]")
        print("Example: python midi_to_arduino.py song.mid 0 500000 50")
        sys.exit(1)
        
    filename = sys.argv[1]
    track_num = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    tempo = int(sys.argv[3]) if len(sys.argv) > 3 else 500000
    quantization = int(sys.argv[4]) if len(sys.argv) > 4 else 50
    
    melody, duration, size = process_midi_file(filename, track_num, tempo, quantization)
    
    if melody and duration:
        print("// Generated from:", filename)
        print("// Track number:", track_num)
        print("// Tempo:", tempo, "microseconds per quarter note")
        print("// Quantization:", quantization, "ms")
        print("// Number of notes:", size)
        print("\n" + melody + "\n")
        print(duration + "\n")
        print(f"const int sizeofMelody = sizeof(melody) / sizeof(melody[0]);")
    else:
        print("Failed to process MIDI file")