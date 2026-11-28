# Future features

Ideas for more background effects and a more interesting random face, not done yet.
Every effect is computed from the time alone (no state between frames), and all of them fit that, except Game of Life.

## Effects

### Easy: they reuse the Cube's code (3D, smooth lines, bouncing)
- **Shapes**: other wireframe solids (pyramid, octahedron, icosahedron), or one that switches shape at each bounce.
  Each is just a list of corners and edges.
- **Lissajous**: a glowing curve, like an oscilloscope, that slowly changes shape as its two frequencies drift.
  Very cheap.
- **Spirograph**: a looping flower curve that draws itself, fades, and starts again with new ratios.

### Tied to the clock
- **Seconds runner**: a light with a fading tail that runs around the edge of the panel once a minute, so it's
  also a seconds hand.
- **Orbits**: three dots orbiting the middle at the speed of the hour, minute and second hands, a hidden analog
  clock.

### Others
- **Bouncing balls**: balls dropping with gravity and bouncing on the floor. The arcs follow a formula, so they're
  computed from the time too.
- **Code rain**: columns of falling tiny characters (the "digital rain" look), using a 3x5 digit font. Clearly
  different from Rain.
- **Ocean**: layered waves at the bottom of the panel, the far ones dimmer and slower.
- **Equalizer**: bars jumping like a spectrum analyzer. Later it could react to the audio we play.
- **Game of Life**: the only one that would need memory between frames. Doable, but it breaks the current model.

## A more interesting random face

1. **Themes** (top pick): today every choice is made separately. A theme would pick the effect, palette, fonts and
   spacing together, so a face looks designed rather than shuffled. For example:
   - *Night*: dim blues with Twinkle or Snow.
   - *Fire*: warm colors with Fire or Lava and a heavy font.
   - *Retro terminal*: one green, the digital font, Cube or Radar.
   - *Ocean*: teal and blue with Aurora.

   Pure random stays one of the "themes", so the current variety isn't lost.
2. **Texts filled with the effect**: the digits are colored by Plasma, Fire or Aurora inside their own shape, and
   the background stays dark. The most striking change. It costs more: the effect is computed into a buffer and the
   text is drawn through it as a mask.
3. **Color harmony**: palettes built on color theory (complementary, triadic, analogous, monochrome with one accent)
   instead of a base hue plus steps.
4. **New layouts**:
   - big stacked HH over MM filling the panel, with a small date in a corner;
   - the date tucked into a corner;
   - an occasional vertical text in portrait mode.
5. **Time-aware**: calmer, dimmer and warmer faces at night, livelier ones in the day, and maybe Snow more likely in
   winter.
6. **Transitions**: when the face changes, a crossfade or wipe instead of a jump.
7. **Slow drift**: the texts drift a pixel or two over the minutes. Subtle, keeps a face from feeling static, and
   reduces burn-in.

Suggested first picks: Themes, texts filled with the effect, and the Seconds runner.
