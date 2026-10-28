# LINKED LIST ARENA

An interactive, arcade-style **singly linked list visualizer** written in **C++ / raylib**.
Insert nodes by shooting them out of a cannon, delete them by blowing them up, traverse the
list by lighting the nodes like bulbs — and when an operation is impossible, watch a cursor
walk the list, hit `NULL`, and explain exactly which precondition failed.

Every box on screen is a real `Node` allocated with `new`. Every arrow is a real `next`
pointer. Every explosion is a real `delete`.

---

## 1. Architecture (and the assumptions behind it)

```
src/
  main.cpp          window, main loop, control-panel wiring, demo mode
  LinkedList.h/.cpp the real data structure: Node{data,next} + head pointer
  Operation.h/.cpp  the "step machine": plans + runs one animated operation
  Visualizer.h/.cpp layout, node boxes, arrows, HEAD/NULL markers, cannon, overlay
  Animation.h/.cpp  particles, explosions, screen shake, easing curves
  UI.h/.cpp         buttons, text fields, panels, word-wrapped text
```

The key design idea is the **step machine** in `Operation.cpp`:

1. You click an operation. `Operation::begin()` *plans* it against the **real** list.
2. Planning is where validation happens. If the operation is impossible, the plan becomes a
   teaching animation (walk the cursor, reach `NULL`, explain the rule) and **no pointer is
   ever touched**.
3. If it is possible, the plan is a list of `Step` objects. Exactly one of them — the
   `Commit` step — carries an `onBegin()` callback that performs the real pointer surgery by
   calling `LinkedList::insertAfter()`, `removeFront()`, etc.
4. `Operation::update(dt)` walks the steps using frame timing. Pause / Resume / Next Step /
   the speed control simply modulate that walk.

So the picture can never drift from memory: the renderer draws whatever the list currently
contains, and the mutation happens *inside* the step the animation is currently showing.

**Assumptions**

* raylib **5.5** (the code also compiles against 5.0 — the one signature that changed,
  `DrawRectangleRoundedLines`, is wrapped in `UIRoundedLines()`).
