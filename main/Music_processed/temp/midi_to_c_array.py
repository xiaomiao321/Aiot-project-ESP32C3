import mido
import os
import re

def sanitize_c_identifier(name):
    """将字符串转换为合法的 C 标识符"""
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    if name and name[0].isdigit():
        name = '_' + name
    return name

def midi_to_c_array(filename):
    try:
        mid = mido.MidiFile(filename)
    except Exception as e:
        print(f"Error opening MIDI file {filename}: {e}")
        return

    melody = []      # 频率数组，0 表示停顿
    durations = []   # 持续时间（毫秒）
    
    ticks_per_beat = mid.ticks_per_beat if mid.ticks_per_beat else 480
    tempo = 500000  # 默认 120 BPM

    # 获取第一个 set_tempo 事件
    for track in mid.tracks:
        for msg in track:
            if msg.type == 'set_tempo' and msg.is_meta:
                tempo = msg.tempo
                break
        if tempo != 500000:
            break

    # 合并所有 track 的消息，并按时间排序
    events = []
    for track in mid.tracks:
        absolute_time = 0
        for msg in track:
            absolute_time += msg.time
            if msg.type in ('note_on', 'note_off') or (msg.type == 'note_on' and msg.velocity == 0):
                events.append({'time': absolute_time, 'message': msg})

    events.sort(key=lambda x: x['time'])  # 按时间排序

    active_notes = {}  # (note, channel) -> start_tick
    current_time = 0   # 当前时间（ticks）
    first_note_played = False  # 标志：是否已播放第一个音符

    for event in events:
        tick = event['time']
        msg = event['message']

        # === 计算当前时间和上一个事件之间的空白 ===
        gap_ticks = tick - current_time
        if gap_ticks > 0 and len(active_notes) == 0:
            gap_ms = int(mido.tick2second(gap_ticks, ticks_per_beat, tempo) * 1000)

            # 只有在第一个音符之后，且时长 ≤ 5000ms 才记录空白
            if first_note_played and gap_ms <= 5000:
                melody.append(0)
                durations.append(gap_ms)
            # 如果还没开始，且有空白，跳过（不记录开头空白）

        current_time = tick

        # === 处理 note_on ===
        if msg.type == 'note_on' and msg.velocity > 0:
            key = (msg.note, msg.channel)
            if key not in active_notes:
                active_notes[key] = tick

        # === 处理 note_off 或 note_on velocity=0 ===
        elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
            key = (msg.note, msg.channel)
            if key in active_notes:
                start_tick = active_notes.pop(key)
                note_duration_ticks = tick - start_tick
                note_duration_ms = int(mido.tick2second(note_duration_ticks, ticks_per_beat, tempo) * 1000)

                frequency = int(440 * (2 ** ((msg.note - 69) / 12)))
                melody.append(frequency)
                durations.append(note_duration_ms)

                # 标记已播放第一个音符
                if not first_note_played:
                    first_note_played = True

    if not melody:
        print(f"Warning: No notes or valid rests found in {filename}")
        return

    # === 生成唯一数组名 ===
    base_name = os.path.splitext(os.path.basename(filename))[0]
    safe_name = sanitize_c_identifier(base_name.lower())
    melody_array_name = f"melody_{safe_name}"
    durations_array_name = f"durations_{safe_name}"

    # === 写入头文件 ===
    output_filename = f"{base_name}.h"
    try:
        with open(output_filename, 'w') as f:
            f.write(f"// Generated from {filename}\n")
            f.write(f"// Song length: {len(melody)} events (notes + rests)\n")
            f.write(f"// Skipped leading rests and rests > 5000ms\n\n")

            # 旋律数组
            f.write(f"const int {melody_array_name}[] PROGMEM = {{\n")
            for i, note in enumerate(melody):
                f.write(f"  {note},")
                if (i + 1) % 10 == 0:
                    f.write("\n")
                else:
                    f.write(" ")
            f.write("\n};\n\n")

            # 时长数组
            f.write(f"const int {durations_array_name}[] PROGMEM = {{\n")
            for i, duration in enumerate(durations):
                f.write(f"  {duration},")
                if (i + 1) % 10 == 0:
                    f.write("\n")
                else:
                    f.write(" ")
            f.write("\n};\n")

            # 长度宏
            f.write(f"\n#define MELODY_{safe_name.upper()}_LENGTH {len(melody)}\n")

        print(f"✅ Successfully wrote: {output_filename} ({len(melody)} events)")
        print(f"   Arrays: {melody_array_name}[], {durations_array_name}[]")
        if not any(melody):  # 全是 0？
            print("   ⚠️  Warning: melody contains only rests or low notes")
    except Exception as e:
        print(f"❌ Error writing {output_filename}: {e}")

# ========================
# Main Execution
# ========================
if __name__ == "__main__":
    current_dir = "."
    midi_files = [f for f in os.listdir(current_dir) if f.lower().endswith(('.mid', '.midi'))]

    if not midi_files:
        print("No MIDI files found in the current directory.")
    else:
        print(f"Found {len(midi_files)} MIDI file(s):")
        for f in midi_files:
            print(f"  → {f}")
        print("-" * 40)

        for midi_file in midi_files:
            midi_to_c_array(midi_file)