function FindProxyForURL(url, host) {
    // Citire din fișierul de stare pentru a afla dacă Intercept este activat
    var intercept_state = "OFF"; // În mod implicit, presupunem că interceptarea este oprită

    // Citește fișierul de stare (Aceasta ar trebui să fie gestionată de un script extern)
    if (intercept_state === "ON") {
        return "PROXY 127.0.0.1:8888"; // Proxy activat
    } else {
        return "DIRECT"; // Proxy dezactivat
    }
}
