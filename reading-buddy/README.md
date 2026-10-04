# Reading Buddy — 3.2" Cheap Yellow Display (ESP32-2432S032)

Flashcard-style reading practice for a 3rd grader.

**How it works**
- A word shows up in big, plain letters. She tries to read it out loud.
- **I read it!** → she gets a star and the word shows up less often.
- **Help me** → the word splits into colored syllables with underlines ("but · ter · fly"),
  tells her how many beats to clap, and highlights sneaky letters (like the *augh* in *laugh*) in red.
  That word will come back more often until she's got it.
- A word counts as "learned" after she reads it on her own 4 times. Every 10 stars gets a little party.
- Progress is saved, so unplugging is fine.

It can't hear her, so it's best if you listen along the first few times — then it works well solo.

## Flashing it (about 10 minutes the first time)
1. Install **VS Code**, then the **PlatformIO** extension.
2. Unzip this folder and open it in VS Code (File → Open Folder).
3. Check which touch version you have (it's in the listing or on the back sticker):
   - **ESP32-2432S032R** = resistive (screen flexes a little when pressed) → env `cyd32_resistive` (default)
   - **ESP32-2432S032C** = capacitive (hard glass like a phone) → env `cyd32_capacitive`
   Choose it in the PlatformIO env picker at the bottom of VS Code.
4. Plug the board in with a USB **data** cable and click the → (Upload) arrow.
5. First boot shows **"Grown-up setup"** — tap the 3 red dots. That lines up the touchscreen.

## Changing the words
Edit `src/words.h`. Use `-` to split syllables and `[ ]` around tricky letters:
`"but-ter-fly"`, `"fr[ie]nd"`, `"e-n[ough]"`. Her weekly spelling list works great here.
Changing the list resets progress automatically.

## Grown-up controls
- **Hold the blue top bar for 3 seconds** → reset stars and progress.
- **Hold a finger on the screen while plugging it in** → redo the touch setup.

## If something looks off
- **Blank screen but it uploaded:** unplug/replug. Make sure you picked the right env.
- **Colors wrong** (cream looks blue, red looks blue): in `platformio.ini`, add `-DTFT_RGB_ORDER=TFT_BGR` under `[env] build_flags`.
- **Looks like a photo negative:** add `-DTFT_INVERSION_OFF=1` in the same place.
- **Taps land in the wrong spot:** redo the touch setup (above).
- **Upload fails / no port:** try another USB cable (many are charge-only), or hold the **BOOT** button while it starts uploading.
