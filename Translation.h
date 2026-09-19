// =============================================================================
// Translation.h — OctoGlance localisation
// =============================================================================
//
// HOW TO USE
// ----------
// 1. Open Language.h and uncomment ONE language only:
//
//      #define LANG_EN   // English  (default)
//      #define LANG_FR   // French
//      #define LANG_DE   // German
//      #define LANG_ES   // Spanish
//      #define LANG_NL   // Dutch
//      #define LANG_PT   // Portuguese
//      #define LANG_TR   // Turkish
//
// 2. Recompile and flash. That's it!
//
// FONT NOTE
// ---------
// NotoSansBold15 / NotoSansBold36 cover the full Latin-1 Supplement block
// (U+00C0–U+00FF), so accented characters for French, German, Spanish, Dutch,
// and Portuguese work without regenerating fonts.
// Turkish uses ASCII approximations to avoid characters outside Latin-1.
//
// GAUGE HEADING NOTE
// ------------------
// strNozzle, strProgress, strBed must be short — limited pixel width on
// the 240px wide screen. strPrep must be 5 chars or fewer — confined space
// inside the gauge arc.
//
// FPT (Finish Print Time) and ETA are universal — no translation needed.
// OctoGlance stays as-is on the network regardless of language.
// =============================================================================

#ifndef TRANSLATION_H
#define TRANSLATION_H

// -----------------------------------------------------------------------------
// Default to English if nothing is defined
// -----------------------------------------------------------------------------
#if !defined(LANG_EN) && !defined(LANG_FR) && !defined(LANG_DE) && \
    !defined(LANG_ES) && !defined(LANG_NL) && !defined(LANG_PT) && \
    !defined(LANG_TR)
  #define LANG_EN
#endif

// =============================================================================
//  ENGLISH
// =============================================================================
#ifdef LANG_EN

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Nozzle";
const char strProgress[]    = "Progress";
const char strBed[]         = "Bed";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Idle";
const char strPrep[]        = "PREP";
const char strPrinterPaused[] = "Printer Paused";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Connecting...";
const char strOctoOffline[]     = "OctoPrint Offline";
const char strOctoOnline[]      = "OctoPrint Online";
const char strPrinterOffline[]  = "Printer Not Connected";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Print Failed";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Total";
const char strHrs[]         = "hrs";
const char strMins[]        = "mins";

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Printer Settings";
const char wcSecWifi[]          = "WiFi Control";
const char wcPrinterIP[]        = "Printer IP";
const char wcPrinterPort[]      = "Printer Port";
const char wcClock24[]          = "24 Hour Clock";
const char wcSaveBtn[]          = "Update &amp; Save";
const char wcWifiResetBtn[]     = "&#x26A0; Reset WiFi";
const char wcWifiResetTitle[]   = "&#x26A0; Reset WiFi?";
const char wcWifiResetBody[]    = "Erases saved WiFi credentials and reboots into setup mode. Reconnect via the OctoGlance hotspot.";
const char wcWifiResetYes[]     = "Yes, reset";
const char wcWifiResetCancel[]  = "Cancel";
const char wcWifiResetting[]    = "Resetting WiFi...";
const char wcWifiResetHotspot[] = "Rebooting. Connect to the <strong>OctoGlance</strong> hotspot.";
const char wcSaved[]            = "&#10003; Settings saved and applied.";
const char wcSaveFailedNotFound[] = "&#x26A0; OctoPrint not found at that address - check the IP and port.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint was found, but the API key was rejected - check Settings &rarr; Application Keys.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Push Notifications (ntfy)";
const char wcNtfyEnabled[]      = "Enable ntfy";
const char wcNtfyServer[]       = "Server URL";
const char wcNtfyTopic[]        = "Topic";
const char wcNtfyToken[]        = "Token (optional)";
const char wcNtfyStallMin[]     = "Stall Timeout (minutes)";
const char wcNtfyPort[]         = "Port (Local Host)";

