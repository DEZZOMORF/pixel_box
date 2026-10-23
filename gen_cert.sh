#!/bin/sh
# Генерує самопідписаний сертифікат для HTTPS-сторінки камери у cert.h.
# Використання: ./gen_cert.sh [IP-лампи]
set -e
IP=${1:-192.168.50.64}
T=$(mktemp -d)
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -days 3650 \
  -keyout "$T/key.pem" -out "$T/cert.pem" -subj "/CN=lamp" \
  -addext "subjectAltName=IP:$IP,DNS:lamp.local" 2>/dev/null
openssl pkcs8 -topk8 -nocrypt -in "$T/key.pem" -out "$T/key8.pem"
{
  echo '#pragma once'
  echo '// Самопідписаний сертифікат для HTTPS-сторінки камери (тільки локальна мережа)'
  printf 'const char CERT_PEM[] = R"PEM('; cat "$T/cert.pem"; echo ')PEM";'
  printf 'const char KEY_PEM[] = R"PEM('; cat "$T/key8.pem"; echo ')PEM";'
} > "$(dirname "$0")/cert.h"
rm -r "$T"
echo "cert.h для $IP готовий"
