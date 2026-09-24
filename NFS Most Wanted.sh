#!/bin/bash

# Port-Pfade definieren
GAMEDIR="/roms/ports/nfs_mw"
LOGFILE="$GAMEDIR/nfsmw.log"

cd "$GAMEDIR" || exit 1

# Performance-Modus auf ArkOS aktivieren
if [ -f /usr/sbin/cpu_performance ]; then
    /usr/sbin/cpu_performance
fi

# Log-Datei initialisieren
echo "=== Launching NFS Most Wanted ===" > "$LOGFILE"
echo "Arbeitsverzeichnis: $GAMEDIR" >> "$LOGFILE"

# GPTK Controller-Mapping laden
if [ -f "/usr/bin/gptokeyb" ]; then
    /usr/bin/gptokeyb -c "$GAMEDIR/nfs_mw.gptk" &
fi

# Umgebungsvariablen setzen
export LD_LIBRARY_PATH="$GAMEDIR:$LD_LIBRARY_PATH"
export SDL_GAMECONTROLLERCONFIG_FILE="$GAMEDIR/gamecontrollerdb.txt"

# Loader ausführen
taskset -c 0,1,2,3 ./nfs_loader >> "$LOGFILE" 2>&1

# Aufräumen nach Beendigung
killall -9 gptokeyb 2>/dev/null

if [ -f /usr/sbin/cpu_normal ]; then
    /usr/sbin/cpu_normal
fi

printf "\033c" > /dev/tty1