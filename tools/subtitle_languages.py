"""Shared language routing and TranslateGemma's documented text prompt."""
from pathlib import Path
import json

def languages():
    here=Path(__file__).resolve().parent
    path=here/'subtitle-languages.json'
    if not path.is_file():
        path=here.parent/'docs/subtitle-languages.json'
    return {x['code']:x for x in json.loads(path.read_text('utf-8'))['languages']}

def validate_pair(backend,source,target):
    data=languages()
    if source not in data or target not in data:
        raise ValueError('Unknown subtitle language')
    if source==target:
        raise ValueError('Choose different source and target languages')
    if backend not in ('openai','custom') and not backend.startswith('translategemma-') and (source,target)!=('en','cs'):
        raise ValueError('OPUS-MT and CUNI support English > Czech only. Select TranslateGemma for this language pair.')

def gemma_prompt(text,source,target):
    validate_pair('translategemma-',source,target)
    data=languages();a=data[source]['name'];b=data[target]['name']
    # Gemma role delimiters are framing, not allowed as literal OCR control tokens.
    for token in ('<start_of_turn>','<end_of_turn>','<bos>','<eos>'):
        text=text.replace(token,token.replace('<','‹').replace('>','›'))
    instruction=(f'You are a professional {a} ({source}) to {b} ({target}) translator. '
        f'Your goal is to accurately convey the meaning and nuances of the original {a} text '
        f'while adhering to {b} grammar, vocabulary, and cultural sensitivities.\n'
        f'Produce only the {b} translation, without any additional explanations or commentary. '
        f'Please translate the following {a} text into {b}:\n\n\n{text}')
    return '<start_of_turn>user\n'+instruction+'<end_of_turn>\n<start_of_turn>model\n'
