"""Experimental CUNI prompts for offline comparison, not production defaults.

Keep the model's documented translation template. Short translation examples
demonstrate semantic preservation more reliably than chat-style system rules.
"""

BASE_PROMPT = '### Instruction:\nTranslate Input from English to Czech\n### Glossary:\n\n### Previous text:\n\n### Input:\n{text}\n### Response:\n'
PROMPT_ID = 'subtitle-fidelity-v1'
EXAMPLES = (
    ('He is not wrong.', 'Nemýlí se.'),
    ('Pepper is my dog. Pepper is not a vegetable.',
     'Pepper je můj pes. Pepper není zelenina.'),
    ('Someone keeps calling the QRS on me about my cat.',
     'Někdo mě pořád nahlašuje QRS kvůli mé kočce.'),
    ("I'm fine. Just keeping busy.", 'Mám se dobře. Pořád mám co dělat.'),
)
PREFIX = '\n'.join(BASE_PROMPT.replace('{text}', source) + target
                   for source, target in EXAMPLES) + '\n'


def build_prompt(text, profile=PROMPT_ID):
    if profile not in ('baseline', PROMPT_ID, 'subtitle-fidelity-v2-quoted'):
        raise ValueError('Unknown subtitle prompt profile: ' + profile)
    if profile == 'subtitle-fidelity-v2-quoted':
        text = '"' + text + '"'
    return ('' if profile == 'baseline' else PREFIX) + BASE_PROMPT.replace('{text}', text)


def normalize_response(text, profile):
    text = text.strip()
    if profile == 'subtitle-fidelity-v2-quoted' and len(text) >= 2 and text[0] == '"' and text[-1] == '"':
        return text[1:-1].strip()
    return text
