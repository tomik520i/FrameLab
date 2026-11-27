# Subtitle recovery checks

Run `python -B -m unittest discover -s tests -p "test_*.py" -v` from the source root. The adapter tests use mocks or localhost; they do not call paid APIs.

Build with `BUILD_TESTING=ON`, then run `ctest --test-dir build -C Release --output-on-failure`.

The native integration harness needs the clean full portable package for its bundled Python and model/language manifests:

```powershell
python -B tests/run_openai_deadline_pipeline.py --base-app "D:/FrameLab/app" --binary "build/Release/openai_deadline_pipeline_test.exe" --output "build/deadline-results"
build/Release/custom_prompt_editor_test.exe tests/fixtures/subtitle-settings-v14.txt build/editor-results
```

The deadline test copies workers into a separate temporary app, supplies synthetic OCR and redirects only that copy to a localhost fake API with a dummy key. It runs the actual C++ queue and Python adapter. It verifies a two-second deadline, obsolete queued captions being discarded, continued translation, 1000 output tokens and retained history. It never reads the Windows credential vault. Output includes synthetic request traces and test paths; review them before sharing.

The editor test checks v14 migration, v15 persistence, draft/apply/reset behavior and modal rendering in English/Czech at two viewport sizes. It does not capture the desktop.

`subtitle_dialogue_pipeline_test` is a separate manual real-OCR/server test. It requires generated BGRA fixtures and an explicitly supplied authorized endpoint matching the test app's settings. It sends fixture text to that server and is not part of the automatic CTest suite.
