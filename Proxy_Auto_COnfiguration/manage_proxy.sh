#!/bin/bash

# Profilul activ Firefox (înlocuiește cu calea corectă pentru profilul tău Firefox)
PROFILE_DIR="$HOME/.mozilla/firefox/ccd1vpyr.default-release"  # Calea profilului Firefox

# Verifică existența fișierului prefs.js
PREFS_FILE="$PROFILE_DIR/prefs.js"

if [ ! -f "$PREFS_FILE" ]; then
    echo "Fișierul prefs.js nu a fost găsit în profilul: $PROFILE_DIR"
    exit 1
fi

# Funcție pentru activarea proxy-ului
activate_proxy() {
    echo "Activare proxy..."
    # Șterge setările anterioare
    sed -i '/user_pref("network.proxy.type",/d' "$PREFS_FILE"
    sed -i '/user_pref("network.proxy.http",/d' "$PREFS_FILE"
    sed -i '/user_pref("network.proxy.http_port",/d' "$PREFS_FILE"
    sed -i '/user_pref("network.proxy.no_proxies_on",/d' "$PREFS_FILE"

    # Adaugă noile setări
    echo 'user_pref("network.proxy.type", 1);' >> "$PREFS_FILE"
    echo 'user_pref("network.proxy.http", "127.0.0.1");' >> "$PREFS_FILE"
    echo 'user_pref("network.proxy.http_port", 8888);' >> "$PREFS_FILE"
    echo 'user_pref("network.proxy.no_proxies_on", "localhost, 127.0.0.1");' >> "$PREFS_FILE"

    echo "Proxy activat."
}

# Funcție pentru dezactivarea proxy-ului
deactivate_proxy() {
    echo "Dezactivare proxy..."
    # Șterge setările anterioare
    sed -i '/user_pref("network.proxy.type",/d' "$PREFS_FILE"
    # Setează proxy-ul la dezactivat
    echo 'user_pref("network.proxy.type", 0);' >> "$PREFS_FILE"
    echo "Proxy dezactivat."
}

# Afișează meniul utilizatorului
echo "Selectează opțiunea:"
echo "1. Activează proxy"
echo "2. Dezactivează proxy"
read -p "Introdu opțiunea (1/2): " option

case $option in
    1)
        activate_proxy
        ;;
    2)
        deactivate_proxy
        ;;
    *)
        echo "Opțiune invalidă."
        ;;
esac
