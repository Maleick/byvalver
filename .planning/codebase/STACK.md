# Technology Stack

**Analysis Date:** 2026-02-25

## Languages

**Primary:**
- C (C99) - Core CLI, transformation engine, ML strategist, and TUI in `src/main.c`, `src/core.c`, `src/strategy_registry.c`, `src/ml_strategist.c`, `src/tui/tui_menu.c`

**Secondary:**
- Python (3.6+ documented minimum) - Verification tooling and agent pipeline in `verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`, `run_technique_generator.py`, `agents/implementation_agent.py`, `AjintK/framework/base_agent.py`
- Bash - Build, test, and install automation in `tests/run_tests.sh` and `install.sh`

## Runtime

**Environment:**
- Native POSIX runtime for compiled executable (`src/main.c`) with optional ncurses TUI (`src/tui/tui_menu.c`)
- Python 3 runtime for verification tooling and agent workflows (`requirements-dev.txt`, `AjintK/requirements.txt`)
- Containerized Ubuntu runtime in `Dockerfile`

**Package Manager:**
- OS package managers for toolchain/deps (apt/dnf/brew/pacman) documented in `CONTRIBUTING.md` and `docs/BUILD.md`
- Python package installation via `pip` from `requirements-dev.txt` and `AjintK/requirements.txt`
- Lockfile: missing

## Frameworks

**Core:**
- Capstone Disassembly Framework (4.0+ documented) - instruction decode/analysis across processing and training paths in `src/core.c`, `src/training_pipeline.c`, `src/utils.h`, with linker integration in `Makefile`
- ncurses (optional) - interactive TUI in `src/tui/tui_menu.c`, `src/tui/tui_widgets.h`, with feature gating in `Makefile`
- AjintK framework (`__version__ = "2.0.0"`) - async multi-agent orchestration in `AjintK/framework/__init__.py`, `AjintK/framework/orchestrator.py`, `AjintK/framework/base_agent.py`

**Testing:**
- Custom Bash and Python verification flow (no pytest/unittest test framework detected) in `tests/run_tests.sh`, `verify_denulled.py`, `verify_functionality.py`, `verify_semantic.py`

**Build/Dev:**
- GNU Make build pipeline in `Makefile`
- NASM + xxd decoder generation in `decoder.asm` and `Makefile`
- Docker and Docker Compose environments in `Dockerfile` and `docker-compose.yml`
- GitHub Actions CI in `.github/workflows/ci.yml`

## Key Dependencies

**Critical:**
- `capstone` / `libcapstone-dev` - core disassembly dependency used by strategy engine and training pipeline (`Makefile`, `src/core.c`, `src/strategy.h`, `docs/BUILD.md`)
- `ncurses` / `libncurses-dev` - required for TUI mode (`Makefile`, `src/tui/tui_menu.c`, `src/tui/tui_menu.h`)
- `anthropic`, `google-generativeai`, `mistralai`, `httpx`, `tenacity`, `pyyaml` - agent pipeline provider and transport stack (`AjintK/requirements.txt`, `run_technique_generator.py`, `AjintK/framework/enhanced_llm_provider.py`)

**Infrastructure:**
- `gcc`, `make`, `nasm`, `xxd`, `pkg-config` - native build chain (`Makefile`, `CONTRIBUTING.md`, `.github/workflows/ci.yml`)
- `python3` and `objdump` (binutils) - verification tooling runtime (`requirements-dev.txt`, `verify_functionality.py`)

## Configuration

**Environment:**
- LLM provider credentials are supplied through environment variables (`ANTHROPIC_API_KEY`, `DEEPSEEK_API_KEY`, `QWEN_API_KEY`, `MISTRAL_API_KEY`, `GOOGLE_API_KEY`) in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- Runtime behavior is CLI-flag-driven (`--arch`, `--profile`, `--bad-bytes`, `--ml`, `--format`) via `src/cli.c` and `docs/USAGE.md`
- No `.env` files detected at repository root (listing of `.env*` returned no files)

**Build:**
- Primary build configuration in `Makefile`
- CI build recipe in `.github/workflows/ci.yml`
- Container build config in `Dockerfile` and `docker-compose.yml`
- Optional agent-framework config template in `AjintK/config_template.yaml`

## Platform Requirements

**Development:**
- Linux and macOS are directly supported; Windows path is WSL/MSYS2 oriented (`docs/BUILD.md`, `README.md`)
- C toolchain + Capstone + NASM + optional ncurses required for full local development (`CONTRIBUTING.md`, `.github/workflows/ci.yml`)
- Python 3 required for verification and agent scripts (`requirements-dev.txt`, `AjintK/requirements.txt`)

**Production:**
- Primary deployment artifact is compiled CLI binary `bin/byvalver` built via `Makefile`
- Optional containerized runtime via `Dockerfile` and `docker-compose.yml`
- Managed cloud deployment target: Not detected

---

*Stack analysis: 2026-02-25*
