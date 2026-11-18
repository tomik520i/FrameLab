<?php
declare(strict_types=1);
// Local country lookup. No visitor IP is sent to an external service or retained here.
require_once __DIR__ . '/country_lookup.php';
header('Content-Type: application/json; charset=utf-8');
header('Cache-Control: private, no-store, max-age=0');
header('X-Content-Type-Options: nosniff');
// Forwarded addresses are accepted only behind an explicitly configured exact proxy.
// Client-provided CF-IPCountry and X-Real-IP never select the language.
$country = framelab_request_country($_SERVER, __DIR__ . '/country-data');
echo json_encode(['country' => $country, 'language' => in_array($country, ['CZ', 'SK'], true) ? 'cs' : 'en']);
