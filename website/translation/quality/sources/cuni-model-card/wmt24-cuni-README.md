---
language:
- cs
- en
base_model:
- mistralai/Mistral-7B-v0.1
pipeline_tag: translation
library_name: transformers
license: mit
---
# Model Card for Model ID

<!-- Provide a quick summary of what the model is/does. -->

**CUNI-MH** is a English-to-Czech translation model built on top of Mistral-7B-v0.1 for WMT24. It was trained using QLoRA+SFT, SLERP merging, LoRA+Contrastive Preference Optimization (CPO), linear weight merging. For details, please check the paper. Note that the model was fine-tuned for the translation task only and we don't expect it to perform well in other tasks. It may also be very sensitive to using the exact prompt template that was used during the training. Note that we **did not** train it on Czech->English translion.

## Model Details

### Model Description

<!-- Provide a longer summary of what this model is. -->

- **Developed by:** hrabal@ufal.mff.cuni.cz
- **Language(s) (NLP):** Czech, English
- **License:** MIT

### Model Sources [optional]

<!-- Provide the basic links for the model. -->

- **Repository:** [wmt24-cuni/CUNI-MH](https://huggingface.co/wmt24-cuni/CUNI-MH/)
- **Paper [optional]:** [CUNI at WMT24 General Translation Task: LLMs, (Q)LoRA, CPO and Model Merging](https://aclanthology.org/2024.wmt-1.16/)

## Usage

We recommend using vLLM for high-throughput inference, either directly or using the OpenAI-like server. If you use the model, please make sure to use the correct prompt template, as we expect the model to be sensitive to its changes.

### vLLM Python

A minimal usage example using vLLM:

```python
import vllm


MODEL = "wmt24-cuni/CUNI-MH"
llm = vllm.LLM(
    MODEL,
    enforce_eager=True,
    seed=42,
)


def format_prompt(src_lang, tgt_lang, src):
    return "### Instruction:\nTranslate Input from {src_lang} to {tgt_lang}\n### Glossary:\n\n### Previous text:\n\n### Input:\n{src}\n### Response:\n".format(
        src_lang=src_lang,
        tgt_lang=tgt_lang,
        src=src,
    )

prompt = format_prompt("English", "Czech", "Yesterday, there was a whole lot of umbrellas falling from the sky.")


sampling_params = vllm.SamplingParams(
    temperature=0,
    max_tokens=512,
    stop=["\n"],
)

outputs = llm.generate(
    prompt,
    sampling_params=sampling_params,
)
print(outputs)
```

### vLLM OpenAI-like Server

```bash
vllm serve \
        wmt24-cuni/CUNI-MH \
        --dtype auto \
        --host 0.0.0.0 \
```

### Notes

Note that for the WMT25 submission, we split the input segments into segments of at most 256 input tokens on sentence boundaries.
For this purpose, we used the sentence-splitter library.

## Citation [optional]

<!-- If there is a paper or blog post introducing the model, the APA and Bibtex information for that should go in this section. -->

**BibTeX:**

```
@inproceedings{hrabal-etal-2024-cuni,
    title = "{CUNI} at {WMT}24 General Translation Task: {LLM}s, ({Q}){L}o{RA}, {CPO} and Model Merging",
    author = "Hrabal, Miroslav  and
      Jon, Josef  and
      Popel, Martin  and
      Luu, Nam  and
      Semin, Danil  and
      Bojar, Ond{\v{r}}ej",
    editor = "Haddow, Barry  and
      Kocmi, Tom  and
      Koehn, Philipp  and
      Monz, Christof",
    booktitle = "Proceedings of the Ninth Conference on Machine Translation",
    month = nov,
    year = "2024",
    address = "Miami, Florida, USA",
    publisher = "Association for Computational Linguistics",
    url = "https://aclanthology.org/2024.wmt-1.16/",
    doi = "10.18653/v1/2024.wmt-1.16",
    pages = "232--246",
    abstract = "This paper presents the contributions of Charles University teams to the WMT24 General Translation task (English to Czech, German and Russian, and Czech to Ukrainian), and the WMT24 Translation into Low-Resource Languages of Spain task.Our most elaborate submission, CUNI-MH for en2cs, is the result of fine-tuning Mistral 7B v0.1 for translation using a three-stage process: Supervised fine-tuning using QLoRA, Contrastive Preference Optimization, and merging of model checkpoints. We also describe the CUNI-GA, CUNI-Transformer and CUNI-DocTransformer submissions, which are based on our systems from the previous year.Our en2ru system CUNI-DS uses a similar first stage as CUNI-MH (QLoRA for en2cs) and follows with transferring to en2ru.For en2de (CUNI-NL), we experimented with a LLM-based speech translation system, to translate without the speech input.For the Translation into Low-Resource Languages of Spain task, we performed QLoRA fine-tuning of a large LLM on a small amount of synthetic (backtranslated) data."
}
```

## Model Card Contact

[More Information Needed]