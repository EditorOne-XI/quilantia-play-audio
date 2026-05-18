# Quilantia Play-Audio

A simple music playlist wrapper built for `play-audio` utility, specifically for [**Termux**](https://github.com/termux/termux-app) terminal users.
> * Just call it Q play-audio if it's hard to pronounce /kɪˈlæn.ʃə/ (Quilantia) :D
> * The TUI was inspired from ncdu.

---

## Playback Modes

Common music player features:

* **Basic Play/Stop:** Select a music to play. Stops when triggered or the music is finished.
* **Loop Mode:** Play a single music repeatedly or cycle the playlist.
* **Queue Mode:** Select multiple music to play at Loop Mode in select order.

> [!NOTE]
> This wrapper as mentioned uses [`play-audio`](https://github.com/termux/play-audio) as a dependency, it sends audio directly to the system's output therefore **pausing is not supported**. A more common commandline tool is `mpv` and a more designed TUI music player is `kew`. However, using this is significantly smaller in size than mpv, and if you wish to have a simple TUI setup, this one is for you.

---

## Dependencies

The music playlist works with most common utilities in coreutils package such as find and grep to implement these features:

* **Folder Navigation (Manual):** Change current working directory or "Playlist". Environment shell variables works excluding the `~` variable.
* **Recursive Search:** When enabled, it will search through sub-folders to find every accepted audio files.
* **Playlist Sorting:**
    * The playlist is shuffled by default.
    * Can be sorted alphabetically, ascending or descending.
* **Playlist Filter:** By using `grep`'s syntax, it will filter the list of files to be shown.
 
For more usage, check `qpa --help` after [installation](#installation).

---

## Cava in Loop Mode (Optional)

Enabling [`cava`](https://github.com/karlstav/cava) within the Loop Mode display is a good thing! <br> It looks as simple as this:
 
<p align="center">
  <img src="samples/loop_mode_cava.gif" width="70%" alt="Loop Mode playing Aria Math by C418">
</p>

* To check if cava is available to run, check `qpa --status` after installing required dependencies from the [installation below](#install-with-cava-extension).
* Cava colors are randomly selected from these combinations:
    * Yellow-Red
    * Red-Blue
    * Blue-Magenta
    * Magenta-Cyan
    * Cyan-Blue
* Sometimes the window vanishes because of how cava were implemented here, pressing `g` or `G` reloads the stdout to show the window again when it vanished.

---

## Installation

### Prerequisite
To build this package, install these tools first: 
```bash
pkg install git clang make -y
```

When installed, clone this repo into your local device:
```bash
git clone https://github.com/EditorOne-XI/quilantia-play-audio.git
cd ./quilantia-play-audio
```

From the directory, run this to show build options:
```bash
make help
```

### Install with cava extension

> If no idea about this, check [Cava in Loop Mode](#cava-in-loop-mode-optional).

Choose whether to use [`ffmpeg`](https://github.com/ffmpeg/ffmpeg) or [`sox`](https://github.com/rbouqueau/SoX) as a data stream to cava. SoX are limited to flac, mp3, ogg, opus, and wav but smaller in package size.

```bash
# ffmpeg dependency
make dep

# sox dependency
make dep-sox
```

## Keybinds Note

List of all implemented keybinding from the code.

| Keys | Actions |
| :---: | :--- |
| j | Cursor down <br> Next (Loop Mode) |
| k | Cursor up <br> Previous (Loop Mode) |
| h | Cursor left |
| l | Cursor right |
| Enter <br> Space <br> p | Play <br> Replay (Loop Mode) <br> Add (Queue Mode) |
| s | Stop <br> Remove (Queue Mode) |
| c | variables |
| g | grep filter <br> REF_STDOUT (Loop Mode) |
| i | Toggle Metadata window |
| m <br> M | Toggle mouse/screen touch |
| r | Recursive search |
| v | Sort List <br> REF_CAVA (Loop Mode) |
| w <br> W | Loop Mode |
| y <br> Y | Toggle Queue Mode |
| C | Toggle [Cava in Loop Mode](#cava-in-loop-mode-optional) |
| D | Change directory |
| E | Force Error |
| G | REF_STDOUT (Loop Mode) |
| H <br> ? | Help window |
| N | Toggle Termux notification |
| R | RELOAD |
| T | Terminal size |
| V | REF_CAVA (Loop Mode) |
| X | Delete file |
| q <br> Q | Close/Exit (Global) |

* Terminal Resize also triggers close windows.
* Keys `g`, `G`, `v`, `V` in Loop Mode are implemented to resolve cava stdout issues.
* Actions grep filter, recursive search, and sort list also triggers RELOAD with changes.

---

Project Started on 2026, April 10th.
 
Thank You! <br> - EditorOne XI
