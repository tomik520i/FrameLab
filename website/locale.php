<?php
declare(strict_types=1);
// Local country lookup. No visitor IP is sent to an external service or retained here.
require_once __DIR__ . '/country_lookup.php';
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: private, no-store, max-age=0');
header('X-Content-Type-Options: nosniff');
// Deliberately do not trust client-provided X-Forwarded-For / CF-IPCountry headers.
// If a trusted reverse proxy is added, configure Apache mod_remoteip at the origin.
$country = framelab_country((string)($_SERVER['REMOTE_ADDR'] ?? ''), __DIR__ . '/country-data');
echo json_encode(['country' => $country, 'language' => in_array($country, ['CZ', 'SK'], true) ? 'cs' : 'en']);
