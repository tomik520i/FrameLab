"""Preview.17 walkthrough; retain labelled historical captures of unchanged steps."""
from visual_guide_preview16 import content as previous

def content(legacy):
    shots, steps = previous(legacy)
    shots['fg'] = ('preview17/framegen-backends.png', (1171,714), (750,235,420,440), [
        (('Tři výpočetní backendy','Three frame-generation backends'),775,300,800,286,355,108)],
        ('Skutečný vývojový snímek na RTX 4050. RIFE a Optical Flow mají vlastní cestu pro vhodná GPU jiných výrobců; text AMD v nabídce není potvrzením otestované karty.',
         'Real RTX 4050 development capture. RIFE and Optical Flow have portable GPU paths; the AMD menu label is not evidence of an AMD hardware test.'))
    shots['models'] = ('preview17/settings-ja.png',(1171,714),(750,260,420,450),[
        (('Zdroj a cílový jazyk','Source and target language'),775,365,800,339,345,80),
        (('Model a stav','Model and status'),775,575,800,563,345,65),
        (('Zapnout překlad','Enable translation'),775,679,800,664,200,30)],
        ('Skutečný test EN → JA na RTX 4050. Ve vývojové kopii byla Q4 stažená. Ve veřejném ZIPu ji nejprve stáhneš tlačítkem.',
         'Actual EN → JA test on RTX 4050. Q4 was already downloaded in the development copy; download it in the public package first.'))
    shots['gemma-download'] = ('preview17/download-q8.png',(1171,714),(750,260,420,450),[
        (('Volba modelu','Choose a model'),775,575,800,563,345,28),
        (('Stažení a ověření','Download and verify'),775,646,800,634,205,28)],
        ('Q8 je dostupná ke stažení, nikoli otestovaná. Na 6GB GPU začni menší Q4.',
         'Q8 is available to download, not benchmarked. Start with smaller Q4 on a 6GB GPU.'))
    new=[]
    for key,heading,body,expected,pictures in steps:
        body=tuple(x.replace('v0.2.0-preview.16','v0.2.0-preview.17').replace('FrameLab-0.2.0-preview.16','FrameLab-0.2.0-preview.17').replace('Download preview.16','Download preview.17').replace('Stáhnout preview.16','Stáhnout preview.17') for x in body)
        if key=='fg':
            body=('<p>V <strong>Frame gen</strong> zvol <strong>RIFE 4.22 lite / DirectML</strong>, <strong>D3D11 Optical Flow</strong> nebo <strong>NVIDIA VFG</strong>. Začni s 2×. RIFE na RTX 4050 zkus Low; na zde testovaném Intel UHD začni Optical Flow. Modely RIFE jsou v ZIPu.</p><p>RIFE potřebuje vhodnou DirectX 12 GPU, Optical Flow Direct3D 11. AMD a starší GeForce nebyly fyzicky otestované. VFG zůstává pro RTX 40/50; NR má samostatná omezení. <a href="gpu.html">Tabulka GPU a měření →</a></p>',
                  '<p>In <strong>Frame gen</strong>, choose <strong>RIFE 4.22 lite / DirectML</strong>, <strong>D3D11 Optical Flow</strong> or <strong>NVIDIA VFG</strong>. Start at 2×. Try RIFE Low on RTX 4050; use Optical Flow on the tested Intel UHD. RIFE models are bundled.</p><p>RIFE needs suitable DirectX 12 hardware; Optical Flow uses Direct3D 11. AMD and older GeForce have not been physically tested. VFG remains RTX 40/50; NR has separate limits. <a href="gpu-en.html">GPU matrix and measurements →</a></p>')
        if key=='models':
            heading=('Vyber jazyky, OCR a překladový model','Choose languages, OCR and a translation model')
            body=('<p>V <strong>Titulky</strong> zvol jazyk v obraze a cílový jazyk. <strong>RapidOCR / PP-OCRv5 multilingual</strong> je přibalené se všemi 12 sadami. Pro EN → CS funguje hned <strong>OPUS-MT / CPU</strong>; volitelné CUNI Q3/Q5/Q8 zůstávají pro stejný pár.</p><p>Pro jiné jazyky zvol <strong>TranslateGemma 4B Q4_K_M</strong>, klikni <strong>Stáhnout / pokračovat a ověřit</strong> a počkej. Pak zapni <strong>Překládat titulky</strong>. Q4 má 2,49 GB; první GPU runtime navíc 1,62 GB. Q8 a 12B jsou větší neotestované volby. <a href="multilingual.html">Paměť, jazyky a omezení →</a></p>',
                  '<p>In <strong>Subtitles</strong>, choose the language in the picture and the target. <strong>RapidOCR / PP-OCRv5 multilingual</strong> includes all 12 packs. <strong>OPUS-MT / CPU</strong> works immediately for EN → CS; optional CUNI Q3/Q5/Q8 retain that same pair.</p><p>For other languages choose <strong>TranslateGemma 4B Q4_K_M</strong>, click <strong>Download / resume and verify</strong>, and wait. Then enable <strong>Translate subtitles</strong>. Q4 is 2.49 GB; first GPU runtime adds 1.62 GB. Q8 and 12B are larger untested options. <a href="multilingual-en.html">Memory, languages and limits →</a></p>')
            expected=('Připravený model a nový titulek ve zvoleném jazyce. První načtení může trvat desítky sekund. Malá Gemma chybuje, zejména v češtině.',
                      'A ready model and a caption in the chosen language. First loading can take tens of seconds. Small Gemma makes mistakes, especially in Czech.')
            pictures=['models','gemma-download']
        if key=='nr':
            prefix=('<p><strong>NR je na nepodporovaném hardwaru blokované.</strong> AMD, Intel a GTX tímto krokem nezískají DLSS. Externí NR DLL nejsou v ZIPu ani na zrcadle.</p>',
                    '<p><strong>NR is blocked on unsupported hardware.</strong> This step does not bring DLSS to AMD, Intel or GTX. External NR DLLs are not bundled or mirrored.</p>')
            body=tuple(prefix[i]+body[i] for i in range(2))
        new.append((key,heading,body,expected,pictures))
    return shots,new