#endif // LANG_EN

// =============================================================================
//  FRENCH
// =============================================================================
#ifdef LANG_FR

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Buse";
const char strProgress[]    = "Progrès";
const char strBed[]         = "Lit";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Repos";
const char strPrep[]        = "PREP";
const char strPrinterPaused[] = "Imprimante en pause";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Connexion...";
const char strOctoOffline[]     = "OctoPrint hors ligne";
const char strOctoOnline[]      = "OctoPrint en ligne";
const char strPrinterOffline[]  = "Imprimante non connectée";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Échec d'impression";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Total";
const char strHrs[]         = "hrs";
const char strMins[]        = "mins";

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Paramètres imprimante";
const char wcSecWifi[]          = "Contrôle WiFi";
const char wcPrinterIP[]        = "IP imprimante";
const char wcPrinterPort[]      = "Port imprimante";
const char wcClock24[]          = "Horloge 24h";
const char wcSaveBtn[]          = "Enregistrer";
const char wcWifiResetBtn[]     = "&#x26A0; Réinitialiser WiFi";
const char wcWifiResetTitle[]   = "&#x26A0; Réinitialiser WiFi ?";
const char wcWifiResetBody[]    = "Efface les identifiants WiFi et redémarre en mode configuration. Reconnectez-vous via le hotspot OctoGlance.";
const char wcWifiResetYes[]     = "Oui, réinitialiser";
const char wcWifiResetCancel[]  = "Annuler";
const char wcWifiResetting[]    = "Réinitialisation WiFi...";
const char wcWifiResetHotspot[] = "Redémarrage. Connectez-vous au hotspot <strong>OctoGlance</strong>.";
const char wcSaved[]            = "&#10003; Paramètres enregistrés.";
const char wcSaveFailedNotFound[] = "&#x26A0; OctoPrint introuvable à cette adresse - vérifiez l'IP et le port.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint trouvé, mais la clé API a été rejetée - voir Paramètres &rarr; Clés d'application.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Notifications push (ntfy)";
const char wcNtfyEnabled[]      = "Activer ntfy";
const char wcNtfyServer[]       = "URL du serveur";
const char wcNtfyTopic[]        = "Sujet";
const char wcNtfyToken[]        = "Jeton (optionnel)";
const char wcNtfyStallMin[]     = "Délai de blocage (minutes)";
const char wcNtfyPort[]         = "Port (Hôte local)";

#endif // LANG_FR

// =============================================================================
//  GERMAN
// =============================================================================
#ifdef LANG_DE

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Düse";
const char strProgress[]    = "Fortsch.";   // Fortschritt truncated
const char strBed[]         = "Bett";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Frei";
const char strPrep[]        = "VOR";        // Vorbereitung
const char strPrinterPaused[] = "Drucker pausiert";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Verbindung...";
const char strOctoOffline[]     = "OctoPrint offline";
const char strOctoOnline[]      = "OctoPrint online";
const char strPrinterOffline[]  = "Drucker nicht verbunden";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Druck fehlgeschlagen";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Gesamt";
const char strHrs[]         = "Std";        // Stunden
const char strMins[]        = "Min";        // Minuten

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Druckereinstellungen";
const char wcSecWifi[]          = "WiFi-Steuerung";
const char wcPrinterIP[]        = "Drucker IP";
const char wcPrinterPort[]      = "Drucker Port";
const char wcClock24[]          = "24-Stunden-Uhr";
const char wcSaveBtn[]          = "Speichern";
const char wcWifiResetBtn[]     = "&#x26A0; WiFi zurücksetzen";
const char wcWifiResetTitle[]   = "&#x26A0; WiFi zurücksetzen?";
const char wcWifiResetBody[]    = "Löscht WLAN-Daten und startet im Konfigurationsmodus neu. Verbinde mit dem OctoGlance Hotspot.";
const char wcWifiResetYes[]     = "Ja, zurücksetzen";
const char wcWifiResetCancel[]  = "Abbrechen";
const char wcWifiResetting[]    = "WiFi wird zurückgesetzt...";
const char wcWifiResetHotspot[] = "Neustart. Verbinde mit dem Hotspot <strong>OctoGlance</strong>.";
const char wcSaved[]            = "&#10003; Einstellungen gespeichert.";
const char wcSaveFailedNotFound[] = "&#x26A0; OctoPrint wurde unter dieser Adresse nicht gefunden - IP und Port prüfen.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint gefunden, aber der API-Schlüssel wurde abgelehnt - siehe Einstellungen &rarr; Application Keys.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Push-Benachrichtigungen (ntfy)";
const char wcNtfyEnabled[]      = "ntfy aktivieren";
const char wcNtfyServer[]       = "Server-URL";
const char wcNtfyTopic[]        = "Thema";
const char wcNtfyToken[]        = "Token (optional)";
const char wcNtfyStallMin[]     = "Stillstandszeit (Minuten)";
const char wcNtfyPort[]         = "Port (Lokaler Host)";

