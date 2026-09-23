#!/bin/bash
# High-Performance PortMaster Launch Script fuer NFS: Most Wanted Mobile (RK3326 / ArkOS)

GAMEDIR="/roms/ports/nfs_mw"
LOG_FILE="${GAMEDIR}/debug.log"

# Standard-Output und Fehler in debug.log umleiten
exec > >(tee -a "$LOG_FILE") 2>&1

# --- 1. PortMaster Environment & Hardware-Profile laden ------------------
XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}

if [ -d "/opt/system/Tools/PortMaster/" ]; then
    controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
    controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
    controlfolder="$XDG_DATA_HOME/PortMaster"
else
    controlfolder="/roms/ports/PortMaster"
fi

if [ -f "${controlfolder}/control.txt" ]; then
    source "${controlfolder}/control.txt"
    [ -f "${controlfolder}/mod_${CFW_NAME}.txt" ] && source "${controlfolder}/mod_${CFW_NAME}.txt"
    get_controls
fi

# Laden von Tasksetter (CPU-Pinning) und Hardware-Infos
[ -f "${controlfolder}/tasksetter" ] && source "${controlfolder}/tasksetter"
[ -f "${controlfolder}/device_info.txt" ] && source "${controlfolder}/device_info.txt"

cd "$GAMEDIR" || exit 1
export HOME="${GAMEDIR}"

# --- 2. CPU-Performance Maximierung (Verhindert Ruckler) -----------------
# Erzwingt den dauerhaften Maximaltakt aller 4 Cortex-A35 Kerne während des Spielens
echo "performance" | $ESUDO tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null 2>&1 || true

# --- 3. Direct Rendering & Driver Optimierung ----------------------------
export SDL_AUDIODRIVER=alsa
export SDL_VIDEODRIVER=kmsdrm
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"

export LD_LIBRARY_PATH="$GAMEDIR:$GAMEDIR/libs:${LD_LIBRARY_PATH}"
export LIBGL_ES=2
export LIBGL_GL=21

# --- 4. Steuerung & Spielstart mit CPU-Taskset --------------------------
$GPTOKEYB "nfs_mw_loader" -c "$GAMEDIR/nfs_mw.gptk" &

# Startet den Loader mit erzwungener CPU-Core-Verteilung ($TASKSET)
if [ -n "$TASKSET" ]; then
    $TASKSET ./nfs_mw_loader
else
    ./nfs_mw_loader
fi

# --- 5. Cleanup & Energiesparmodus wiederherstellen ----------------------
# Drosselt die CPU nach dem Beenden wieder, um Akku im ArkOS-Menü zu sparen
echo "ondemand" | $ESUDO tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor > /dev/null 2>&1 || true

$ESUDO kill -9 $(pidof gptokeyb) 2>/dev/null || true
unset LD_LIBRARY_PATH
unset SDL_GAMECONTROLLERCONFIG

echo "NFS: Most Wanted sauber beendet."
exit 0