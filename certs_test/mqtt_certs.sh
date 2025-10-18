#!/bin/bash

# ==========================
#  MQTT TLS Certificate Generator
#  Supports multiple domains and IPs in SAN
# ==========================

# --- Default values (can be overridden via arguments) ---
DEFAULT_DOMAINS=("bootdomain.duckdns.org")
DEFAULT_IPS=("149.50.131.123")

# --- Certificate Subjects ---
SUBJECT_CA="/C=AR/ST=CABA/L=CABA/O=FiUBA/OU=CA/CN=MQTT-CA"

# ==========================
#  Functions
# ==========================

function generate_CA() {
    echo "🔧 Generating Certificate Authority (CA)..."
    openssl req -x509 -nodes -sha256 -newkey rsa:2048 -subj "$SUBJECT_CA" \
        -days 365 -keyout ca.key -out ca.crt
}

function generate_SAN_config() {
    local domains=("${!1}")
    local ips=("${!2}")

    echo "🔧 Creating SAN configuration..."
    cat > san.cnf <<EOF
subjectAltName = @alt_names
[alt_names]
EOF

    local i=1
    for d in "${domains[@]}"; do
        echo "DNS.$i = $d" >> san.cnf
        ((i++))
    done

    local j=1
    for ip in "${ips[@]}"; do
        echo "IP.$j = $ip" >> san.cnf
        ((j++))
    done
}

function generate_server() {
    local domains=("${!1}")
    local ips=("${!2}")
    local cn="${domains[0]}"

    echo "🔧 Generating Server Certificate for CN=$cn"
    generate_SAN_config domains[@] ips[@]

    local subject="/C=AR/ST=CABA/L=CABA/O=FiUBA/OU=Server/CN=$cn"

    openssl req -nodes -sha256 -new -subj "$subject" \
        -keyout server.key -out server.csr

    openssl x509 -req -sha256 -in server.csr \
        -CA ca.crt -CAkey ca.key -CAcreateserial \
        -out server.crt -days 365 -extfile san.cnf
}

function generate_client() {
    local cn=${1:-client}
    local subject="/C=AR/ST=CABA/L=CABA/O=FiUBA/OU=Client/CN=$cn"

    echo "🔧 Generating Client Certificate for CN=$cn"
    openssl req -new -nodes -sha256 -subj "$subject" \
        -out "${cn}.csr" -keyout "${cn}.key"

    openssl x509 -req -sha256 -in "${cn}.csr" \
        -CA ca.crt -CAkey ca.key -CAcreateserial \
        -out "${cn}.crt" -days 365
}

function usage() {
    echo ""
    echo "Usage: $0 [action] [extra params]"
    echo ""
    echo "Actions:"
    echo "  ca                          Generate only the Certificate Authority"
    echo "  server [domain ...] [ip ...] Generate Server cert with optional SAN overrides"
    echo "                               (defaults to mqtt.example.com, broker.local, etc.)"
    echo "  client [CN]                 Generate Client cert with custom Common Name (default: client)"
    echo "  all                         Generate CA, Server, and Client (default CN: client)"
    echo ""
    echo "Examples:"
    echo "  $0 ca"
    echo "  $0 server mqtt.mydomain.com 192.168.1.10"
    echo "  $0 client mydevice"
    echo "  $0 all"
    echo ""
}

# ==========================
#  Execution
# ==========================

ACTION=${1:-all}
shift || true

case "$ACTION" in
    ca)
        generate_CA
        ;;
    server)
        if [[ ! -f ca.crt || ! -f ca.key ]]; then
            echo "❌ CA not found. Please generate CA first using: $0 ca"
            exit 1
        fi

        # Separate domains and IPs if provided
        DOMAINS=()
        IPS=()
        for arg in "$@"; do
            if [[ "$arg" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                IPS+=("$arg")
            else
                DOMAINS+=("$arg")
            fi
        done

        # Fallback to defaults if not provided
        [[ ${#DOMAINS[@]} -eq 0 ]] && DOMAINS=("${DEFAULT_DOMAINS[@]}")
        [[ ${#IPS[@]} -eq 0 ]] && IPS=("${DEFAULT_IPS[@]}")

        generate_server DOMAINS[@] IPS[@]
        ;;
    client)
        if [[ ! -f ca.crt || ! -f ca.key ]]; then
            echo "❌ CA not found. Please generate CA first using: $0 ca"
            exit 1
        fi

        CN=${1:-client}
        generate_client "$CN"
        ;;
    all)
        generate_CA
        generate_server DEFAULT_DOMAINS[@] DEFAULT_IPS[@]
        generate_client "client"
        ;;
    *)
        usage
        exit 1
        ;;
esac

echo ""
echo "✅ Done!"
echo "Generated files:"
ls -1 *.crt *.key *.csr 2>/dev/null
echo ""