#endif // LANG_DE

// =============================================================================
//  SPANISH
// =============================================================================
#ifdef LANG_ES

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Boquil.";    // Boquilla truncated
const char strProgress[]    = "Progreso";
const char strBed[]         = "Cama";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Libre";
const char strPrep[]        = "PREP";
const char strPrinterPaused[] = "Impresora en pausa";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Conectando...";
const char strOctoOffline[]     = "OctoPrint sin conexión";
const char strOctoOnline[]      = "OctoPrint en línea";
const char strPrinterOffline[]  = "Impresora no conectada";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Impresión fallida";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Total";
const char strHrs[]         = "hrs";
const char strMins[]        = "mins";

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Configuración impresora";
const char wcSecWifi[]          = "Control WiFi";
const char wcPrinterIP[]        = "IP impresora";
const char wcPrinterPort[]      = "Puerto impresora";
const char wcClock24[]          = "Reloj de 24 horas";
const char wcSaveBtn[]          = "Guardar";
const char wcWifiResetBtn[]     = "&#x26A0; Restablecer WiFi";
const char wcWifiResetTitle[]   = "&#x26A0; Restablecer WiFi?";
const char wcWifiResetBody[]    = "Borra las credenciales WiFi y reinicia en modo configuración. Reconéctese al hotspot OctoGlance.";
const char wcWifiResetYes[]     = "Sí, restablecer";
const char wcWifiResetCancel[]  = "Cancelar";
const char wcWifiResetting[]    = "Restableciendo WiFi...";
const char wcWifiResetHotspot[] = "Reiniciando. Conéctese al hotspot <strong>OctoGlance</strong>.";
const char wcSaved[]            = "&#10003; Configuración guardada.";
const char wcSaveFailedNotFound[] = "&#x26A0; No se encontró OctoPrint en esa dirección - compruebe la IP y el puerto.";
const char wcSaveFailedBadKey[]   = "&#x26A0; Se encontró OctoPrint, pero la clave API fue rechazada - vea Configuración &rarr; Claves de aplicación.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Notificaciones push (ntfy)";
const char wcNtfyEnabled[]      = "Activar ntfy";
const char wcNtfyServer[]       = "URL del servidor";
const char wcNtfyTopic[]        = "Tema";
const char wcNtfyToken[]        = "Token (opcional)";
const char wcNtfyStallMin[]     = "Tiempo de espera (minutos)";
const char wcNtfyPort[]         = "Puerto (Host local)";

#endif // LANG_ES