* C++17, MinGW-w64 `g++` on Windows 11 (MSVC works too, see §2C).
* Node data is a 32-bit `int`; the value field accepts up to 5 characters incl. a minus sign.
* Positions shown in the UI are **1-based**, consistently (`#1` is the first node).
* Visual-only effects: no audio, so no sound assets are needed.
* Rendering state (a node's screen position / glow) lives in a small `NodeVisual` struct
  inside `Node`. The list operations never read or write it; only `Visualizer` does. That
  keeps "where the box is" out of the algorithm while avoiding a parallel lookup table.

---

## 2. Installing raylib on Windows 11 + VS Code

### A. The easy path (recommended): raylib's own installer with w64devkit

1. Go to <https://github.com/raysan5/raylib/releases> and download
   **`raylib-5.5_win64_mingw-w64.zip`** (the MinGW build, *not* the MSVC one).
2. Extract it so you end up with:
   ```
   C:\raylib\raylib-5.5_win64_mingw-w64\include\raylib.h
   C:\raylib\raylib-5.5_win64_mingw-w64\lib\libraylib.a
   ```
3. You also need a compiler. Either:
   * run the **raylib Windows installer** (`raylib_installer_v5.5.exe` from the same releases
     page), which drops a ready-to-use `g++` in `C:\raylib\w64devkit\bin`, **or**
   * download **w64devkit** from <https://github.com/skeeto/w64devkit/releases> and extract it
     to `C:\raylib\w64devkit`.
4. Add the compiler to your PATH so VS Code can find it:
   * Press `Win`, type *"environment variables"* → **Edit the system environment variables**
     → **Environment Variables…** → under *User variables* select **Path** → **Edit** →
     **New** → paste `C:\raylib\w64devkit\bin` → OK on every dialog.
   * Open a **new** terminal and check:
     ```
     g++ --version
     ```
5. In VS Code install the extension **C/C++** (by Microsoft).

### B. Build and run

Open this folder in VS Code (`File → Open Folder…`), open a terminal
(`` Ctrl+` ``) and run **one** of these:

```bat
REM one-click: builds and launches
build.bat
```

```bat
REM or the explicit command (all on one line)
g++ -std=c++17 -O2 -Wall src\*.cpp -o LinkedListArena.exe -I C:\raylib\raylib-5.5_win64_mingw-w64\include -L C:\raylib\raylib-5.5_win64_mingw-w64\lib -lraylib -lopengl32 -lgdi32 -lwinmm
LinkedListArena.exe
```

```bat
REM or with make
mingw32-make
```

`.vscode/tasks.json` is included, so **Ctrl+Shift+B** builds too. If you extracted raylib
somewhere else, change the path in **three** places: `build.bat` (`RAYLIB_PATH`),
`.vscode/tasks.json`, and `.vscode/c_cpp_properties.json`.

**Order matters on the command line:** the `.cpp` files come *before* `-lraylib`, and
`-lraylib -lopengl32 -lgdi32 -lwinmm` stay in that order. Swapping them is the usual cause of
`undefined reference to 'InitWindow'`.

### C. Alternative: MSVC + vcpkg

```bat
vcpkg install raylib:x64-windows
cl /std:c++17 /EHsc src\*.cpp /I <vcpkg>\installed\x64-windows\include ^
   /link /LIBPATH:<vcpkg>\installed\x64-windows\lib raylib.lib
```

### D. Linux / macOS

```bash
sudo apt install libraylib-dev      # or: brew install raylib
make && ./LinkedListArena
```

### Troubleshooting

| Symptom | Fix |
| --- | --- |
| `'g++' is not recognized` | w64devkit's `bin` folder is not on PATH (step A4), or you did not open a *new* terminal. |
| `fatal error: raylib.h: No such file` | The `-I` path is wrong — it must point at the folder that literally contains `raylib.h`. |
| `undefined reference to 'InitWindow'` | The `-L` path is wrong, you used the MSVC zip with MinGW, or the libraries come before the sources. |
| `undefined reference to 'DrawRectangleRoundedLinesEx'` | You are on raylib < 5.5; the code supports it, but make sure you are not mixing a 5.5 `raylib.h` with a 5.0 `libraylib.a`. |
| Window opens then closes instantly | Run it from a terminal to see the error message. |

---

## 3. Controls

**Input fields** — click a field to focus it, `TAB` switches, `BACKSPACE` deletes,
`DELETE` clears. `VALUE` accepts digits and a leading minus; `POSITION` is 1-based.

| Button | Key | What it does |
| --- | --- | --- |
| INS FRONT | `Q` | Insert VALUE at position 1 — O(1) |
| INS END | `W` | Append VALUE — O(n), because there is no tail pointer |
| INSERT AT POSITION | `E` | Insert VALUE at POSITION — O(n) |
| DEL FRONT | `A` | Delete the first node — O(1) |
| DEL END | `S` | Delete the last node — O(n) |
| DEL AT POS | `D` | Delete the node at POSITION |
| DEL VALUE | `F` | Delete the first node holding VALUE |
| SEARCH | `Z` | Linear search for VALUE |
| TRAVERSE | `X` | Light up every node, print the values |
| COUNT | `C` | Walk and increment a visible counter |
| CLEAR LIST | `V` | Animated chain demolition, then `head = nullptr` |
| PAUSE / RESUME | `SPACE` | Freeze the current animation |
| NEXT STEP | `N` | Advance one algorithm step (use with STEP MODE) |
| STEP MODE | `M` | Stop at every step until you press NEXT |
| DEMO MODE | `G` | Automatic tour of all the operations, successes and failures |
| SPEED − / + | `-` / `=` | 0.25× … 3× animation speed |
| RESET LIST | `R` | Instantly free every node and start over |

The mouse wheel scrolls the arena when the list grows past the visible rows (nodes wrap onto
new rows automatically, with the `next` arrow curving down to the next row).

---

## 4. Example demonstrations

### A successful insert in the middle

1. Type `10` → `Q`, then `20` → `W`, then `30` → `W`. You now have
   `HEAD → [10] → [20] → [30] → NULL`.
2. Type VALUE `99`, POSITION `2`, press `E`.
3. Watch: the cursor walks HEAD → node 1 (it must stop at position 1, the node *before* the
   insert point) → the cannon fires `99` into the gap → the panel shows
   `newNode->next = current->next;  current->next = newNode;` → confetti,
   **NODE INSERTED SUCCESSFULLY!** Result: `HEAD → [10] → [99] → [20] → [30] → NULL`.

Turn on **STEP MODE** (`M`) first and press `N` to walk through those pointer updates one at
a time.

### A failed insert (the headline example)

With `HEAD → [10] → [20] → [30] → NULL`, type VALUE `99`, POSITION `8`, press `E`:

* a red **INSERTION FAILED** banner appears,
* the cursor visits node 1, node 2, node 3 and then slides onto the `NULL` marker,
* the panel explains: *there are only 3 nodes; to insert at position 8 a previous node at
  position 7 must exist; traversal reached NULL before reaching that position; the list was
  NOT modified.*

Check the arena afterwards — nothing changed, and no node was allocated.

### Other failures worth trying

| Try this | What you are taught |
| --- | --- |
| `R`, then `A` (delete front on an empty list) | `head == nullptr`, so there is nothing to unlink or free |
| `R`, then `X` (traverse an empty list) | `while (current != nullptr)` never runs |
| `R`, then `Z` with any value | search cannot even start from a null head |
| Search for a value that is not there | every node compared, pointer reaches NULL ⇒ definitely absent |
| `DEL VALUE` with a value that is not there | same walk, and nothing is freed |
| `DEL AT POS` with a position past the end | traversal reaches NULL before that position |
| Insert at position `0` or a negative one | positions are 1-based; nothing to redirect |

Press `G` for **DEMO MODE**, which plays a scripted tour: three inserts, an insert in the
middle, a traversal, a successful search, a failing search, a failing insert at position 8,
a delete-by-value, a delete-from-end, a failing delete, a count and finally the animated
clear.

---

## 5. How the linked list connects to the animations

* **The list is the single source of truth.** `Visualizer::update()` walks `head → next →
  next …` every frame and assigns each node a target slot. The boxes you see are produced by
  that walk, so the on-screen order *is* the pointer order — it is not an array being drawn
  left to right.
* **Arrows are drawn from `cur` to `cur->next`.** A node whose `next` is `nullptr` draws the
  `0`/slash symbol in its `next` cell and points at the `NULL` marker instead.
* **Insertion:** planning allocates `new Node(value)` immediately, but the node is *not* in
  the list yet — it is the projectile. While the `Shoot` step runs, the visualizer reports the
  projectile's position back to the operation. The `Commit` step drops the node at exactly
  that position and calls `insertFront()` / `insertAfter()`. From the next frame on, the
  normal layout pass eases it into its slot and pushes the following nodes along. If you press
  `RESET` mid-flight, `Operation::cancel()` deletes that unlinked node so it cannot leak.
* **Deletion:** the `Arm` and `Explode` steps only set the red pulse and spawn particles — the
  node is still fully alive and linked. The `Commit` step is what calls `removeFront()` /
  `removeAfter()`, which unlinks first and `delete`s second. Because steps are strictly
  sequential, no later step ever dereferences the freed node.
* **Traversal / search / count** generate one step per node *at plan time by following the
  real `next` chain*, so the lights come on in pointer order and the spark travels along the
  actual arrow that was followed.
* **Memory:** `RESET` and `~LinkedList()` both call `clear()`, which saves `next` before each
  `delete`. The whole project was tested headless under AddressSanitizer / UBSan across every
  operation, including aborted insertions — no leaks, no dangling reads.

---

## 6. If you want to extend it

Good next exercises, in increasing difficulty:

1. **Reverse the list** — three pointers (`prev`, `cur`, `next`), one step per iteration;
   it fits the step machine perfectly.
2. **Insert in sorted order** — a walk plus the existing `insertAfter` commit.
3. **Detect a cycle** (Floyd's tortoise and hare) — two cursors instead of one.
4. **Doubly linked list** — add a `prev` pointer and draw the back-arrows; deletion becomes
   O(1) once you hold the node.

Each one is a new `plan*()` function in `Operation.cpp` plus a button in `main.cpp`; the
renderer needs no changes at all.
