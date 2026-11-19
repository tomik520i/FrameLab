# Testované sestavy a vytížení GPU

[Podrobná metodika a původní data v angličtině](test-system.md) · [Český návod](README.cs.md)

## Notebook — preview.12, 19. listopadu 2026

**Acer Nitro ANV15-51 · Intel Core i5-13420H (8 jader / 12 vláken) · NVIDIA GeForce RTX 4050 Laptop GPU · 32 GB RAM · Windows 11 Pro, build 26200.** Konfigurace ověřena přímo ve Windows.

Na notebooku bylo ověřeno přehrávání přibalených videí, NVIDIA VFG, místní experimentální NR enhancer, automaticky čistý fullscreen, návrat porovnání v okně a vykreslování českých titulků při 1920×1080. Samostatný test překladu použil 300 anglických herních textů a 300 vykreslených obrazů titulků přes skutečnou OCR pipeline. Vstupní testovací obrazy zahrnují 720p, 1080p, 1440p a 4K; nejde o tvrzení, že byl fyzicky otestován 4K displej. Překlad je experimentální: byly zjištěny vynechané věty a chyby herní terminologie.

Šlo o lokální videa a testovací obrazy. Na notebooku nebylo nově měřeno zpoždění PS5/střihovky ani zopakován níže uvedený test vytížení GPU. Čísla níže patří stolnímu PC s RTX 5070 Ti.

## Stolní PC — živý vstup, 18. listopadu 2026

Měřeno **při živém frame genu během pohybu ve hře, bez nahrávání a exportu**, přibližně 30 sekund na režim. Avast a běžné aplikace zůstaly spuštěné. Jde o měření celé GPU, nikoli pouze samotného FrameLabu.

Sestava: **Ryzen 9 5900X (12 jader / 24 vláken), RTX 5070 Ti 16 GB, 64 GiB RAM Kingston (4×16, hlášená rychlost 3600), Gigabyte X570 AORUS PRO, Samsung SSD 980 1 TB NVMe, Windows 11 Pro build 26200, ovladač NVIDIA 617.14**. Použitá střihovka **AVerMedia GC573**, vstup z PS5 2560×1440 přibližně 60 FPS, měnící se obsah kolem 40 FPS. Režim výstupu aplikace 2560×1440 / 165 Hz. Připojené monitory Odyssey G5 a HP Z24i G2. Druhá karta GC550 se při testu nepoužívala.

NVIDIA VFG, Medium, dedup a detekce střihu zapnuté, VSync vypnutý, porovnávací panel zapnutý:

| Průměr za živý test | 2× | 4× |
| --- | ---: | ---: |
| Vytížení GPU | **43,5 %** | **50,0 %** |
| VRAM celé GPU | 3,89 GiB | 3,96 GiB |
| Příkon GPU | **79,1 W** | **96,3 W** |
| Teplota | 53,3 °C | 54,3 °C |
| Odeslaná obrazová ID/s | 79,88 | 111,03 |
| Potvrzená ID Windows/s | **79,88** | **111,03** |
| Z toho potvrzená generovaná ID/s | 39,94 | 83,27 |
| Vzorkovaný čas VFG + kopie výstupu | 3,31 ms | 3,06 ms |
| Vstupní callback → Present | 39,06 ms | 49,46 ms |

Každý režim má 29 vzorků nvidia-smi a 31 vzorků aplikace. Šlo o různé průchody hrou, nikoli identickou nahrávku. Čas VFG je vzorek **jednoho mezisnímku**, ne součet tří fází 4×. Potvrzení Windows není měření panelu kamerou a různá ID mohou mít stejné pixely. Latence nezahrnuje zdroj, střihovku ani monitor.

Diagnostický 15sekundový 4× záznam dosáhl jen 87,87 odeslaných ID/s: kopírování a komprese záznamu přidávají práci. Proto jsou výsledky běžného provozu uvedené zvlášť. Vyšší FPS také neopravuje [viditelné artefakty 4×](../examples/84744531/README.cs.md).
