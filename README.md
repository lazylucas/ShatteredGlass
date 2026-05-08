# Cinematic Shattered Glass — FFGL2 Plugin for Resolume

Physically-based real-time glass fracture effect. Straight-line cracks, branching, per-shard refraction, Fresnel rim specular, spall zone, and conchoidal micro-texture.

---

## Getting the compiled DLL (no setup required)

### Method 1 — Download from GitHub Releases (easiest)

1. Go to the **Releases** tab of this repo (right sidebar)
2. Download `CinematicShatteredGlass.dll` from the latest release
3. Copy it to:
   - Arena: `%USERPROFILE%\Documents\Resolume Arena\Extra Effects\`
   - Avenue: `%USERPROFILE%\Documents\Resolume Avenue\Extra Effects\`
4. Restart Resolume → **Preferences → Video → Rescan**

Done. The effect appears as **Shattered Glass** in the Effects panel.

---

### Method 2 — Build automatically via GitHub Actions

Every push to `main` automatically compiles the DLL. To download it:

1. Click the **Actions** tab at the top of this repo
2. Click the most recent **Build & Release** run (green tick = success)
3. Scroll to the bottom → **Artifacts** → click **CinematicShatteredGlass-Windows-x64**
4. Unzip the download — you'll get `CinematicShatteredGlass.dll`
5. Copy to your Resolume Extra Effects folder (see above)

### Method 3 — Create a tagged release with the DLL attached

To create an official release with the DLL as a downloadable file:

1. Click **Actions** → **Build & Release** → **Run workflow**
2. Enter a version tag in the `Release tag` field, e.g. `v1.0.0`
3. Click **Run workflow**
4. When it finishes, go to **Releases** — your new release will have the DLL attached

Or from your local machine:
```bash
git tag v1.0.0
git push origin v1.0.0
```
GitHub Actions detects the tag push and creates the release automatically.

---

## Building locally (optional)

**Requirements:**
- Windows 10/11 x64
- [Visual Studio 2022 Community](https://visualstudio.microsoft.com/downloads/) — workload: "Desktop development with C++"
- [CMake 3.20+](https://cmake.org/download/) — check "Add CMake to system PATH"

```bat
cmake -B build -A x64
cmake --build build --config Release
```

Output: `build\bin\Release\CinematicShatteredGlass.dll`

---

## Parameters (15 total)

| # | Name | Default | Description |
|---|---|---|---|
| 1 | Impact X | 0.50 | Horizontal impact point (map to MIDI X) |
| 2 | Impact Y | 0.50 | Vertical impact point (map to MIDI Y) |
| 3 | Impact Force | 0.65 | Master fracture intensity + propagation radius |
| 4 | Crack Spread | 0.50 | Low = many tiny shards, high = few large shards |
| 5 | Radial Cracks | 0.70 | Number of primary straight cracks |
| 6 | Stress Rings | 0.40 | Concentric arc rings around impact |
| 7 | Refraction Depth | 0.50 | Per-shard tilt and image displacement |
| 8 | Shard Separation | 0.30 | Shards push apart radially from impact |
| 9 | Edge Light | 0.55 | Fresnel specular rim on crack edges |
| 10 | Chromatic | 0.30 | RGB dispersion at crack edges |
| 11 | Dirt | 0.20 | Grime in cracks, scratches on shard faces |
| 12 | Bloom | 0.40 | Light scatter around crack rims |
| 13 | Animation Speed | 0.30 | Ring propagation rate |
| 14 | Seed | 0.50 | Randomises crack pattern |
| 15 | Freeze | OFF | Lock the static pattern |

---

## Live performance tips

- **Map Impact X/Y to a MIDI XY pad** — drag to place the shatter anywhere on the image
- **Automate Impact Force with a bass-kick LFO** — glass fractures on every beat
- **Freeze + Seed** — lock a pattern you like, automate Chromatic for slow colour shifts
- **Low Crack Spread + High Radial Cracks** — dense spiderweb near impact point
- **Bullet Hole preset**: Force 1.0, Spread 0.2, Radial 0.95, Rings 0.85, Dirt 0.5

---

## How it works

Single-pass fragment shader, ~27 texture fetches per pixel. No intermediate FBOs.

**Crack geometry** — Straight polyline segments computed per-pixel via point-to-segment distance. Real tensile fracture follows minimum-energy straight paths, not curved Voronoi borders.

**Branching** — Each of the N primary cracks spawns 2 secondaries at 30–65° at 55–80% along the primary's length. Secondaries taper faster.

**Per-shard physics** — Voronoi tessellation assigns each shard a stable random ID used for: tilt normal (random orientation), face Phong shading, conchoidal texture rotation, and scratch orientation.

**Refraction** — Each shard's tilt normal drives the UV sample offset. Chromatic dispersion is radial from the tilt axis (not horizontal), scaled by distance to the nearest crack.

**Fresnel edge** — Two-band model: near-black void (crack blocks all light) + cool-white specular rim (refracted skylight on the glass edge). Much more realistic than the flat-tint approach.

**Spall zone** — At the epicentre, a pulverised-glass cloud (opaque grey-white with conchoidal noise) replaces the source image. Radius scales with Impact Force.

**Propagation** — All cracks, rings, and shard effects stop at a radius proportional to Impact Force. Far from the impact, the image is undamaged.

---

## Repo structure

```
CinematicShatteredGlass/
├── src/
│   ├── CinematicShatteredGlass.cpp   ← plugin logic + FFGL DLL exports
│   ├── CinematicShatteredGlass.def   ← Windows DLL export ordinals
│   ├── Shaders.h                     ← vertex + fragment GLSL (inline)
│   └── glext.h                       ← minimal GL extension prototypes
├── FFGLSDK/
│   └── FFGL.h                        ← FFGL 2.1 header subset
├── .github/
│   └── workflows/
│       └── build.yml                 ← CI: build + artifact + release
├── CMakeLists.txt
└── README.md
```

---

## Changing the plugin ID

If you build a variant, change the 4-byte unique ID in `CinematicShatteredGlass.cpp` to avoid collisions in Resolume:

```cpp
static const char kPluginUniqueID[4] = {'C','S','G','1'};
```

---

## License

MIT — free for commercial VJ / IMAG / broadcast use.
