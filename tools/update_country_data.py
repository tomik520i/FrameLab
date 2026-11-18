"""Refresh local CZ/SK IPv4/IPv6 country ranges; never sends visitor IPs."""
from pathlib import Path
import datetime, hashlib, ipaddress, json, urllib.request
root = Path(__file__).resolve().parents[1]
out = root/'website/country-data'
out.mkdir(parents=True,exist_ok=True)
sources=[]
for country in ['cz','sk']:
    for family,url in [
        ('ipv4',f'https://www.ipdeny.com/ipblocks/data/countries/{country}.zone'),
        ('ipv6',f'https://www.ipdeny.com/ipv6/ipaddresses/aggregated/{country}-aggregated.zone')
    ]:
        with urllib.request.urlopen(url,timeout=30) as r: data=r.read()
        lines=data.decode('ascii').splitlines()
        networks=[ipaddress.ip_network(line.strip()) for line in lines if line.strip()]
        assert len(networks)>50
        assert all(n.version==(4 if family=='ipv4' else 6) for n in networks)
        name=f'{country}-{family}.zone';(out/name).write_bytes(data)
        sources.append({'file':name,'url':url,'networks':len(networks),'sha256':hashlib.sha256(data).hexdigest()})
(out/'sources.json').write_text(json.dumps({'retrieved_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'attribution':'IPdeny.com country IP blocks; approximate IP registration/geolocation, not physical location.',
    'usage':'Only CZ/SK select Czech. Other, private, invalid or unknown addresses select English.', 'sources':sources},indent=2),encoding='utf-8')
print(json.dumps(sources,indent=2))
