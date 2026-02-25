# External Integrations

**Analysis Date:** 2026-02-25

## APIs & External Services

**LLM Providers:**
- Anthropic Claude API - used by the auto-technique generator and AjintK provider layer (`run_technique_generator.py`, `AjintK/framework/enhanced_llm_provider.py`)
  - SDK/Client: `anthropic` (`AsyncAnthropic`)
  - Auth: `ANTHROPIC_API_KEY`
- Google Gemini API - optional provider path in AjintK (`AjintK/framework/enhanced_llm_provider.py`)
  - SDK/Client: `google-generativeai`
  - Auth: `GOOGLE_API_KEY`
- DeepSeek Chat API - optional provider path (`AjintK/framework/enhanced_llm_provider.py`)
  - SDK/Client: `httpx` POST to `https://api.deepseek.com/chat/completions`
  - Auth: `DEEPSEEK_API_KEY`
- Qwen (via MuleRouter OpenAI-compatible endpoint) - optional provider path (`AjintK/framework/enhanced_llm_provider.py`)
  - SDK/Client: `httpx` POST to `https://api.mulerouter.ai/vendors/openai/v1/chat/completions`
  - Auth: `QWEN_API_KEY`
- Mistral API - optional provider path (`AjintK/framework/enhanced_llm_provider.py`)
  - SDK/Client: `mistralai` (`Mistral`)
  - Auth: `MISTRAL_API_KEY`

**Source/Release Platform:**
- GitHub Releases API - installer release metadata lookup in `install.sh`
  - SDK/Client: `curl` or `wget` against `https://api.github.com/repos/mrnob0dy666/byvalver/releases/latest`
  - Auth: Not required for public release endpoint

**Generic HTTP Fetch:**
- Arbitrary URL fetching for agent tool usage in `AjintK/framework/tools.py`
  - SDK/Client: `httpx.AsyncClient`
  - Auth: Not built in

## Data Storage

**Databases:**
- Not detected
  - Connection: Not applicable
  - Client: Not applicable

**File Storage:**
- Local filesystem only for input/output shellcode and generated artifacts (`src/main.c`, `tests/run_tests.sh`, `docker-compose.yml`)
- ML model persistence path defaults to `./ml_models/byvalver_ml_model.bin` in `src/strategy_registry.c`, `src/training_pipeline.c`, `src/train_model.c`

**Caching:**
- Local response cache in `.llm_cache.json` managed by `AjintK/framework/llm_provider_abc.py`
- Local agent memory/messages in `.agent_memory/` and `.agent_messages/` via `AjintK/framework/memory.py` and `AjintK/framework/communication.py`

## Authentication & Identity

**Auth Provider:**
- Custom
  - Implementation: Environment-variable API key loading per provider in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`

## Monitoring & Observability

**Error Tracking:**
- None

**Logs:**
- Console logging and status output in `src/main.c`, `src/strategy_registry.c`, `AjintK/framework/base_agent.py`
- Local log files include `ml_metrics.log` from `src/ml_strategist.c` and optional `agent_framework.log` in `AjintK/config_template.yaml`

## CI/CD & Deployment

**Hosting:**
- Local binary installation target (`/usr/local/bin`) in `Makefile` install target
- Containerized execution path in `Dockerfile` and `docker-compose.yml`

**CI Pipeline:**
- GitHub Actions workflow in `.github/workflows/ci.yml` (dependency install, build, test, artifact upload)

## Environment Configuration

**Required env vars:**
- `ANTHROPIC_API_KEY` - Anthropic provider auth in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- `DEEPSEEK_API_KEY` - DeepSeek provider auth in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- `QWEN_API_KEY` - Qwen provider auth in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- `MISTRAL_API_KEY` - Mistral provider auth in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- `GOOGLE_API_KEY` - Google provider auth in `run_technique_generator.py` and `AjintK/framework/enhanced_llm_provider.py`
- `HOME` - TUI path behavior in `src/tui/tui_screens.c`

**Secrets location:**
- Runtime environment variables in local shell or CI runtime
- No committed secrets file detected; `.env*` files are not present at repository root

## Webhooks & Callbacks

**Incoming:**
- None

**Outgoing:**
- HTTPS calls to LLM provider endpoints in `AjintK/framework/enhanced_llm_provider.py`
- HTTPS release metadata call to GitHub API in `install.sh`
- Optional user-targeted HTTP GET requests via `AjintK/framework/tools.py` (`web_fetch`)

---

*Integration audit: 2026-02-25*