// =============================================================================
//  DUTCH
// =============================================================================
#ifdef LANG_NL

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Nozzle";
const char strProgress[]    = "Voortg.";    // Voortgang truncated
const char strBed[]         = "Bed";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Vrij";
const char strPrep[]        = "PREP";
const char strPrinterPaused[] = "Printer gepauzeerd";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Verbinden...";
const char strOctoOffline[]     = "OctoPrint offline";
const char strOctoOnline[]      = "OctoPrint online";
const char strPrinterOffline[]  = "Printer niet verbonden";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Afdruk mislukt";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Totaal";
const char strHrs[]         = "uur";
const char strMins[]        = "min";

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Printerinstellingen";
const char wcSecWifi[]          = "WiFi-beheer";
const char wcPrinterIP[]        = "Printer IP";
const char wcPrinterPort[]      = "Printer poort";
const char wcClock24[]          = "24-uursklok";
const char wcSaveBtn[]          = "Opslaan";
const char wcWifiResetBtn[]     = "&#x26A0; WiFi resetten";
const char wcWifiResetTitle[]   = "&#x26A0; WiFi resetten?";
const char wcWifiResetBody[]    = "Verwijdert WiFi-gegevens en herstart in configuratiemodus. Verbind opnieuw via de OctoGlance hotspot.";
const char wcWifiResetYes[]     = "Ja, resetten";
const char wcWifiResetCancel[]  = "Annuleren";
const char wcWifiResetting[]    = "WiFi wordt gereset...";
const char wcWifiResetHotspot[] = "Herstarten. Verbind met het <strong>OctoGlance</strong> hotspot.";
const char wcSaved[]            = "&#10003; Instellingen opgeslagen.";
const char wcSaveFailedNotFound[] = "&#x26A0; OctoPrint niet gevonden op dat adres - controleer het IP-adres en de poort.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint gevonden, maar de API-sleutel is geweigerd - zie Instellingen &rarr; Application Keys.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Pushmeldingen (ntfy)";
const char wcNtfyEnabled[]      = "ntfy inschakelen";
const char wcNtfyServer[]       = "Server URL";
const char wcNtfyTopic[]        = "Onderwerp";
const char wcNtfyToken[]        = "Token (optioneel)";
const char wcNtfyStallMin[]     = "Wachttijd (minuten)";
const char wcNtfyPort[]         = "Poort (Lokale host)";

#endif // LANG_NL

// =============================================================================
//  PORTUGUESE
// =============================================================================
#ifdef LANG_PT

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Bocal";
const char strProgress[]    = "Progresso";
const char strBed[]         = "Cama";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Livre";
const char strPrep[]        = "PREP";
const char strPrinterPaused[] = "Impressora pausada";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Conectando...";
const char strOctoOffline[]     = "OctoPrint offline";
const char strOctoOnline[]      = "OctoPrint online";
const char strPrinterOffline[]  = "Impressora não conectada";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Impressão falhou";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Total";
const char strHrs[]         = "hrs";
const char strMins[]        = "mins";

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Configurações da impressora";
const char wcSecWifi[]          = "Controle WiFi";
const char wcPrinterIP[]        = "IP da impressora";
const char wcPrinterPort[]      = "Porta da impressora";
const char wcClock24[]          = "Relógio 24h";
const char wcSaveBtn[]          = "Salvar";
const char wcWifiResetBtn[]     = "&#x26A0; Redefinir WiFi";
const char wcWifiResetTitle[]   = "&#x26A0; Redefinir WiFi?";
const char wcWifiResetBody[]    = "Apaga credenciais WiFi e reinicia no modo configuração. Reconecte-se ao hotspot OctoGlance.";
const char wcWifiResetYes[]     = "Sim, redefinir";
const char wcWifiResetCancel[]  = "Cancelar";
const char wcWifiResetting[]    = "Redefinindo WiFi...";
const char wcWifiResetHotspot[] = "Reiniciando. Conecte-se ao hotspot <strong>OctoGlance</strong>.";
const char wcSaved[]            = "&#10003; Configurações salvas.";
const char wcSaveFailedNotFound[] = "&#x26A0; OctoPrint não encontrado nesse endereço - verifique o IP e a porta.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint encontrado, mas a chave API foi rejeitada - veja Configurações &rarr; Application Keys.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Notificações push (ntfy)";
const char wcNtfyEnabled[]      = "Ativar ntfy";
const char wcNtfyServer[]       = "URL do servidor";
const char wcNtfyTopic[]        = "Tópico";
const char wcNtfyToken[]        = "Token (opcional)";
const char wcNtfyStallMin[]     = "Tempo de espera (minutos)";
const char wcNtfyPort[]         = "Porta (Host local)";

