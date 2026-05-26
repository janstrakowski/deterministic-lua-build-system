# Programmatic Linux (WIP)
## About (quick and dirty version edited by Gemini LLM)
Programmatic Linux (PL) is a fully reproducible Linux operating-system builder. It takes Lua code—and optionally other resources—and processes it in an isolated, deterministic environment to build an output (e.g., a complete Linux OS). Because the build environment is strictly isolated from the host system, all side-effects are eliminated. Every build is entirely deterministic: given the exact same input, it is guaranteed to produce the exact same output.

The implementation is designed to be highly performant. Written entirely in C, it completely bypasses the shell, using Lua as its sole scripting interface. It also leverages a robust caching system that stores every output indexed by a hash of its inputs. If a build is requested multiple times, only the first run is computed; subsequent runs are instantly retrieved from the cache.

The core philosophy is similar to Nix and NixOS, but it differs in several critical ways:

Language: Nix uses its own custom functional DSL for describing derivations; Programmatic Linux uses standard Lua.

Architecture: Nix relies heavily on the system shell, a massive standard environment, and a custom file hierarchy. PL is a lean, system-level C program that orchestrates the environment using only direct kernel APIs.

Portability: Nix requires a heavy installation footprint on the host device; PL can be run simply as a portable, standalone executable.
## Contribute
(WIP)
## Donate
If you would like to support me financially, you may "Buy Me a Coffee":

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/janstrakowski)
