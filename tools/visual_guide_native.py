"""24 Nov native PNG captures; coordinates refer to untouched source pixels."""
def current_shot(key, lang):
    if key not in ('overview', 'fg', 'models'): return None
    panel = 'subtitles' if key == 'models' else 'framegen'
    filename = f'current/preview18-{panel}-{lang}-native.png'
    capture = ('Skutečná preview.18 na RTX 4050, 24. 11. 2026. Bezztrátový PNG 1920 × 1020. ',
               'Actual preview.18 on RTX 4050, 24 November 2026. Lossless 1920 × 1020 PNG. ')
    if key == 'models':
        crop=(1415,270,505,750)
        marks=[(('Jazyk v obraze a jazyk překladu','Source and target language'),1438,488,1458,456,435,98),
               (('Lokální model','Local model'),1438,750,1458,731,435,37),
               (('Výpočet překladu','Translation compute'),1438,815,1458,796,435,37)]
        note=('Ukázka nastavení EN → JA; v této čisté kopii není Gemma stažená a překlad není zapnutý. Výsledky skutečného výpočtu jsou v jazykovém testu.',
              'EN → JA settings example; Gemma is not downloaded in this clean copy and translation is off. Actual inference outputs are in the language test.')
    elif key == 'fg':
        crop=(1415,270,505,490)
        marks=[(('Volba backendu','Backend selection'),1438,366,1458,349,442,35),
               (('Zapnutí a kvalita','Enable and quality'),1438,406,1458,389,291,35),
               (('Násobič snímků','Frame multiplier'),1438,475,1458,451,442,43)]
        note=('Zde je zapnuté NVIDIA VFG 2×; RIFE a Optical Flow vybereš v nabídce backendu. Čítače jsou okamžitý stav, nikoli benchmark.',
              'NVIDIA VFG 2× is selected here; choose RIFE or Optical Flow in the backend menu. Counters show an instant, not a benchmark.')
    else:
        crop=None; marks=[]
        note=('Přiložená GTA ukázka, NVIDIA VFG 2×, NR vypnuté. Herní video má 1280 × 720; kvalita záznamu hry se nemění. Čítače nejsou benchmark.',
              'Bundled GTA sample, NVIDIA VFG 2×, NR off. Gameplay video is 1280 × 720; its recording quality is unchanged. Counters are not a benchmark.')
    return filename,(1920,1020),crop,marks,tuple(capture[i]+note[i] for i in range(2))