#endif // LANG_PT

// =============================================================================
//  TURKISH
// =============================================================================
#ifdef LANG_TR

// ── Gauge headings ────────────────────────────────────────────────────────
const char strNozzle[]      = "Nozul";
const char strProgress[]    = "Ilerleme";
const char strBed[]         = "Tabla";

// ── Progress gauge centre (max 5 chars) ──────────────────────────────────
const char strIdle[]        = "Bosta";
const char strPrep[]        = "HAZR";       // Hazırlık
const char strPrinterPaused[] = "Yazici Duraklatildi";

// ── Connection-state screens (center graphic zone) ───────────────────────
const char strConnecting[]      = "Baglaniyor...";
const char strOctoOffline[]     = "OctoPrint Cevrimdisi";
const char strOctoOnline[]      = "OctoPrint Cevrimici";
const char strPrinterOffline[]  = "Yazici Bagli Degil";

// ── Failure screen ─────────────────────────────────────────────────────────
const char strPrintFailed[] = "Baski Basarisiz";

// ── Success screen ────────────────────────────────────────────────────────
const char strTotal[]       = "Toplam";
const char strHrs[]         = "saat";
const char strMins[]        = "dak";        // dakika

// ── Web UI ────────────────────────────────────────────────────────────────
const char wcTitle[]            = "OctoGlance";
const char wcSecPrinter[]       = "Yazici Ayarlari";
const char wcSecWifi[]          = "WiFi Kontrolü";
const char wcPrinterIP[]        = "Yazici IP";
const char wcPrinterPort[]      = "Yazici Portu";
const char wcClock24[]          = "24 saat formati";
const char wcSaveBtn[]          = "Kaydet";
const char wcWifiResetBtn[]     = "&#x26A0; WiFi Sifirla";
const char wcWifiResetTitle[]   = "&#x26A0; WiFi Sifirla?";
const char wcWifiResetBody[]    = "WiFi bilgilerini siler ve yapilandirma modunda yeniden baslatir. OctoGlance hotspotuna baglanin.";
const char wcWifiResetYes[]     = "Evet, sifirla";
const char wcWifiResetCancel[]  = "Iptal";
const char wcWifiResetting[]    = "WiFi sifirlanıyor...";
const char wcWifiResetHotspot[] = "Yeniden baslatiliyor. <strong>OctoGlance</strong> hotspotuna baglanin.";
const char wcSaved[]            = "&#10003; Ayarlar kaydedildi.";
const char wcSaveFailedNotFound[] = "&#x26A0; O adreste OctoPrint bulunamadi - IP ve portu kontrol edin.";
const char wcSaveFailedBadKey[]   = "&#x26A0; OctoPrint bulundu, ancak API anahtari reddedildi - Ayarlar &rarr; Application Keys kismina bakin.";

// ── ntfy Notifications (Web UI) ──────────────────────────────────────────
const char wcSecNtfy[]          = "Anlik Bildirimler (ntfy)";
const char wcNtfyEnabled[]      = "ntfy'yi etkinlestir";
const char wcNtfyServer[]       = "Sunucu URL";
const char wcNtfyTopic[]        = "Konu";
const char wcNtfyToken[]        = "Token (istege bagli)";
const char wcNtfyStallMin[]     = "Bekleme suresi (dakika)";
const char wcNtfyPort[]         = "Port (Yerel host)";

#endif // LANG_TR

#endif // TRANSLATION_H
