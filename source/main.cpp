// IPTV360 - gerenciador de listas M3U para Xbox 360 (libxenon / Free60)
// Roda via XeLL (.elf32). NAO e um XEX.
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

extern "C" {
#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>
#include <diskio/ata.h>
#include <time/time.h>
#include <ppc/timebase.h>
#include <ppc/cache.h>
#include <xenon_soc/xenon_power.h>
#include <libfat/fat.h>
int bdev_enum(int handle, const char** name);  // definido no newlib do libxenon
extern char bss_start[];                       // simbolo do linker script (fim do codigo/dados)
}

#include "m3u.h"
#include "player_xenon.h"
#include "diag.h"

static const int   STICK_THRESHOLD = 20000;
static const int   PAGE_ROWS       = 24;
static const char* kSearchPaths[]  = { "playlist.m3u", "iptv360/playlist.m3u", "playlist.m3u8", 0 };

static std::vector<Channel> g_all;      // todos os canais
static std::vector<int>     g_view;     // indices visiveis (apos filtro)
static std::vector<std::string> g_groups;
static int g_groupSel = -1;             // -1 = todos
static int g_sel = 0;
static std::string g_source;

static void poll_pad(controller_data_s& pad) {
    usb_do_poll();
    get_controller_data(&pad, 0);
}

static void rebuild_groups() {
    g_groups.clear();
    for (size_t i = 0; i < g_all.size(); ++i) {
        const std::string& g = g_all[i].group;
        if (g.empty()) continue;
        bool found = false;
        for (size_t k = 0; k < g_groups.size(); ++k)
            if (g_groups[k] == g) { found = true; break; }
        if (!found) g_groups.push_back(g);
    }
}

static void rebuild_view() {
    g_view.clear();
    for (size_t i = 0; i < g_all.size(); ++i)
        if (g_groupSel < 0 || g_all[i].group ==
