<?php
declare(strict_types=1);

function framelab_in_network(string $address, string $cidr): bool {
    $parts = explode('/', trim($cidr), 2);
    if (count($parts) !== 2 || !ctype_digit($parts[1])) return false;
    $network = @inet_pton($parts[0]);
    if ($network === false || strlen($network) !== strlen($address)) return false;
    $bits = (int)$parts[1];
    if ($bits < 0 || $bits > strlen($address) * 8) return false;
    $bytes = intdiv($bits, 8); $remaining = $bits % 8;
    if (substr($address, 0, $bytes) !== substr($network, 0, $bytes)) return false;
    if ($remaining === 0) return true;
    $mask = (0xff << (8 - $remaining)) & 0xff;
    return (ord($address[$bytes]) & $mask) === (ord($network[$bytes]) & $mask);
}

function framelab_country(string $ip, string $directory): string {
    if (str_starts_with(strtolower($ip), '::ffff:')) {
        $mapped = @inet_pton($ip);
        if ($mapped !== false && strlen($mapped) === 16) $ip = inet_ntop(substr($mapped, 12));
    }
    if (!filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE)) return 'ZZ';
    $address = @inet_pton($ip);
    if ($address === false) return 'ZZ';
    $family = strlen($address) === 4 ? 'ipv4' : 'ipv6';
    foreach (['CZ', 'SK'] as $country) {
        $file = $directory . '/' . strtolower($country) . '-' . $family . '.zone';
        if (!is_readable($file)) continue;
        $stream = fopen($file, 'r');
        if ($stream === false) continue;
        while (($line = fgets($stream)) !== false) {
            if (framelab_in_network($address, $line)) { fclose($stream); return $country; }
        }
        fclose($stream);
    }
    return 'ZZ';
}

function framelab_client_ip(array $server, array $trustedProxies): string {
    $peer = (string)($server['REMOTE_ADDR'] ?? '');
    if (!filter_var($peer, FILTER_VALIDATE_IP)) return '';
    if (!in_array($peer, $trustedProxies, true)) return $peer;
    $forwarded = trim((string)($server['HTTP_X_FORWARDED_FOR'] ?? ''));
    if ($forwarded === '' || strlen($forwarded) > 2048) return $peer;
    $chain = array_map('trim', explode(',', $forwarded));
    foreach ($chain as $ip) if (!filter_var($ip, FILTER_VALIDATE_IP)) return $peer;
    $chain[] = $peer;
    // Walk from the known socket peer. Never accept an attacker-supplied leftmost value.
    for ($i = count($chain) - 1; $i >= 0; --$i) {
        if (!in_array($chain[$i], $trustedProxies, true)) return $chain[$i];
    }
    return $peer;
}

function framelab_request_country(array $server, string $directory): string {
    $configFile = $directory . '/runtime-config.json';
    $config = is_readable($configFile) ? json_decode((string)file_get_contents($configFile), true) : [];
    if (!is_array($config)) $config = [];
    $trusted = is_array($config['trusted_proxies'] ?? null) ? $config['trusted_proxies'] : [];
    $ip = framelab_client_ip($server, $trusted);
    $country = framelab_country($ip, $directory);
    // Optional mapping for a verified local LAN/hairpin connection with no public client IP.
    // Applied only to private addresses, never a visitor's public address.
    if ($country === 'ZZ' && filter_var($ip, FILTER_VALIDATE_IP)
        && !filter_var($ip, FILTER_VALIDATE_IP, FILTER_FLAG_NO_PRIV_RANGE | FILTER_FLAG_NO_RES_RANGE)) {
        $address = inet_pton($ip);
        $local = is_array($config['local_network_countries'] ?? null) ? $config['local_network_countries'] : [];
        foreach ($local as $network => $code) {
            if (in_array($code, ['CZ', 'SK'], true) && framelab_in_network($address, (string)$network)) return $code;
        }
    }
    return $country;
}
