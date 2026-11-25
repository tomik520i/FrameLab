# FrameLab 0.2.0-preview.19

## English

The complete portable download now includes custom AI server support and optional
conversation memory. Existing local models, OpenAI, window capture, subtitle-only
overlay, RIFE/DirectML, Optical Flow, NVIDIA VFG and the NR import wizard remain
available. Unpack the ZIP into a **new folder** and run `1 - Spustit FrameLab.cmd`.
Keep the old folder until the new version works; do not overwrite your downloaded
models, recordings or settings. GPU model/runtime directories can be copied from
the previous installation's matching `app/translation/models` and
`app/translation/gpu-runtime` folders. Do not copy old Python helper files or DLLs
over the new package. API credentials remain in Windows Credential Manager.

- Connect an OpenAI-compatible or Ollama server, refresh models or enter its ID,
  configure supported parameters and optionally store a key per endpoint.
- Enable recent source dialogue and, separately, background selection of important
  original lines by the same chosen model. Both default off. CUNI cannot build the
  selected long memory. Translation specialists may perform worse with context.
- Inspect the context used for the last translation. Stop and restart translation
  to clear it. Failed memory selection falls back to recent dialogue.
- Complete online/offline Czech and English server and memory guides are included.
- The published 25 Czech + 50 multilingual evaluation contains 300 actual native
  runs and 900 original framebuffer proofs, with OCR, VFG and NR active. It keeps
  wrong, stale, late and missing results, exact timing, model settings and sources.
  Memory improved the aggregate result, but did not improve topic changes. These
  are controlled diagnostic scenes, not general game accuracy. The measured Gemma
  ran on a separate RTX 3090 server. Codex reviewed the outputs without a paid judge.

The full package contains both game samples, the OCR sample, OCR language packs,
OPUS CPU translation, the local entity filter, RIFE/DirectML, existing licensed
runtime dependencies and illustrated guides. Large optional GGUF models download
inside the application. External NR model DLLs are **not** redistributed; use the
included import wizard and instructions. This release does not change the already
measured inference policy or fix the recorded topic-switch failures.

## Česky

Kompletní přenosný balíček nyní obsahuje vlastní AI server a volitelnou paměť
rozhovoru. Dosavadní místní modely, OpenAI, zachytávání okna, samostatné titulky,
RIFE/DirectML, Optical Flow, NVIDIA VFG i průvodce importem NR zůstávají dostupné.
ZIP rozbal do **nové složky** a spusť `1 - Spustit FrameLab.cmd`. Starou složku
ponech, dokud novou verzi neověříš; nepřepisuj stažené modely, záznamy a nastavení.
GPU modely a jejich runtime lze přenést ze stejných složek `app/translation/models`
a `app/translation/gpu-runtime` předchozí instalace. Staré pomocné Python soubory
ani DLL nekopíruj přes nové. Klíče API zůstávají ve Správci pověření Windows.

- Připojení OpenAI-compatible nebo Ollama serveru, načtení seznamu či ruční zadání
  modelu, podporované parametry a volitelný klíč uložený pro danou adresu.
- Krátká historie původních titulků a samostatná možnost uchovat důležité repliky,
  které vybírá tentýž model na pozadí. Výchozí stav obou je vypnuto. CUNI delší
  výběr paměti nepodporuje; překladovým specialistům může kontext škodit.
- Zobrazení skutečně použitého kontextu. Paměť smaže zastavení a opětovné spuštění
  překladu. Při chybě výběru se použijí poslední repliky.
- České a anglické návody k vlastnímu serveru a paměti na webu i uvnitř ZIPu.
- Dokončených 25 českých + 50 vícejazyčných scén: 300 běhů aplikace, 900 původních
  snímků, zapnuté OCR, VFG a NR. Zahrnuté jsou chybné, staré, opožděné i chybějící
  titulky. Paměť zlepšila souhrnný výsledek, ale samotné změny tématu ne. Jde o
  řízený diagnostický test, ne obecnou přesnost ve hrách. Gemma běžela na oddělené
  serverové RTX 3090. Výstupy přímo posoudil Codex bez placeného hodnoticího API.

Přibalené jsou obě herní ukázky, OCR ukázka, jazykové balíčky OCR, překladač OPUS
pro CPU, místní filtr, RIFE/DirectML, dosavadní licencované runtime komponenty a
obrazové návody. Velké volitelné GGUF modely se stahují v aplikaci. Externí modelové
DLL pro NR **nejsou přibalené**; slouží k nim průvodce importem a návod. Vydání
nemění již naměřené chování překladače ani neopravuje zaznamenané chyby změny tématu.

[Download / Stažení](https://opicebot.cz/framelab/#download) ·
[75 scenarios / 75 situací](https://opicebot.cz/framelab/runs/2026-11-24-context75/) ·
[Server guide](https://opicebot.cz/framelab/guide/custom-ai-en.html) ·
[Paměť rozhovoru](https://opicebot.cz/framelab/guide/memory.html)
