"""Experimental CUNI prompts for offline comparison, not production defaults.

Keep the model's documented translation template. Short translation examples
demonstrate semantic preservation more reliably than chat-style system rules.
"""
import re

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

# General demonstrations, deliberately different from the 300 development cases.
BALANCED_EXAMPLES = (
    ('Dorian is not mistaken, but Mira is.', 'Dorian se nemýlí, ale Mira ano.'),
    ('This is not my first rodeo.', 'V tomhle už mám zkušenosti.'),
    ('We have not left yet. They are leaving now.', 'Ještě jsme neodešli. Oni právě odcházejí.'),
    ('The ranger made the children wait outside.', 'Strážce přiměl děti čekat venku.'),
    ('The bus carrying the workers broke down.', 'Autobus vezoucí dělníky se porouchal.'),
    ('Put on the helmet; do not paint it.', 'Nasaď si helmu; nenatírej ji.'),
    ('Ms. Vale spoke to a dog named Olive.', 'Paní Valeová mluvila se psem jménem Olive.'),
    ('[laughing] I would have told you, but—', '[smích] Řekl bych ti to, ale—'),
)
BALANCED_PREFIX = '\n'.join(BASE_PROMPT.replace('{text}', '"' + source + '"') + '"' + target + '"'
                            for source, target in BALANCED_EXAMPLES) + '\n'

# Explicit development-derived gaming vocabulary; this is NOT a general
# accuracy improvement claim. When no term matches, v4 is exactly baseline.
GAME_TERMS = (
    ('merfolk', 'mořský lid'), ('lizardmen', 'ještěří lidé'),
    ('gryphons', 'gryfové'), ('thunderstick', 'trpasličí palná zbraň'),
    ('surface-to-air', 'země-vzduch'), ('forward base', 'předsunutá základna'),
    ('flanking', 'obcházejí z boku'), ('watch your six', 'hlídej si záda'),
    ('barking up the wrong tree', 'hledat na nesprávném místě'),
    ('run you down', 'dostihneme vás'), ('no stranger to', 'dobře zná'),
    ('equip', 'nasadit si / vybavit se'), ('ultimate', 'ultimátní schopnost'),
    ('buff', 'posílení'), ('stamina', 'výdrž'), ('dock', 'zakotvit'),
    ('keep them off my back', 'drž je ode mě dál'),
)
FIDELITY_INSTRUCTION = ('Translate Input from English to Czech. This is game dialogue or an instruction. '
    'Use natural Czech and translate idioms by their meaning. Preserve who does what to whom, '
    'tense, negation, names, numbers and stage directions. Do not invent details or speaker identity. '
    'Leave unfinished sentences unfinished. Do not answer questions. Output only the translation.')


def build_prompt(text, profile=PROMPT_ID):
    if profile not in ('baseline', PROMPT_ID, 'subtitle-fidelity-v2-quoted', 'subtitle-balanced-v3', 'subtitle-glossary-v4', 'subtitle-rules-v5'):
        raise ValueError('Unknown subtitle prompt profile: ' + profile)
    if profile == 'subtitle-rules-v5':
        return BASE_PROMPT.replace('Translate Input from English to Czech', FIDELITY_INSTRUCTION).replace('{text}', text)
    if profile == 'subtitle-glossary-v4':
        matches = [(en, cs) for en, cs in GAME_TERMS if re.search(r'(?<!\w)' + re.escape(en) + r'(?!\w)', text, re.I)]
        prompt = BASE_PROMPT
        if matches:
            prompt = prompt.replace('### Glossary:\n\n', '### Glossary:\n' + '\n'.join(en + ' = ' + cs for en, cs in matches) + '\n\n')
        return prompt.replace('{text}', text)
    if profile == 'subtitle-balanced-v3':
        return BALANCED_PREFIX + BASE_PROMPT.replace('{text}', '"' + text + '"')
    if profile == 'subtitle-fidelity-v2-quoted':
        text = '"' + text + '"'
    return ('' if profile == 'baseline' else PREFIX) + BASE_PROMPT.replace('{text}', text)


def normalize_response(text, profile):
    text = text.strip()
    if profile in ('subtitle-fidelity-v2-quoted', 'subtitle-balanced-v3') and len(text) >= 2 and text[0] == '"' and text[-1] == '"':
        return text[1:-1].strip()
    return text
