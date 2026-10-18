# Repository rules

These instructions apply to the entire repository. Keep repository-specific
agent rules in this file; the root `AGENTS.md` points here.

## Project context

- This is a C application using Zephyr on Raspberry Pi Pico 2 W, despite the
  repository name. The board target is `rpi_pico2/rp2350a/m33_0/w`.
- `src/main.c` wires together application threads and HTTP routes.
- `lib/wifi` owns Wi-Fi connection and reconnection management.
- `lib/wifi_api` owns the reusable HTTP server. Public library headers live in
  each library's `include` directory.
- `prj.conf` contains Zephyr configuration; `CMakeLists.txt` files define sources
  and library integration. `.vscode/tasks.json` defines local build/flash tasks.

## Making changes

- Read the relevant code and `README.md` before editing. Keep changes focused on
  the requested task and preserve unrelated user changes.
- Follow the coding conventions in `doc/contributing.md`. Before running any
  tooling mentioned there, verify its scripts and configuration exist locally;
  do not assume formatting hooks or CI are installed.
- Follow the surrounding C style and document public interfaces and non-obvious
  behavior. Keep reusable library logic separate from application-specific routes.
- Use Zephyr APIs and existing project patterns. Avoid busy waiting; use kernel
  sleeps, synchronization, or events as appropriate.
- Check error returns and buffer bounds. Consider thread stack use, shared state,
  and object lifetimes when changing concurrent or networking code.
- Preserve the `#pico-zephyr-project` marker in the root `CMakeLists.txt`.
- Do not hand-edit generated files in `build/` or treat old build logs as proof
  that a new change works.
- Keep real Wi-Fi credentials in the ignored `src/wifi_env.h`. Never commit or
  print passwords or introduce credentials into documentation or logs.
- Update user-facing documentation when setup, configuration, APIs, or behavior
  changes. Keep these rules aligned with intentional project structure changes.

## Validation

- For firmware changes, use the configured **Compile Project** / **Build WiFi**
  VS Code task when available. For command-line builds, use the configured Zephyr
  workspace and toolchain with the board target above; do not guess local SDK paths.
- Run checks relevant to the change. Documentation-only edits do not require a
  firmware build.
- Report what was checked and any missing tools or hardware that prevented
  validation. Distinguish a successful build from verification on the device.
- Flash or run firmware on hardware only when the task calls for it. For HTTP
  changes, use the endpoint and Wi-Fi recovery checks documented in `README.md`
  when hardware is available.

## Completion

- Summarize the changes, validation results, and remaining limitations briefly.
- Do not commit, push, or publish changes unless requested.
