//
// JK_Botti - be more human!
//
// sgtc.cpp - StargateTC (game dir "stargatetc") support:
//   weapon table, team/class joining and per-class spawn loadouts.
//

#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "bot_skill.h"
#include "bot_trace.h"
#include "sgtc.h"

extern bot_weapon_select_t weapon_select[NUM_OF_WEAPON_SELECTS];
extern bot_fire_delay_t fire_delay[NUM_OF_WEAPON_SELECTS];
extern bot_weapon_t weapon_defs[MAX_WEAPONS];

// One SGTC weapon entry. Secondary attacks are left unused: the bots only
// need the primary fire of each SGTC weapon.
#define SGTC_WEAPON(name, type, aim, avoid, pmin, pmax, opt, use, hold) \
   {SGTC_UNRESOLVED_WEAPON_ID, WEAPON_SUBMOD_SGTC, name, type, aim, \
    SKILL5, NOSKILL, avoid, FALSE, \
    pmin, pmax, 0.0, 0.0, opt, \
    use, TRUE, 100, 1, 0, hold, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, -1, -1, \
    0, 0, 0, FALSE, FALSE }

#define SGTC_DELAY(base) \
   {SGTC_UNRESOLVED_WEAPON_ID, \
    base, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}, \
    0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}}

// weapons are stored in priority order, most desired weapon first
bot_weapon_select_t sgtc_weapon_select[NUM_OF_WEAPON_SELECTS] =
{
   //          classname                 type              aim  avoid  min    max     opt    use% hold
   SGTC_WEAPON("weapon_usas",            WEAPON_FIRE,      1.0, FALSE, 0.0,   1200.0,  300.0, 80,  TRUE),
   SGTC_WEAPON("weapon_glstaff",         WEAPON_FIRE,      1.0, FALSE, 100.0, 4000.0, 600.0, 80,  TRUE),
   SGTC_WEAPON("weapon_m16",             WEAPON_FIRE,      1.0, FALSE, 0.0,   4000.0, 700.0, 70,  TRUE),
   SGTC_WEAPON("weapon_p90",             WEAPON_FIRE,      1.0, FALSE, 0.0,   3000.0, 500.0, 70,  TRUE),
   SGTC_WEAPON("weapon_9mmAR",           WEAPON_FIRE,      1.0, FALSE, 0.0,   3000.0, 500.0, 65,  TRUE),  // MP5
   SGTC_WEAPON("weapon_spas",            WEAPON_FIRE,      1.0, FALSE, 0.0,   900.0,  200.0, 65,  FALSE),
   SGTC_WEAPON("weapon_lance",           WEAPON_FIRE,      1.0, FALSE, 64.0,  4000.0, 700.0, 70,  FALSE),
   SGTC_WEAPON("weapon_zatarc",          WEAPON_FIRE,      1.0, FALSE, 0.0,   3000.0, 600.0, 60,  TRUE),
   SGTC_WEAPON("weapon_reetou",          WEAPON_FIRE,      1.0, FALSE, 0.0,   2500.0, 500.0, 55,  TRUE),
   SGTC_WEAPON("weapon_zat",             WEAPON_FIRE,      1.0, FALSE, 0.0,   2500.0, 500.0, 55,  FALSE),
   SGTC_WEAPON("weapon_psg1",            WEAPON_FIRE_ZOOM, 0.8, FALSE, 300.0, 8000.0, 1500.0, 50, FALSE),
   SGTC_WEAPON("weapon_main",            WEAPON_FIRE,      1.0, FALSE, 0.0,   400.0,  150.0, 40,  TRUE),
   SGTC_WEAPON("weapon_m92s",            WEAPON_FIRE,      1.0, FALSE, 0.0,   2500.0, 400.0, 30,  FALSE),
   SGTC_WEAPON("weapon_larve",           WEAPON_THROW,     1.0, FALSE, 150.0, 700.0,  400.0, 20,  FALSE),
   SGTC_WEAPON("weapon_handgrenade",     WEAPON_THROW,     1.0, FALSE, 300.0, 900.0,  500.0, 15,  FALSE),
   SGTC_WEAPON("weapon_nervegrenade",    WEAPON_THROW,     1.0, FALSE, 300.0, 900.0,  550.0, 10,  FALSE),
   SGTC_WEAPON("weapon_narcogrenade",    WEAPON_THROW,     1.0, FALSE, 300.0, 900.0,  550.0, 10,  FALSE),
   SGTC_WEAPON("weapon_mustardgrenade",  WEAPON_THROW,     1.0, FALSE, 300.0, 900.0,  550.0, 10,  FALSE),
   SGTC_WEAPON("weapon_gacgoa",          WEAPON_THROW,     1.0, TRUE,  350.0, 900.0,  600.0, 10,  FALSE),
   // melee: use% 0 = never drawn while a gun can fire, only the fallback when nothing else can
   SGTC_WEAPON("weapon_batsup",          WEAPON_MELEE,     1.0, FALSE, 0.0,   50.0,   1.0,   0,   TRUE),
   SGTC_WEAPON("weapon_coujaf",          WEAPON_MELEE,     1.0, FALSE, 0.0,   45.0,   1.0,   0,   TRUE),
   SGTC_WEAPON("weapon_couteau",         WEAPON_MELEE,     1.0, FALSE, 0.0,   45.0,   1.0,   0,   TRUE),

   /* terminator */
   {0, 0, "", 0, 0.0,
    NOSKILL, NOSKILL, FALSE, FALSE,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0, FALSE, 0, 1, 1, FALSE, FALSE, FALSE, FALSE, 0.0, 0.0, FALSE, 0, 0,
    0, 0, 0 },
};

// THESE MUST MATCH THE SAME ORDER AS THE WEAPON SELECT ARRAY!!!
bot_fire_delay_t sgtc_fire_delay[NUM_OF_WEAPON_SELECTS] =
{
   SGTC_DELAY(0.0),   // usas
   SGTC_DELAY(0.0),   // glstaff
   SGTC_DELAY(0.0),   // m16
   SGTC_DELAY(0.0),   // p90
   SGTC_DELAY(0.0),   // mp5
   SGTC_DELAY(0.0),   // spas
   SGTC_DELAY(0.0),   // lance
   SGTC_DELAY(0.0),   // zatarc
   SGTC_DELAY(0.0),   // reetou
   SGTC_DELAY(0.3),   // zat
   SGTC_DELAY(0.5),   // psg1
   SGTC_DELAY(0.0),   // main
   SGTC_DELAY(0.15),  // m92s
   SGTC_DELAY(0.5),   // larve
   SGTC_DELAY(0.5),   // handgrenade
   SGTC_DELAY(0.5),   // nervegrenade
   SGTC_DELAY(0.5),   // narcogrenade
   SGTC_DELAY(0.5),   // mustardgrenade
   SGTC_DELAY(0.5),   // gacgoa
   SGTC_DELAY(0.0),   // batsup
   SGTC_DELAY(0.0),   // coujaf
   SGTC_DELAY(0.0),   // couteau

   /* terminator */
   {0, 0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0},
       0.0, {0.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 0.0}}
};


//
// Weapon ids differ between SGTC versions, so take them from the
// WeaponList message the server sends to every client.
//
void SGTC_OnWeaponList(const bot_weapon_t &weapon)
{
   static unsigned int reported = 0;
   qboolean known = FALSE;

   for (int i = 0; weapon_select[i].iId && i < NUM_OF_WEAPON_SELECTS; i++)
   {
      bot_weapon_select_t &select = weapon_select[i];

      if (stricmp(select.weapon_name, weapon.szClassname) != 0)
         continue;

      known = TRUE;
      select.iId = weapon.iId;
      fire_delay[i].iId = weapon.iId;

      // weapons without an ammo type (melee, energy) must not wait for ammo
      if (weapon.iAmmo1 < 0)
         select.min_primary_ammo = 0;
   }

   if (weapon.iId >= 0 && weapon.iId < 32 && !(reported & (1u << weapon.iId)))
   {
      reported |= 1u << weapon.iId;
      UTIL_ConsolePrintf("[sgtc] weapon %2d %-24s ammo1=%d/%d ammo2=%d/%d slot=%d/%d %s\n",
         weapon.iId, weapon.szClassname, weapon.iAmmo1, weapon.iAmmo1Max,
         weapon.iAmmo2, weapon.iAmmo2Max, weapon.iSlot, weapon.iPosition,
         known ? "(bot weapon)" : "(not used by bots)");
   }
}


//
// Classes, identified by the player model the server assigns.
// Loadouts are the class packs from info/pack<n>_<class>.txt (C4 included:
// attackers plant it in C4 zones, SGTC_BotC4Think).
//
typedef struct
{
   const char *model;
   int team;
   int weight;               // how often bots pick this class
   const char *packs[4][8];  // NULL terminated lists, empty pack = unused
} sgtc_class_t;

static const sgtc_class_t sgtc_classes[] =
{
   // Tau'ri: classe <n> 1
   {"oneill", SGTC_TEAM_TAURI, 16, {
      {"weapon_couteau", "weapon_zat", "weapon_p90", "weapon_c4", "weapon_handgrenade", NULL},
      {"weapon_couteau", "weapon_m92s", "weapon_lance", "weapon_spas", "weapon_handgrenade", NULL}}},
   {"carter", SGTC_TEAM_TAURI, 16, {
      {"weapon_couteau", "weapon_m16", "weapon_reetou", "weapon_nervegrenade", NULL},
      {"weapon_couteau", "weapon_zat", "weapon_mp5", "weapon_c4", NULL}}},
   {"jackson", SGTC_TEAM_TAURI, 16, {
      {"weapon_couteau", "weapon_m92s", "weapon_psg1", "weapon_c4", "weapon_handgrenade", "weapon_narcogrenade", NULL},
      {"weapon_couteau", "weapon_zat", "weapon_p90", "weapon_mustardgrenade", "weapon_nervegrenade", NULL}}},
   {"tealc", SGTC_TEAM_TAURI, 16, {
      {"weapon_couteau", "weapon_m16", "weapon_lance", "weapon_nervegrenade", NULL},
      {"weapon_couteau", "weapon_zat", "weapon_reetou", "weapon_spas", "weapon_handgrenade", "weapon_narcogrenade", NULL}}},
   {"commander", SGTC_TEAM_TAURI, 36, {
      {"weapon_couteau", "weapon_m16", "weapon_handgrenade", "weapon_narcogrenade", NULL},
      {"weapon_couteau", "weapon_m92s", "weapon_mp5", "weapon_mustardgrenade", "weapon_c4", NULL},
      {"weapon_couteau", "weapon_m92s", "weapon_p90", "weapon_handgrenade", NULL},
      {"weapon_couteau", "weapon_zat", "weapon_psg1", "weapon_nervegrenade", "weapon_mustardgrenade", NULL}}},

   // Goa'uld: classe <n> 2
   // pack 2 (cloak, healing device, zat, Tak sentry) leaves a bot one gun: knife fights
   {"systemlord", SGTC_TEAM_GOAULD, 14, {
      {"weapon_coujaf", "weapon_main", "weapon_zat", "weapon_reetou", NULL}}},
   {"anubis", SGTC_TEAM_GOAULD, 20, {
      {"weapon_coujaf", "weapon_zat", "weapon_lance", "weapon_larve", NULL},
      {"weapon_coujaf", "weapon_batsup", "weapon_zat", "weapon_zatarc", "weapon_reetou", "weapon_gacgoa", NULL}}},
   {"goauld3", SGTC_TEAM_GOAULD, 30, {
      {"weapon_coujaf", "weapon_batsup", "weapon_zat", "weapon_gacgoa", NULL},
      {"weapon_coujaf", "weapon_lance", "weapon_larve", NULL},
      {"weapon_coujaf", "weapon_zatarc", "weapon_reetou", NULL},
      {"weapon_coujaf", "weapon_zat", "weapon_larve", NULL}}},
   {"snakegoauld", SGTC_TEAM_GOAULD, 14, {
      {"weapon_coujaf", "weapon_lance", "weapon_larve", "weapon_gacgoa", NULL},
      {"weapon_coujaf", "weapon_batsup", "weapon_zat", "weapon_reetou", NULL}}},
   {"horus", SGTC_TEAM_GOAULD, 22, {
      {"weapon_coujaf", "weapon_batsup", "weapon_zatarc", NULL},
      {"weapon_coujaf", "weapon_batsup", "weapon_lance", "weapon_larve", NULL}}},

   // Unas game mode
   {"unas", SGTC_TEAM_UNAS, 0, {{NULL}}},

   {NULL, SGTC_TEAM_NONE, 0, {{NULL}}}
};

// classe <slot> <team>: slot order of each team inside sgtc_classes[]
static const int sgtc_team_first_class[3] = {0, 0, 5};

// per-class player limits of the server (server.cfg, 0 = unlimited), in the
// order of sgtc_classes[]
static const char *sgtc_class_limit_cvar[10] = {
   "max_oniell", "max_carter", "max_jackson", "max_tealc", "max_soldier",
   "max_syslord", "max_anubis", "max_jaffa", "max_serpent", "max_horus"};


// Player models are also the class models, so the model alone doesn't say a
// player joined: the server sets pev->team when a side is chosen.
static const sgtc_class_t *SGTC_GetModelClass(edict_t *pEdict)
{
   const char *model = INFOKEY_VALUE(GET_INFOKEYBUFFER(pEdict), "model");

   for (int i = 0; sgtc_classes[i].model; i++)
      if (!stricmp(model, sgtc_classes[i].model))
         return &sgtc_classes[i];

   return NULL;
}

// pev->team value -> side: 2 = Tau'ri, 1 = Goa'uld in SGTC 2.0 (71 of 71
// samples of living bots in tm1_sg_land3). It used to be "learned" from the
// class models, but a model lags behind a team change (mp_autobalance, a new
// class) and the mapping flipped 2-8 times per match, turning bots against
// their own side.
static const int sgtc_side_of_pevteam[8] = {SGTC_TEAM_NONE, SGTC_TEAM_GOAULD, SGTC_TEAM_TAURI,
   SGTC_TEAM_UNAS, SGTC_TEAM_NONE, SGTC_TEAM_NONE, SGTC_TEAM_NONE, SGTC_TEAM_NONE};

static const sgtc_class_t *SGTC_GetPlayerClass(edict_t *pEdict)
{
   if (pEdict->v.team <= 0)
      return NULL;

   return SGTC_GetModelClass(pEdict);
}


int SGTC_GetPlayerTeam(edict_t *pEdict)
{
   int pevteam = pEdict->v.team;

   if (pevteam <= 0 || pevteam >= 8)
      return SGTC_TEAM_NONE;

   return sgtc_side_of_pevteam[pevteam];
}


const char *SGTC_TeamName(int team)
{
   switch (team)
   {
   case SGTC_TEAM_TAURI:  return "Torri";
   case SGTC_TEAM_GOAULD: return "Goa'uld";
   case SGTC_TEAM_UNAS:   return "Unas";
   default:               return "";
   }
}


static int SGTC_CountTeam(int team)
{
   int count = 0;

   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);

      if (!pPlayer || pPlayer->free || FNullEnt(pPlayer) || STRING(pPlayer->v.netname)[0] == 0)
         continue;

      if (SGTC_GetPlayerTeam(pPlayer) == team)
         count++;
   }

   // count bots that asked for this team but haven't spawned with a model yet
   for (int i = 0; i < 32; i++)
      if (bots[i].is_used && bots[i].sgtc_team == team && bots[i].pEdict->v.team <= 0)
         count++;

   return count;
}


// players of 'team' playing 'model', pSelf excluded
static int SGTC_CountClass(int team, const char *model, edict_t *pSelf)
{
   int count = 0;

   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);

      if (!pPlayer || pPlayer == pSelf || pPlayer->free || FNullEnt(pPlayer) || STRING(pPlayer->v.netname)[0] == 0)
         continue;

      if (SGTC_GetPlayerTeam(pPlayer) == team &&
          !stricmp(INFOKEY_VALUE(GET_INFOKEYBUFFER(pPlayer), "model"), model))
         count++;
   }

   return count;
}


static int SGTC_PickClassSlot(edict_t *pEdict, int team, qboolean uniform)
{
   int first_index = sgtc_team_first_class[team];
   const sgtc_class_t *first = &sgtc_classes[first_index];
   int weight[5], limit[5], used[5], total = 0, unique_free = 0;

   // classes at their server limit are refused: don't ask for them. Leave one
   // of the one-per-team classes (the SG-1 members...) free for a human.
   for (int i = 0; i < 5; i++)
   {
      limit[i] = (int)CVAR_GET_FLOAT(sgtc_class_limit_cvar[first_index + i]);
      used[i] = SGTC_CountClass(team, first[i].model, pEdict);
      if (limit[i] == 1 && used[i] == 0)
         unique_free++;
   }

   for (int i = 0; i < 5; i++)
   {
      weight[i] = uniform ? 1 : first[i].weight;
      if (limit[i] > 0 && used[i] >= limit[i])
         weight[i] = 0;
      else if (limit[i] == 1 && unique_free <= 1)
         weight[i] = 0;
      total += weight[i];
   }

   if (total <= 0)
      return RANDOM_LONG2(1, 5);

   int roll = RANDOM_LONG2(1, total);

   for (int i = 0; i < 5; i++)
   {
      roll -= weight[i];
      if (roll <= 0)
         return i + 1;
   }

   return 5;
}


//
// Returns TRUE while the bot is still choosing team/class (it should idle).
//
qboolean SGTC_BotJoinThink(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   char arg_team[8], arg_class[8];

   if (pEdict->v.team > 0)
   {
      // mp_autobalance moves dead players to the smaller team: rejoin that one.
      // Not before the bot picked a side itself: the server sets a team for a
      // moment on connect, the same one for every bot.
      int side = SGTC_GetPlayerTeam(pEdict);
      if (pBot.sgtc_team != SGTC_TEAM_NONE && (side == SGTC_TEAM_TAURI || side == SGTC_TEAM_GOAULD))
         pBot.sgtc_team = side;
      pBot.sgtc_join_step = 3;
      pBot.sgtc_joined = TRUE;
      pBot.sgtc_rejoins = 0;
      return FALSE;
   }

   // test mode: commands are sent by hand with "sgtc_cmd"
   if (!sgtc_autojoin)
      return TRUE;

   // Dead after having played: SGTC clears pev->team until the respawn (the
   // next round in Normal mode). Sending "classe" every second meanwhile
   // (hundreds per match) ended in hl.dll crashes on corrupted entities
   // (CMultiManager::ManagerThink, the game rules think), each time right
   // after such a command: ask at most sgtc_rejoin times per death.
   if (pBot.sgtc_joined && pEdict->v.deadflag != DEAD_NO && pBot.sgtc_rejoins >= sgtc_rejoin)
      return TRUE;

   // the server can set a team briefly on connect, or move a player back to
   // spectator later: start the join sequence (again)
   if (pBot.sgtc_join_step >= 3)
   {
      // keep the side picked before, so a dead bot doesn't change teams
      pBot.sgtc_join_step = 0;
      pBot.f_sgtc_join_time = gpGlobals->time + 0.5;
      return TRUE;
   }

   if (pBot.f_sgtc_join_time > gpGlobals->time)
      return TRUE;

   switch (pBot.sgtc_join_step)
   {
   case 0:  // pick the smaller team
      if (pBot.sgtc_team == SGTC_TEAM_NONE)
      {
         int tauri = SGTC_CountTeam(SGTC_TEAM_TAURI);
         int goauld = SGTC_CountTeam(SGTC_TEAM_GOAULD);

         if (tauri == goauld)
            pBot.sgtc_team = RANDOM_LONG2(SGTC_TEAM_TAURI, SGTC_TEAM_GOAULD);
         else
            pBot.sgtc_team = (tauri < goauld) ? SGTC_TEAM_TAURI : SGTC_TEAM_GOAULD;
      }

      // menu numbering: 1 = Tau'ri, 2 = Goa'uld (pev->team uses the opposite)
      safevoid_snprintf(arg_team, sizeof(arg_team), "%d", pBot.sgtc_team);
      SGTC_TraceCommand(pEdict, "jointeam", arg_team, NULL);
      FakeClientCommand(pEdict, "jointeam", arg_team, NULL);

      pBot.sgtc_join_step = 1;
      pBot.f_sgtc_join_time = gpGlobals->time + 0.2;
      break;

   case 1:  // pick a class; retry with any class if the server refused the last one
      pBot.sgtc_class = SGTC_PickClassSlot(pEdict, pBot.sgtc_team, pBot.sgtc_join_tries > 0);

      // "classe <class slot> <team>" is what actually puts the player in a team
      safevoid_snprintf(arg_team, sizeof(arg_team), "%d", pBot.sgtc_team);
      safevoid_snprintf(arg_class, sizeof(arg_class), "%d", pBot.sgtc_class);
      SGTC_TraceCommand(pEdict, "classe", arg_class, arg_team);
      FakeClientCommand(pEdict, "classe", arg_class, arg_team);
      if (pEdict->v.deadflag != DEAD_NO)
         pBot.sgtc_rejoins++;

      pBot.sgtc_join_step = 2;
      pBot.f_sgtc_join_time = gpGlobals->time + 1.5;
      break;

   default:  // still no class model: try again
      pBot.sgtc_join_tries++;

      if (pBot.sgtc_join_tries % 4 == 0)
      {
         // team may be full or locked, start over
         pBot.sgtc_team = SGTC_TEAM_NONE;
         pBot.sgtc_join_step = 0;
      }
      else
         pBot.sgtc_join_step = 1;

      pBot.f_sgtc_join_time = gpGlobals->time + 0.5;
      break;
   }

   return TRUE;
}


// button-triggered transporters (rt rings, obelisks, telepo pads): end of this file
static void SGTC_TransportBotReset(bot_t &pBot);
static void SGTC_TransportKeyValue(edict_t *pent, KeyValueData *pkvd);
static void SGTC_ResetTransports(void);
static void SGTC_BuildTransports(void);
static void SGTC_WatchTransports(void);
static void SGTC_TransportSafety(bot_t &pBot);
static qboolean SGTC_BotTransportThink(bot_t &pBot);
static qboolean SGTC_TransportRefuses(bot_t &pBot, int goal);
qboolean SGTC_TransportOwns(edict_t *pent);

void SGTC_BotSpawned(bot_t &pBot)
{
   pBot.sgtc_door = 0;
   SGTC_TransportBotReset(pBot);
   if (pBot.f_sgtc_last_loadout > 0 && gpGlobals->time - pBot.f_sgtc_last_loadout < 2.0f)
      return;

   // give the server a moment to strip/equip the player first
   pBot.f_sgtc_loadout_time = gpGlobals->time + 0.3;
   pBot.sgtc_ring = 0;
   pBot.sgtc_ring_hold = FALSE;
}


// weapon id from the WeaponList names (the MP5 is created as weapon_mp5 but listed as weapon_9mmAR)
static int SGTC_WeaponId(const char *classname)
{
   if (!stricmp(classname, "weapon_mp5"))
      classname = "weapon_9mmAR";

   for (int id = 1; id < 32; id++)
      if (!stricmp(weapon_defs[id].szClassname, classname))
         return id;

   return 0;
}


static void SGTC_GiveNamedItem(edict_t *pEdict, const char *classname)
{
   int id = SGTC_WeaponId(classname);

   // never hand out a weapon the bot already has (every class starts with its knife):
   // SGTC merges duplicates its own way and removing the extra copy could leave the
   // player's inventory pointing at a freed weapon
   if (id > 0 && (pEdict->v.weapons & (1u << id)))
      return;

   edict_t *pItem = CREATE_NAMED_ENTITY(ALLOC_STRING(classname));

   if (FNullEnt(pItem))
      return;

   pItem->v.origin = pEdict->v.origin;
   pItem->v.spawnflags |= (1 << 30);  // SF_NORESPAWN

   MDLL_Spawn(pItem);
   MDLL_Touch(pItem, pEdict);

   if (sgtc_trace)
      UTIL_ConsolePrintf("[sgtc] give %s -> %s: aiment=%d owner=%d effects=%d solid=%d movetype=%d flags=%d nextthink=%.2f (t=%.2f)\n",
         classname, STRING(pEdict->v.netname),
         pItem->v.aiment == pEdict, pItem->v.owner == pEdict, pItem->v.effects, pItem->v.solid,
         pItem->v.movetype, pItem->v.flags, pItem->v.nextthink, gpGlobals->time);

   // refused by the game (class/team rules): don't leave it lying around, but only
   // if it really didn't end up in the inventory in any form
   if (pItem->v.aiment != pEdict && pItem->v.owner != pEdict &&
       !(id > 0 && (pEdict->v.weapons & (1u << id))))
      pItem->v.flags |= FL_KILLME;
}


void SGTC_BotLoadoutThink(bot_t &pBot)
{
   if (pBot.f_sgtc_loadout_time <= 0 || pBot.f_sgtc_loadout_time > gpGlobals->time)
      return;

   pBot.f_sgtc_loadout_time = 0;

   if (!sgtc_loadout)
      return;

   const sgtc_class_t *pClass = SGTC_GetPlayerClass(pBot.pEdict);

   if (!pClass)
      return;

   int num_packs = 0;
   while (num_packs < 4 && pClass->packs[num_packs][0])
      num_packs++;

   if (num_packs == 0)
      return;

   const char * const *pack = pClass->packs[RANDOM_LONG2(0, num_packs - 1)];

   pBot.f_sgtc_last_loadout = gpGlobals->time;

   for (int i = 0; pack[i]; i++)
      SGTC_GiveNamedItem(pBot.pEdict, pack[i]);

   // out with the best gun now: the server hands out the knife first, and
   // jk_botti waits a moment before changing weapons, so the bot charged
   // its first enemy with the knife
   for (int i = 0; weapon_select[i].iId; i++)
   {
      const bot_weapon_select_t &w = weapon_select[i];
      if ((w.type & (WEAPON_MELEE | WEAPON_THROW)) || w.iId <= 0 || w.iId >= 32 ||
          !(pBot.pEdict->v.weapons & (1u << w.iId)))
         continue;
      FakeClientCommand(pBot.pEdict, w.weapon_name, NULL, NULL);
      break;
   }

   // super weapons are rare and can be disabled by the server
   if (CVAR_GET_FLOAT("mp_superweapons") > 0 && RANDOM_LONG2(1, 100) <= 8)
      SGTC_GiveNamedItem(pBot.pEdict, pClass->team == SGTC_TEAM_TAURI ? "weapon_usas" : "weapon_glstaff");
}


//
// SGTC runs with sv_maxspeed 1000 and limits each class through the client
// maxspeed. Humans also can't go over cl_forwardspeed (400), so cap the bots the
// same way, or they race at up to 1000 whenever the class limit isn't set.
//
void SGTC_ClampSpeed(bot_t &pBot)
{
   float limit = 400.0f;
   float class_speed = pBot.pEdict->v.maxspeed;

   if (class_speed > 0.0f && class_speed < limit)
      limit = class_speed;

   // pev->maxspeed is what SET_CLIENT_MAXSPEED sets, follow it both ways (stun, sprint...)
   pBot.f_max_speed = limit;
}


void SGTC_DumpPlayers(void (*print)(const char *))
{
   char msg[512];

   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);

      if (!pPlayer || pPlayer->free || FNullEnt(pPlayer) || STRING(pPlayer->v.netname)[0] == 0)
         continue;

      char *infobuffer = GET_INFOKEYBUFFER(pPlayer);
      char weapons[256] = "";
      size_t len = 0;

      for (int id = 1; id < 32; id++)
      {
         if (!(pPlayer->v.weapons & (1u << id)))
            continue;

         const char *name = weapon_defs[id].szClassname[0] ? weapon_defs[id].szClassname + 7 : "?";
         safevoid_snprintf(weapons + len, sizeof(weapons) - len, "%s%s(%d)", len ? "," : "", name, id);
         len = strlen(weapons);
      }

      safevoid_snprintf(msg, sizeof(msg),
         "[sgtc] #%d %-20s team=%s(%d) pev.team=%d class=%d model=%s infoteam=%s hp=%.0f dead=%d frags=%.0f pos=%.0f,%.0f,%.0f weapons=%s\n",
         i, STRING(pPlayer->v.netname), SGTC_TeamName(SGTC_GetPlayerTeam(pPlayer)), SGTC_GetPlayerTeam(pPlayer),
         pPlayer->v.team, pPlayer->v.playerclass, INFOKEY_VALUE(infobuffer, "model"),
         INFOKEY_VALUE(infobuffer, "team"), pPlayer->v.health, pPlayer->v.deadflag, pPlayer->v.frags,
         pPlayer->v.origin.x, pPlayer->v.origin.y, pPlayer->v.origin.z, weapons);
      print(msg);
   }
}


//
// Debug trace ("sgtc_trace 1"): log user messages sent to bots and the
// team/class commands they send, to follow the server's join sequence.
//
int sgtc_trace = 0;
int sgtc_autojoin = 1;
int sgtc_wptdump = 0;  // write the final waypoint graph to waypoints/<map>_graph.txt
int sgtc_loadout = 1;  // test switches: give class packs, dial DHDs
int sgtc_dial = 1;
int sgtc_defend_travel = 1;  // defenders may follow the attackers through an open gate ("sgtc_defend_travel" command)
static int SGTC_BotSideRank(bot_t &pBot);
int sgtc_ringdebug = 0;  // ring cycles and players near platforms, 4 times a second
int sgtc_stats = 0;     // [sgtc-stat] lines on the console too (always in the log when "log on")
int sgtc_rejoin = 1;  // class requests a dead bot may send per death ("sgtc_rejoin" command)
int sgtc_humanreset = 1;  // the first human to pick a class restarts the round ("sgtc_humanreset")
static float sgtc_humanreset_off;   // when to set mp_resetround back to 0
static float sgtc_humanreset_last = -1000.0f;

//
// In Normal mode a player who joins mid-round waits for the next round (up to
// mp_roundtime). When the first human picks a class while only bots play,
// restart the round so that they play right away: mp_resetround 1 respawns
// everyone at once (tested: 3 s later every bot was back at its spawn with
// 100 hp), set back to 0 a few seconds later.
//
void SGTC_HumanPicksClass(edict_t *pEntity)
{
   if (!sgtc_humanreset || (int)CVAR_GET_FLOAT("mp_gamemode") != 0)
      return;
   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *p = INDEXENT(i);
      if (!p || p == pEntity || p->free || !p->pvPrivateData || STRING(p->v.netname)[0] == 0 ||
          (p->v.flags & FL_FAKECLIENT))
         continue;
      if (p->v.team > 0 && IsAlive(p))
         return;  // another human is already playing
   }
   if (gpGlobals->time - sgtc_state.f_round_start < 10.0f)
      return;  // the round has just begun
   if (sgtc_humanreset_last <= gpGlobals->time && gpGlobals->time - sgtc_humanreset_last < 60.0f)
      return;
   sgtc_humanreset_last = gpGlobals->time;
   sgtc_humanreset_off = gpGlobals->time + 3.0f;
   UTIL_ConsolePrintf("[sgtc-stat] first human %s picks a class: round restarted\n", STRING(pEntity->v.netname));
   SERVER_COMMAND("mp_resetround 1\n");
}
static int sgtc_trace_msg_active = 0;
static char sgtc_trace_buf[512];

void SGTC_TraceBegin(int msg_type, edict_t *pBotEdict)
{
   sgtc_trace_msg_active = 0;

   if (!sgtc_trace)
      return;

   const char *name = GET_USER_MSG_NAME(PLID, msg_type, NULL);

   // skip noisy per-frame messages
   if (name && (!strcmp(name, "Health") || !strcmp(name, "CurWeapon") || !strcmp(name, "AmmoX") ||
                !strcmp(name, "WeaponList") || !strcmp(name, "Battery") || !strcmp(name, "Damage")))
      return;

   sgtc_trace_msg_active = 1;
   safevoid_snprintf(sgtc_trace_buf, sizeof(sgtc_trace_buf), "[sgtc] %.1f msg->%s %s(%d):",
      gpGlobals->time, STRING(pBotEdict->v.netname), name ? name : "?", msg_type);
}

void SGTC_TraceValue(const char *str, int value)
{
   if (!sgtc_trace_msg_active)
      return;

   size_t len = strlen(sgtc_trace_buf);

   if (str)
      safevoid_snprintf(sgtc_trace_buf + len, sizeof(sgtc_trace_buf) - len, " \"%s\"", str);
   else
      safevoid_snprintf(sgtc_trace_buf + len, sizeof(sgtc_trace_buf) - len, " %d", value);
}

void SGTC_TraceEnd(void)
{
   if (!sgtc_trace_msg_active)
      return;

   sgtc_trace_msg_active = 0;

   // keep each message on one console line
   for (char *p = sgtc_trace_buf; *p; p++)
      if (*p == '\n' || *p == '\r')
         *p = '|';

   UTIL_ConsolePrintf("%s\n", sgtc_trace_buf);
}

void SGTC_TraceCommand(edict_t *pEdict, const char *a1, const char *a2, const char *a3)
{
   if (sgtc_trace)
      UTIL_ConsolePrintf("[sgtc] %.1f cmd<-%s: %s %s %s (pev.team=%d dead=%d)\n", gpGlobals->time,
         STRING(pEdict->v.netname), a1, a2 ? a2 : "", a3 ? a3 : "", pEdict->v.team, pEdict->v.deadflag);
}


//
// Game state from the server's TextMsg titles (#KEY_COLLECTED, #BOMB_ARMED,
// #Torri_goal...). Every client gets its own copy, so repeats are filtered.
//
sgtc_state_t sgtc_state;
static int sgtc_textmsg_state = 0;

static void SGTC_OnGameEvent(const char *key)
{
   static char last_key[64];
   static float last_time = -100;

   if (!strcmp(key, last_key) && gpGlobals->time - last_time < 1.0f && gpGlobals->time >= last_time)
      return;
   safe_strcopy(last_key, sizeof(last_key), key);
   last_time = gpGlobals->time;

   if (!stricmp(key, "#KEY_COLLECTED"))
      sgtc_state.key_taken = TRUE;
   else if (!stricmp(key, "#KEY_DROPPED"))
      sgtc_state.key_taken = FALSE;
   else if (!stricmp(key, "#BOMB_ARMING"))
      sgtc_state.f_bomb_arming_time = gpGlobals->time;
   else if (!stricmp(key, "#BOMB_ARMING_INTERRUPTED"))
      sgtc_state.f_bomb_arming_time = 0;
   else if (!stricmp(key, "#BOMB_ARMED"))
      sgtc_state.f_bomb_armed_time = gpGlobals->time;
   else if (!stricmp(key, "#Torri_goal") || !stricmp(key, "#Goauld_goal") || !stricmp(key, "#Round_draw"))
   {
      int winner = !stricmp(key, "#Torri_goal") ? SGTC_TEAM_TAURI : !stricmp(key, "#Goauld_goal") ? SGTC_TEAM_GOAULD : 0;
      sgtc_state.wins[winner]++;
      UTIL_ConsolePrintf("[sgtc-stat] round_end winner=%s round_time=%.0f tauri_wins=%d goauld_wins=%d draws=%d\n",
         winner ? SGTC_TeamName(winner) : "draw", gpGlobals->time - sgtc_state.f_round_start,
         sgtc_state.wins[SGTC_TEAM_TAURI], sgtc_state.wins[SGTC_TEAM_GOAULD], sgtc_state.wins[0]);
      SGTC_RoundReset();
      return;
   }
   else
      return;

   UTIL_ConsolePrintf("[sgtc-stat] event %s t=%.1f\n", key + 1, gpGlobals->time - sgtc_state.f_round_start);
}

void SGTC_BotClient_TextMsg(void *p, int bot_index)
{
   // TextMsg: byte destination, then strings
   if (sgtc_textmsg_state++ == 0 || !p)
      return;

   const char *str = (const char *)p;

   if (str[0] == '#')
      SGTC_OnGameEvent(str);
}

void SGTC_BotClient_TextMsgEnd(void *p, int bot_index)
{
   sgtc_textmsg_state = 0;
}

// Concuss: byte concussion amount, byte game event (client.dll turns the
// event into a Titles.txt message; 0 = a real concussion)
static const char *sgtc_concuss_events[] = {NULL, "#KEY_COLLECTED", "#KEY_DROPPED", "#BOMB_ARMING",
   "#BOMB_ARMING_INTERRUPTED", "#BOMB_ARMED", "#CTRL_CAP_ENMY", "#CTRL_CAP_ALLY", "#STRUCT_ATTACKED",
   "#ENMY_CAP_KILLED", "#ALLY_CAP_KILLED", "#TOURN_START", "#TOURN_ABORTED", "#TOURN_AWARD",
   "#CTP_STND_TAU", "#CTP_STND_GOU"};
static int sgtc_concuss_state = 0;

void SGTC_BotClient_Concuss(void *p, int bot_index)
{
   if (sgtc_concuss_state++ != 1 || !p)
      return;

   int event = *(int *)p;
   if (event > 0 && event < (int)(sizeof(sgtc_concuss_events) / sizeof(sgtc_concuss_events[0])))
      SGTC_OnGameEvent(sgtc_concuss_events[event]);
}

void SGTC_BotClient_ConcussEnd(void *p, int bot_index)
{
   sgtc_concuss_state = 0;
}

// the round winner is announced with music: music/tau0N, goa0N, draw0N
void SGTC_OnSound(const char *sample)
{
   if (!sample || strncmp(sample, "music/", 6))
      return;
   if (!strncmp(sample + 6, "tau", 3))
      SGTC_OnGameEvent("#Torri_goal");
   else if (!strncmp(sample + 6, "goa", 3))
      SGTC_OnGameEvent("#Goauld_goal");
   else if (!strncmp(sample + 6, "draw", 4))
      SGTC_OnGameEvent("#Round_draw");
}

// when a bot last planted C4 in each C4 zone (one planter at a time)
static float sgtc_c4_planted[8];
// where a player can stand inside each C4 zone (found on first use per map)
static Vector sgtc_c4_spot[8][16];
static int sgtc_c4_num_spots[8];
static qboolean sgtc_c4_spots_done[8];
// standing spots around the nuke from which +use reaches it (found on first use per map)
static Vector sgtc_nuke_spot[16];
static int sgtc_nuke_num_spots;
static qboolean sgtc_nuke_spots_done;
static Vector sgtc_nuke_spots_at;
static int sgtc_c4_spot_fails[8][16];  // failed plants from each spot
static qboolean sgtc_c4_zone_blown[8];  // a charge went off inside it this round

void SGTC_RoundReset(void)
{
   memset(sgtc_c4_planted, 0, sizeof(sgtc_c4_planted));
   memset(sgtc_c4_zone_blown, 0, sizeof(sgtc_c4_zone_blown));
   sgtc_state.key_taken = FALSE;
   sgtc_state.f_bomb_arming_time = 0;
   sgtc_state.f_bomb_armed_time = 0;
   sgtc_state.f_round_start = gpGlobals->time;
}

void SGTC_MapReset(void)
{
   memset(&sgtc_state, 0, sizeof(sgtc_state));
   memset(sgtc_c4_planted, 0, sizeof(sgtc_c4_planted));
   memset(sgtc_c4_spots_done, 0, sizeof(sgtc_c4_spots_done));
   sgtc_nuke_spots_done = FALSE;
}


//
// Map objectives: found by classname twice per second, so they follow round
// restarts and the bomb key moving around with its carrier.
//
#include "waypoint.h"

extern WAYPOINT waypoints[MAX_WAYPOINTS];

// a drop deeper than 100 units 'dist' units away in that direction? A wall
// before that: tested just in front of it (no drop when the wall is close)
static qboolean SGTC_LedgeAt(edict_t *pEdict, const Vector &dir, float dist)
{
   int hull = (pEdict->v.flags & FL_DUCKING) ? head_hull : human_hull;
   Vector o = pEdict->v.origin, p = o + dir * dist;
   TraceResult tr;

   UTIL_TraceHull(o, p, ignore_monsters, hull, pEdict, &tr);
   if (tr.fStartSolid || tr.fAllSolid)
      return FALSE;
   if (tr.flFraction < 1.0f)
   {
      float d = tr.flFraction * dist - 4.0f;
      if (d < 36.0f)
         return FALSE;
      p = o + dir * d;
   }
   UTIL_TraceHull(p, p - Vector(0, 0, 100), ignore_monsters, hull, pEdict, &tr);
   return !tr.fStartSolid && tr.flFraction >= 1.0f;
}

// a drop deeper than 100 units 40 units away in that direction (no wall first)?
static qboolean SGTC_LedgeAhead(edict_t *pEdict, const Vector &dir)
{
   return SGTC_LedgeAt(pEdict, dir, 40.0f);
}

// the func_ladder whose brush is within 'r' units of o sideways, o within its height
static qboolean SGTC_LadderNear(const Vector &o, float r, Vector &mins, Vector &maxs)
{
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "func_ladder")))
   {
      Vector mn = pent->v.origin + pent->v.mins, mx = pent->v.origin + pent->v.maxs;
      if (o.x < mn.x - r || o.x > mx.x + r || o.y < mn.y - r || o.y > mx.y + r ||
          o.z < mn.z - 37 || o.z > mx.z + 37)
         continue;
      mins = mn;
      maxs = mx;
      return TRUE;
   }
   return FALSE;
}

// the waypoint after the bot's current one on its route, -1 if none
int SGTC_RouteNext(bot_t &pBot)
{
   int w = pBot.curr_waypoint_index, goal = pBot.waypoint_goal;
   if (w < 0 || goal < 0 || w == goal)
      return -1;
   int next = WaypointRouteFromTo(w, goal);
   return next >= 0 && next != WAYPOINT_UNREACHABLE ? next : -1;
}

//
// Last check before each move: don't walk or strafe off a ledge into a pit
// (sg_crypt's broken bridge: about 9 falls per 10 minutes, all along the
// bridgehead, mostly while fighting). Not when the route itself jumps,
// climbs or drops there.
//
void SGTC_AvoidLedges(bot_t &pBot, const float *viewangles, float &forwardmove, float &sidemove)
{
   edict_t *pEdict = pBot.pEdict;
   float yaw = viewangles[1] * (M_PI / 180.0f);
   Vector forward(cosf(yaw), sinf(yaw), 0), right(sinf(yaw), -cosf(yaw), 0);
   qboolean grounded = (pEdict->v.flags & FL_ONGROUND) && !pBot.b_on_ladder && !pBot.b_in_water &&
                       pEdict->v.movetype == MOVETYPE_WALK;

   // a move asked in world space (the rings, the transporters): forward/side
   // from the yaw, stopped short of a drop
   if (pBot.f_sgtc_move_until > gpGlobals->time && pBot.f_sgtc_move_until < gpGlobals->time + 1.0f)
   {
      forwardmove = DotProduct(pBot.v_sgtc_move, forward);
      sidemove = DotProduct(pBot.v_sgtc_move, right);
      Vector m(pBot.v_sgtc_move.x, pBot.v_sgtc_move.y, 0);
      float len = m.Length();
      if (grounded && len > 1.0f && SGTC_LedgeAt(pEdict, m * (1.0f / len), 24.0f))
         forwardmove = sidemove = 0;
      return;
   }

   float max_speed = pBot.f_max_speed > 0 ? pBot.f_max_speed : 300.0f;

   // in the air after stepping off an edge toward a ladder (sg_pyramide's ropes
   // hang ~60 units out over their shafts): jk_botti moves backwards to catch a
   // ladder it walked off the top of, right for a wall ladder behind the bot but
   // a brake in the air for a rope ahead of it. Head for the ladder brush.
   if (!grounded && !pBot.b_on_ladder && pEdict->v.movetype == MOVETYPE_WALK && !(pEdict->v.flags & FL_ONGROUND) &&
       pBot.curr_waypoint_index != -1 && (waypoints[pBot.curr_waypoint_index].flags & W_FL_LADDER))
   {
      Vector mins, maxs;
      if (SGTC_LadderNear(pEdict->v.origin, 120.0f, mins, maxs))
      {
         Vector to((mins.x + maxs.x) * 0.5f - pEdict->v.origin.x, (mins.y + maxs.y) * 0.5f - pEdict->v.origin.y, 0);
         float d = to.Length();
         if (d > 1.0f)
         {
            to = to * (1.0f / d);
            forwardmove = DotProduct(to, forward) * max_speed;
            sidemove = DotProduct(to, right) * max_speed;
         }
         return;
      }
   }

   if (!grounded)
      return;
   int w = pBot.curr_waypoint_index, prev = pBot.prev_waypoint_index[0];
   int next = SGTC_RouteNext(pBot);
   // the run-up waypoint before a jump is part of the run (braking there at the
   // gap made sg_crypt's bots jump at 80 u/s)
   if (w != -1 && !(waypoints[w].flags & W_FL_JUMP) && next != -1 && (waypoints[next].flags & W_FL_JUMP) &&
       (waypoints[next].origin - pEdict->v.origin).Length() < 200.0f)
      w = next;
   if (w != -1 && (waypoints[w].flags & W_FL_JUMP))
   {
      // the run to a gap jump: full speed, no strafe (a slow or sideways
      // takeoff falls short: sg_crypt's broken bridge)
      if (forwardmove > 0 && (waypoints[w].origin - pEdict->v.origin).Length() < 200.0f)
      {
         forwardmove = max_speed;
         sidemove = 0;
      }
      return;
   }
   if (w != -1 && ((waypoints[w].flags & W_FL_LADDER) || waypoints[w].origin.z < pEdict->v.origin.z - 60.0f))
      return;
   if (prev != -1 && (waypoints[prev].flags & W_FL_JUMP))
      return;  // on the way from a jump to its landing

   // moving fast toward a drop: HL's ground friction needs speed/4 units to
   // stop (100 at 400 u/s), more than the 40-unit look-ahead below, so the
   // bots slid off sg_target2a's walkways and sg_pyramide's atrium. Brake.
   Vector vel(pEdict->v.velocity.x, pEdict->v.velocity.y, 0);
   float speed = vel.Length();
   if (speed > 150.0f)
   {
      Vector vdir = vel * (1.0f / speed);
      if (SGTC_LedgeAt(pEdict, vdir, 24.0f + speed * 0.3f))
      {
         forwardmove = -DotProduct(vdir, forward) * max_speed;
         sidemove = -DotProduct(vdir, right) * max_speed;
         return;
      }
   }

   if (forwardmove != 0 && SGTC_LedgeAhead(pEdict, forward * (forwardmove > 0 ? 1.0f : -1.0f)))
      forwardmove = 0;
   if (sidemove != 0 && SGTC_LedgeAhead(pEdict, right * (sidemove > 0 ? 1.0f : -1.0f)))
      sidemove = 0;
}

extern int num_waypoints;

#define SGTC_MAX_OBJ 8

typedef struct
{
   edict_t *goal[SGTC_MAX_OBJ];     int num_goals;      // trigger_goal (reach it)
   edict_t *capture[SGTC_MAX_OBJ];  int num_captures;   // zone_capture
   edict_t *bombzone[SGTC_MAX_OBJ]; int num_bombzones;  // zone_bomb_target
   edict_t *shelter[SGTC_MAX_OBJ];  int num_shelters;   // zone_shelter (survive the bomb)
   edict_t *key;                    // item_objective
   edict_t *key_carrier;            // player holding the key, if known
   Vector v_key_last;               // where the key was last seen
   int key_state;                   // 0 unknown, 1 lying there, 2 carried by key_carrier
   edict_t *nuke;                   // game_nuke
   edict_t *goal_button;            // no nuke: the card button that unlocks the goal (sg_conquest's "Stolen_btn")
   qboolean goal_unlocked;          // ... shows pressed
   Vector spawn_center[3];          // per side
   int attack_team;                 // side that has to do the objective, 0 = none
   float f_next_scan;
   qboolean paths_built;            // gate/DHD approach waypoints added
   float f_next_pos_dump;
   int cov_state;                   // SGTC_CoverageThink: 0 start ... 4 done
   int cov_num, cov_next;           // flood fill cells, next one to expand / check
   int cov_num_new, cov_added;      // cells picked for a waypoint, added so far
   int cov_num_jumps;               // gaps to jump found by the flood fill
   double f_cov_secs;               // real time it started
} sgtc_objectives_t;

static sgtc_objectives_t sgtc_obj;
static void SGTC_ScanGates(void);
static void SGTC_ResetGates(void);
static void SGTC_BuildGatePaths(void);
static void SGTC_BuildRings(void);
static void SGTC_BuildDoors(void);

// doors a button opens. A button (func_button, func_rot_button) reaches its
// doors (func_door, func_door_rotating) directly or through one or two
// multi_managers / trigger_relays; a door also fires its own target once
// open or shut (sg_strike3: door01 -> door1). Shut = still where the map
// drew it (a start-open door is moved open when it spawns).
#define SGTC_MAX_DOORS 24
#define SGTC_MAX_PANELS 48
#define SGTC_MAX_BUTTONS 32
#define SGTC_DOOR_PANELS 4
#define SGTC_DOOR_BUTTONS 4
#define SGTC_BTN_REACH 8
#define SGTC_BTN_WPTS 3
typedef struct
{
   edict_t *ent;
   string_t classname;  // a freed and reused edict is no longer this panel
   Vector closed;       // origin (func_door) or angles (func_door_rotating) where it is shut
   qboolean rotating;
   qboolean start_open; // spawnflags 1: open when the round starts, a press shuts it
} sgtc_panel_t;
typedef struct
{
   edict_t *ent;
   char target[64];     // at map start: a one-shot button gets another one once used ("nowhere")
   int team;            // m_team: SGTC's ButtonUse refuses the other side (1 Tau'ri, 2 Goa'uld)
   int needcard;        // SGTC's needcard: only a player carrying a card may press it
   qboolean touch;      // spawnflags 256: fires when walked into, +use does nothing
   int reach[SGTC_BTN_REACH];             // every door panel a press moves (sgtc_panels)
   float reach_delay[SGTC_BTN_REACH];
   qboolean reach_opens[SGTC_BTN_REACH];  // through the button's own chain, within 10 s
   int num_reach;
   int w;               // the first of wpt[]
   int wpt[SGTC_BTN_WPTS], num_wpt;  // waypoints to press it from: in use reach, one per side of a wall
} sgtc_button_t;
typedef struct
{
   edict_t *door;       // its first panel
   int panel[SGTC_DOOR_PANELS], num_panels;     // the panels of that name next to it
   int button[SGTC_DOOR_BUTTONS], num_buttons;  // the buttons that open it
   Vector mins, maxs;   // where it is when shut
   qboolean passage;    // a wall slab, not a floor plate, a hatch or a canopy
   int crossing;        // waypoint paths through it
   int w_a, w_b;        // forced path across it (-1: paths already cross it)
   float f_lock;        // just pressed: nobody presses again before then
   float f_pressed;
   edict_t *presser;
   qboolean press_checked;  // the last press moved it, or was logged as doing nothing
   int last_state;      // for the log
} sgtc_door_t;
static sgtc_door_t sgtc_doors[SGTC_MAX_DOORS];
static int sgtc_num_doors;
static sgtc_panel_t sgtc_panels[SGTC_MAX_PANELS];
static int sgtc_num_panels;
static sgtc_button_t sgtc_buttons[SGTC_MAX_BUTTONS];
static int sgtc_num_buttons;

// keyvalues seen while the map loads: multi_manager targets (name -> delay),
// m_team / needcard / delay of buttons and relays, origin / angles of doors
#define SGTC_MAX_MM_KV 1024
static struct { edict_t *ent; char name[32]; float delay; } sgtc_mm_kv[SGTC_MAX_MM_KV];
static int sgtc_num_mm_kv;
#define SGTC_MAX_ENT_KV 512
static struct { edict_t *ent; int team, needcard; float delay; Vector origin, angles; } sgtc_ent_kv[SGTC_MAX_ENT_KV];
static int sgtc_num_ent_kv;

static int SGTC_EntKv(edict_t *pent)
{
   for (int i = 0; i < sgtc_num_ent_kv; i++)
      if (sgtc_ent_kv[i].ent == pent)
         return i;
   return -1;
}

static void SGTC_DoorKeyValue(edict_t *pent, KeyValueData *pkvd)
{
   const char *cls = pkvd->szClassName, *key = pkvd->szKeyName;

   if (!cls || !key || !pkvd->szValue)
      return;
   if (!stricmp(cls, "multi_manager"))
   {
      // its other keys are the targets it fires, with their delays ("name#2": once more)
      static const char *skip[] = {"classname", "targetname", "origin", "wait", "spawnflags", "angles",
                                   "target", "delay", "killtarget", "model", NULL};
      for (int i = 0; skip[i]; i++)
         if (!stricmp(key, skip[i]))
            return;
      if (sgtc_num_mm_kv >= SGTC_MAX_MM_KV || strlen(key) >= sizeof(sgtc_mm_kv[0].name))
         return;
      strcpy(sgtc_mm_kv[sgtc_num_mm_kv].name, key);
      char *hash = strchr(sgtc_mm_kv[sgtc_num_mm_kv].name, '#');
      if (hash)
         *hash = 0;
      sgtc_mm_kv[sgtc_num_mm_kv].ent = pent;
      sgtc_mm_kv[sgtc_num_mm_kv].delay = atof(pkvd->szValue);
      sgtc_num_mm_kv++;
      return;
   }
   // a func_door is shut at its "origin" (0 0 0 without one), a func_door_rotating at its "angles"
   if (!stricmp(cls, "func_door") || !stricmp(cls, "func_door_rotating"))
   {
      if (stricmp(key, !stricmp(cls, "func_door") ? "origin" : "angles"))
         return;
   }
   else if (stricmp(cls, "func_button") && stricmp(cls, "func_rot_button") && stricmp(cls, "trigger_relay") &&
            stricmp(cls, "trigger_hurt"))
      return;
   else if (stricmp(key, "m_team") && stricmp(key, "needcard") && stricmp(key, "delay"))
      return;
   int i = SGTC_EntKv(pent);
   if (i == -1)
   {
      if (sgtc_num_ent_kv >= SGTC_MAX_ENT_KV)
         return;
      i = sgtc_num_ent_kv++;
      sgtc_ent_kv[i].ent = pent;
      sgtc_ent_kv[i].team = sgtc_ent_kv[i].needcard = 0;
      sgtc_ent_kv[i].delay = 0;
      sgtc_ent_kv[i].origin = sgtc_ent_kv[i].angles = Vector(0, 0, 0);
   }
   float v[3] = {0, 0, 0};
   if (!stricmp(key, "m_team"))
      sgtc_ent_kv[i].team = atoi(pkvd->szValue);
   else if (!stricmp(key, "needcard"))
      sgtc_ent_kv[i].needcard = atoi(pkvd->szValue);
   else if (!stricmp(key, "delay"))
      sgtc_ent_kv[i].delay = atof(pkvd->szValue);
   else if (sscanf(pkvd->szValue, "%f %f %f", &v[0], &v[1], &v[2]) == 3)
   {
      if (!stricmp(key, "origin"))
         sgtc_ent_kv[i].origin = Vector(v[0], v[1], v[2]);
      else
         sgtc_ent_kv[i].angles = Vector(v[0], v[1], v[2]);
   }
}
static void SGTC_RingDebug(void);
static void SGTC_WatchC4Walls(void);
static void SGTC_WatchC4Charges(void);
static void SGTC_WatchRings(void);
static void SGTC_RingSeparate(edict_t *pEdict);
static void SGTC_RingEscape(bot_t &pBot);
static void SGTC_ResetRings(void);
static qboolean SGTC_CoverageThink(void);
static void SGTC_DumpWaypointGraph(void);

// trigger_goal "m_team" (1 = Tau'ri, 2 = Goa'uld) and "master" keyvalues, seen while the map loads
static struct { edict_t *ent; int team; char master[32]; } sgtc_goal_teams[SGTC_MAX_OBJ];
static int sgtc_num_goal_teams = 0;

// trigger_changetarget "m_iszNewTarget" (private data): the targets the map
// may give an entity (sg_conquest: "pc_on" points the PC at "sgmm")
#define SGTC_MAX_CT_KV 512
static struct { edict_t *ent; char newtarget[32]; } sgtc_ct_kv[SGTC_MAX_CT_KV];
static int sgtc_num_ct_kv = 0;

void SGTC_KeyValue(edict_t *pent, KeyValueData *pkvd)
{
   SGTC_TransportKeyValue(pent, pkvd);
   if (pkvd->szClassName && !stricmp(pkvd->szClassName, "trigger_goal") &&
       (!stricmp(pkvd->szKeyName, "m_team") || !stricmp(pkvd->szKeyName, "master")))
   {
      int i;
      for (i = 0; i < sgtc_num_goal_teams && sgtc_goal_teams[i].ent != pent; i++)
         ;
      if (i == sgtc_num_goal_teams && sgtc_num_goal_teams < SGTC_MAX_OBJ)
      {
         sgtc_goal_teams[i].ent = pent;
         sgtc_goal_teams[i].team = 0;
         sgtc_goal_teams[i].master[0] = 0;
         sgtc_num_goal_teams++;
      }
      if (i < sgtc_num_goal_teams)
      {
         if (!stricmp(pkvd->szKeyName, "m_team"))
            sgtc_goal_teams[i].team = atoi(pkvd->szValue);
         else
            safevoid_snprintf(sgtc_goal_teams[i].master, sizeof(sgtc_goal_teams[i].master), "%s", pkvd->szValue);
      }
   }
   if (pkvd->szClassName && !stricmp(pkvd->szClassName, "trigger_changetarget") &&
       !stricmp(pkvd->szKeyName, "m_iszNewTarget") && sgtc_num_ct_kv < SGTC_MAX_CT_KV &&
       strlen(pkvd->szValue) < sizeof(sgtc_ct_kv[0].newtarget))
   {
      sgtc_ct_kv[sgtc_num_ct_kv].ent = pent;
      strcpy(sgtc_ct_kv[sgtc_num_ct_kv].newtarget, pkvd->szValue);
      sgtc_num_ct_kv++;
   }
   SGTC_DoorKeyValue(pent, pkvd);
}

static int SGTC_GoalTeam(edict_t *pGoal)
{
   for (int i = 0; i < sgtc_num_goal_teams; i++)
      if (sgtc_goal_teams[i].ent == pGoal)
         return sgtc_goal_teams[i].team;
   return 0;
}

static const char *SGTC_GoalMaster(edict_t *pGoal)
{
   for (int i = 0; i < sgtc_num_goal_teams; i++)
      if (sgtc_goal_teams[i].ent == pGoal)
         return sgtc_goal_teams[i].master;
   return "";
}

static Vector SGTC_EntCenter(edict_t *pent)
{
   if (pent->v.absmax != pent->v.absmin)
      return (pent->v.absmin + pent->v.absmax) * 0.5f;
   // inactive triggers aren't linked (no absmin/absmax): use the brush model bounds
   if (pent->v.maxs != pent->v.mins)
      return pent->v.origin + (pent->v.mins + pent->v.maxs) * 0.5f;
   return pent->v.origin;
}

static int SGTC_FindAll(const char *classname, edict_t **list)
{
   int n = 0;
   edict_t *pent = NULL;

   while (n < SGTC_MAX_OBJ && !FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", classname)))
      list[n++] = pent;
   return n;
}

static qboolean SGTC_IsPlayerAlive(edict_t *pent)
{
   return !FNullEnt(pent) && (pent->v.flags & FL_CLIENT) && pent->v.deadflag == DEAD_NO && pent->v.health > 0;
}

// each side's spawn points grouped by distance (single linkage, 800 units),
// biggest first: sg_conquest's Tau'ri spawn in the SGC (15, z -3095) and on
// the planet (4, z 95); their average is a point in mid-air
#define SGTC_MAX_CLUSTERS 4
static struct { Vector center; int num; } sgtc_spawn_cluster[3][SGTC_MAX_CLUSTERS];
static int sgtc_num_spawn_clusters[3];

// the card button that unlocks a trigger_goal (found once per map)
static edict_t *sgtc_goal_btn;
static edict_t *sgtc_goal_btn_goal;  // the trigger_goal it unlocks
static qboolean sgtc_goal_btn_done;
static int sgtc_goal_btn_w = -1;     // the waypoint to press it from (after the coverage)
static float sgtc_goal_btn_log;      // last "card carrier presses" line

// does firing 'target' fire 'name' (through up to 3 multi_managers, relays, buttons...)?
static qboolean SGTC_ChainFires(const char *target, const char *name, int depth)
{
   if (!target || !target[0] || depth > 3)
      return FALSE;
   if (!strcmp(target, name))
      return TRUE;
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_TARGETNAME(pent, target)))
   {
      const char *cls = STRING(pent->v.classname);
      if (!strcmp(cls, "multi_manager"))
      {
         for (int k = 0; k < sgtc_num_mm_kv; k++)
            if (sgtc_mm_kv[k].ent == pent && SGTC_ChainFires(sgtc_mm_kv[k].name, name, depth + 1))
               return TRUE;
      }
      else if (strcmp(cls, "trigger_changetarget") && SGTC_ChainFires(STRING(pent->v.target), name, depth + 1))
         return TRUE;
   }
   return FALSE;
}

// a trigger_goal with a master that a button needing the card enables
// (sg_conquest: "Stolen_btn" -> Stolen_mm -> "Stolen_ms", master of the goal
// around the button in the SGC): with no game_nuke, that button is where
// the card carrier goes
static void SGTC_FindGoalButton(void)
{
   static const char *btn_class[2] = {"func_button", "func_rot_button"};
   for (int gi = 0; gi < sgtc_obj.num_goals; gi++)
   {
      edict_t *pGoal = sgtc_obj.goal[gi];
      const char *master = SGTC_GoalMaster(pGoal);
      if (!master[0])
         continue;
      edict_t *best = NULL;
      int best_card = 0;
      for (int c = 0; c < 2; c++)
      {
         edict_t *pButton = NULL;
         while (!FNullEnt(pButton = FIND_ENTITY_BY_CLASSNAME(pButton, btn_class[c])))
         {
            if (!SGTC_ChainFires(STRING(pButton->v.target), master, 0))
               continue;
            int kv = SGTC_EntKv(pButton), card = kv != -1 ? sgtc_ent_kv[kv].needcard : 0;
            if (!best || card > best_card)
            {
               best = pButton;
               best_card = card;
            }
         }
      }
      Vector gc = SGTC_EntCenter(pGoal);
      if (!best || !best_card)
      {
         UTIL_ConsolePrintf("[sgtc-stat] goal %d at %.0f %.0f %.0f: locked by master \"%s\", %s\n", gi, gc.x, gc.y, gc.z, master,
            best ? "unlocked by a button that needs no card: not handled" : "no button unlocks it");
         continue;
      }
      int kv = SGTC_EntKv(best), team = kv != -1 ? sgtc_ent_kv[kv].team : 0;
      Vector bc = SGTC_EntCenter(best);
      UTIL_ConsolePrintf("[sgtc-stat] goal %d at %.0f %.0f %.0f: locked by master \"%s\", unlocked by %s \"%s\" at %.0f %.0f %.0f (needs the card, %s): the card carrier presses it\n",
         gi, gc.x, gc.y, gc.z, master, STRING(best->v.classname), STRING(best->v.targetname), bc.x, bc.y, bc.z,
         team == 1 ? "Tau'ri only" : team == 2 ? "Goa'uld only" : "anyone");
      sgtc_goal_btn = best;
      sgtc_goal_btn_goal = pGoal;
      return;
   }
}

static void SGTC_ScanObjectives(void)
{
   edict_t *list[SGTC_MAX_OBJ];
   edict_t *old_carrier = sgtc_obj.key_carrier;

   sgtc_obj.num_goals = SGTC_FindAll("trigger_goal", sgtc_obj.goal);
   sgtc_obj.num_captures = SGTC_FindAll("zone_capture", sgtc_obj.capture);
   sgtc_obj.num_bombzones = SGTC_FindAll("zone_bomb_target", sgtc_obj.bombzone);
   sgtc_obj.num_shelters = SGTC_FindAll("zone_shelter", sgtc_obj.shelter);
   sgtc_obj.key = SGTC_FindAll("item_objective", list) ? list[0] : NULL;
   sgtc_obj.nuke = SGTC_FindAll("game_nuke", list) ? list[0] : NULL;

   // SGTC removes the key entity when someone picks it up, and spawns a new one
   // where the carrier dies: the carrier is whoever stood where it vanished
   if (sgtc_obj.key)
   {
      sgtc_obj.key_carrier = NULL;
      sgtc_obj.v_key_last = sgtc_obj.key->v.origin;
      sgtc_obj.key_state = 1;
      // a carried item following its owner, should a map do it that way
      if (SGTC_IsPlayerAlive(sgtc_obj.key->v.owner))
         sgtc_obj.key_carrier = sgtc_obj.key->v.owner;
      else if (SGTC_IsPlayerAlive(sgtc_obj.key->v.aiment))
         sgtc_obj.key_carrier = sgtc_obj.key->v.aiment;
   }
   else if (sgtc_obj.key_state == 1)  // just vanished
   {
      float best = 250.0f;
      sgtc_obj.key_carrier = NULL;
      for (int i = 1; i <= gpGlobals->maxClients; i++)
      {
         edict_t *pPlayer = INDEXENT(i);
         if (!SGTC_IsPlayerAlive(pPlayer))
            continue;
         float d = (pPlayer->v.origin - sgtc_obj.v_key_last).Length();
         if (d < best)
         {
            best = d;
            sgtc_obj.key_carrier = pPlayer;
         }
      }
      sgtc_obj.key_state = sgtc_obj.key_carrier ? 2 : 0;
   }
   else if (sgtc_obj.key_state == 2 && !SGTC_IsPlayerAlive(sgtc_obj.key_carrier))
   {
      sgtc_obj.key_carrier = NULL;  // died: the key shows up again
      sgtc_obj.key_state = 0;
   }
   if (old_carrier != sgtc_obj.key_carrier)
      UTIL_ConsolePrintf("[sgtc-stat] key carrier: %s (key ent %s, effects=%d solid=%d)\n",
         sgtc_obj.key_carrier ? STRING(sgtc_obj.key_carrier->v.netname) : "none",
         sgtc_obj.key ? "present" : "gone", sgtc_obj.key ? sgtc_obj.key->v.effects : -1, sgtc_obj.key ? sgtc_obj.key->v.solid : -1);

   // spawn areas: average of each side's spawn points, and their clusters
   static const char *spawn_class[3] = {NULL, "info_player_torri", "info_player_goauld"};
   static int last_clusters = -1;
   int clusters_sig = 0;
   for (int side = SGTC_TEAM_TAURI; side <= SGTC_TEAM_GOAULD; side++)
   {
      Vector sum(0, 0, 0), pts[64], csum[64];
      int n = 0, num_pts = 0, label[64], count[64];
      edict_t *pent = NULL;
      while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", spawn_class[side])))
      {
         sum = sum + pent->v.origin;
         n++;
         if (num_pts < 64)
         {
            label[num_pts] = num_pts;
            pts[num_pts++] = pent->v.origin;
         }
      }
      sgtc_obj.spawn_center[side] = n ? sum * (1.0f / n) : Vector(0, 0, 0);

      for (int i = 0; i < num_pts; i++)
         for (int j = 0; j < i; j++)
            if (label[i] != label[j] && (pts[i] - pts[j]).Length() < 800.0f)
            {
               int from = label[i], to = label[j];
               for (int k = 0; k < num_pts; k++)
                  if (label[k] == from)
                     label[k] = to;
            }
      for (int l = 0; l < num_pts; l++)
      {
         count[l] = 0;
         csum[l] = Vector(0, 0, 0);
      }
      for (int k = 0; k < num_pts; k++)
      {
         count[label[k]]++;
         csum[label[k]] = csum[label[k]] + pts[k];
      }
      int num = 0;
      while (num < SGTC_MAX_CLUSTERS)
      {
         int best = -1;
         for (int l = 0; l < num_pts; l++)
            if (count[l] > 0 && (best == -1 || count[l] > count[best]))
               best = l;
         if (best == -1)
            break;
         sgtc_spawn_cluster[side][num].center = csum[best] * (1.0f / count[best]);
         sgtc_spawn_cluster[side][num].num = count[best];
         clusters_sig = clusters_sig * 31 + count[best];
         count[best] = 0;
         num++;
      }
      sgtc_num_spawn_clusters[side] = num;
      clusters_sig = clusters_sig * 7 + num;
   }
   if (clusters_sig != last_clusters)
   {
      last_clusters = clusters_sig;
      for (int side = SGTC_TEAM_TAURI; side <= SGTC_TEAM_GOAULD; side++)
      {
         char buf[256] = "";
         for (int k = 0; k < sgtc_num_spawn_clusters[side]; k++)
         {
            const Vector &c = sgtc_spawn_cluster[side][k].center;
            size_t len = strlen(buf);
            safevoid_snprintf(buf + len, sizeof(buf) - len, "%s%d at %.0f %.0f %.0f", k ? ", " : "",
               sgtc_spawn_cluster[side][k].num, c.x, c.y, c.z);
         }
         UTIL_ConsolePrintf("[sgtc-stat] spawn clusters %s: %d (%s)\n", SGTC_TeamName(side), sgtc_num_spawn_clusters[side], buf);
      }
   }

   // who has to play the objective: the Tau'ri bring the bomb or the C4,
   // trigger_goal says which side must reach it
   sgtc_obj.attack_team = 0;
   if (sgtc_obj.key || sgtc_obj.nuke || sgtc_obj.num_bombzones)
      sgtc_obj.attack_team = SGTC_TEAM_TAURI;
   else if (sgtc_obj.num_goals)
      sgtc_obj.attack_team = SGTC_GoalTeam(sgtc_obj.goal[0]);

   // a trigger_goal locked by a master that a card button unlocks
   if (!sgtc_goal_btn_done && sgtc_obj.key && !sgtc_obj.nuke && sgtc_obj.num_goals)
   {
      sgtc_goal_btn_done = TRUE;
      SGTC_FindGoalButton();
   }
   sgtc_obj.goal_button = (!sgtc_obj.nuke && !FNullEnt(sgtc_goal_btn) && !sgtc_goal_btn->free) ? sgtc_goal_btn : NULL;
   // pressed: a func_button shows its "on" texture (frame 1) until it comes back
   // (for the log; the carrier presses it whatever it shows)
   qboolean unlocked = sgtc_obj.goal_button && sgtc_obj.goal_button->v.frame > 0;
   if (unlocked != sgtc_obj.goal_unlocked && sgtc_obj.goal_button)
      UTIL_ConsolePrintf("[sgtc-stat] event GOAL_%s t=%.1f (button \"%s\" shows %s, card carrier %s)\n", unlocked ? "UNLOCKED" : "LOCKED",
         gpGlobals->time - sgtc_state.f_round_start, STRING(sgtc_obj.goal_button->v.targetname), unlocked ? "pressed" : "released",
         sgtc_obj.key_carrier ? STRING(sgtc_obj.key_carrier->v.netname) : "none");
   sgtc_obj.goal_unlocked = unlocked;

   SGTC_ScanGates();

   // report what was found whenever it changes (new map, round restart)
   static int last_summary = -1;
   int summary = sgtc_obj.num_goals | sgtc_obj.num_captures << 4 | sgtc_obj.num_bombzones << 8 |
                 sgtc_obj.num_shelters << 12 | (sgtc_obj.key != NULL) << 16 | (sgtc_obj.nuke != NULL) << 17 |
                 sgtc_obj.attack_team << 18;
   if (summary != last_summary)
   {
      last_summary = summary;
      UTIL_ConsolePrintf("[sgtc-stat] objectives: goals=%d captures=%d c4zones=%d shelters=%d key=%d nuke=%d attackers=%s goal_teams_seen=%d\n",
         sgtc_obj.num_goals, sgtc_obj.num_captures, sgtc_obj.num_bombzones, sgtc_obj.num_shelters,
         sgtc_obj.key != NULL, sgtc_obj.nuke != NULL, SGTC_TeamName(sgtc_obj.attack_team), sgtc_num_goal_teams);
      for (int i = 0; i < sgtc_obj.num_goals; i++)
      {
         Vector c = SGTC_EntCenter(sgtc_obj.goal[i]);
         UTIL_ConsolePrintf("[sgtc-stat]   goal %d at %.0f %.0f %.0f solid=%d team=%d\n", i, c.x, c.y, c.z,
            sgtc_obj.goal[i]->v.solid, SGTC_GoalTeam(sgtc_obj.goal[i]));
      }
   }
}

//
// SGTC 2.0's PlayerKilled (hl.dll+0xc2b68, game rules vtable slot 0x80)
// takes the killer as a player whenever its pev->team differs from the
// victim's: killer->[0x920] += 1 (kill points) and, with a skull pending,
// killer->[0x490] = a time. When the killer isn't a player (trigger_hurt of a
// pit or of a gate's kawoosh room, the world, a projectile) those offsets are
// past the end of its object: they land in whatever the heap put next, very
// often the start of another entity, whose vtable pointer then ends 1 byte
// off. SGTC's round restart (hl.dll+0xbbb53) calls a virtual on every
// trigger_teleport, func_illusionary, func_wall... and jumps into garbage:
// 1 server in 4 per 10 minutes on sg_crypt and sg_stargate. A player killed
// by a non-player also crashed in PlayerKilled itself, reading 0x920 past
// an object at the end of the heap.
//
// Fix: the rules object gets a copy of its vtable whose PlayerKilled slot
// passes a player killer: the owner of a non-player killer when that is a
// player (a projectile), else the victim itself (killed by the world, which
// SGTC already scores as a suicide).
//
#ifdef _WIN32
#define SGTC_RVA_GAMERULES     0x13ba28   // g_pGameRules (StartFrame at hl.dll+0x289c3)
#define SGTC_RVA_PLAYERKILLED  0xc2b68
#define SGTC_RVA_PLAYERKILLED_CP 0xb839f  // the capture-the-point rules' one (same bug)
#define SGTC_PLAYERKILLED_SLOT (0x80 / 4)
#define SGTC_RULES_VTBL_SLOTS  128

typedef void (__thiscall *sgtc_playerkilled_t)(void *rules, void *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
static sgtc_playerkilled_t sgtc_orig_playerkilled;
static void *sgtc_rules_vtbl[1 + SGTC_RULES_VTBL_SLOTS];  // [0]: the RTTI locator before the vtable
static unsigned int sgtc_hl_lo, sgtc_hl_hi;

static void __thiscall SGTC_PlayerKilled(void *rules, void *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
   entvars_t *pevVictim = pVictim ? *(entvars_t **)((char *)pVictim + 4) : NULL;  // CBaseEntity::pev

   if (pKiller && pevVictim && pKiller != pevVictim && !(pKiller->flags & FL_CLIENT))
   {
      edict_t *pOwner = pKiller->owner;
      entvars_t *pevNew = (!FNullEnt(pOwner) && (pOwner->v.flags & FL_CLIENT)) ? &pOwner->v : pevVictim;
      UTIL_ConsolePrintf("[sgtc-stat] PlayerKilled: %s killed by %s, credited to %s (victim at %.0f %.0f %.0f; killer \"%s\" origin %.0f %.0f %.0f vel %.0f %.0f %.0f)\n",
         STRING(pevVictim->netname), STRING(pKiller->classname), pevNew == pevVictim ? "the victim" : STRING(pevNew->netname),
         pevVictim->origin.x, pevVictim->origin.y, pevVictim->origin.z, STRING(pKiller->targetname),
         pKiller->origin.x, pKiller->origin.y, pKiller->origin.z, pKiller->velocity.x, pKiller->velocity.y, pKiller->velocity.z);
      pKiller = pevNew;
   }
   sgtc_orig_playerkilled(rules, pVictim, pKiller, pInflictor);
}

static void SGTC_HookRules(void)
{
   if (!sgtc_hl_hi)
   {
      unsigned char *base = (unsigned char *)GetModuleHandleA("hl.dll");
      if (!base)
         return;
      IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
      sgtc_hl_lo = (unsigned int)base;
      sgtc_hl_hi = sgtc_hl_lo + nt->OptionalHeader.SizeOfImage;
   }

   void ***pRules = *(void ****)(sgtc_hl_lo + SGTC_RVA_GAMERULES);
   if (!pRules || *pRules == &sgtc_rules_vtbl[1])
      return;

   // only this hl.dll (SGTC 2.0): PlayerKilled where it's expected
   void **vtbl = *pRules;
   if ((unsigned int)vtbl[SGTC_PLAYERKILLED_SLOT] != sgtc_hl_lo + SGTC_RVA_PLAYERKILLED &&
       (unsigned int)vtbl[SGTC_PLAYERKILLED_SLOT] != sgtc_hl_lo + SGTC_RVA_PLAYERKILLED_CP)
   {
      static qboolean warned = FALSE;
      if (!warned)
         UTIL_ConsolePrintf("[sgtc] unknown hl.dll: PlayerKilled crash fix not installed\n");
      warned = TRUE;
      return;
   }
   memcpy(sgtc_rules_vtbl, vtbl - 1, sizeof(sgtc_rules_vtbl));
   sgtc_orig_playerkilled = (sgtc_playerkilled_t)vtbl[SGTC_PLAYERKILLED_SLOT];
   sgtc_rules_vtbl[1 + SGTC_PLAYERKILLED_SLOT] = (void *)SGTC_PlayerKilled;
   *pRules = &sgtc_rules_vtbl[1];
   UTIL_ConsolePrintf("[sgtc-stat] game rules at %p: PlayerKilled crash fix installed (%s rules)\n", (void *)pRules,
      (unsigned int)sgtc_orig_playerkilled == sgtc_hl_lo + SGTC_RVA_PLAYERKILLED ? "deathmatch" : "capture");
}
#elif defined(__linux__)
//
// Linux, stc_i386.so (GCC 4.0, Itanium C++ ABI, not stripped): the same bug
// in CStargateTeamDM::PlayerKilled and CStargateTeamCP::PlayerKilled, at
// killer+0x934 / killer+0x4a4 (the Windows 0x920/0x490 shifted by 0x14).
// Under glibc those writes land in malloc chunk headers or the next entity.
// Everything is found by symbol: the game .so is the module that holds the
// game's StartFrame; g_pGameRules and both PlayerKilled are exported. The
// vtable has 2 words before its address point (offset-to-top, typeinfo),
// PlayerKilled is slot 32 (+0x80) as on Windows, and member functions are
// cdecl with 'this' first. CBaseEntity::pev is at +4 as on Windows.
// Keep SGTC_PlayerKilled's body in sync with the Windows one above.
//
#include <dlfcn.h>
#include <link.h>

#define SGTC_PLAYERKILLED_SLOT (0x80 / 4)
#define SGTC_RULES_VTBL_SLOTS  128   // CStargateTeamDM has 73, CStargateTeamCP 74
#define SGTC_VTBL_HDR          2     // offset-to-top, typeinfo

typedef void (*sgtc_playerkilled_t)(void *rules, void *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
static sgtc_playerkilled_t sgtc_orig_playerkilled;
static void *sgtc_rules_vtbl[SGTC_VTBL_HDR + SGTC_RULES_VTBL_SLOTS];
static unsigned int sgtc_hl_lo, sgtc_hl_hi;  // the game .so's mapped range (also for SGTC_GuardVtables)
static void ****sgtc_pp_gamerules;           // &g_pGameRules
static void *sgtc_pk_dm, *sgtc_pk_cp;        // CStargateTeamDM / CStargateTeamCP::PlayerKilled

static FORCE_STACK_ALIGN void SGTC_PlayerKilled(void *rules, void *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
   entvars_t *pevVictim = pVictim ? *(entvars_t **)((char *)pVictim + 4) : NULL;  // CBaseEntity::pev

   if (pKiller && pevVictim && pKiller != pevVictim && !(pKiller->flags & FL_CLIENT))
   {
      edict_t *pOwner = pKiller->owner;
      entvars_t *pevNew = (!FNullEnt(pOwner) && (pOwner->v.flags & FL_CLIENT)) ? &pOwner->v : pevVictim;
      UTIL_ConsolePrintf("[sgtc-stat] PlayerKilled: %s killed by %s, credited to %s (victim at %.0f %.0f %.0f; killer \"%s\" origin %.0f %.0f %.0f vel %.0f %.0f %.0f)\n",
         STRING(pevVictim->netname), STRING(pKiller->classname), pevNew == pevVictim ? "the victim" : STRING(pevNew->netname),
         pevVictim->origin.x, pevVictim->origin.y, pevVictim->origin.z, STRING(pKiller->targetname),
         pKiller->origin.x, pKiller->origin.y, pKiller->origin.z, pKiller->velocity.x, pKiller->velocity.y, pKiller->velocity.z);
      pKiller = pevNew;
   }
   sgtc_orig_playerkilled(rules, pVictim, pKiller, pInflictor);
}

// dl_iterate_phdr callback: the PT_LOAD range of the module holding f[0]
static int SGTC_ModuleRange(struct dl_phdr_info *info, size_t size, void *data)
{
   unsigned int *f = (unsigned int *)data;  // in: address; out: lo, hi
   unsigned int lo = 0xffffffffu, hi = 0;

   for (int i = 0; i < info->dlpi_phnum; i++)
   {
      if (info->dlpi_phdr[i].p_type != PT_LOAD)
         continue;
      unsigned int a = (unsigned int)(info->dlpi_addr + info->dlpi_phdr[i].p_vaddr);
      if (a < lo)
         lo = a;
      if (a + info->dlpi_phdr[i].p_memsz > hi)
         hi = a + info->dlpi_phdr[i].p_memsz;
   }
   if (f[0] < lo || f[0] >= hi)
      return 0;
   f[1] = lo;
   f[2] = hi;
   return 1;
}

static void SGTC_FindGameSo(void)
{
   void *probe = (gpGamedllFuncs && gpGamedllFuncs->dllapi_table) ? (void *)gpGamedllFuncs->dllapi_table->pfnStartFrame : NULL;
   Dl_info di;

   if (!probe || !dladdr(probe, &di) || !di.dli_fname)
   {
      UTIL_ConsolePrintf("[sgtc] game dll not found: PlayerKilled crash fix not installed\n");
      return;
   }

   unsigned int range[3] = {(unsigned int)probe, 0, 0};
   void *h = dlopen(di.dli_fname, RTLD_NOW | RTLD_NOLOAD);  // already loaded by metamod: no load, just a handle
   if (!dl_iterate_phdr(SGTC_ModuleRange, range) || !h)
   {
      UTIL_ConsolePrintf("[sgtc] game dll %s: no module range or handle: PlayerKilled crash fix not installed\n", di.dli_fname);
      if (h)
         dlclose(h);
      return;
   }
   sgtc_pp_gamerules = (void ****)dlsym(h, "g_pGameRules");
   sgtc_pk_dm = dlsym(h, "_ZN15CStargateTeamDM12PlayerKilledEP11CBasePlayerP9entvars_sS3_");
   sgtc_pk_cp = dlsym(h, "_ZN15CStargateTeamCP12PlayerKilledEP11CBasePlayerP9entvars_sS3_");
   dlclose(h);  // metamod keeps it loaded

   sgtc_hl_lo = range[1];
   sgtc_hl_hi = range[2];
   UTIL_ConsolePrintf("[sgtc] game dll %s at %x-%x: g_pGameRules %p, PlayerKilled DM %p CP %p\n", di.dli_fname,
      sgtc_hl_lo, sgtc_hl_hi, (void *)sgtc_pp_gamerules, sgtc_pk_dm, sgtc_pk_cp);
   if (!sgtc_pp_gamerules || (!sgtc_pk_dm && !sgtc_pk_cp))
   {
      UTIL_ConsolePrintf("[sgtc] unknown game dll (not stc_i386.so of SGTC 2.0): PlayerKilled crash fix not installed\n");
      sgtc_pp_gamerules = NULL;
   }
}

static void SGTC_HookRules(void)
{
   static qboolean looked_up = FALSE;
   if (!looked_up)
   {
      looked_up = TRUE;
      SGTC_FindGameSo();
   }
   if (!sgtc_pp_gamerules)
      return;

   void ***pRules = *sgtc_pp_gamerules;
   if (!pRules || *pRules == &sgtc_rules_vtbl[SGTC_VTBL_HDR])
      return;

   // only CStargateTeamDM / CStargateTeamCP (sg_, sgi_ and ctp_ maps): the
   // other rules don't have the bug
   void **vtbl = *pRules;
   void *pk = vtbl[SGTC_PLAYERKILLED_SLOT];
   if ((pk != sgtc_pk_dm && pk != sgtc_pk_cp) || !pk ||
       (unsigned int)(vtbl - SGTC_VTBL_HDR) < sgtc_hl_lo || (unsigned int)(vtbl + SGTC_RULES_VTBL_SLOTS) > sgtc_hl_hi)
   {
      static void **warned_vtbl = NULL;
      if (warned_vtbl != vtbl)
      {
         Dl_info di;
         UTIL_ConsolePrintf("[sgtc] game rules PlayerKilled is %s: PlayerKilled crash fix not installed\n",
            (pk && dladdr(pk, &di) && di.dli_sname) ? di.dli_sname : "unknown");
      }
      warned_vtbl = vtbl;
      return;
   }
   memcpy(sgtc_rules_vtbl, vtbl - SGTC_VTBL_HDR, sizeof(sgtc_rules_vtbl));
   sgtc_orig_playerkilled = (sgtc_playerkilled_t)pk;
   sgtc_rules_vtbl[SGTC_VTBL_HDR + SGTC_PLAYERKILLED_SLOT] = (void *)SGTC_PlayerKilled;
   *pRules = &sgtc_rules_vtbl[SGTC_VTBL_HDR];
   UTIL_ConsolePrintf("[sgtc-stat] game rules at %p: PlayerKilled crash fix installed (%s rules)\n", (void *)pRules,
      pk == sgtc_pk_dm ? "deathmatch" : "capture");
}
#else
static void SGTC_HookRules(void) {}
#endif

//
// Safety net for the above: vtables are 4-byte aligned, so put back any
// vtable pointer into hl.dll with its low bits set, and log it. 'where':
// "frame" (entity thinks since the last frame) or "bots" (this frame's bot
// moves).
//
void SGTC_GuardVtables(const char *where)
{
#if defined(_WIN32) || defined(__linux__)
   if (!sgtc_hl_hi)
      return;

   edict_t *first = INDEXENT(0);
   if (!first)
      return;
   for (int i = 1; i < gpGlobals->maxEntities; i++)
   {
      edict_t *pent = first + i;
      if (pent->free || !pent->pvPrivateData)
         continue;
      unsigned int *vptr = (unsigned int *)pent->pvPrivateData;
      unsigned int v = *vptr;
      if (!(v & 3) || v < sgtc_hl_lo || v >= sgtc_hl_hi)
         continue;
      *vptr = v & ~3u;
      UTIL_ConsolePrintf("[sgtc-stat] vtable fix (%s): #%d %s \"%s\" vtable hl.dll+%x had +%u, t=%.2f\n", where, i,
         STRING(pent->v.classname), STRING(pent->v.targetname), (v & ~3u) - sgtc_hl_lo, v & 3u, gpGlobals->time);
   }
#endif
}

//
// SGTC's round restart (hl.dll+0xbbb53; +0xb1c39 for the capture rules)
// UTIL_Removes every weapon_* entity by classname, the carried ones too, then
// strips and re-equips only the players whose team name is valid
// (IsValidTeam(TeamID()), team list "Torri;Goa'uld"). Anyone else keeps
// m_rgpPlayerItems pointing at weapons the engine frees in this frame's
// entity loop, and the next PlayerPreThink -> UpdateClientData (fault at
// hl.dll+0x8a9d3, called from hl.dll+0x85607) calls a virtual on freed
// memory, or jumps into garbage.
//
// Fix: before the engine frees them (StartFrame is called before its entity
// loop), unlink the weapons marked for removal from every inventory.
//
#ifdef _WIN32
#define SGTC_OFS_PLAYER_ITEMS   0xca4  // CBasePlayer::m_rgpPlayerItems[7]
#define SGTC_OFS_ACTIVE_ITEM    0xcc0  // m_pActiveItem
#define SGTC_OFS_CLIENT_ITEM    0xcc4  // m_pClientActiveItem
#define SGTC_OFS_LAST_ITEM      0xcc8  // m_pLastItem
#define SGTC_OFS_ITEM_PLAYER    0x1a8  // CBasePlayerItem::m_pPlayer
#define SGTC_OFS_ITEM_NEXT      0x1ac  // m_pNext
#define SGTC_OFS_ITEM_ID        0x1b0  // m_iId
#define SGTC_RVA_UCD_ITEMLOOP   0x8a9a9  // UpdateClientData: cmp [eax+edx*4+0xca4], 0
#elif defined(__linux__)
// stc_i386.so has the same restart (CStargateTeamDM::NewRound at 0x184f42,
// CStargateTeamCP::NewRound at 0x177b34). GCC's pointers to member functions
// are 8 bytes (4 in CBaseEntity, 1 in CBaseToggle), so CBasePlayer's fields
// are the Windows ones + 0x14 and CBasePlayerItem's + 0x10. Read from the code:
#define SGTC_OFS_PLAYER_ITEMS   0xcb8  // UpdateClientData's item loop, AddPlayerItem, RemovePlayerItem
#define SGTC_OFS_ACTIVE_ITEM    0xcd4  // RemovePlayerItem, SelectItem
#define SGTC_OFS_CLIENT_ITEM    0xcd8  // UpdateClientData: m_pClientActiveItem = m_pActiveItem
#define SGTC_OFS_LAST_ITEM      0xcdc  // RemovePlayerItem, SelectItem, SelectLastItem
#define SGTC_OFS_ITEM_PLAYER    0x1b8  // CBasePlayerItem::AddToPlayer
#define SGTC_OFS_ITEM_NEXT      0x1bc  // AddPlayerItem, RemovePlayerItem, CBasePlayerWeapon::UpdateClientData
#define SGTC_OFS_ITEM_ID        0x1c0  // CBasePlayerWeapon::AddToPlayer: pev->weapons |= 1 << m_iId
#define SGTC_OFS_UCD            0x146a92 // CBasePlayer::UpdateClientData
#define SGTC_OFS_UCD_ITEMLOOP   0x14743a // its loop: if (m_rgpPlayerItems[i]) ->UpdateClientData(this), i = 0..6
#define SGTC_OFS_CORPSE_LINK    0x1a0    // corpse_control: the entity CStargateTeamDM::NewRound removes with it
#define SGTC_OFS_NEWROUND_CORPSES 0x185493 // NewRound's corpse loop: if (corpse->link) UTIL_Remove(corpse->link)
#endif

//
// The round restart (CStargateTeamDM::NewRound) removes every corpse_control
// and, first, the entity it links to (+0x1a0: its flames, it seems) without
// checking that one still exists. Freed, glibc reuses the block and
// UTIL_Remove -> UpdateOnRemove dereferences garbage: 19 of the 20 crashes of
// the public server's first night. Windows' heap mostly leaves freed blocks
// alone. Called every frame before the game's StartFrame: a link that no
// longer points to a live entity is cleared.
//
void SGTC_CheckCorpses(void)
{
#ifdef __linux__
   static int known = -1;
   if (known == -1)
   {
      if (!sgtc_hl_hi)
         return;
      static const unsigned char sig[] = {
         0x8b, 0x85, 0x5c, 0xff, 0xff, 0xff, 0x8b, 0x80, 0xa0, 0x01, 0x00, 0x00, 0x85, 0xc0, 0x74, 0x18,
         0x8b, 0x85, 0x5c, 0xff, 0xff, 0xff, 0x8b, 0x80, 0xa0, 0x01, 0x00, 0x00};
      known = sgtc_hl_lo + SGTC_OFS_NEWROUND_CORPSES + sizeof(sig) <= sgtc_hl_hi &&
              !memcmp((void *)(sgtc_hl_lo + SGTC_OFS_NEWROUND_CORPSES), sig, sizeof(sig));
      UTIL_ConsolePrintf("[sgtc] round-restart corpse fix %s\n", known ? "installed" : "not installed (unknown game dll)");
   }
   if (!known)
      return;

   edict_t *base = INDEXENT(0);
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "corpse_control")))
   {
      if (pent->free || !pent->pvPrivateData)
         continue;
      void **link = (void **)((char *)pent->pvPrivateData + SGTC_OFS_CORPSE_LINK);
      if (!*link)
         continue;
      qboolean live = FALSE;
      for (int i = 0; i < gpGlobals->maxEntities && !live; i++)
         live = !base[i].free && base[i].pvPrivateData == *link;
      if (!live)
      {
         UTIL_ConsolePrintf("[sgtc-stat] corpse_control %d: its linked entity is gone, link cleared (t=%.1f)\n",
            ENTINDEX(pent), gpGlobals->time);
         *link = NULL;
      }
   }
#endif
}

// StargateTC's game rules Think UTIL_Remove()s a living player with no team
// who isn't an observer, after moving it to 4096 4096 4096: a bot between its
// connection and its "classe" command (seen on Linux with gdb). The engine
// would then free the client edict and leave a ghost (below). Keep the player:
// it waits there until it joins a team, which respawns it. Called right after
// the game's StartFrame, before the engine runs the player.
void SGTC_KeepRemovedPlayers(void)
{
   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);
      if (!pPlayer || pPlayer->free || !pPlayer->pvPrivateData || !(pPlayer->v.flags & FL_KILLME))
         continue;
      if (STRING(pPlayer->v.netname)[0] == 0)
         continue;  // a slot left by a disconnected player: nothing to keep
      pPlayer->v.flags &= ~FL_KILLME;
      UTIL_ConsolePrintf("[sgtc-stat] game removed player %s (no team yet): kept\n", STRING(pPlayer->v.netname));
   }
}

// A bot left without a player object (the game freed it, see above) is driven
// by nothing, and the game crashes when it disconnects at the next
// changelevel: kick it, jk_botti adds a new one.
static void SGTC_KickGhostBots(void)
{
   static float kick_time[32];

   for (int i = 0; i < 32; i++)
   {
      bot_t &pBot = bots[i];
      if (!pBot.is_used || !pBot.pEdict || (!pBot.pEdict->free && pBot.pEdict->pvPrivateData))
         continue;
      if (kick_time[i] > gpGlobals->time && kick_time[i] < gpGlobals->time + 10.0f)
         continue;  // kicked already, waiting for the engine
      kick_time[i] = gpGlobals->time + 5.0f;

      int userid = GETPLAYERUSERID(pBot.pEdict);
      UTIL_ConsolePrintf("[sgtc] bot %s has no player object (free=%d): kicked\n",
         STRING(pBot.pEdict->v.netname), pBot.pEdict->free);
      if (userid > 0)
      {
         char cmd[32];
         safevoid_snprintf(cmd, sizeof(cmd), "kick # %d\n", userid);
         SERVER_COMMAND(cmd);
      }
   }
}


#if defined(_WIN32) || defined(__linux__)
// The offsets above hold for this game dll (checked once).
static int SGTC_ItemsKnown(void)
{
   static int known = -1;
   if (known == -1)
   {
      if (!sgtc_hl_hi)
         return 0;
#ifdef _WIN32
      static const unsigned char sig[] = {0x83, 0xbc, 0x90, 0xa4, 0x0c, 0x00, 0x00, 0x00};
      known = !memcmp((void *)(sgtc_hl_lo + SGTC_RVA_UCD_ITEMLOOP), sig, sizeof(sig));
      if (!known)
         UTIL_ConsolePrintf("[sgtc] unknown hl.dll: removed-weapon crash fix not installed\n");
#else
      // only stc_i386.so of SGTC 2.0: the symbol first (dladdr doesn't read the
      // address, so this is safe whatever .so was found), then the item loop
      // and 'm_pClientActiveItem = m_pActiveItem' byte for byte
      static const unsigned char sig[] = {
         0x8b, 0x45, 0xf0, 0x8b, 0x55, 0x08, 0x8b, 0x84, 0x82, 0xb8, 0x0c, 0x00, 0x00, 0x85, 0xc0, 0x74,
         0x2f, 0x8b, 0x55, 0xf0, 0x8b, 0x45, 0x08, 0x8b, 0x84, 0x90, 0xb8, 0x0c, 0x00, 0x00, 0x8b, 0x00,
         0x05, 0x3c, 0x01, 0x00, 0x00, 0x8b, 0x08, 0x8b, 0x55, 0xf0, 0x8b, 0x45, 0x08, 0x8b, 0x84, 0x90,
         0xb8, 0x0c, 0x00, 0x00, 0x83, 0xec, 0x08, 0xff, 0x75, 0x08, 0x50, 0xff, 0xd1, 0x83, 0xc4, 0x10,
         0x8d, 0x45, 0xf0, 0xff, 0x00, 0x83, 0x7d, 0xf0, 0x06, 0x7e, 0xb5, 0x8b, 0x45, 0x08, 0x8b, 0x90,
         0xd4, 0x0c, 0x00, 0x00, 0x8b, 0x45, 0x08, 0x89, 0x90, 0xd8, 0x0c, 0x00, 0x00};
      Dl_info di;
      known = sgtc_hl_lo + SGTC_OFS_UCD_ITEMLOOP + sizeof(sig) <= sgtc_hl_hi &&
         dladdr((void *)(sgtc_hl_lo + SGTC_OFS_UCD_ITEMLOOP), &di) && di.dli_sname &&
         !strcmp(di.dli_sname, "_ZN11CBasePlayer16UpdateClientDataEv") &&
         (unsigned int)di.dli_saddr == sgtc_hl_lo + SGTC_OFS_UCD &&
         !memcmp((void *)(sgtc_hl_lo + SGTC_OFS_UCD_ITEMLOOP), sig, sizeof(sig));
      if (!known)
         UTIL_ConsolePrintf("[sgtc] unknown game dll (not stc_i386.so of SGTC 2.0): removed-weapon crash fix not installed\n");
      else
         UTIL_ConsolePrintf("[sgtc] removed-weapon crash fix installed (UpdateClientData at %x)\n", sgtc_hl_lo + SGTC_OFS_UCD);
#endif
   }
   return known;
}

// Is item, a CBasePlayerItem * found in an inventory, still the private data
// of a live entity? Only the item's pev slot is read, never through it: in a
// freed block it holds the allocator's words (glibc: a mangled list pointer
// and a random key), so it has to point into the edict array first.
static qboolean SGTC_ItemLive(char *item)
{
   edict_t *base = INDEXENT(0);
   char *pev = *(char **)(item + 4);  // CBaseEntity::pev
   char *first = (char *)&base[0].v;
   if (pev < first || pev >= (char *)&base[gpGlobals->maxEntities].v ||
       (unsigned int)(pev - first) % sizeof(edict_t))
      return FALSE;
   edict_t *pent = base + (pev - first) / sizeof(edict_t);
   return !pent->free && pent->pvPrivateData == item;
}

// Walks every inventory. Unlinks the weapon being freed (freeing), or with
// freeing NULL the ones marked for removal. A list is cut at a weapon that is
// already gone, since its m_pNext went with it.
static void SGTC_CheckInventories(const char *where, char *freeing)
{
   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);
      if (!pPlayer || pPlayer->free || !pPlayer->pvPrivateData)
         continue;
      char *player = (char *)pPlayer->pvPrivateData;

      for (int slot = 0; slot < 7; slot++)
      {
         char **link = (char **)(player + SGTC_OFS_PLAYER_ITEMS + 4 * slot);
         for (int n = 0; *link && n < 32; n++)
         {
            char *item = *link;
            if (!SGTC_ItemLive(item))
            {
               UTIL_ConsolePrintf("[sgtc-stat] freed weapon still carried (%s): %s, slot %d, list cut (t=%.2f)\n", where,
                  STRING(pPlayer->v.netname), slot, gpGlobals->time);
               *link = NULL;
               if (*(char **)(player + SGTC_OFS_ACTIVE_ITEM) == item)
               {
                  *(char **)(player + SGTC_OFS_ACTIVE_ITEM) = NULL;
                  pPlayer->v.viewmodel = 0;
                  pPlayer->v.weaponmodel = 0;
               }
               if (*(char **)(player + SGTC_OFS_CLIENT_ITEM) == item)
                  *(char **)(player + SGTC_OFS_CLIENT_ITEM) = NULL;
               if (*(char **)(player + SGTC_OFS_LAST_ITEM) == item)
                  *(char **)(player + SGTC_OFS_LAST_ITEM) = NULL;
               break;
            }
            entvars_t *pev = *(entvars_t **)(item + 4);
            if (freeing ? item != freeing : !(pev->flags & FL_KILLME))
            {
               link = (char **)(item + SGTC_OFS_ITEM_NEXT);
               continue;
            }

            UTIL_ConsolePrintf("[sgtc-stat] removed weapon still carried (%s): %s had %s, unlinked (t=%.2f)\n", where,
               STRING(pPlayer->v.netname), STRING(pev->classname), gpGlobals->time);
            *link = *(char **)(item + SGTC_OFS_ITEM_NEXT);
            *(char **)(item + SGTC_OFS_ITEM_NEXT) = NULL;
            *(void **)(item + SGTC_OFS_ITEM_PLAYER) = NULL;
            int id = *(int *)(item + SGTC_OFS_ITEM_ID);
            if (id > 0 && id < 32)
               pPlayer->v.weapons &= ~(1u << id);
            if (*(char **)(player + SGTC_OFS_ACTIVE_ITEM) == item)
            {
               *(char **)(player + SGTC_OFS_ACTIVE_ITEM) = NULL;
               pPlayer->v.viewmodel = 0;
               pPlayer->v.weaponmodel = 0;
            }
            if (*(char **)(player + SGTC_OFS_CLIENT_ITEM) == item)
               *(char **)(player + SGTC_OFS_CLIENT_ITEM) = NULL;
            if (*(char **)(player + SGTC_OFS_LAST_ITEM) == item)
               *(char **)(player + SGTC_OFS_LAST_ITEM) = NULL;
         }
      }
   }
}
#endif

void SGTC_DropRemovedWeapons(const char *where)
{
#if defined(_WIN32) || defined(__linux__)
   if (SGTC_ItemsKnown())
      SGTC_CheckInventories(where, NULL);
#endif
}

//
// The engine is about to free an entity's private data. A carried weapon
// removed during the engine's entity loop (UTIL_Remove from something that
// ran earlier in the same loop: a death on a CTP map) is freed in that loop,
// before StartFrame can unlink it, and the next walk through the inventory
// reads freed memory. glibc reuses the block at once: the public server
// crashed in the check above on ctp_conquest (2026-10-23). Unlink it here,
// while it is still valid.
//
void SGTC_OnFreeEntPrivateData(edict_t *pEnt)
{
#if defined(_WIN32) || defined(__linux__)
   static qboolean said = FALSE;
   if (!pEnt || !pEnt->pvPrivateData || !SGTC_ItemsKnown())
      return;
   if (!said)
   {
      said = TRUE;
      UTIL_ConsolePrintf("[sgtc] freed-weapon fix active (OnFreeEntPrivateData)\n");
   }
   SGTC_CheckInventories("free", (char *)pEnt->pvPrivateData);
#endif
}

// transits: always-on teleports and long lifts (defined before SGTC_SetGoal)
static void SGTC_BuildTransits(void);
static void SGTC_WatchTransits(void);
static void SGTC_ResetTransits(void);
static void SGTC_TransitHazards(void);
static qboolean SGTC_CrossesTeleport(const Vector &a, const Vector &b);
static qboolean SGTC_InTeleport(const Vector &p);
static qboolean SGTC_BotTransitThink(bot_t &pBot);

// the last 2.4 s of each bot (every 0.2 s), logged when it dies below where
// it was: where it left the floor, how fast, toward which waypoint
#define SGTC_TRAIL 12
typedef struct
{
   float t;
   Vector o;
   int speed, wpt, prev;
   char flags[8];
} sgtc_trail_t;
static sgtc_trail_t sgtc_trail[32][SGTC_TRAIL];
static int sgtc_trail_head[32], sgtc_trail_count[32], sgtc_alive_team[32];
static float sgtc_trail_next[32];

static void SGTC_TrailSample(int i, bot_t &pBot)
{
   if (sgtc_trail_next[i] > gpGlobals->time && sgtc_trail_next[i] < gpGlobals->time + 1.0f)
      return;
   sgtc_trail_next[i] = gpGlobals->time + 0.2f;

   edict_t *pEdict = pBot.pEdict;
   sgtc_trail_t &s = sgtc_trail[i][sgtc_trail_head[i]];
   s.t = gpGlobals->time;
   s.o = pEdict->v.origin;
   s.speed = (int)Vector(pEdict->v.velocity.x, pEdict->v.velocity.y, 0).Length();
   s.wpt = pBot.curr_waypoint_index;
   s.prev = pBot.prev_waypoint_index[0];
   int n = 0;
   if (pEdict->v.flags & FL_ONGROUND)
      s.flags[n++] = 'G';
   if (pEdict->v.movetype == MOVETYPE_FLY)
      s.flags[n++] = 'L';
   if (pBot.pBotEnemy != NULL)
      s.flags[n++] = 'E';
   if (pBot.b_narrow_path)
      s.flags[n++] = 'N';
   if (pBot.f_sgtc_move_until > gpGlobals->time)
      s.flags[n++] = 'M';
   if (pEdict->v.button & IN_JUMP)
      s.flags[n++] = 'J';
   if (pEdict->v.flags & FL_DUCKING)
      s.flags[n++] = 'D';
   s.flags[n] = 0;
   sgtc_trail_head[i] = (sgtc_trail_head[i] + 1) % SGTC_TRAIL;
   if (sgtc_trail_count[i] < SGTC_TRAIL)
      sgtc_trail_count[i]++;
}

static void SGTC_TrailLog(int i, bot_t &pBot)
{
   int n = sgtc_trail_count[i];
   if (n == 0)
      return;
   float top = pBot.v_sgtc_last_pos.z;
   for (int k = 0; k < n; k++)
   {
      const sgtc_trail_t &s = sgtc_trail[i][(sgtc_trail_head[i] - n + k + SGTC_TRAIL) % SGTC_TRAIL];
      if (s.o.z > top)
         top = s.o.z;
   }
   if (top - pBot.v_sgtc_last_pos.z < 80.0f)
      return;  // not a fall

   char line[400], item[96];
   line[0] = 0;
   for (int k = 0; k < n; k++)
   {
      const sgtc_trail_t &s = sgtc_trail[i][(sgtc_trail_head[i] - n + k + SGTC_TRAIL) % SGTC_TRAIL];
      safevoid_snprintf(item, sizeof(item), " [%.1f %.0f,%.0f,%.0f v%d w%d<%d %s]",
         s.t - gpGlobals->time, s.o.x, s.o.y, s.o.z, s.speed, s.wpt, s.prev, s.flags);
      if (strlen(line) + strlen(item) < sizeof(line))
         strcat(line, item);
      if (k % 4 == 3 || k == n - 1)
      {
         UTIL_ConsolePrintf("[sgtc-stat] fall %s:%s\n", STRING(pBot.pEdict->v.netname), line);
         line[0] = 0;
      }
   }
}

void SGTC_StartFrame(void)
{
   SGTC_WatchRings();
   SGTC_WatchTransports();
   SGTC_RingDebug();
   SGTC_WatchC4Charges();
   SGTC_HookRules();
   SGTC_GuardVtables("frame");
   SGTC_DropRemovedWeapons("frame");

   if (sgtc_obj.f_next_scan > gpGlobals->time + 2.0f)  // new map: time went back
      sgtc_obj.f_next_scan = 0;

   // where bots die (pits, kawoosh...): last position while alive
   for (int i = 0; i < 32; i++)
   {
      bot_t &pBot = bots[i];
      if (!pBot.is_used || FNullEnt(pBot.pEdict))
         continue;
      qboolean alive = IsAlive(pBot.pEdict);
      if (alive)
      {
         SGTC_RingSeparate(pBot.pEdict);
         SGTC_RingEscape(pBot);
         SGTC_TransportSafety(pBot);
         pBot.v_sgtc_last_pos = pBot.pEdict->v.origin;
         pBot.sgtc_last_wpt = pBot.curr_waypoint_index;
         sgtc_alive_team[i] = pBot.pEdict->v.team;
         SGTC_TrailSample(i, pBot);
      }
      else if (pBot.sgtc_was_alive)
      {
         // a bot entering the game is "alive" with no team for a moment: not a death
         if (sgtc_alive_team[i] != 0)
         {
            UTIL_ConsolePrintf("[sgtc-stat] death %s at %.0f %.0f %.0f wpt=%d\n", STRING(pBot.pEdict->v.netname),
               pBot.v_sgtc_last_pos.x, pBot.v_sgtc_last_pos.y, pBot.v_sgtc_last_pos.z, pBot.sgtc_last_wpt);
            SGTC_TrailLog(i, pBot);
         }
         sgtc_trail_count[i] = 0;
      }
      pBot.sgtc_was_alive = alive;
   }

   // once the map items have their waypoints and the gates are known: cover
   // the map with waypoints (over many frames), then the gate/DHD/spawn paths
   if (!sgtc_obj.paths_built && sgtc_obj.f_next_scan > 0 && gpGlobals->time > 2.0f && SGTC_CoverageThink())
   {
      sgtc_obj.paths_built = TRUE;
      SGTC_BuildGatePaths();
      SGTC_BuildRings();
      SGTC_BuildTransports();
      SGTC_BuildDoors();
      SGTC_BuildTransits();
      if (sgtc_wptdump)
         SGTC_DumpWaypointGraph();
   }

   if (sgtc_obj.f_next_scan > gpGlobals->time)
      return;

   sgtc_obj.f_next_scan = gpGlobals->time + 0.5f;
   if (sgtc_humanreset_off > 0 && (gpGlobals->time >= sgtc_humanreset_off || sgtc_humanreset_off > gpGlobals->time + 10.0f))
   {
      sgtc_humanreset_off = 0;
      SERVER_COMMAND("mp_resetround 0\n");
   }
   SGTC_KickGhostBots();
   SGTC_ScanObjectives();
   SGTC_WatchC4Walls();
   SGTC_WatchTransits();

   // where the bots are, for tools/navstats.py (stuck bots, unvisited areas)
   if (sgtc_obj.f_next_pos_dump <= gpGlobals->time)
   {
      sgtc_obj.f_next_pos_dump = gpGlobals->time + 20.0f;
      for (int i = 0; i < 32; i++)
      {
         bot_t &pBot = bots[i];
         if (!pBot.is_used || FNullEnt(pBot.pEdict) || !IsAlive(pBot.pEdict))
            continue;
         const Vector &o = pBot.pEdict->v.origin;
         int w = pBot.curr_waypoint_index;
         Vector wo = (w >= 0 && w < num_waypoints) ? waypoints[w].origin : Vector(0, 0, 0);
         char *info = GET_INFOKEYBUFFER(pBot.pEdict);
         UTIL_ConsolePrintf("[sgtc-stat] pos %s side=%d at %.0f %.0f %.0f wpt=%d (%.0f %.0f %.0f flags=%x) goal=%d enemy=%d pevteam=%d infoteam=%s model=%s class=%d\n",
            STRING(pBot.pEdict->v.netname), SGTC_GetPlayerTeam(pBot.pEdict), o.x, o.y, o.z,
            w, wo.x, wo.y, wo.z, (w >= 0 && w < num_waypoints) ? waypoints[w].flags : 0,
            pBot.waypoint_goal, pBot.pBotEnemy != NULL, pBot.pEdict->v.team,
            INFOKEY_VALUE(info, "team"), INFOKEY_VALUE(info, "model"), pBot.pEdict->v.playerclass);
      }
   }
}

void SGTC_MapObjectivesReset(void)
{
   SGTC_ResetTransports();  // before the next map's keyvalues
   memset(&sgtc_obj, 0, sizeof(sgtc_obj));
   sgtc_num_goal_teams = 0;
   sgtc_num_doors = 0;
   sgtc_num_mm_kv = sgtc_num_ent_kv = 0;  // the next map's keyvalues come before its first frame
   sgtc_num_ct_kv = 0;
   sgtc_goal_btn = sgtc_goal_btn_goal = NULL;
   sgtc_goal_btn_done = FALSE;
   sgtc_goal_btn_w = -1;
   sgtc_goal_btn_log = 0;
   memset(sgtc_num_spawn_clusters, 0, sizeof(sgtc_num_spawn_clusters));
   SGTC_ResetGates();
   SGTC_ResetTransits();
}


// Waypoint close to target, by plain distance: objective centers are often inside
// brushes (buttons, trigger volumes), where jk_botti's hull-trace lookup fails.
// radius 0: the nearest one. radius > 0: a random one within radius, so defenders
// spread out; ones reachable from the bot are preferred, others are left to the
// gate detour in SGTC_SetGoal.
static qboolean SGTC_InGateZone(const Vector &p);

static int SGTC_WaypointNearEx(bot_t &pBot, const Vector &target, float radius, qboolean allow_zones)
{
   int reach[64], other[64];
   int num_reach = 0, num_other = 0;
   int nearest = -1;
   float nearest_dist = 1500.0f;

   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;

      qboolean excluded = FALSE;  // goals the bot recently got stuck on
      for (int e = 0; e < EXCLUDE_POINTS_COUNT; e++)
         if (pBot.exclude_points[e] == i)
            excluded = TRUE;
      if (excluded)
         continue;

      if (!allow_zones && SGTC_InGateZone(waypoints[i].origin))
         continue;

      float d = (waypoints[i].origin - target).Length();
      if (d < nearest_dist)
      {
         nearest = i;
         nearest_dist = d;
      }

      // an area goal: not halfway up a ladder or at the edge of a jump
      if (radius <= 0 || d > radius || (waypoints[i].flags & (W_FL_LADDER | W_FL_JUMP)))
         continue;
      if (pBot.curr_waypoint_index == -1 || WaypointDistanceFromTo(pBot.curr_waypoint_index, i) < WAYPOINT_MAX_DISTANCE)
      {
         if (num_reach < 64)
            reach[num_reach++] = i;
      }
      else if (num_other < 64)
         other[num_other++] = i;
   }

   if (num_reach)
      return reach[RANDOM_LONG2(0, num_reach - 1)];
   if (num_other)
      return other[RANDOM_LONG2(0, num_other - 1)];
   return nearest;
}

static int SGTC_WaypointNear(bot_t &pBot, const Vector &target, float radius)
{
   return SGTC_WaypointNearEx(pBot, target, radius, FALSE);
}

static float SGTC_Dist(edict_t *a, const Vector &b)
{
   return (a->v.origin - b).Length();
}

//
// Stargates. A DHD is a func_button named "dial<N>" (or "2dial<N>"): each press
// locks one more chevron (the map retargets the button), and once dialed the
// wormhole teleports "g<N>tele_*" become active. Some sides of a map are only
// reachable through a gate, so bots dial and travel when their goal is cut off.
//
#define SGTC_MAX_GATES 4

typedef struct
{
   int number;            // N in dial<N> / g<N>tele
   edict_t *dhd;          // DHD button; a gate without one: the button that dials it, or NULL
   edict_t *tele[8];      // wormhole teleport triggers
   int num_tele;
   Vector entrance;       // center of the teleports
   qboolean open;         // dialed, and the arrival is safe
   qboolean blocked;      // dialed, but the arrival is a kill zone (sg_conquest: the SGC's iris is closed)
   qboolean kawoosh;      // the vortex in front of it sends to its kill room right now
   qboolean one_press;    // no dial<N>: one press of its dialer runs the whole sequence (sg_conquest's "PC")
   edict_t *zone[2];      // other teleports in front of it: the opening vortex (kawoosh)
   int num_zones;         // sends whoever stands there to a death room
} sgtc_gate_t;

static sgtc_gate_t sgtc_gates[SGTC_MAX_GATES];
static int sgtc_num_gates = 0;
static float sgtc_gate_last_press[10];  // by gate number: last DHD press by any bot
static float sgtc_gate_closed_time;     // when a gate last closed
static edict_t *sgtc_gate_dialer[10];   // by gate number: the dialer of a gate without DHD
static qboolean sgtc_gate_dialer_done[10];

static void SGTC_ResetGates(void)
{
   sgtc_num_gates = 0;
   SGTC_ResetRings();
   memset(sgtc_gate_last_press, 0, sizeof(sgtc_gate_last_press));
   sgtc_gate_closed_time = 0;
   memset(sgtc_gate_dialer, 0, sizeof(sgtc_gate_dialer));
   memset(sgtc_gate_dialer_done, 0, sizeof(sgtc_gate_dialer_done));
}

static int SGTC_GateNumber(const char *name, const char *prefix)
{
   // "dial2", "2dial2" -> 2 for prefix "dial"; "g1tele_3" -> 1 for prefix "g"
   const char *p = strstr(name, prefix);
   if (!p)
      return 0;
   p += strlen(prefix);
   return (*p >= '1' && *p <= '9') ? *p - '0' : 0;
}

// A kill zone at p: in a damaging (100+), non-team trigger_hurt that is on,
// or that a trigger_multiple / trigger_once there switches on. sg_conquest:
// the iris arrivals g2_N_kill are in "iris_kill_hurt" (start off, turned on by
// the trigger_multiple around it), the vortex rooms killbox1/2 in a hurt that
// is always on; the real SGC arrivals are under the gas "mission_kill", off
// with no trigger there (on 2 s per round): not a kill zone.
static qboolean SGTC_KillZoneAt(const Vector &p)
{
   static const char *trap_class[2] = {"trigger_multiple", "trigger_once"};
   edict_t *pHurt = NULL;
   while (!FNullEnt(pHurt = FIND_ENTITY_BY_CLASSNAME(pHurt, "trigger_hurt")))
   {
      if (pHurt->v.dmg < 100.0f)
         continue;
      int kv = SGTC_EntKv(pHurt);
      if (kv != -1 && sgtc_ent_kv[kv].team)
         continue;  // kills one side (sg_pyramide: the Goa'uld arriving in the SGC)
      Vector mins = pHurt->v.origin + pHurt->v.mins - Vector(16, 16, 36);
      Vector maxs = pHurt->v.origin + pHurt->v.maxs + Vector(16, 16, 36);
      if (p.x < mins.x || p.x > maxs.x || p.y < mins.y || p.y > maxs.y || p.z < mins.z || p.z > maxs.z)
         continue;
      if (pHurt->v.solid != SOLID_NOT)
         return TRUE;
      for (int c = 0; c < 2; c++)
      {
         edict_t *pTrap = NULL;
         while (!FNullEnt(pTrap = FIND_ENTITY_BY_CLASSNAME(pTrap, trap_class[c])))
         {
            Vector tmin = pTrap->v.origin + pTrap->v.mins - Vector(16, 16, 36);
            Vector tmax = pTrap->v.origin + pTrap->v.maxs + Vector(16, 16, 36);
            if (p.x >= tmin.x && p.x <= tmax.x && p.y >= tmin.y && p.y <= tmax.y && p.z >= tmin.z && p.z <= tmax.z)
               return TRUE;
         }
      }
   }
   return FALSE;
}

// a teleport target (by name) that lands in a kill zone
static qboolean SGTC_DeadlyTarget(const char *target)
{
   if (!target || !target[0])
      return FALSE;
   edict_t *pDest = NULL;
   while (!FNullEnt(pDest = FIND_ENTITY_BY_TARGETNAME(pDest, target)))
      if (SGTC_KillZoneAt(pDest->v.origin))
         return TRUE;
   return FALSE;
}

static qboolean SGTC_IsButton(edict_t *pent)
{
   const char *cls = STRING(pent->v.classname);
   return !strcmp(cls, "func_button") || !strcmp(cls, "func_rot_button");
}

// a button that can be pressed now: its target names something
static qboolean SGTC_DialerArmed(edict_t *pButton)
{
   const char *target = STRING(pButton->v.target);
   return target[0] && !FNullEnt(FIND_ENTITY_BY_TARGETNAME(NULL, target));
}

static qboolean SGTC_NameIn(const char **set, int n, const char *name)
{
   if (!name || !name[0])
      return FALSE;
   for (int i = 0; i < n; i++)
      if (!strcmp(set[i], name))
         return TRUE;
   return FALSE;
}

// does firing this entity fire one of the names: its target, a target a
// trigger_changetarget may give it, a multi_manager's targets?
static qboolean SGTC_FiresInto(edict_t *pent, const char **set, int n)
{
   const char *cls = STRING(pent->v.classname);
   if (!strcmp(cls, "trigger_changetarget"))
      return FALSE;  // retargets, fires nothing
   if (!strcmp(cls, "multi_manager"))
   {
      for (int k = 0; k < sgtc_num_mm_kv; k++)
         if (sgtc_mm_kv[k].ent == pent && SGTC_NameIn(set, n, sgtc_mm_kv[k].name))
            return TRUE;
      return FALSE;
   }
   if (SGTC_NameIn(set, n, STRING(pent->v.target)))
      return TRUE;
   const char *name = STRING(pent->v.targetname);
   if (!name[0])
      return FALSE;
   for (int k = 0; k < sgtc_num_ct_kv; k++)
   {
      edict_t *pCt = sgtc_ct_kv[k].ent;
      if (!FNullEnt(pCt) && !pCt->free && !strcmp(STRING(pCt->v.target), name) && SGTC_NameIn(set, n, sgtc_ct_kv[k].newtarget))
         return TRUE;
   }
   return FALSE;
}

//
// A gate without a DHD: the button whose firing ends up pointing the gate's
// teleports at a live, safe arrival (a trigger_changetarget on a g<N>tele_*),
// searched backwards from those changetargets through multi_managers,
// relays, buttons and the targets trigger_changetargets may give them.
// sg_conquest's g2: "PC" -> sgmm -> sgmm2_but -> sgmm2 ... sgmm8 -> sgmm8b ->
// g2tele_x_on -> g2tele_N_on; sgmm2_but..sgmm8_but are in the logic room.
// Of those, the one with a waypoint within 200 units nearest to the gate.
// Once per map, after the coverage (the waypoints around the buttons).
//
#define SGTC_MAX_FIRE 384
static edict_t *SGTC_GateDialer(sgtc_gate_t &g)
{
   int n = g.number < 10 ? g.number : 0;
   edict_t *pCached = sgtc_gate_dialer[n];
   if (sgtc_gate_dialer_done[n])
      return (!FNullEnt(pCached) && !pCached->free && SGTC_IsButton(pCached)) ? pCached : NULL;
   if (!sgtc_obj.paths_built || !g.num_tele)
      return NULL;
   sgtc_gate_dialer_done[n] = TRUE;

   static const char *fire[SGTC_MAX_FIRE];
   int num_fire = 0;
   for (int k = 0; k < sgtc_num_ct_kv && num_fire < SGTC_MAX_FIRE; k++)
   {
      edict_t *pCt = sgtc_ct_kv[k].ent;
      if (FNullEnt(pCt) || pCt->free)
         continue;
      const char *ct_name = STRING(pCt->v.targetname), *ct_target = STRING(pCt->v.target);
      if (!ct_name[0] || SGTC_NameIn(fire, num_fire, ct_name))
         continue;
      qboolean ours = FALSE;
      for (int t = 0; t < g.num_tele && !ours; t++)
         ours = !strcmp(ct_target, STRING(g.tele[t]->v.targetname));
      const char *dest = sgtc_ct_kv[k].newtarget;
      if (ours && !FNullEnt(FIND_ENTITY_BY_TARGETNAME(NULL, dest)) && !SGTC_DeadlyTarget(dest))
         fire[num_fire++] = ct_name;
   }

   edict_t *cand[16];
   int num_cand = 0;
   edict_t *first = INDEXENT(0);
   for (int pass = 0; pass < 64 && num_fire && first; pass++)
   {
      qboolean grew = FALSE;
      for (int e = 1; e < gpGlobals->maxEntities; e++)
      {
         edict_t *pent = first + e;
         if (pent->free || !pent->pvPrivateData || !SGTC_FiresInto(pent, fire, num_fire))
            continue;
         if (SGTC_IsButton(pent))
         {
            int c;
            for (c = 0; c < num_cand && cand[c] != pent; c++)
               ;
            if (c == num_cand && num_cand < 16)
               cand[num_cand++] = pent;
         }
         const char *name = STRING(pent->v.targetname);
         if (name[0] && num_fire < SGTC_MAX_FIRE && !SGTC_NameIn(fire, num_fire, name))
         {
            fire[num_fire++] = name;
            grew = TRUE;
         }
      }
      if (!grew)
         break;
   }

   edict_t *best = NULL;
   float best_d = 0;
   qboolean best_w = FALSE;
   for (int c = 0; c < num_cand; c++)
   {
      if (SGTC_GateNumber(STRING(cand[c]->v.targetname), "dial"))
         continue;  // another gate's DHD
      Vector bc = SGTC_EntCenter(cand[c]);
      qboolean has_w = FALSE;
      for (int w = 0; w < num_waypoints && !has_w; w++)
         has_w = !(waypoints[w].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP)) &&
                 (waypoints[w].origin - bc).Length() < 200.0f;
      float d = (bc - g.entrance).Length();
      if (!best || (has_w && !best_w) || (has_w == best_w && d < best_d))
      {
         best = cand[c];
         best_d = d;
         best_w = has_w;
      }
   }
   sgtc_gate_dialer[n] = best;
   if (best)
   {
      Vector bc = SGTC_EntCenter(best);
      int kv = SGTC_EntKv(best), team = kv != -1 ? sgtc_ent_kv[kv].team : 0;
      UTIL_ConsolePrintf("[sgtc-stat] gate %d: no DHD, dialed by %s \"%s\" at %.0f %.0f %.0f (%d button(s) reach it, %d names on the way, %s%s, target now \"%s\")\n",
         g.number, STRING(best->v.classname), STRING(best->v.targetname), bc.x, bc.y, bc.z, num_cand, num_fire,
         team == 1 ? "Tau'ri only" : team == 2 ? "Goa'uld only" : "anyone", best_w ? "" : ", no waypoint near it",
         STRING(best->v.target));
   }
   else
      UTIL_ConsolePrintf("[sgtc-stat] gate %d: no DHD and no button dials it (%d names on the way): only the map opens it\n",
         g.number, num_fire);
   return best;
}

static void SGTC_ScanGates(void)
{
   qboolean was_open[10] = {FALSE}, was_blocked[10] = {FALSE}, was_kawoosh[10] = {FALSE};
   for (int i = 0; i < sgtc_num_gates; i++)
      if (sgtc_gates[i].number < 10)
      {
         was_open[sgtc_gates[i].number] = sgtc_gates[i].open;
         was_blocked[sgtc_gates[i].number] = sgtc_gates[i].blocked;
         was_kawoosh[sgtc_gates[i].number] = sgtc_gates[i].kawoosh;
      }

   sgtc_num_gates = 0;
   memset(sgtc_gates, 0, sizeof(sgtc_gates));

   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "func_button")))
   {
      const char *name = STRING(pent->v.targetname);
      int n = SGTC_GateNumber(name, "dial");
      if (!n || sgtc_num_gates >= SGTC_MAX_GATES)
         continue;
      sgtc_gates[sgtc_num_gates].number = n;
      sgtc_gates[sgtc_num_gates].dhd = pent;
      sgtc_num_gates++;
   }

   pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "trigger_teleport")))
   {
      const char *name = STRING(pent->v.targetname);
      if (name[0] != 'g' || !strstr(name, "tele"))
         continue;
      int n = SGTC_GateNumber(name, "g");
      if (!n)
         continue;
      // dialed: the map points the teleports at the other gate (and at
      // "nowhere" or nothing while closed); at a kill zone instead, the way
      // is shut (sg_conquest: the SGC's iris is closed, g1tele_N -> g2_N_kill)
      const char *target = STRING(pent->v.target);
      int state = 0;  // 0 closed, 1 open, 2 blocked
      if (pent->v.solid == SOLID_TRIGGER && target[0] && !FNullEnt(FIND_ENTITY_BY_TARGETNAME(NULL, target)))
         state = SGTC_DeadlyTarget(target) ? 2 : 1;
      qboolean found = FALSE;
      for (int i = 0; i < sgtc_num_gates; i++)
      {
         sgtc_gate_t &g = sgtc_gates[i];
         if (g.number != n)
            continue;
         found = TRUE;
         if (g.num_tele >= 8)
            continue;
         g.tele[g.num_tele++] = pent;
         if (state == 1)
            g.open = TRUE;
         else if (state == 2)
            g.blocked = TRUE;
      }
      // a gate with no DHD (sg_conquest's SGC gate g2: the map dials it at
      // round start, and the "PC" console in the control room)
      if (!found && sgtc_num_gates < SGTC_MAX_GATES)
      {
         sgtc_gate_t &g = sgtc_gates[sgtc_num_gates++];
         g.number = n;
         g.one_press = TRUE;
         g.tele[g.num_tele++] = pent;
         if (state == 1)
            g.open = TRUE;
         else if (state == 2)
            g.blocked = TRUE;
      }
   }

   // a DHD with no wormhole teleports is decoration (sg_strike3's dial1)
   int kept = 0;
   for (int i = 0; i < sgtc_num_gates; i++)
      if (sgtc_gates[i].num_tele)
         sgtc_gates[kept++] = sgtc_gates[i];
   sgtc_num_gates = kept;

   // kawoosh: other trigger_teleports right in front of a gate (sg_nox: hurt1, 2hurt1)
   pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "trigger_teleport")))
   {
      const char *name = STRING(pent->v.targetname);
      if (name[0] == 'g' && strstr(name, "tele"))
         continue;
      for (int i = 0; i < sgtc_num_gates; i++)
      {
         sgtc_gate_t &g = sgtc_gates[i];
         if (g.num_tele && g.num_zones < 2 && (SGTC_EntCenter(pent) - SGTC_EntCenter(g.tele[0])).Length() < 500.0f)
            g.zone[g.num_zones++] = pent;
      }
   }

   for (int i = 0; i < sgtc_num_gates; i++)
   {
      sgtc_gate_t &g = sgtc_gates[i];
      if (g.blocked)
         g.open = FALSE;
      // the vortex sends whoever stands in front to its kill room right now
      for (int z = 0; z < g.num_zones; z++)
      {
         const char *target = STRING(g.zone[z]->v.target);
         if (target[0] && SGTC_DeadlyTarget(target))
            g.kawoosh = TRUE;
      }
      Vector sum(0, 0, 0);
      for (int t = 0; t < g.num_tele; t++)
         sum = sum + SGTC_EntCenter(g.tele[t]);
      g.entrance = g.num_tele ? sum * (1.0f / g.num_tele) : SGTC_EntCenter(g.dhd);
      if (g.one_press)
         g.dhd = SGTC_GateDialer(g);

      if (g.number >= 10)
         continue;
      float t_round = gpGlobals->time - sgtc_state.f_round_start;
      if (g.open != was_open[g.number])
      {
         Vector d = !FNullEnt(g.dhd) ? SGTC_EntCenter(g.dhd) : Vector(0, 0, 0);
         UTIL_ConsolePrintf("[sgtc-stat]   gate %d: %s %.0f %.0f %.0f entrance %.0f %.0f %.0f teleports=%d\n",
            g.number, g.one_press ? "dialer" : "dhd", d.x, d.y, d.z, g.entrance.x, g.entrance.y, g.entrance.z, g.num_tele);
         UTIL_ConsolePrintf("[sgtc-stat] event GATE%d_%s t=%.1f\n", g.number, g.open ? "OPEN" : "CLOSED", t_round);
      }
      if (g.blocked != was_blocked[g.number])
         UTIL_ConsolePrintf("[sgtc-stat] event GATE%d_%s t=%.1f%s\n", g.number, g.blocked ? "BLOCKED" : "UNBLOCKED", t_round,
            g.blocked ? " (the arrival is a kill zone: iris closed)" : "");
      if (g.kawoosh != was_kawoosh[g.number])
         UTIL_ConsolePrintf("[sgtc-stat]   gate %d: vortex %s t=%.1f\n", g.number, g.kawoosh ? "on (kill zone in front)" : "off", t_round);
      if ((was_open[g.number] || was_blocked[g.number]) && !g.open && !g.blocked)
         sgtc_gate_closed_time = gpGlobals->time;
   }
}

//
// Approach paths. The converted 2012 waypoints often stop far from the gates
// (sg_nox: 1700 units), so bots can't walk into the wormhole or reach the DHD.
// At map start, search a walkable path on a grid (A*, hull traces) from the
// gate / DHD to the nearest waypoint and add waypoints along it.
//
#define SGTC_GRID         48.0f
#define SGTC_MAX_NODES    4000
#define SGTC_HASH_SIZE    16384  // power of 2
#define SGTC_MAX_PATH     256

typedef struct
{
   Vector pos;
   int ix, iy, iz;
   int parent;
   float g;
   qboolean closed;
} sgtc_pathnode_t;

static sgtc_pathnode_t sgtc_nodes[SGTC_MAX_NODES];
static int sgtc_num_nodes;
static short sgtc_node_hash[SGTC_HASH_SIZE];  // node index + 1, 0 = free
static struct { float f; int node; } sgtc_heap[SGTC_MAX_NODES * 8];
static int sgtc_heap_size;
static int sgtc_targets[MAX_WAYPOINTS];
static int sgtc_num_targets;

static void SGTC_HeapPush(float f, int node)
{
   if (sgtc_heap_size >= SGTC_MAX_NODES * 8)
      return;
   int i = sgtc_heap_size++;
   while (i > 0 && sgtc_heap[(i - 1) / 2].f > f)
   {
      sgtc_heap[i] = sgtc_heap[(i - 1) / 2];
      i = (i - 1) / 2;
   }
   sgtc_heap[i].f = f;
   sgtc_heap[i].node = node;
}

static int SGTC_HeapPop(void)
{
   int node = sgtc_heap[0].node;
   float f = sgtc_heap[--sgtc_heap_size].f;
   int last = sgtc_heap[sgtc_heap_size].node;
   int i = 0;
   for (;;)
   {
      int c = i * 2 + 1;
      if (c >= sgtc_heap_size)
         break;
      if (c + 1 < sgtc_heap_size && sgtc_heap[c + 1].f < sgtc_heap[c].f)
         c++;
      if (sgtc_heap[c].f >= f)
         break;
      sgtc_heap[i] = sgtc_heap[c];
      i = c;
   }
   sgtc_heap[i].f = f;
   sgtc_heap[i].node = last;
   return node;
}

// node at grid cell (ix, iy) and height bucket iz; create it if new
static int SGTC_NodeAt(const Vector &pos, int ix, int iy, qboolean &created)
{
   int iz = (int)floorf(pos.z / 40.0f + 0.5f);
   unsigned int h = ((unsigned int)ix * 73856093u ^ (unsigned int)iy * 19349663u ^ (unsigned int)iz * 83492791u) & (SGTC_HASH_SIZE - 1);

   created = FALSE;
   while (sgtc_node_hash[h])
   {
      sgtc_pathnode_t &n = sgtc_nodes[sgtc_node_hash[h] - 1];
      if (n.ix == ix && n.iy == iy && n.iz == iz)
         return sgtc_node_hash[h] - 1;
      h = (h + 1) & (SGTC_HASH_SIZE - 1);
   }
   if (sgtc_num_nodes >= SGTC_MAX_NODES)
      return -1;

   int index = sgtc_num_nodes++;
   sgtc_pathnode_t &n = sgtc_nodes[index];
   n.pos = pos;
   n.ix = ix;
   n.iy = iy;
   n.iz = iz;
   n.parent = -1;
   n.g = 0;
   n.closed = FALSE;
   sgtc_node_hash[h] = index + 1;
   created = TRUE;
   return index;
}

// standing position on the ground below pos (player origin height), FALSE if none
static qboolean SGTC_GroundAt(const Vector &pos, float depth, Vector &out)
{
   TraceResult tr;
   UTIL_TraceHull(pos, pos - Vector(0, 0, depth), ignore_monsters, human_hull, NULL, &tr);
   if (tr.fStartSolid || tr.fAllSolid || tr.flFraction >= 1.0f || tr.vecPlaneNormal.z < 0.7f)
      return FALSE;
   out = tr.vecEndPos;
   return TRUE;
}

// why steps failed during the last search (debug output when no path is found)
static int sgtc_walk_walls, sgtc_walk_ledges;
static Vector sgtc_walk_ledge_pos[8];

// walk from 'from' by 'move' (horizontal) in small steps: stairs, slopes and
// ledges a jump gets onto (44 units, jk_botti jumps up to 45), nothing higher
// or deeper, so that the way back is walkable too
static qboolean SGTC_WalkStep(const Vector &from, const Vector &move, Vector &out)
{
   static const float lifts[3] = {18, 44, 0};  // step up, jump, low ceiling
   const int steps = 3;
   Vector d = move * (1.0f / steps);
   Vector cur = from;
   TraceResult tr;

   for (int k = 0; k < steps; k++)
   {
      Vector hor;
      int lift;
      for (lift = 0; lift < 3; lift++)
      {
         Vector up = cur + Vector(0, 0, lifts[lift]);
         hor = up + d;
         UTIL_TraceHull(up, hor, ignore_monsters, human_hull, NULL, &tr);
         if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction >= 1.0f)
            break;
      }
      if (lift == 3)
      {
         sgtc_walk_walls++;
         return FALSE;
      }
      if (!SGTC_GroundAt(hor, hor.z - (cur.z - 44.0f), cur))
      {
         if (sgtc_walk_ledges < 8)
            sgtc_walk_ledge_pos[sgtc_walk_ledges] = hor;
         sgtc_walk_ledges++;
         return FALSE;
      }
   }
   out = cur;
   return TRUE;
}

static qboolean SGTC_SegmentHitsBox(const Vector &va, const Vector &vb, const Vector &vmins, const Vector &vmaxs)
{
   const float a[3] = {va.x, va.y, va.z}, b[3] = {vb.x, vb.y, vb.z};
   const float mins[3] = {vmins.x, vmins.y, vmins.z}, maxs[3] = {vmaxs.x, vmaxs.y, vmaxs.z};
   float t0 = 0, t1 = 1;
   for (int axis = 0; axis < 3; axis++)
   {
      float d = b[axis] - a[axis];
      if (fabs(d) < 0.001f)
      {
         if (a[axis] < mins[axis] || a[axis] > maxs[axis])
            return FALSE;
         continue;
      }
      float u0 = (mins[axis] - a[axis]) / d, u1 = (maxs[axis] - a[axis]) / d;
      if (u0 > u1) { float t = u0; u0 = u1; u1 = t; }
      if (u0 > t0) t0 = u0;
      if (u1 < t1) t1 = u1;
      if (t0 > t1)
         return FALSE;
   }
   return TRUE;
}

// would walking this segment touch a wormhole (and teleport the bot away)?
static qboolean SGTC_CrossesGate(const Vector &a, const Vector &b)
{
   for (int i = 0; i < sgtc_num_gates; i++)
      for (int t = 0; t < sgtc_gates[i].num_tele; t++)
      {
         edict_t *pTele = sgtc_gates[i].tele[t];
         Vector mins = pTele->v.origin + pTele->v.mins - Vector(16, 16, 36);
         Vector maxs = pTele->v.origin + pTele->v.maxs + Vector(16, 16, 36);
         if (SGTC_SegmentHitsBox(a, b, mins, maxs))
            return TRUE;
      }
   return SGTC_CrossesTeleport(a, b);
}

// distance to the nearest existing waypoint; 'touch' = one is close and in sight
static float SGTC_PathHeuristic(const Vector &pos, int &touch)
{
   float best = 99999.0f;
   touch = -1;
   for (int i = 0; i < sgtc_num_targets; i++)
   {
      const Vector &w = waypoints[sgtc_targets[i]].origin;
      float d = (w - pos).Length();
      if (d < best)
         best = d;
      if (d < 96.0f && fabs(w.z - pos.z) < 40.0f && touch == -1)
      {
         TraceResult tr;
         UTIL_TraceHull(pos, w, ignore_monsters, head_hull, NULL, &tr);
         if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction >= 1.0f)
            touch = sgtc_targets[i];
      }
   }
   return best;
}

// A* from the start points to the waypoint network; fills out[] from the start
// to the last grid point before the waypoint. Returns the number of points.
// 'enclosed': no path because everything walkable from the start was searched.
static int SGTC_FindApproachPath(const Vector *starts, int num_starts, Vector *out, int &expanded, qboolean &enclosed)
{
   enclosed = FALSE;
   static const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

   sgtc_num_targets = 0;
   for (int i = 0; i < num_waypoints; i++)
      if (!(waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING)))
         sgtc_targets[sgtc_num_targets++] = i;

   sgtc_num_nodes = 0;
   sgtc_heap_size = 0;
   memset(sgtc_node_hash, 0, sizeof(sgtc_node_hash));
   expanded = 0;
   sgtc_walk_walls = sgtc_walk_ledges = 0;

   Vector origin = starts[0];
   for (int s = 0; s < num_starts; s++)
   {
      int touch;
      qboolean created;
      int ix = (int)floorf((starts[s].x - origin.x) / SGTC_GRID + 0.5f);
      int iy = (int)floorf((starts[s].y - origin.y) / SGTC_GRID + 0.5f);
      int n = SGTC_NodeAt(starts[s], ix, iy, created);
      if (n != -1 && created)
         SGTC_HeapPush(SGTC_PathHeuristic(starts[s], touch), n);
   }

   while (sgtc_heap_size > 0)
   {
      int n = SGTC_HeapPop();
      if (sgtc_nodes[n].closed)
         continue;
      sgtc_nodes[n].closed = TRUE;
      expanded++;

      int touch;
      SGTC_PathHeuristic(sgtc_nodes[n].pos, touch);
      if (touch != -1)
      {
         int len = 0;
         for (int k = n; k != -1; k = sgtc_nodes[k].parent)
            len++;
         if (len > SGTC_MAX_PATH)
            return 0;
         int k = n;
         for (int i = len - 1; i >= 0; i--, k = sgtc_nodes[k].parent)
            out[i] = sgtc_nodes[k].pos;
         return len;
      }

      for (int d = 0; d < 8; d++)
      {
         Vector move(dirs[d][0] * SGTC_GRID, dirs[d][1] * SGTC_GRID, 0);
         Vector from = sgtc_nodes[n].pos;
         Vector next;
         if (!SGTC_WalkStep(from, move, next) || SGTC_CrossesGate(from, next))
            continue;

         qboolean created;
         int m = SGTC_NodeAt(next, sgtc_nodes[n].ix + dirs[d][0], sgtc_nodes[n].iy + dirs[d][1], created);
         if (m == -1)
            return 0;  // out of nodes
         float g = sgtc_nodes[n].g + (next - from).Length();
         if (!created && (sgtc_nodes[m].closed || g >= sgtc_nodes[m].g))
            continue;
         sgtc_nodes[m].g = g;
         sgtc_nodes[m].parent = n;
         int t;
         SGTC_HeapPush(g + SGTC_PathHeuristic(next, t), m);
      }
   }
   enclosed = TRUE;
   return 0;
}

static void SGTC_LinkApproach(const char *what, int number, const Vector *starts, int num_starts)
{
   static Vector path[SGTC_MAX_PATH];
   int expanded;

   if (!num_starts)
   {
      UTIL_ConsolePrintf("[sgtc-stat] path to %s %d: no standing room around it\n", what, number);
      return;
   }
   qboolean enclosed;
   int len = SGTC_FindApproachPath(starts, num_starts, path, expanded, enclosed);
   if (len == 0 && enclosed && sgtc_num_nodes > 1)
   {
      // a place cut off from the waypoints (a gate room you can only arrive in):
      // cover what was searched with waypoints every 3 grid steps, so bots
      // coming out of the wormhole can find their way around in it
      static Vector cover[SGTC_MAX_NODES];
      int n = 0;
      for (int i = 0; i < sgtc_num_nodes; i++)
         if (i < num_starts || (((sgtc_nodes[i].ix % 3) + 3) % 3 == 0 && ((sgtc_nodes[i].iy % 3) + 3) % 3 == 0))
            cover[n++] = sgtc_nodes[i].pos;
      int added = WaypointAddChain(cover, n, FALSE, TRUE);
      UTIL_ConsolePrintf("[sgtc-stat] path to %s %d: enclosed area (%d grid points), %d waypoints added to cover it\n",
         what, number, sgtc_num_nodes, added);
      return;
   }
   if (len <= 1)
   {
      // 1: already next to a waypoint
      if (len == 0)
      {
         Vector mins = starts[0], maxs = starts[0];
         for (int i = 0; i < sgtc_num_nodes; i++)
         {
            const Vector &p = sgtc_nodes[i].pos;
            mins = Vector(fminf(mins.x, p.x), fminf(mins.y, p.y), fminf(mins.z, p.z));
            maxs = Vector(fmaxf(maxs.x, p.x), fmaxf(maxs.y, p.y), fmaxf(maxs.z, p.z));
         }
         UTIL_ConsolePrintf("[sgtc-stat] path to %s %d: none found (%d nodes searched, area %.0f %.0f %.0f - %.0f %.0f %.0f, %d walls, %d ledges)\n",
            what, number, expanded, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, sgtc_walk_walls, sgtc_walk_ledges);
         for (int i = 0; i < sgtc_walk_ledges && i < 8; i++)
            UTIL_ConsolePrintf("[sgtc-stat]   ledge at %.0f %.0f %.0f\n",
               sgtc_walk_ledge_pos[i].x, sgtc_walk_ledge_pos[i].y, sgtc_walk_ledge_pos[i].z);
      }
      return;
   }
   int added = WaypointAddChain(path, len, TRUE, TRUE);
   UTIL_ConsolePrintf("[sgtc-stat] path to %s %d: %d waypoints added from %.0f %.0f %.0f (%d grid points, %d searched)\n",
      what, number, added, path[0].x, path[0].y, path[0].z, len, expanded);
}

//
// Transport rings. The SGTC prefab (sg_chateau, sg_prison, sg_stargate):
// button "rg_but<N>" -> multi_managers -> 4.4 s later, for half a second, the
// masters of the teleports "rg<N>_tele1" (on the platform) and
// "rg<N>_tele1b" (a holding room) let them work: platform -> holding room ->
// the other ring platform. Bots get a forced path from the platform to the
// arrival, and SGTC_BotRingThink presses the button and waits on the platform.
//
#define SGTC_MAX_RINGS 4

typedef struct
{
   int number;            // N
   Vector platform;       // standing position in the middle of rg<N>_tele1
   Vector dest;           // where rg<N>_tele1b (or rg<N>_tele1) finally sends
   edict_t *button;       // rg_but<N>
   int w_platform, w_dest;
   qboolean has_hold;     // a holding room rg<N>_tele1b: the first hop lands there
   Vector hold;           // standing position in its middle
   Vector hold_col[10];   // its teleport columns: the second hop takes whoever stands in one
   int num_hold_cols;
   Vector spot[5];        // standing spots near the middle of the platform
   int num_spots;
   edict_t *train[9];     // the rings themselves, rg<N>_down<1-9>
   Vector train_rest[9];  // and where they are when up
   float train_moved[9];  // when each last moved (8 s still: at rest)
   int num_trains;
} sgtc_ring_t;

static sgtc_ring_t sgtc_rings[SGTC_MAX_RINGS];
static int sgtc_num_rings;
static float sgtc_ring_last_press[SGTC_MAX_RINGS];  // by any bot

// center of the trigger_teleports named 'name' and of their destinations
static int SGTC_TeleportGroup(const char *name, Vector &center, Vector &dest)
{
   int n = 0, nd = 0;
   Vector sum(0, 0, 0), dsum(0, 0, 0);
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_TARGETNAME(pent, name)))
   {
      if (strcmp(STRING(pent->v.classname), "trigger_teleport"))
         continue;
      sum = sum + SGTC_EntCenter(pent);
      n++;
      edict_t *pDest = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pent->v.target));
      if (!FNullEnt(pDest))
      {
         dsum = dsum + pDest->v.origin;
         nd++;
      }
   }
   if (n)
      center = sum * (1.0f / n);
   if (nd)
      dest = dsum * (1.0f / nd);
   return nd ? n : 0;
}

static int SGTC_WaypointAt(const Vector &pos, float radius, int flags)
{
   int best = -1;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;
      float d = (waypoints[i].origin - pos).Length();
      if (d < radius && fabs(waypoints[i].origin.z - pos.z) < 40.0f)
      {
         radius = d;
         best = i;
      }
   }
   return best != -1 ? best : WaypointAddOne(pos, flags);
}

static void SGTC_ResetRings(void)
{
   sgtc_num_rings = 0;
}

static void SGTC_BuildRings(void)
{
   sgtc_num_rings = 0;
   memset(sgtc_ring_last_press, 0, sizeof(sgtc_ring_last_press));

   for (int n = 1; n <= 9 && sgtc_num_rings < SGTC_MAX_RINGS; n++)
   {
      char name[32], name_b[32], button[32];
      safevoid_snprintf(name, sizeof(name), "rg%d_tele1", n);
      safevoid_snprintf(name_b, sizeof(name_b), "rg%d_tele1b", n);
      safevoid_snprintf(button, sizeof(button), "rg_but%d", n);

      sgtc_ring_t &r = sgtc_rings[sgtc_num_rings];
      Vector center, dest, center_b, dest_b;
      if (!SGTC_TeleportGroup(name, center, dest))
         continue;
      r.has_hold = FALSE;
      r.num_hold_cols = 0;
      if (SGTC_TeleportGroup(name_b, center_b, dest_b))
      {
         dest = dest_b;  // through the holding room
         // the first hop lands in the room's teleport columns and the second
         // hop, 0.4 s later, takes whoever is still in one: SGTC_BotRingHold
         // keeps the bot there (bots that ran on were left in the room, which
         // has no exit, no button and no waypoints, for minutes)
         r.has_hold = TRUE;
         r.hold = center_b;
         Vector g;
         if (SGTC_GroundAt(center_b + Vector(0, 0, 36), 160.0f, g))
            r.hold = g;
         edict_t *pent = NULL;
         while (!FNullEnt(pent = FIND_ENTITY_BY_TARGETNAME(pent, name_b)) && r.num_hold_cols < 10)
            if (!strcmp(STRING(pent->v.classname), "trigger_teleport"))
               r.hold_col[r.num_hold_cols++] = SGTC_EntCenter(pent);
      }

      // standing positions on the platform and at the arrival
      if (!SGTC_GroundAt(center + Vector(0, 0, 36), 160.0f, r.platform) ||
          !SGTC_GroundAt(dest + Vector(0, 0, 36), 160.0f, r.dest))
         continue;
      r.number = n;
      r.button = FIND_ENTITY_BY_TARGETNAME(NULL, button);

      // standing spots: the middle and 4 around it, 20 units out, so that
      // bots waiting together don't stand in each other. Not further out:
      // the rings come back down at 450 units/s on a band starting about 50
      // units from the middle, and the outer teleport columns are under it
      static const float around[5][2] = {{0, 0}, {20, 0}, {-20, 0}, {0, 20}, {0, -20}};
      r.num_spots = 0;
      for (int k = 0; k < 5; k++)
         if (SGTC_GroundAt(r.platform + Vector(around[k][0], around[k][1], 18), 18.0f + 24.0f, r.spot[r.num_spots]))
            r.num_spots++;
      r.num_trains = 0;
      for (int t = 1; t <= 9 && r.num_trains < 9; t++)
      {
         char train[32];
         safevoid_snprintf(train, sizeof(train), "rg%d_down%d", n, t);
         edict_t *pTrain = FIND_ENTITY_BY_TARGETNAME(NULL, train);
         if (!FNullEnt(pTrain))
         {
            r.train_rest[r.num_trains] = pTrain->v.origin;
            r.train_moved[r.num_trains] = gpGlobals->time;
            r.train[r.num_trains++] = pTrain;
         }
      }
      r.w_platform = SGTC_WaypointAt(r.platform, 64.0f, 0);
      r.w_dest = SGTC_WaypointAt(r.dest, 96.0f, 0);
      if (r.w_platform == -1 || r.w_dest == -1)
         continue;
      // the route matrix drops paths that long: give it a cost, lower than
      // the 4.4 s wait would be at ~250 units/s, so that routes prefer the
      // rings to the long ladders bots fumble on
      WaypointForceLongPath(r.w_platform, r.w_dest, 600.0f);
      UTIL_ConsolePrintf("[sgtc-stat] rings %d: platform %.0f %.0f %.0f (wpt %d) -> %.0f %.0f %.0f (wpt %d), button %s, %d spots, %d trains\n",
         n, r.platform.x, r.platform.y, r.platform.z, r.w_platform, r.dest.x, r.dest.y, r.dest.z, r.w_dest,
         FNullEnt(r.button) ? "none" : "found", r.num_spots, r.num_trains);
      if (r.has_hold)
         UTIL_ConsolePrintf("[sgtc-stat]   holding room at %.0f %.0f %.0f, %d columns\n", r.hold.x, r.hold.y, r.hold.z, r.num_hold_cols);
      for (int t = 0; t < r.num_trains; t++)
         UTIL_ConsolePrintf("[sgtc-stat]   ring train \"%s\" rest at %.0f %.0f %.0f\n", STRING(r.train[t]->v.targetname),
            r.train_rest[t].x, r.train_rest[t].y, r.train_rest[t].z);
      sgtc_num_rings++;
   }
   if (sgtc_num_rings)
      WaypointRebuildRoutes();
}

//
// Doors a button opens (sg_stargate's "gateroomdoor" between the Goa'uld
// ship corridor and its ring/nuke room: 5 of the 6 ship spawns are behind
// it; sg_prison's secret door and Goa'uld wing doors; sg_shunky's "pr" in
// front of the goal; sg_strike3's spawn exits). A route that crosses a shut
// door sends the bot to a button it may use first (SGTC_BotDoorThink). A
// door no waypoint path crosses gets a forced path, between the nearest
// waypoints on its two sides.
//
static int SGTC_PanelIndex(edict_t *pent)
{
   for (int i = 0; i < sgtc_num_panels; i++)
      if (sgtc_panels[i].ent == pent)
         return i;
   if (sgtc_num_panels >= SGTC_MAX_PANELS)
      return -1;
   sgtc_panel_t &p = sgtc_panels[sgtc_num_panels];
   int kv = SGTC_EntKv(pent);
   p.ent = pent;
   p.classname = pent->v.classname;
   p.rotating = !strcmp(STRING(pent->v.classname), "func_door_rotating");
   // where the map drew it is where it is shut (no "origin" key: 0 0 0)
   p.closed = kv == -1 ? Vector(0, 0, 0) : (p.rotating ? sgtc_ent_kv[kv].angles : sgtc_ent_kv[kv].origin);
   p.start_open = (pent->v.spawnflags & 1) != 0;  // SF_DOOR_START_OPEN
   return sgtc_num_panels++;
}

static Vector SGTC_PanelClosedOrigin(edict_t *pent)
{
   if (!strcmp(STRING(pent->v.classname), "func_door_rotating"))
      return pent->v.origin;  // turns about it
   int kv = SGTC_EntKv(pent);
   return kv == -1 ? Vector(0, 0, 0) : sgtc_ent_kv[kv].origin;
}

static qboolean SGTC_PanelValid(const sgtc_panel_t &p)
{
   return !FNullEnt(p.ent) && p.ent->v.classname == p.classname;
}

static qboolean SGTC_PanelStill(const sgtc_panel_t &p)
{
   return p.ent->v.velocity.Length() < 1.0f && p.ent->v.avelocity.Length() < 1.0f;
}

// still, where it shuts the passage
static qboolean SGTC_PanelShut(const sgtc_panel_t &p)
{
   if (!SGTC_PanelValid(p) || !SGTC_PanelStill(p))
      return FALSE;
   return ((p.rotating ? p.ent->v.angles : p.ent->v.origin) - p.closed).Length() < 2.0f;
}

#define SGTC_DOOR_OPEN   0
#define SGTC_DOOR_SHUT   1
#define SGTC_DOOR_MOVING 2
static int SGTC_DoorState(const sgtc_door_t &d)
{
   int shut = 0;
   for (int i = 0; i < d.num_panels; i++)
   {
      const sgtc_panel_t &p = sgtc_panels[d.panel[i]];
      if (!SGTC_PanelValid(p))
         return SGTC_DOOR_OPEN;  // gone
      if (!SGTC_PanelStill(p))
         return SGTC_DOOR_MOVING;
      if (SGTC_PanelShut(p))
         shut++;
   }
   return shut == d.num_panels ? SGTC_DOOR_SHUT : SGTC_DOOR_OPEN;
}

static qboolean SGTC_DoorOnSegment(const sgtc_door_t &d, const Vector &a, const Vector &b)
{
   return SGTC_SegmentHitsBox(a, b, d.mins - Vector(1, 1, 0), d.maxs + Vector(1, 1, 0));
}

// the door (panels of one name next to each other) a panel belongs to, made on first use
static int SGTC_DoorOfPanel(int p)
{
   for (int i = 0; i < sgtc_num_doors; i++)
      for (int k = 0; k < sgtc_doors[i].num_panels; k++)
         if (sgtc_doors[i].panel[k] == p)
            return i;
   if (sgtc_num_doors >= SGTC_MAX_DOORS)
      return -1;
   sgtc_door_t &d = sgtc_doors[sgtc_num_doors];
   edict_t *pFirst = sgtc_panels[p].ent;
   d.door = pFirst;
   d.panel[0] = p;
   d.num_panels = 1;
   d.num_buttons = 0;
   d.mins = SGTC_PanelClosedOrigin(pFirst) + pFirst->v.mins;
   d.maxs = SGTC_PanelClosedOrigin(pFirst) + pFirst->v.maxs;
   d.crossing = 0;
   d.w_a = d.w_b = -1;
   d.f_lock = d.f_pressed = 0;
   d.presser = NULL;
   d.press_checked = TRUE;
   d.last_state = -1;
   // the other panels of a double door: same name and class, touching it
   // (twice: one may only touch a panel found the first time)
   const char *name = STRING(pFirst->v.targetname);
   for (int pass = 0; pass < 2 && name[0]; pass++)
   {
      edict_t *pent = NULL;
      while (d.num_panels < SGTC_DOOR_PANELS && !FNullEnt(pent = FIND_ENTITY_BY_TARGETNAME(pent, name)))
      {
         if (strcmp(STRING(pent->v.classname), STRING(pFirst->v.classname)))
            continue;
         Vector o = SGTC_PanelClosedOrigin(pent), mn = o + pent->v.mins, mx = o + pent->v.maxs;
         if (mn.x > d.maxs.x + 32 || mx.x < d.mins.x - 32 || mn.y > d.maxs.y + 32 || mx.y < d.mins.y - 32 ||
             mn.z > d.maxs.z + 32 || mx.z < d.mins.z - 32)
            continue;
         int q = SGTC_PanelIndex(pent), k;
         if (q == -1)
            continue;
         for (k = 0; k < d.num_panels && d.panel[k] != q; k++)
            ;
         if (k < d.num_panels)
            continue;
         d.panel[d.num_panels++] = q;
         d.mins = Vector(mn.x < d.mins.x ? mn.x : d.mins.x, mn.y < d.mins.y ? mn.y : d.mins.y, mn.z < d.mins.z ? mn.z : d.mins.z);
         d.maxs = Vector(mx.x > d.maxs.x ? mx.x : d.maxs.x, mx.y > d.maxs.y ? mx.y : d.maxs.y, mx.z > d.maxs.z ? mx.z : d.maxs.z);
      }
   }
   // a wall slab: thin across, taller than that and than a player crouching
   Vector size = d.maxs - d.mins;
   float thin = size.x < size.y ? size.x : size.y;
   d.passage = thin <= 48.0f && size.z >= 64.0f && size.z > thin;
   return sgtc_num_doors++;
}

// every door panel a press moves, through up to two levels of multi_manager /
// trigger_relay ('chain'), and the doors a moved door fires itself
static void SGTC_ReachDoors(sgtc_button_t &b, const char *target, int depth, float delay, qboolean chain)
{
   if (!target || !target[0] || depth > 3)
      return;
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_TARGETNAME(pent, target)))
   {
      const char *cls = STRING(pent->v.classname);
      if (!strcmp(cls, "func_door") || !strcmp(cls, "func_door_rotating"))
      {
         int p = SGTC_PanelIndex(pent), k;
         if (p == -1)
            continue;
         for (k = 0; k < b.num_reach && b.reach[k] != p; k++)
            ;
         if (k == b.num_reach)
         {
            if (k >= SGTC_BTN_REACH)
               continue;
            b.reach[k] = p;
            b.reach_delay[k] = delay;
            b.reach_opens[k] = FALSE;
            b.num_reach++;
            // a door fires its own target once open or shut (sg_strike3: door01 -> door1)
            SGTC_ReachDoors(b, STRING(pent->v.target), depth + 1, delay, FALSE);
         }
         if (chain && delay <= 10.0f && !b.reach_opens[k])
         {
            b.reach_opens[k] = TRUE;
            b.reach_delay[k] = delay;
         }
      }
      else if (depth < 2 && !strcmp(cls, "multi_manager"))
      {
         for (int i = 0; i < sgtc_num_mm_kv; i++)
            if (sgtc_mm_kv[i].ent == pent)
               SGTC_ReachDoors(b, sgtc_mm_kv[i].name, depth + 1, delay + sgtc_mm_kv[i].delay, chain);
      }
      else if (depth < 2 && !strcmp(cls, "trigger_relay"))
      {
         int kv = SGTC_EntKv(pent);
         SGTC_ReachDoors(b, STRING(pent->v.target), depth + 1, delay + (kv != -1 ? sgtc_ent_kv[kv].delay : 0.0f), chain);
      }
   }
}

// the nearest waypoint in sight of the button, on its side of every door
// (a start-open door is open now; ctp_conquest has a button on each side)
static int SGTC_ButtonWaypoint(edict_t *pButton)
{
   Vector c = SGTC_EntCenter(pButton);
   Vector bmin = pButton->v.absmin - Vector(8, 8, 8), bmax = pButton->v.absmax + Vector(8, 8, 8);
   int best = -1, nearest = -1;
   float best_dist = 400.0f, nearest_dist = 1500.0f;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP))
         continue;
      float dist = (waypoints[i].origin - c).Length();
      if (dist < nearest_dist)
      {
         nearest_dist = dist;
         nearest = i;
      }
      if (dist >= best_dist)
         continue;
      qboolean across = FALSE;
      for (int k = 0; k < sgtc_num_doors && !across; k++)
      {
         const sgtc_door_t &d = sgtc_doors[k];
         qboolean in_door = c.x >= d.mins.x - 4 && c.x <= d.maxs.x + 4 && c.y >= d.mins.y - 4 && c.y <= d.maxs.y + 4 &&
                            c.z >= d.mins.z - 4 && c.z <= d.maxs.z + 4;  // sg_prison's goa_door1_fb: in both halves
         across = d.passage && !in_door && SGTC_DoorOnSegment(d, waypoints[i].origin, c);
      }
      if (across)
         continue;
      TraceResult tr;
      UTIL_TraceLine(waypoints[i].origin, c, ignore_monsters, NULL, &tr);
      const Vector &e = tr.vecEndPos;
      if (tr.flFraction >= 1.0f || tr.pHit == pButton ||
          (e.x >= bmin.x && e.x <= bmax.x && e.y >= bmin.y && e.y <= bmax.y && e.z >= bmin.z && e.z <= bmax.z))
      {
         best_dist = dist;
         best = i;
      }
   }
   return best != -1 ? best : nearest;
}

// What hl.dll's CBasePlayer::PlayerUse does with +use: of the usable
// entities whose box (absmin/absmax) comes within 64 units of the player's
// origin, the one lying the most straight ahead: the vector from the eyes to
// the middle of its box (absmin + size / 2), each axis shortened by half the
// box size (UTIL_ClampVectorToBox: towards its nearest face or corner),
// dotted with v_forward, more than 0.7. No trace: it works through a wall.
// A button made of several pieces has its middle between them (sg_stargate's
// gateroomdoor button: a 4x1 plate on each side of a wall, the middle inside
// the wall, 20 units under the eyes).
static float SGTC_UseDist(edict_t *pButton, const Vector &origin)
{
   const Vector &mn = pButton->v.absmin, &mx = pButton->v.absmax;
   float dx = origin.x < mn.x ? mn.x - origin.x : (origin.x > mx.x ? origin.x - mx.x : 0.0f);
   float dy = origin.y < mn.y ? mn.y - origin.y : (origin.y > mx.y ? origin.y - mx.y : 0.0f);
   float dz = origin.z < mn.z ? mn.z - origin.z : (origin.z > mx.z ? origin.z - mx.z : 0.0f);
   return sqrtf(dx * dx + dy * dy + dz * dz);
}

// the direction PlayerUse wants these eyes to look in (its dot is 1 along it)
static Vector SGTC_UseAim(edict_t *pButton, const Vector &eye)
{
   Vector size = pButton->v.size;
   if (size.x <= 0 && size.y <= 0 && size.z <= 0)
      size = pButton->v.absmax - pButton->v.absmin;
   Vector half = size * 0.5f, mid = pButton->v.absmin + half, los = mid - eye;
   los.x = los.x > half.x ? los.x - half.x : (los.x < -half.x ? los.x + half.x : 0.0f);
   los.y = los.y > half.y ? los.y - half.y : (los.y < -half.y ? los.y + half.y : 0.0f);
   los.z = los.z > half.z ? los.z - half.z : (los.z < -half.z ? los.z + half.z : 0.0f);
   if (los.Length() < 1.0f)
      los = mid - eye;  // the eyes inside its box: PlayerUse wouldn't take it anyway
   return los;
}

// nothing solid between these eyes and the point to aim at
static qboolean SGTC_UseSight(edict_t *pButton, const Vector &eye, edict_t *pIgnore)
{
   Vector aim = eye + SGTC_UseAim(pButton, eye);
   Vector bmin = pButton->v.absmin - Vector(8, 8, 8), bmax = pButton->v.absmax + Vector(8, 8, 8);
   TraceResult tr;
   UTIL_TraceLine(eye, aim, ignore_monsters, pIgnore, &tr);
   const Vector &e = tr.vecEndPos;
   return tr.flFraction >= 1.0f || tr.pHit == pButton ||
          (e.x >= bmin.x && e.x <= bmax.x && e.y >= bmin.y && e.y <= bmax.y && e.z >= bmin.z && e.z <= bmax.z);
}

// the waypoints to press a button from: its box within 60 units of a
// player standing there and the point to aim at in sight, the nearest first,
// one per side of a wall or a door (sg_stargate's gateroomdoor button is
// pressed from the ship corridor, wpt 168, and from the gate room, wpt 252).
// None in reach: the nearest one in sight, the rest walked.
static void SGTC_ButtonWaypoints(sgtc_button_t &b)
{
   int cand[32];
   float cand_dist[32];
   int num_cand = 0;

   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP))
         continue;
      const Vector &o = waypoints[i].origin;
      float dist = SGTC_UseDist(b.ent, o);
      if (dist > 60.0f || !SGTC_UseSight(b.ent, o + Vector(0, 0, 28), NULL))
         continue;
      if (num_cand == 32 && dist >= cand_dist[31])
         continue;
      int k = num_cand < 32 ? num_cand++ : 31;
      while (k > 0 && cand_dist[k - 1] > dist)
      {
         cand[k] = cand[k - 1];
         cand_dist[k] = cand_dist[k - 1];
         k--;
      }
      cand[k] = i;
      cand_dist[k] = dist;
   }

   b.num_wpt = 0;
   for (int k = 0; k < num_cand && b.num_wpt < SGTC_BTN_WPTS; k++)
   {
      const Vector &c = waypoints[cand[k]].origin;
      qboolean other_side = TRUE;
      for (int j = 0; j < b.num_wpt && other_side; j++)
      {
         const Vector &a = waypoints[b.wpt[j]].origin;
         TraceResult tr;
         UTIL_TraceLine(a, c, ignore_monsters, NULL, &tr);
         qboolean door = FALSE;
         for (int q = 0; q < sgtc_num_doors && !door; q++)
            door = sgtc_doors[q].passage && SGTC_DoorOnSegment(sgtc_doors[q], a, c);
         other_side = (tr.flFraction < 1.0f && tr.pHit != b.ent) || door;
      }
      if (other_side)
         b.wpt[b.num_wpt++] = cand[k];
   }
   if (!b.num_wpt)
   {
      int w = SGTC_ButtonWaypoint(b.ent);
      if (w != -1)
         b.wpt[b.num_wpt++] = w;
   }
   b.w = b.num_wpt ? b.wpt[0] : -1;
}

static const char *SGTC_DoorStateName(int state)
{
   return state == SGTC_DOOR_SHUT ? "shut" : state == SGTC_DOOR_MOVING ? "moving" : "open";
}

static void SGTC_BuildDoors(void)
{
   static const char *btn_class[2] = {"func_button", "func_rot_button"};
   int forced = 0;

   sgtc_num_doors = sgtc_num_panels = sgtc_num_buttons = 0;
   for (int c = 0; c < 2; c++)
   {
      edict_t *pButton = NULL;
      while (!FNullEnt(pButton = FIND_ENTITY_BY_CLASSNAME(pButton, btn_class[c])) && sgtc_num_buttons < SGTC_MAX_BUTTONS)
      {
         const char *target = STRING(pButton->v.target);
         if (!target[0] || strlen(target) >= sizeof(sgtc_buttons[0].target))
            continue;
         // DHDs and ring buttons have their own code
         qboolean other = FALSE;
         for (int i = 0; i < sgtc_num_gates; i++)
            if (sgtc_gates[i].dhd == pButton)
               other = TRUE;
         for (int i = 0; i < sgtc_num_rings; i++)
            if (sgtc_rings[i].button == pButton)
               other = TRUE;
         if (other)
            continue;
         sgtc_button_t &b = sgtc_buttons[sgtc_num_buttons];
         int kv = SGTC_EntKv(pButton), bi = sgtc_num_buttons, opens = 0;
         b.ent = pButton;
         strcpy(b.target, target);
         b.team = kv != -1 ? sgtc_ent_kv[kv].team : 0;
         b.needcard = kv != -1 ? sgtc_ent_kv[kv].needcard : 0;
         b.touch = (pButton->v.spawnflags & 256) != 0;  // touch activates (both classes)
         b.num_reach = 0;
         b.w = -1;
         b.num_wpt = 0;
         SGTC_ReachDoors(b, target, 0, kv != -1 ? sgtc_ent_kv[kv].delay : 0.0f, TRUE);
         for (int r = 0; r < b.num_reach; r++)
         {
            if (!b.reach_opens[r])
               continue;
            int i = SGTC_DoorOfPanel(b.reach[r]), k;
            if (i == -1)
               continue;
            sgtc_door_t &d = sgtc_doors[i];
            for (k = 0; k < d.num_buttons && d.button[k] != bi; k++)
               ;
            if (k == d.num_buttons && k < SGTC_DOOR_BUTTONS)
               d.button[d.num_buttons++] = bi;
            opens++;
         }
         if (opens)
            sgtc_num_buttons++;
      }
   }

   for (int i = 0; i < sgtc_num_doors; i++)
   {
      sgtc_door_t &d = sgtc_doors[i];
      const sgtc_panel_t &p0 = sgtc_panels[d.panel[0]];
      const char *name = STRING(d.door->v.targetname);
      Vector size = d.maxs - d.mins, center = (d.mins + d.maxs) * 0.5f;
      char desc[160];
      safevoid_snprintf(desc, sizeof(desc), "%s, %d panel%s, spawnflags %d, dmg %.0f, %s now", STRING(d.door->v.classname),
         d.num_panels, d.num_panels > 1 ? "s" : "", d.door->v.spawnflags, d.door->v.dmg, SGTC_DoorStateName(SGTC_DoorState(d)));
      if (!d.passage)
      {
         UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" at %.0f %.0f %.0f (%s): not a passage (%.0fx%.0fx%.0f), skipped\n", name,
            center.x, center.y, center.z, desc, size.x, size.y, size.z);
         continue;
      }

      // waypoint paths already through it?
      for (int w = 0; w < num_waypoints; w++)
      {
         if (waypoints[w].flags & (W_FL_DELETED | W_FL_AIMING))
            continue;
         int path_index = -1, n;
         while ((n = WaypointFindPath(path_index, w)) != -1)
            if (SGTC_DoorOnSegment(d, waypoints[w].origin, waypoints[n].origin))
               d.crossing++;
      }
      if (d.crossing)
         UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" at %.0f %.0f %.0f (%s): %d waypoint paths cross it\n", name,
            center.x, center.y, center.z, desc, d.crossing);
      else if (p0.start_open)
         UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" at %.0f %.0f %.0f (%s): no waypoint path through it, none added (start open)\n", name,
            center.x, center.y, center.z, desc);
      else
      {
         // standing positions 80 units out on both sides, along the thin axis
         Vector normal = size.x < size.y ? Vector(1, 0, 0) : Vector(0, 1, 0);
         Vector pa = center + normal * 80.0f, pb = center - normal * 80.0f, ga, gb;
         pa.z = pb.z = d.mins.z + 60.0f;
         int w_a = -1, w_b = -1;
         if (SGTC_GroundAt(pa, 120.0f, ga) && SGTC_GroundAt(pb, 120.0f, gb))
         {
            w_a = SGTC_WaypointAt(ga, 160.0f, 0);
            w_b = SGTC_WaypointAt(gb, 160.0f, 0);
         }
         if (w_a != -1 && w_b != -1 && w_a != w_b)
         {
            d.w_a = w_a;
            d.w_b = w_b;
            WaypointForceLongPath(w_a, w_b, 300.0f);
            WaypointForceLongPath(w_b, w_a, 300.0f);
            forced++;
            UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" at %.0f %.0f %.0f (%s): path wpt %d <-> %d\n", name,
               center.x, center.y, center.z, desc, w_a, w_b);
         }
         else
            UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" at %.0f %.0f %.0f (%s): no floor or waypoint on both sides\n", name,
               center.x, center.y, center.z, desc);
      }

      for (int k = 0; k < d.num_buttons; k++)
      {
         sgtc_button_t &b = sgtc_buttons[d.button[k]];
         if (!b.num_wpt)
            SGTC_ButtonWaypoints(b);
         Vector bc = SGTC_EntCenter(b.ent);
         char wbuf[128] = "";
         for (int j = 0; j < b.num_wpt; j++)
         {
            size_t len = strlen(wbuf);
            safevoid_snprintf(wbuf + len, sizeof(wbuf) - len, "%swpt %d (%.0f u from its box)", j ? ", " : "", b.wpt[j],
               SGTC_UseDist(b.ent, waypoints[b.wpt[j]].origin));
         }
         float delay = 0;
         char also[128] = "";
         for (int r = 0; r < b.num_reach; r++)
         {
            const sgtc_panel_t &rp = sgtc_panels[b.reach[r]];
            int own;
            for (own = 0; own < d.num_panels && d.panel[own] != b.reach[r]; own++)
               ;
            if (own < d.num_panels)
            {
               if (b.reach_opens[r] && b.reach_delay[r] > delay)
                  delay = b.reach_delay[r];
            }
            else if (!strstr(also, STRING(rp.ent->v.targetname)) && strlen(also) + strlen(STRING(rp.ent->v.targetname)) + 4 < sizeof(also))
            {
               strcat(also, also[0] ? ", " : "");
               strcat(also, STRING(rp.ent->v.targetname));
            }
         }
         UTIL_ConsolePrintf("[sgtc-stat] door \"%s\":   %s \"%s\" at %.0f %.0f %.0f: %s, %s, door moves at +%.1f s, target \"%s\", press from %s%s%s%s%s%s\n",
            name, STRING(b.ent->v.classname), STRING(b.ent->v.targetname), bc.x, bc.y, bc.z, b.touch ? "touch" : "use",
            b.team == 1 ? "Tau'ri only" : b.team == 2 ? "Goa'uld only" : "anyone", delay, b.target, wbuf[0] ? wbuf : "no waypoint",
            b.needcard ? ", needs a card: never pressed" : "", p0.start_open ? ", start open: only pressed to reopen it" : "",
            also[0] ? ", also moves \"" : "", also, also[0] ? "\"" : "");
      }
   }
   if (forced)
      WaypointRebuildRoutes();
}

static void SGTC_BuildGatePaths(void)
{
   Vector starts[8];

   // gates without a DHD: the button that dials them (before the doors skip it)
   for (int i = 0; i < sgtc_num_gates; i++)
      if (sgtc_gates[i].one_press)
         sgtc_gates[i].dhd = SGTC_GateDialer(sgtc_gates[i]);

   for (int i = 0; i < sgtc_num_gates; i++)
   {
      sgtc_gate_t &g = sgtc_gates[i];
      int n = 0;

      if (g.num_tele > 0)
      {
         // stand in front of / behind the event horizon: across the line of
         // its teleport triggers (or the thin side of a single trigger)
         Vector mins = g.tele[0]->v.origin + g.tele[0]->v.mins;
         Vector maxs = g.tele[0]->v.origin + g.tele[0]->v.maxs;
         Vector a = SGTC_EntCenter(g.tele[0]), b = a;
         for (int t = 1; t < g.num_tele; t++)
         {
            Vector c = SGTC_EntCenter(g.tele[t]);
            if ((c - a).Length2D() > (b - a).Length2D())
               b = c;
            Vector tmins = g.tele[t]->v.origin + g.tele[t]->v.mins;
            Vector tmaxs = g.tele[t]->v.origin + g.tele[t]->v.maxs;
            mins = Vector(fminf(mins.x, tmins.x), fminf(mins.y, tmins.y), fminf(mins.z, tmins.z));
            maxs = Vector(fmaxf(maxs.x, tmaxs.x), fmaxf(maxs.y, tmaxs.y), fmaxf(maxs.z, tmaxs.z));
         }
         Vector normal;
         if ((b - a).Length2D() > 32.0f)
         {
            Vector along = (b - a);
            along.z = 0;
            along = along.Normalize();
            normal = Vector(-along.y, along.x, 0);
         }
         else
            normal = (maxs.x - mins.x < maxs.y - mins.y) ? Vector(1, 0, 0) : Vector(0, 1, 0);

         Vector center = (mins + maxs) * 0.5f;
         for (int side = -1; side <= 1; side += 2)
         {
            Vector p = center + normal * (side * 52.0f);
            p.z = mins.z + 60.0f;  // just above the bottom of the ring
            if (SGTC_GroundAt(p, 160.0f, starts[n]) && !SGTC_CrossesGate(starts[n], starts[n]))
               n++;
         }
      }
      SGTC_LinkApproach("gate", g.number, starts, n);

      // the DHD: stand on any side of it
      if (!FNullEnt(g.dhd))
      {
         Vector c = SGTC_EntCenter(g.dhd);
         Vector half = (g.dhd->v.absmax - g.dhd->v.absmin) * 0.5f;
         static const float sides[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
         n = 0;
         for (int k = 0; k < 4; k++)
         {
            Vector p = c + Vector(sides[k][0] * (half.x + 36.0f), sides[k][1] * (half.y + 36.0f), 40.0f);
            if (SGTC_GroundAt(p, 160.0f, starts[n]))
               n++;
         }
         SGTC_LinkApproach("dhd", g.number, starts, n);
      }
   }

   // spawn points: a bot that finds no waypoint where it spawns stands still
   // (sg_nox: the Goa'uld spawn room has none)
   static const char *spawn_class[2] = {"info_player_torri", "info_player_goauld"};
   for (int c = 0; c < 2; c++)
   {
      edict_t *pent = NULL;
      int number = 0;
      while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", spawn_class[c])) && number < 16)
      {
         number++;
         if (SGTC_GroundAt(pent->v.origin + Vector(0, 0, 8), 120.0f, starts[0]))
            SGTC_LinkApproach(spawn_class[c] + 12, number, starts, 1);
      }
   }
}

//
// Coverage: a waypoint wherever bots can walk. The converted 2012 waypoints
// leave whole areas out (and most maps have none at all); a bot with no
// waypoint around it wanders blindly. At map start, flood fill the walkable
// floor on the same grid as the approach paths, from the spawn points,
// teleport destinations and existing waypoints (water, trigger_hurt and
// teleport volumes excluded), then add a waypoint wherever none is within
// SGTC_COV_SPACING. Spread over the server frames, a few ms each.
//
#define SGTC_COV_MAX_NODES  40000
#define SGTC_COV_HASH_SIZE  131072  // power of 2
#define SGTC_COV_SPACING    160.0f
#define SGTC_COV_BUDGET     0.004   // seconds of work per server frame

typedef struct
{
   Vector pos;
   short ix, iy, iz;
} sgtc_covnode_t;

static sgtc_covnode_t sgtc_cov[SGTC_COV_MAX_NODES];
static int sgtc_cov_hash[SGTC_COV_HASH_SIZE];  // node index + 1, 0 = free
static Vector sgtc_cov_new[MAX_WAYPOINTS];
static edict_t *sgtc_cov_hazard[128];
static int sgtc_cov_num_hazards;

// a new cell of the flood fill, FALSE if known already or out of room
static qboolean SGTC_CovAdd(const Vector &pos, int ix, int iy)
{
   int iz = (int)floorf(pos.z / 40.0f + 0.5f);
   unsigned int h = ((unsigned int)ix * 73856093u ^ (unsigned int)iy * 19349663u ^ (unsigned int)iz * 83492791u) & (SGTC_COV_HASH_SIZE - 1);

   while (sgtc_cov_hash[h])
   {
      sgtc_covnode_t &n = sgtc_cov[sgtc_cov_hash[h] - 1];
      if (n.ix == ix && n.iy == iy && n.iz == iz)
         return FALSE;
      h = (h + 1) & (SGTC_COV_HASH_SIZE - 1);
   }
   if (sgtc_obj.cov_num >= SGTC_COV_MAX_NODES)
      return FALSE;

   sgtc_covnode_t &n = sgtc_cov[sgtc_obj.cov_num];
   n.pos = pos;
   n.ix = ix;
   n.iy = iy;
   n.iz = iz;
   sgtc_cov_hash[h] = ++sgtc_obj.cov_num;
   return TRUE;
}

// the damaging trigger_hurts of the map (refreshed every 5 s: new map, freed edicts)
static edict_t *sgtc_hurts[32];
static int sgtc_num_hurts;
static float sgtc_hurts_time = -1000.0f;

static void SGTC_RefreshHurts(void)
{
   if (gpGlobals->time >= sgtc_hurts_time && gpGlobals->time < sgtc_hurts_time + 5.0f)
      return;
   sgtc_hurts_time = gpGlobals->time;
   sgtc_num_hurts = 0;
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "trigger_hurt")) && sgtc_num_hurts < 32)
      if (pent->v.dmg > 0)
         sgtc_hurts[sgtc_num_hurts++] = pent;
}

// a floor within 'depth' under p (a player origin), with no trigger_hurt in
// between? Water counts as a floor (no fall damage).
static qboolean SGTC_FloorUnder(const Vector &p, float depth)
{
   TraceResult tr;
   UTIL_TraceLine(p, p - Vector(0, 0, depth), ignore_monsters, NULL, &tr);
   if (tr.fStartSolid || tr.fAllSolid)
      return TRUE;  // in a step or a wall, not over a hole
   if (tr.flFraction >= 1.0f)
   {
      int c = POINT_CONTENTS(tr.vecEndPos);
      return c == CONTENTS_WATER || (c <= -9 && c >= -14);  // water, currents
   }
   SGTC_RefreshHurts();
   for (int i = 0; i < sgtc_num_hurts; i++)
   {
      edict_t *h = sgtc_hurts[i];
      Vector mins = h->v.origin + h->v.mins, maxs = h->v.origin + h->v.maxs;
      if (p.x >= mins.x && p.x <= maxs.x && p.y >= mins.y && p.y <= maxs.y &&
          tr.vecEndPos.z <= maxs.z && p.z >= mins.z)
         return FALSE;
   }
   return TRUE;
}

// Can a bot walk the straight line from a to b without passing over a void?
// Every 16 units (not the first/last 16) the floor under the line's centre
// must be at most 100 units under the lower end, unless there is floor 16
// units to both sides (a crack between planks: the hull stands across it).
// jk_botti's own test (WaypointReachable) only refuses floors rising more
// than a jump, and sees the floor with a 32-unit hull: it linked waypoints
// along sg_pyramide's atrium rim and across sg_target2a's void corners,
// where any sideways error is a fall. Same test offline: voidpaths.py.
qboolean SGTC_PathSafe(const Vector &a, const Vector &b)
{
   Vector d = b - a;
   float len = sqrtf(d.x * d.x + d.y * d.y);
   if (len < 40.0f)
      return TRUE;
   Vector side(-d.y / len * 16.0f, d.x / len * 16.0f, 0);
   float low = a.z < b.z ? a.z : b.z;
   for (float t = 16.0f; t <= len - 16.0f; t += 16.0f)
   {
      Vector p = a + d * (t / len);
      float depth = (p.z - low) + 100.0f;
      if (SGTC_FloorUnder(p, depth))
         continue;
      if (SGTC_FloorUnder(p + side, depth) && SGTC_FloorUnder(p - side, depth))
         continue;
      return FALSE;
   }
   return TRUE;
}

// a missed jump over this gap is a death: deeper than fall damage starts
// (200 under the feet) or over a trigger_hurt
static qboolean SGTC_GapRisky(const Vector &from, const Vector &to)
{
   for (int k = 1; k <= 3; k++)
      if (!SGTC_FloorUnder(from + (to - from) * (k * 0.25f), 236.0f))
         return TRUE;
   return FALSE;
}

// water, damage or teleport volume at this standing position?
static qboolean SGTC_CovHazard(const Vector &p)
{
   // wading is fine (sg_crypt's canals are waist deep), deep water, slime and lava aren't
   int contents = POINT_CONTENTS(p - Vector(0, 0, 30));
   if (contents == CONTENTS_SLIME || contents == CONTENTS_LAVA)
      return TRUE;
   if (POINT_CONTENTS(p + Vector(0, 0, 28)) == CONTENTS_WATER)
      return TRUE;

   for (int i = 0; i < sgtc_cov_num_hazards; i++)
   {
      edict_t *pent = sgtc_cov_hazard[i];
      Vector mins = pent->v.origin + pent->v.mins - Vector(16, 16, 36);
      Vector maxs = pent->v.origin + pent->v.maxs + Vector(16, 16, 36);
      if (p.x >= mins.x && p.x <= maxs.x && p.y >= mins.y && p.y <= maxs.y && p.z >= mins.z && p.z <= maxs.z)
         return TRUE;
   }
   return FALSE;
}

static void SGTC_CovSeed(const Vector &p)
{
   Vector ground;
   if (!SGTC_GroundAt(p + Vector(0, 0, 8), 160.0f, ground) && !SGTC_GroundAt(p + Vector(0, 0, 40), 200.0f, ground))
      return;
   if (!SGTC_CovHazard(ground))
      SGTC_CovAdd(ground, (int)floorf(ground.x / SGTC_GRID + 0.5f), (int)floorf(ground.y / SGTC_GRID + 0.5f));
}

// a waypoint (existing or picked already) close by and in sight?
static qboolean SGTC_CovNearWaypoint(const Vector &p)
{
   TraceResult tr;

   for (int i = 0; i < num_waypoints + sgtc_obj.cov_num_new; i++)
   {
      const Vector &w = i < num_waypoints ? waypoints[i].origin : sgtc_cov_new[i - num_waypoints];
      if (i < num_waypoints && (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING)))
         continue;
      if (fabs(w.z - p.z) > 64.0f || (w - p).Length() > SGTC_COV_SPACING)
         continue;
      UTIL_TraceLine(p, w, ignore_monsters, NULL, &tr);
      if (tr.flFraction >= 1.0f)
         return TRUE;
   }
   return FALSE;
}

// Gaps a running jump clears (sg_crypt's broken bridge over the pit is the
// only way to the Goa'uld spawn): from the last floor in that direction,
// floor again 40-128 units further, no higher than a step, nothing in the
// way at jump height. The flood fill goes on from the landing, and a W_FL_JUMP
// waypoint at the edge gets a forced path to it (jk_botti jumps there).
#define SGTC_MAX_JUMPS 256
static struct { Vector from, to; int risky; } sgtc_jumps[SGTC_MAX_JUMPS];  // risky: 1 deep gap, 2 skipped

static void SGTC_CovTryJump(const sgtc_covnode_t &from, int dx, int dy)
{
   Vector dir = Vector((float)dx, (float)dy, 0).Normalize();
   Vector edge = from.pos, ground;
   TraceResult tr;

   // to the edge, at most 64 units away
   int k;
   for (k = 0; k < 8; k++)
   {
      Vector next = edge + dir * 8.0f;
      UTIL_TraceHull(edge, next, ignore_monsters, human_hull, NULL, &tr);
      if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
         return;  // a wall, not a gap
      if (!SGTC_GroundAt(next + Vector(0, 0, 18), 18.0f + 24.0f, ground))
         break;
      edge = ground;
   }
   if (k == 8)
      return;

   for (float s = 40.0f; s <= 128.0f; s += 8.0f)
   {
      if (!SGTC_GroundAt(edge + dir * s + Vector(0, 0, 18), 18.0f + 24.0f, ground) || ground.z > edge.z + 18.0f)
         continue;

      // land on a real floor, not on a plank or the handrail of a broken
      // bridge (sg_crypt: bots kept falling into the pit). A player hull
      // stands on a 6-unit rail, so test points: floor under the landing 48
      // units past where the hull first finds support (the hull overhangs the
      // edge by up to 16) and 24 units around it, and behind the takeoff.
      // Further on if that fails (sg_crypt: a plank before the far side).
      static const float offs[5][2] = {{0, 0}, {24, 0}, {-24, 0}, {0, 24}, {0, -24}};
      Vector land = ground + dir * 48.0f;
      if (!SGTC_GroundAt(land + Vector(0, 0, 18), 18.0f + 24.0f, ground))
         continue;
      // ... and flat there: all at the height the hull stands on, ±8
      // (sg_crypt: "jumps" onto the 15-unit handrail next to the pit).
      // Measured: ±18 around / ±6 under the center kept a different set of
      // jumps on sg_crypt, without the bridge's south-to-north one, and the
      // Goa'uld never left their side (0-5 kills per 10 minutes, against
      // 10-22 with ±8)
      int o;
      for (o = 0; o < 5; o++)
      {
         Vector p = ground + Vector(offs[o][0], offs[o][1], 0);
         UTIL_TraceLine(p, p - Vector(0, 0, 36 + 24), ignore_monsters, NULL, &tr);
         if (tr.flFraction >= 1.0f || fabs(tr.vecEndPos.z - (ground.z - 36.0f)) > 8.0f)
            break;
      }
      if (o < 5)
         continue;
      // the takeoff too, behind the edge (the hull overhangs it by up to 16)
      // and on both sides: not from a handrail
      Vector side(-dir.y, dir.x, 0), back = edge - dir * 32.0f;
      Vector takeoff[3] = {back, back + side * 24.0f, back - side * 24.0f};
      for (o = 0; o < 3; o++)
      {
         UTIL_TraceLine(takeoff[o], takeoff[o] - Vector(0, 0, 36 + 24), ignore_monsters, NULL, &tr);
         if (tr.flFraction >= 1.0f || fabs(tr.vecEndPos.z - (edge.z - 36.0f)) > 8.0f)
            return;
      }
      UTIL_TraceHull(edge + Vector(0, 0, 40), ground + Vector(0, 0, 40), ignore_monsters, human_hull, NULL, &tr);
      if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f || SGTC_CovHazard(ground) || SGTC_CrossesGate(edge, ground))
         return;

      qboolean known = FALSE;
      for (int j = 0; j < sgtc_obj.cov_num_jumps && !known; j++)
         if ((sgtc_jumps[j].from - edge).Length() < 96.0f &&
             DotProduct((sgtc_jumps[j].to - sgtc_jumps[j].from).Normalize(), dir) > 0.7f)
            known = TRUE;
      if (!known && sgtc_obj.cov_num_jumps < SGTC_MAX_JUMPS)
      {
         sgtc_jumps[sgtc_obj.cov_num_jumps].from = edge;
         sgtc_jumps[sgtc_obj.cov_num_jumps].to = ground;
         sgtc_jumps[sgtc_obj.cov_num_jumps].risky = SGTC_GapRisky(edge, ground) ? 1 : 0;
         sgtc_obj.cov_num_jumps++;
      }
      SGTC_CovAdd(ground, from.ix + (int)floorf((ground.x - from.pos.x) / SGTC_GRID + 0.5f),
                  from.iy + (int)floorf((ground.y - from.pos.y) / SGTC_GRID + 0.5f));
      return;
   }
}

static int sgtc_bfs_queue[MAX_WAYPOINTS];
static unsigned char sgtc_bfs_seen[MAX_WAYPOINTS];

// can a bot get from waypoint 'from' to 'to' on the paths there are?
static qboolean SGTC_GraphReaches(int from, int to)
{
   if (from < 0 || to < 0 || from >= num_waypoints || to >= num_waypoints)
      return FALSE;
   memset(sgtc_bfs_seen, 0, sizeof(sgtc_bfs_seen));
   int head = 0, tail = 0;
   sgtc_bfs_queue[tail++] = from;
   sgtc_bfs_seen[from] = 1;
   while (head < tail)
   {
      int u = sgtc_bfs_queue[head++];
      if (u == to)
         return TRUE;
      int path_index = -1, v;
      while ((v = WaypointFindPath(path_index, u)) != -1)
         if (v >= 0 && v < num_waypoints && !sgtc_bfs_seen[v] && !(waypoints[v].flags & W_FL_DELETED))
         {
            sgtc_bfs_seen[v] = 1;
            sgtc_bfs_queue[tail++] = v;
         }
   }
   return FALSE;
}

// the nearest waypoint within 'radius' of p, in sight, at about its height
static int SGTC_CovNearestWpt(const Vector &p, float radius)
{
   int best = -1;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER))
         continue;
      float d = (waypoints[i].origin - p).Length();
      if (d >= radius || fabs(waypoints[i].origin.z - p.z) > 40.0f)
         continue;
      TraceResult tr;
      UTIL_TraceLine(p, waypoints[i].origin, ignore_monsters, NULL, &tr);
      if (tr.flFraction < 1.0f)
         continue;
      radius = d;
      best = i;
   }
   return best;
}

// the jumps found by the flood fill become W_FL_JUMP waypoints with a forced
// path to a waypoint at the landing. A jump over a deep or deadly gap only
// where there is no way around (sg_pyramide: two jumps across the 1000-unit
// rope shaft, with the floor all around it). Each one gets a straight run-up,
// 96 units behind the edge, the only way to the edge: bots that reached
// sg_crypt's broken-bridge takeoff at an angle jumped into the pit.
static int SGTC_CovAddJumps(void)
{
   int added = 0;

   for (int j = 0; j < sgtc_obj.cov_num_jumps; j++)
   {
      if (sgtc_jumps[j].risky)
      {
         int a = SGTC_CovNearestWpt(sgtc_jumps[j].from, 96.0f), b = SGTC_CovNearestWpt(sgtc_jumps[j].to, 96.0f);
         if (a != -1 && b != -1 && SGTC_GraphReaches(a, b))
         {
            sgtc_jumps[j].risky = 2;
            continue;
         }
      }

      int land = -1;
      float best = 48.0f;
      for (int i = 0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
            continue;
         float d = (waypoints[i].origin - sgtc_jumps[j].to).Length();
         if (d < best && fabs(waypoints[i].origin.z - sgtc_jumps[j].to.z) < 24.0f)
         {
            best = d;
            land = i;
         }
      }
      if (land == -1)
         land = WaypointAddOne(sgtc_jumps[j].to, 0);
      int edge = WaypointAddOne(sgtc_jumps[j].from, W_FL_JUMP);
      if (land == -1 || edge == -1)
         break;
      WaypointForcePath(edge, land);
      added++;

      Vector dir = sgtc_jumps[j].to - sgtc_jumps[j].from, run;
      dir.z = 0;
      if (dir.Length() < 1.0f)
         continue;
      dir = dir.Normalize();
      if (!SGTC_WalkStep(sgtc_jumps[j].from, dir * -96.0f, run) || fabs(run.z - sgtc_jumps[j].from.z) > 18.0f)
         continue;
      int r = WaypointAddOne(run, 0);
      if (r == -1)
         continue;
      for (int i = 0; i < num_waypoints; i++)
         if (i != r)
            WaypointCutPath(i, edge);
      WaypointForcePath(r, edge);
   }
   return added;
}

// at the edge of a drop deeper than 100 units? Bots aren't precise enough to
// walk there (sg_crypt: waypoints along a pit sent them into its trigger_hurt)
static qboolean SGTC_CovNearLedge(const Vector &p)
{
   static const float dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {0.7f, 0.7f}, {0.7f, -0.7f}, {-0.7f, 0.7f}, {-0.7f, -0.7f}};
   TraceResult tr;
   Vector ground;

   for (int d = 0; d < 8; d++)
   {
      Vector side = p + Vector(dirs[d][0] * 40.0f, dirs[d][1] * 40.0f, 0);
      UTIL_TraceHull(p, side, ignore_monsters, human_hull, NULL, &tr);
      if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
         continue;  // wall on that side
      if (!SGTC_GroundAt(side, 100.0f, ground))
         return TRUE;
   }
   return FALSE;
}

// for offline analysis (connectivity, where the graph breaks):
// "index x y z flags path path ..." per waypoint, then the spawn points
static void SGTC_DumpWaypointGraph(void)
{
   char filename[256], name[80];
   safevoid_snprintf(name, sizeof(name), "%s_graph.txt", STRING(gpGlobals->mapname));
   UTIL_BuildFileName_N(filename, sizeof(filename), "addons/jk_botti/waypoints", name);

   FILE *fp = fopen(filename, "w");
   if (!fp)
      return;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;
      fprintf(fp, "w %d %.0f %.0f %.0f %x", i, waypoints[i].origin.x, waypoints[i].origin.y, waypoints[i].origin.z, waypoints[i].flags);
      int path_index = -1, p;
      while ((p = WaypointFindPath(path_index, i)) != -1)
         fprintf(fp, " %d", p);
      fprintf(fp, "\n");
   }
   static const char *spawn_class[2] = {"info_player_torri", "info_player_goauld"};
   for (int c = 0; c < 2; c++)
   {
      edict_t *pent = NULL;
      while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", spawn_class[c])))
         fprintf(fp, "s %d %.0f %.0f %.0f\n", c + 1, pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
   }
   fclose(fp);
   UTIL_ConsolePrintf("[sgtc-stat] waypoint graph written to %s\n", filename);
}

// "sgtc_probe x1,y1,z1 x2,y2,z2": ground below the segment every 16 units
// (depth, contents at feet/head), to see why two places aren't linked
void SGTC_Probe(const Vector &a, const Vector &b)
{
   Vector dir = b - a;
   float len = dir.Length();
   if (len < 1.0f)
      return;
   dir = dir * (1.0f / len);

   static const int hulls[3] = {human_hull, head_hull, point_hull};
   for (int h = 0; h < 3; h++)
   {
      TraceResult tr;
      UTIL_TraceHull(a, b, ignore_monsters, hulls[h], NULL, &tr);
      UTIL_ConsolePrintf("[sgtc-stat] probe hull %d: fraction %.2f startsolid=%d stops at %.0f %.0f %.0f on %s (%s)\n",
         hulls[h], tr.flFraction, tr.fStartSolid, tr.vecEndPos.x, tr.vecEndPos.y, tr.vecEndPos.z,
         FNullEnt(tr.pHit) ? "world" : STRING(tr.pHit->v.classname),
         FNullEnt(tr.pHit) ? "" : STRING(tr.pHit->v.model));
   }

   UTIL_ConsolePrintf("[sgtc-stat] probe reachable a->b: %d, b->a: %d (0 yes, 1 far, 2 blocked, 3 mid-air, 4 ground path)\n",
      WaypointReachableWhy(a, b), WaypointReachableWhy(b, a));

   for (float t = 0; t <= len; t += 16.0f)
   {
      Vector p = a + dir * t;
      TraceResult tr;
      UTIL_TraceHull(p, p - Vector(0, 0, 1000), ignore_monsters, human_hull, NULL, &tr);
      UTIL_ConsolePrintf("[sgtc-stat] probe %.0f %.0f %.0f: ground %s%.0f below (z %.0f) feet=%d head=%d\n",
         p.x, p.y, p.z, tr.fStartSolid ? "startsolid " : "", tr.flFraction * 1000.0f, tr.vecEndPos.z,
         POINT_CONTENTS(p - Vector(0, 0, 30)), POINT_CONTENTS(p + Vector(0, 0, 28)));
   }
}

// "sgtc_wptdump" run after the build (jk_botti.cfg runs late): dump now
void SGTC_WptDumpCommand(void)
{
   if (sgtc_obj.paths_built)
      SGTC_DumpWaypointGraph();
}

//
// Ladders: of the converted waypoints, only sg_chateau's have ladder
// waypoints. For every other func_ladder: W_FL_LADDER waypoints every 64
// units up its climbing side (the side where a player hull fits all along
// it, 17 units off the brush) from the floor at its foot, and a waypoint
// where the player steps off at the top: over the ladder (a ladder on a
// wall) or on the climbing side (a ladder through a hatch). jk_botti's
// ladder code climbs from one ladder waypoint to the next.
//
static int SGTC_BuildLadders(void)
{
   static const float sides[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
   int built = 0;
   edict_t *pent = NULL;

   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "func_ladder")))
   {
      // not linked: absmin/absmax may be empty
      Vector mins = pent->v.origin + pent->v.mins, maxs = pent->v.origin + pent->v.maxs;
      Vector center = (mins + maxs) * 0.5f, size = maxs - mins;
      if (size.z < 48.0f)
         continue;

      // waypointed already (sg_chateau)
      qboolean known = FALSE;
      for (int i = 0; i < num_waypoints && !known; i++)
      {
         const Vector &o = waypoints[i].origin;
         if ((waypoints[i].flags & W_FL_LADDER) && !(waypoints[i].flags & W_FL_DELETED) &&
             o.x > mins.x - 48 && o.x < maxs.x + 48 && o.y > mins.y - 48 && o.y < maxs.y + 48 &&
             o.z > mins.z - 48 && o.z < maxs.z + 96)
            known = TRUE;
      }
      if (known)
         continue;

      // the climbing side: where a hull fits at most heights (the other
      // sides are in the wall); a tie goes to the brush's thin axis
      int side = -1, best = 0;
      for (int s = 0; s < 4; s++)
      {
         Vector n(sides[s][0], sides[s][1], 0);
         float half = n.x != 0 ? size.x * 0.5f : size.y * 0.5f;
         Vector p = center + n * (half + 17.0f);
         int free = 0;
         for (float z = mins.z + 36.0f; z <= maxs.z; z += 32.0f)
         {
            TraceResult tr;
            p.z = z;
            UTIL_TraceHull(p, p + Vector(0, 0, 1), ignore_monsters, human_hull, NULL, &tr);
            if (!tr.fStartSolid && !tr.fAllSolid)
               free += 2;
         }
         if ((n.x != 0) == (size.x < size.y))
            free++;
         if (free > best)
         {
            best = free;
            side = s;
         }
      }
      if (side == -1)
         continue;

      Vector n(sides[side][0], sides[side][1], 0);
      float half = n.x != 0 ? size.x * 0.5f : size.y * 0.5f;
      Vector away[4] = {n * -1.0f, n, Vector(-n.y, n.x, 0), Vector(n.y, -n.x, 0)};
      Vector front, foot, exit;
      qboolean found = FALSE;

      // the climbing column: in front of the brush, or closer, or inside a
      // thick one (sg_land3: a sloped ceiling over the front of a 39-unit
      // ladder stopped the bots at its top). It needs a floor at its foot
      // close enough to grab the ladder, a clear climb, and a way off at the
      // top: a floor near the top of the ladder over it, back on the
      // climbing side or to a side, in the open from the top of the column.
      static const float column[5] = {17.0f, 8.0f, 0.0f, -8.0f, -16.0f};
      for (int c = 0; c < 5 && !found; c++)
      {
         front = center + n * (half + column[c]);
         if (!SGTC_GroundAt(Vector(front.x, front.y, mins.z + 72.0f), 160.0f, foot) || foot.z + 36.0f < mins.z)
            continue;

         for (int k = 0; k < 4 && !found; k++)
            for (float d = 24.0f; d <= 64.0f && !found; d += 8.0f)
            {
               Vector from = (k == 0 ? center - n * half : front) + away[k] * d;
               for (int h = 0; h < 3 && !found; h++)
               {
                  float z0 = maxs.z + (h == 0 ? 54.0f : (h == 1 ? 38.0f : 20.0f));
                  if (!SGTC_GroundAt(Vector(from.x, from.y, z0), z0 - (maxs.z - 60.0f), exit) ||
                      exit.z < maxs.z - 24.0f || exit.z < foot.z + 48.0f)
                     continue;
                  TraceResult tr;
                  UTIL_TraceHull(Vector(front.x, front.y, exit.z + 2.0f), exit + Vector(0, 0, 2), ignore_monsters, human_hull, NULL, &tr);
                  if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
                     continue;
                  UTIL_TraceHull(foot + Vector(0, 0, 2), Vector(front.x, front.y, exit.z + 2.0f), ignore_monsters, human_hull, NULL, &tr);
                  if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction >= 1.0f)
                     found = TRUE;
               }
            }
      }
      if (!found)
      {
         UTIL_ConsolePrintf("[sgtc-stat] ladder at %.0f %.0f %.0f..%.0f: no clear climb with a way off at the top (climbing side %.0f %.0f)\n",
            center.x, center.y, mins.z, maxs.z, n.x, n.y);
         continue;
      }

      // foot, up the ladder, top
      int prev = WaypointAddOne(foot, W_FL_LADDER), count = 1;
      float top = exit.z - 40.0f;
      for (float z = foot.z + 64.0f; prev != -1; z += 64.0f)
      {
         if (z > top)
            z = top;
         int w = WaypointAddOne(Vector(front.x, front.y, z), W_FL_LADDER);
         if (w == -1)
            break;
         WaypointForcePath(prev, w);
         WaypointForcePath(w, prev);
         prev = w;
         count++;
         if (z >= top)
            break;
      }
      int w_exit = prev != -1 ? WaypointAddOne(exit, 0) : -1;
      if (w_exit == -1)
         break;
      WaypointForcePath(prev, w_exit);
      WaypointForcePath(w_exit, prev);
      built++;
      UTIL_ConsolePrintf("[sgtc-stat] ladder at %.0f %.0f %.0f..%.0f: %d ladder waypoints from %.0f %.0f %.0f, off at %.0f %.0f %.0f\n",
         center.x, center.y, mins.z, maxs.z, count, foot.x, foot.y, foot.z, exit.x, exit.y, exit.z);
   }
   return built;
}

// on a ladder with no wall behind it: turn to the func_ladder being touched
qboolean SGTC_FaceLadder(edict_t *pEdict)
{
   const Vector &o = pEdict->v.origin;
   edict_t *pent = NULL;

   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "func_ladder")))
   {
      Vector mins = pent->v.origin + pent->v.mins, maxs = pent->v.origin + pent->v.maxs;
      if (o.x < mins.x - 24 || o.x > maxs.x + 24 || o.y < mins.y - 24 || o.y > maxs.y + 24 ||
          o.z < mins.z - 40 || o.z > maxs.z + 40)
         continue;

      // the nearest point of the brush, horizontally
      Vector p(o.x < mins.x ? mins.x : (o.x > maxs.x ? maxs.x : o.x),
               o.y < mins.y ? mins.y : (o.y > maxs.y ? maxs.y : o.y), o.z);
      Vector to = p - o;
      if (to.Length() < 1.0f)
         to = (mins + maxs) * 0.5f - o;
      to.z = 0;
      if (to.Length() < 1.0f)
         return FALSE;
      pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
      BotFixIdealYaw(pEdict);
      return TRUE;
   }
   return FALSE;
}

// the func_ladder a player at o is on (its hull overlaps the ladder brush)
static qboolean SGTC_LadderBox(const Vector &o, Vector &mins, Vector &maxs)
{
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", "func_ladder")))
   {
      Vector mn = pent->v.origin + pent->v.mins, mx = pent->v.origin + pent->v.maxs;
      if (o.x < mn.x - 17 || o.x > mx.x + 17 || o.y < mn.y - 17 || o.y > mx.y + 17 ||
          o.z < mn.z - 37 || o.z > mx.z + 37)
         continue;
      mins = mn;
      maxs = mx;
      return TRUE;
   }
   return FALSE;
}

// a floor within 'depth' under a standing player at p (or p in a wall)?
static qboolean SGTC_HullFloor(const Vector &p, float depth, edict_t *pIgnore)
{
   TraceResult tr;
   UTIL_TraceHull(p, p - Vector(0, 0, depth), ignore_monsters, human_hull, pIgnore, &tr);
   return tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f;
}

//
// On a ladder, right before the move (sg_pyramide's ropes and poles are 8-24
// units wide over shafts 1000-2000 units deep: 18-30 falls per 10 minutes):
// - no jump off it high above the floor: jk_botti jumps off a ladder when it
//   shoots, the watchdog when stuck, and HL throws the player 270 u/s away;
// - face the ladder brush squarely, now: HL's ladder move keeps the part of
//   the view direction along the ladder face, so a bot aiming at an enemy or
//   still turning slides sideways off a thin pole;
// - up or down from the next waypoint's height, not from jk_botti's guess;
// - at the height of the way off (a non-ladder waypoint): head for it, or
//   jump off toward it when it is behind the bot, a bit above its floor;
// - never climb off the top of a ladder with no floor ahead (sg_pyramide's
//   rope A goes on 180 units above its way off, over the atrium).
//
void SGTC_LadderGuard(bot_t &pBot, unsigned short &buttons)
{
   edict_t *pEdict = pBot.pEdict;
   if (pEdict->v.movetype != MOVETYPE_FLY || (pEdict->v.flags & FL_ONGROUND))
      return;
   const Vector o = pEdict->v.origin;
   Vector mins, maxs;
   if (!SGTC_LadderBox(o, mins, maxs))
      return;

   if ((buttons & IN_JUMP) && !SGTC_HullFloor(o, 120.0f, pEdict))
      buttons &= ~IN_JUMP;

   // the nearest point of the ladder brush, and the side the bot is on
   Vector nearest(o.x < mins.x ? mins.x : (o.x > maxs.x ? maxs.x : o.x),
               o.y < mins.y ? mins.y : (o.y > maxs.y ? maxs.y : o.y), o.z);
   Vector to = nearest - o;
   to.z = 0;
   if (to.Length() < 1.0f)
   {
      to = (mins + maxs) * 0.5f - o;
      to.z = 0;
   }
   if (to.Length() < 1.0f)
      return;
   Vector normal = to.Normalize() * -1.0f;  // from the ladder toward the bot

   float yaw = UTIL_VecToAngles(to).y;
   float pitch = (pBot.ladder_dir == LADDER_DOWN) ? 60.0f : -60.0f;
   int w = pBot.curr_waypoint_index;
   if (w >= 0 && w < num_waypoints)
   {
      const Vector &wo = waypoints[w].origin;
      float dz = wo.z - o.z;
      if (waypoints[w].flags & W_FL_LADDER)
      {
         if (dz > 16.0f)
            pitch = -60.0f;
         else if (dz < -16.0f)
            pitch = 60.0f;
      }
      else if (dz < -64.0f)
         pitch = 60.0f;
      else
      {
         // the way off the ladder
         Vector off = wo - o;
         off.z = 0;
         float dist = off.Length();
         Vector dir = dist > 1.0f ? off * (1.0f / dist) : normal;
         if (DotProduct(dir, normal) > 0.5f)
         {
            // behind the bot: get 24 units above it, then jump off toward it
            pitch = -60.0f;
            if (dz <= -24.0f && dist < 160.0f)
            {
               TraceResult tr;
               UTIL_TraceHull(o, o + normal * dist, ignore_monsters, human_hull, pEdict, &tr);
               if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction >= 1.0f)
               {
                  buttons |= IN_JUMP;
                  BotTrace(pBot, "sgtc ladder: jump off to wpt=%d (%.0f units behind)", w, dist);
               }
            }
         }
         else if (dz > 24.0f)
            pitch = -60.0f;
         else
         {
            // beside or beyond the ladder: head for it
            yaw = UTIL_VecToAngles(off).y;
            pitch = 0;
         }
      }
   }

   // about to climb off the top: only onto a floor ahead
   if (pitch < 0 && o.z > maxs.z + 16.0f)
   {
      Vector fwd(cosf(yaw * (M_PI / 180.0f)), sinf(yaw * (M_PI / 180.0f)), 0);
      if (!SGTC_HullFloor(o + fwd * 48.0f + Vector(0, 0, 8), 104.0f, pEdict) &&
          !SGTC_HullFloor(o + fwd * 64.0f + Vector(0, 0, 8), 104.0f, pEdict))
      {
         buttons &= ~IN_FORWARD;
         pitch = 0;
      }
      else
         buttons |= IN_FORWARD;
   }
   else if (pitch != 0 || !(buttons & IN_JUMP))
      buttons |= IN_FORWARD;

   if (pitch > 0)
      pBot.ladder_dir = LADDER_DOWN;
   else if (pitch < 0)
      pBot.ladder_dir = LADDER_UP;
   pEdict->v.v_angle.y = yaw;
   pEdict->v.v_angle.x = pitch;
   pEdict->v.ideal_yaw = yaw;
}

// returns TRUE once done
static qboolean SGTC_CoverageThink(void)
{
   static const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
   double end = UTIL_GetSecs() + SGTC_COV_BUDGET;

   switch (sgtc_obj.cov_state)
   {
   case 0:  // hazards and seeds
   {
      memset(sgtc_cov_hash, 0, sizeof(sgtc_cov_hash));
      sgtc_obj.cov_num = sgtc_obj.cov_next = sgtc_obj.cov_num_new = sgtc_obj.cov_added = 0;
      sgtc_obj.cov_num_jumps = 0;
      sgtc_obj.f_cov_secs = UTIL_GetSecs();

      // before the seeds: the floor at the top of a ladder gets covered too
      SGTC_BuildLadders();

      static const char *hazards[2] = {"trigger_hurt", "trigger_teleport"};
      sgtc_cov_num_hazards = 0;
      for (int c = 0; c < 2; c++)
      {
         edict_t *pent = NULL;
         while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", hazards[c])) && sgtc_cov_num_hazards < 128)
            if (c != 0 || pent->v.dmg > 0)  // not a healing trigger_hurt
               sgtc_cov_hazard[sgtc_cov_num_hazards++] = pent;
      }
      SGTC_TransitHazards();

      static const char *seeds[3] = {"info_player_torri", "info_player_goauld", "info_teleport_destination"};
      for (int c = 0; c < 3; c++)
      {
         edict_t *pent = NULL;
         while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", seeds[c])))
            SGTC_CovSeed(pent->v.origin);
      }
      for (int i = 0; i < num_waypoints; i++)
         if (!(waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING)))
            SGTC_CovSeed(waypoints[i].origin);

      sgtc_obj.cov_state = 1;
      return FALSE;
   }

   case 1:  // flood fill
      while (sgtc_obj.cov_next < sgtc_obj.cov_num)
      {
         if (UTIL_GetSecs() > end)
            return FALSE;

         sgtc_covnode_t from = sgtc_cov[sgtc_obj.cov_next++];
         for (int d = 0; d < 8; d++)
         {
            Vector next;
            Vector move(dirs[d][0] * SGTC_GRID, dirs[d][1] * SGTC_GRID, 0);
            if (!SGTC_WalkStep(from.pos, move, next))
            {
               SGTC_CovTryJump(from, dirs[d][0], dirs[d][1]);
               continue;
            }
            if (SGTC_CrossesGate(from.pos, next) || SGTC_CovHazard(next))
               continue;
            SGTC_CovAdd(next, from.ix + dirs[d][0], from.iy + dirs[d][1]);
         }
      }
      sgtc_obj.cov_next = 0;
      sgtc_obj.cov_state = 2;
      return FALSE;

   case 2:  // the cells that get a waypoint
   {
      int room = MAX_WAYPOINTS - 64 - num_waypoints;
      while (sgtc_obj.cov_next < sgtc_obj.cov_num && sgtc_obj.cov_num_new < room)
      {
         if (UTIL_GetSecs() > end)
            return FALSE;

         const Vector &p = sgtc_cov[sgtc_obj.cov_next++].pos;
         if (!SGTC_CovNearWaypoint(p) && !SGTC_CovNearLedge(p))
            sgtc_cov_new[sgtc_obj.cov_num_new++] = p;
      }
      sgtc_obj.cov_state = 3;
      return FALSE;
   }

   case 3:  // add them, a few at a time (jk_botti traces paths to all neighbours)
      while (sgtc_obj.cov_added < sgtc_obj.cov_num_new)
      {
         if (UTIL_GetSecs() > end)
            return FALSE;

         int n = sgtc_obj.cov_num_new - sgtc_obj.cov_added;
         if (n > 4)
            n = 4;
         WaypointAddChain(&sgtc_cov_new[sgtc_obj.cov_added], n, FALSE, FALSE);
         sgtc_obj.cov_added += n;
      }
   {
      // and the paths the converted waypoints never had between themselves,
      // before the jumps: one over a deep gap is kept only with no way around
      int repaired = WaypointRepairPaths();
      int jumps = SGTC_CovAddJumps();
      if (sgtc_obj.cov_num_new || repaired || jumps)
         WaypointRebuildRoutes();
      UTIL_ConsolePrintf("[sgtc-stat] coverage: %d cells%s, %d waypoints added, %d jumps, %d paths repaired, %.1f s\n",
         sgtc_obj.cov_num, sgtc_obj.cov_num >= SGTC_COV_MAX_NODES ? " (limit)" : "",
         sgtc_obj.cov_num_new, jumps, repaired, UTIL_GetSecs() - sgtc_obj.f_cov_secs);
      for (int j = 0; j < sgtc_obj.cov_num_jumps; j++)
         UTIL_ConsolePrintf("[sgtc-stat]   jump %.0f %.0f %.0f -> %.0f %.0f %.0f%s\n", sgtc_jumps[j].from.x, sgtc_jumps[j].from.y,
            sgtc_jumps[j].from.z, sgtc_jumps[j].to.x, sgtc_jumps[j].to.y, sgtc_jumps[j].to.z,
            sgtc_jumps[j].risky == 2 ? " skipped: deep gap, walkable around" :
            (sgtc_jumps[j].risky ? " (deep gap, the only way)" : ""));
   }
      sgtc_obj.cov_state = 4;
      return TRUE;

   default:
      return TRUE;
   }
}

// inside the kawoosh of a gate (with the player hull)?
static qboolean SGTC_InGateZone(const Vector &p)
{
   for (int i = 0; i < sgtc_num_gates; i++)
      for (int z = 0; z < sgtc_gates[i].num_zones; z++)
      {
         edict_t *pZone = sgtc_gates[i].zone[z];
         Vector mins = pZone->v.origin + pZone->v.mins - Vector(16, 16, 36);
         Vector maxs = pZone->v.origin + pZone->v.maxs + Vector(16, 16, 36);
         if (p.x >= mins.x && p.x <= maxs.x && p.y >= mins.y && p.y <= maxs.y && p.z >= mins.z && p.z <= maxs.z)
            return TRUE;
      }
   return SGTC_InTeleport(p);
}

static qboolean SGTC_Reachable(bot_t &pBot, int index)
{
   return pBot.curr_waypoint_index == -1 || index == -1 ||
          WaypointDistanceFromTo(pBot.curr_waypoint_index, index) < WAYPOINT_MAX_DISTANCE;
}

//
// The goal can't be walked to from here: find a gate on our side of the map.
// Attackers: closed -> go dial its DHD; open -> walk into the wormhole.
// Defenders: the enemy has to come through that gate, guard it (on sg_nox the
// Tau'ri must keep the Goa'uld from their gate; dialing it would let them in).
// Returns the waypoint to use instead, or -1 (keep the original goal).
//
static int SGTC_GateDetour(bot_t &pBot, int goal, const char **why)
{
   if (SGTC_Reachable(pBot, goal) || !sgtc_num_gates)
      return -1;

   int side = SGTC_GetPlayerTeam(pBot.pEdict);
   qboolean defender = sgtc_obj.attack_team && side != SGTC_TEAM_NONE && side != sgtc_obj.attack_team;

   for (int i = 0; i < sgtc_num_gates; i++)
   {
      sgtc_gate_t &g = sgtc_gates[i];
      // no DHD and no dialer found: only the map opens it
      int w_dhd = !FNullEnt(g.dhd) ? SGTC_WaypointNearEx(pBot, SGTC_EntCenter(g.dhd), 0, TRUE) : -1;
      int w_gate = SGTC_WaypointNearEx(pBot, g.entrance, 0, TRUE);
      int dhd_kv = !FNullEnt(g.dhd) ? SGTC_EntKv(g.dhd) : -1;
      qboolean may_dial = dhd_kv == -1 || !sgtc_ent_kv[dhd_kv].team || sgtc_ent_kv[dhd_kv].team == side;

      if (defender)
      {
         // the attackers dialed it and went through: half the side follows
         // them instead of guarding an open door (sg_stargate: 6 of the 16
         // Goa'uld spawns are on the gate-2 plateau, cut off from the rest
         // of the map; they guarded that DHD all match, 121 "guard gate"
         // goals in 7 matches). Never dials: on sg_nox dialing gate 1 would
         // let the Goa'uld in.
         if (sgtc_defend_travel && g.open && !g.kawoosh && w_gate != -1 && SGTC_Reachable(pBot, w_gate) && SGTC_BotSideRank(pBot) % 2 == 0)
         {
            pBot.sgtc_gate = 0;
            pBot.v_sgtc_enter = g.entrance;
            pBot.f_sgtc_enter_until = gpGlobals->time + 30.0f;
            BotTrace(pBot, "sgtc gate %d: follow the attackers through the open wormhole wpt=%d", g.number, w_gate);
            *why = "follow gate";
            return w_gate;
         }
         if (w_dhd == -1 || !SGTC_Reachable(pBot, w_dhd))
            continue;
         int guard = SGTC_WaypointNear(pBot, SGTC_EntCenter(g.dhd), 450);
         if (guard == -1 || !SGTC_Reachable(pBot, guard))
            guard = w_dhd;
         pBot.sgtc_gate = 0;
         *why = "guard gate";
         return guard;
      }

      if (g.open && !g.kawoosh && w_gate != -1 && SGTC_Reachable(pBot, w_gate))
      {
         pBot.sgtc_gate = 0;
         pBot.v_sgtc_enter = g.entrance;
         pBot.f_sgtc_enter_until = gpGlobals->time + 30.0f;
         BotTrace(pBot, "sgtc gate %d: enter wormhole wpt=%d", g.number, w_gate);
         *why = "enter gate";
         return w_gate;
      }
      // just opening: its vortex still sends to the kill room, wait out of it
      if (g.open && g.kawoosh)
      {
         int w_wait = SGTC_WaypointNear(pBot, g.entrance, 400);
         if (w_wait != -1 && SGTC_Reachable(pBot, w_wait))
         {
            pBot.sgtc_gate = 0;
            BotTrace(pBot, "sgtc gate %d: vortex on, wait wpt=%d", g.number, w_wait);
            *why = "wait gate";
            return w_wait;
         }
      }
      if (!g.open && sgtc_dial && w_dhd != -1 && SGTC_Reachable(pBot, w_dhd) && may_dial)
      {
         // no DHD: the map runs the whole sequence, and its button only works
         // at times (sg_conquest's PC: the SGC gate opens by itself at about
         // 13 s, the console is armed again 3 s after a gate closed). Until it
         // can be pressed, wait near the gate (never in the vortex); dialing
         // state, so that it re-plans as soon as the gate opens.
         if (g.one_press && !g.blocked && !SGTC_DialerArmed(g.dhd))
         {
            int w_wait = SGTC_WaypointNear(pBot, g.entrance, 400);
            if (w_wait != -1 && SGTC_Reachable(pBot, w_wait))
            {
               pBot.sgtc_gate = i + 1;
               pBot.f_sgtc_gate_time = gpGlobals->time;
               BotTrace(pBot, "sgtc gate %d: wait for it to open wpt=%d (dialer \"%s\" not armed)", g.number, w_wait,
                  STRING(g.dhd->v.targetname));
               *why = "wait gate";
               return w_wait;
            }
         }
         pBot.sgtc_gate = i + 1;  // 0 = not dialing
         pBot.f_sgtc_gate_time = gpGlobals->time;
         BotTrace(pBot, "sgtc gate %d: go dial wpt=%d", g.number, w_dhd);
         *why = "dial gate";
         return w_dhd;
      }
   }
   return -1;
}

//
// At the DHD: face it and press "use" once per second (the button must come
// back up between presses) until the gate opens, then head for the wormhole.
//
static qboolean SGTC_BotDialThink(bot_t &pBot);

//
// Rings that are away from their rest under the floor, or moving, around a
// bot: waiting to leave, just arrived at the other end, or only crossing the
// platform. Returns the horizontal offset from the platform center and the
// ring set, or FALSE.
// The rings rise, teleport at +4.4 s, then come back down at 450 units/s
// and crush whatever is still on the platform ("killed by func_train", the
// middle too: one victim was 14 units from it).
//
static qboolean SGTC_UnderActiveRings(edict_t *pEdict, Vector &offset, int &set)
{
   const Vector &o = pEdict->v.origin;

   for (int i = 0; i < sgtc_num_rings; i++)
   {
      sgtc_ring_t &r = sgtc_rings[i];
      Vector d = o - r.platform;
      if (fabs(d.z) > 100.0f || d.Make2D().Length() > 150.0f)
         continue;
      for (int t = 0; t < r.num_trains; t++)
      {
         edict_t *pTrain = r.train[t];
         if (!FNullEnt(pTrain) && (pTrain->v.velocity.Length() > 1.0f || (pTrain->v.origin - r.train_rest[t]).Length() > 4.0f))
         {
            offset = Vector(d.x, d.y, 0);
            set = i;
            return TRUE;
         }
      }
   }
   return FALSE;
}

// near active rings and inside another player (two arrivals on the same
// spot): move to a free spot inside the ring wall
static void SGTC_RingSeparate(edict_t *pEdict)
{
   Vector offset;
   int set;
   if (!SGTC_UnderActiveRings(pEdict, offset, set) || offset.Length() > 70.0f)
      return;

   TraceResult tr;
   int hull = (pEdict->v.flags & FL_DUCKING) ? head_hull : human_hull;
   UTIL_TraceHull(pEdict->v.origin, pEdict->v.origin + Vector(0, 0, 1), dont_ignore_monsters, hull, pEdict, &tr);
   if (!tr.fStartSolid || FNullEnt(tr.pHit) || !(tr.pHit->v.flags & (FL_CLIENT | FL_FAKECLIENT)))
      return;

   static const float spot[9][2] = {{0, 0}, {34, 0}, {-34, 0}, {0, 34}, {0, -34}, {24, 24}, {-24, 24}, {24, -24}, {-24, -24}};
   Vector c = sgtc_rings[set].platform;
   c.z = pEdict->v.origin.z;
   for (int k = 0; k < 9; k++)
   {
      Vector p = c + Vector(spot[k][0], spot[k][1], 0);
      UTIL_TraceHull(p, p + Vector(0, 0, 1), dont_ignore_monsters, hull, pEdict, &tr);
      if (tr.fStartSolid || tr.fAllSolid)
         continue;
      SET_ORIGIN(pEdict, p);
      UTIL_ConsolePrintf("[sgtc-stat] rings %d: %s was inside another player, moved to %.0f %.0f\n", set + 1,
         STRING(pEdict->v.netname), spot[k][0], spot[k][1]);
      return;
   }
}

// near active rings and not waiting there to leave: inside the ring wall go
// to the middle, outside it step away (world-space move, every frame)
static void SGTC_RingEscape(bot_t &pBot)
{
   Vector offset;
   int set;
   if (!SGTC_UnderActiveRings(pBot.pEdict, offset, set))
      return;
   float d = offset.Length();
   if (d < 20.0f || (d < 25.0f && pBot.sgtc_ring == set + 1 && sgtc_ring_last_press[set] + 5.0f > gpGlobals->time))
      return;  // in the middle, or on its spot to leave with them (SGTC_BotRingThink)
   pBot.v_sgtc_move = (d < 70.0f ? offset * -1.0f : offset).Normalize() * (d < 70.0f ? 200.0f : pBot.f_max_speed);
   pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
}

static void SGTC_RingDebug(void)
{
   static float next = 0;
   if (!sgtc_ringdebug || !sgtc_num_rings)
      return;
   if (next > gpGlobals->time + 1.0f)
      next = 0;
   if (next > gpGlobals->time)
      return;
   next = gpGlobals->time + 0.25f;

   qboolean active = FALSE;
   for (int i = 0; i < sgtc_num_rings; i++)
      for (int t = 0; t < sgtc_rings[i].num_trains; t++)
      {
         edict_t *pTrain = sgtc_rings[i].train[t];
         if (!FNullEnt(pTrain) && (pTrain->v.velocity.Length() > 1.0f || (pTrain->v.origin - sgtc_rings[i].train_rest[t]).Length() > 4.0f))
            active = TRUE;
      }
   if (!active)
      return;

   char line[1024];
   int n = snprintf(line, sizeof(line), "[sgtc-stat] ringdbg t=%.2f", gpGlobals->time);
   for (int i = 0; i < sgtc_num_rings && n < (int)sizeof(line) - 64; i++)
   {
      sgtc_ring_t &r = sgtc_rings[i];
      for (int t = 0; t < r.num_trains && n < (int)sizeof(line) - 64; t++)
         n += snprintf(line + n, sizeof(line) - n, " s%dt%d=%.0f/%.0f", r.number, t + 1,
            r.train[t]->v.origin.z - r.train_rest[t].z, r.train[t]->v.velocity.z);
   }
   for (int p = 1; p <= gpGlobals->maxClients && n < (int)sizeof(line) - 64; p++)
   {
      edict_t *pPlayer = INDEXENT(p);
      if (FNullEnt(pPlayer) || pPlayer->free || !IsAlive(pPlayer))
         continue;
      for (int i = 0; i < sgtc_num_rings; i++)
      {
         Vector d = pPlayer->v.origin - sgtc_rings[i].platform;
         if (fabs(d.z) < 120.0f && d.Make2D().Length() < 200.0f)
         {
            TraceResult tr;
            UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin + Vector(0, 0, 1), dont_ignore_monsters,
               (pPlayer->v.flags & FL_DUCKING) ? head_hull : human_hull, pPlayer, &tr);
            n += snprintf(line + n, sizeof(line) - n, " %.12s@%d(%.0f,%.0f,%.0f fl=%x v=%.0f%s%s)", STRING(pPlayer->v.netname),
               sgtc_rings[i].number, d.x, d.y, d.z, pPlayer->v.flags & (FL_FROZEN | FL_ONGROUND), pPlayer->v.velocity.Length(),
               tr.fStartSolid ? " IN " : "", tr.fStartSolid ? (FNullEnt(tr.pHit) ? "world" : STRING(tr.pHit->v.targetname)[0] ? STRING(tr.pHit->v.targetname) : STRING(tr.pHit->v.classname)) : "");
         }
      }
   }
   UTIL_ConsolePrintf("%s\n", line);
}

//
// The rings leaving their rest, whoever pressed and at whichever end (one
// press runs every set): count it as a press for every set. Bots only knew
// about their own presses, so one waiting on a platform went off to press
// the button while rings started from the other end came without it. Rest
// is under the floor on sg_chateau, in the ceiling for sg_stargate's planet
// set: what counts is leaving it. Called every frame from SGTC_StartFrame.
//
static void SGTC_WatchRings(void)
{
   static float last_check = -1;
   static qboolean was_active = FALSE;
   if (last_check == gpGlobals->time)
      return;
   last_check = gpGlobals->time;

   // rest = where a train stays still for 8 s (the snapshot taken when the
   // rings were found can be mid-motion: map start, an early press)
   for (int i = 0; i < sgtc_num_rings; i++)
      for (int t = 0; t < sgtc_rings[i].num_trains; t++)
      {
         sgtc_ring_t &r = sgtc_rings[i];
         edict_t *pTrain = r.train[t];
         if (FNullEnt(pTrain))
            continue;
         if (pTrain->v.velocity.Length() > 1.0f || r.train_moved[t] > gpGlobals->time)
            r.train_moved[t] = gpGlobals->time;
         else if (r.train_moved[t] + 8.0f < gpGlobals->time && (pTrain->v.origin - r.train_rest[t]).Length() > 4.0f)
         {
            UTIL_ConsolePrintf("[sgtc-stat] rings %d: train \"%s\" rest corrected from %.0f to %.0f\n", r.number,
               STRING(pTrain->v.targetname), r.train_rest[t].z, pTrain->v.origin.z);
            r.train_rest[t] = pTrain->v.origin;
         }
      }

   qboolean active = FALSE;
   for (int i = 0; i < sgtc_num_rings && !active; i++)
      for (int t = 0; t < sgtc_rings[i].num_trains && !active; t++)
      {
         edict_t *pTrain = sgtc_rings[i].train[t];
         if (!FNullEnt(pTrain) && (pTrain->v.velocity.Length() > 1.0f || (pTrain->v.origin - sgtc_rings[i].train_rest[t]).Length() > 4.0f))
            active = TRUE;
      }
   if (active && !was_active)
      for (int i = 0; i < sgtc_num_rings; i++)
         if (sgtc_ring_last_press[i] + 10.0f < gpGlobals->time || sgtc_ring_last_press[i] > gpGlobals->time)
            sgtc_ring_last_press[i] = gpGlobals->time;
   was_active = active;
}

// nobody but pSelf near any set's platform (within 130 units of its middle,
// where the rings come down and where arrivals walk out), except bots
// waiting on their spot to leave with that set: pressing then crushes no one
static qboolean SGTC_RingBandClear(edict_t *pSelf)
{
   for (int p = 1; p <= gpGlobals->maxClients; p++)
   {
      edict_t *pPlayer = INDEXENT(p);
      if (FNullEnt(pPlayer) || pPlayer == pSelf || pPlayer->free || !IsAlive(pPlayer))
         continue;
      bot_t *pOther = UTIL_GetBotPointer(pPlayer);
      for (int i = 0; i < sgtc_num_rings; i++)
      {
         Vector d = pPlayer->v.origin - sgtc_rings[i].platform;
         float d2 = d.Make2D().Length();
         if (fabs(d.z) >= 120.0f || d2 >= 130.0f)
            continue;
         if (pOther && pOther->sgtc_ring == i + 1 && d2 < 35.0f)
            continue;  // waiting to leave on its spot
         return FALSE;
      }
   }
   return TRUE;
}

// in the holding room of that ring set (sg_chateau: a 160-unit room, 120
// units below the Tau'ri base, reached only by the rings)
static qboolean SGTC_InHoldingRoom(const sgtc_ring_t &r, const Vector &o)
{
   if (!r.has_hold)
      return FALSE;
   Vector d = o - r.hold;
   return fabs(d.z) < 100.0f && d.Make2D().Length() < 200.0f;
}

//
// Between the two hops of a ring trip (sg_chateau: A -> B at +4.4 s of the
// cycle, B -> C at +4.8 s; sg_stargate's planet set too): stand in the
// nearest teleport column and wait. A bot that walked on after the first hop
// missed the second one and was left in a room with no exit, no button and
// no waypoints (Ba'al, Jonas_Quinn: 2-3 minutes each, with the key). The
// next cycle, whoever presses at either end, carries it on.
//
static qboolean SGTC_BotRingHold(bot_t &pBot, sgtc_ring_t &r)
{
   edict_t *pEdict = pBot.pEdict;
   const Vector &o = pEdict->v.origin;

   if (!SGTC_InHoldingRoom(r, o))
   {
      // carried on to the arrival platform
      BotTrace(pBot, "sgtc rings %d: through the holding room, now at %.0f %.0f %.0f", r.number, o.x, o.y, o.z);
      pBot.sgtc_ring = 0;
      pBot.sgtc_ring_hold = FALSE;
      pBot.curr_waypoint_index = -1;
      return FALSE;
   }

   // the nearest column, then stand still
   Vector c = r.hold;
   float best = 99999.0f;
   for (int k = 0; k < r.num_hold_cols; k++)
   {
      float d = (r.hold_col[k] - o).Make2D().Length();
      if (d < best)
      {
         best = d;
         c = r.hold_col[k];
      }
   }
   Vector to = c - o;
   to.z = 0;
   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.b_not_maxspeed = TRUE;
   pEdict->v.button &= ~(IN_JUMP | IN_DUCK);
   if (to.Length() > 10.0f)
   {
      pBot.v_sgtc_move = to.Normalize() * 100.0f;
      pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
   }
   if (pBot.f_sgtc_ring_check <= gpGlobals->time)
   {
      if (pBot.f_sgtc_ring_check > 0)
         BotTrace(pBot, "sgtc rings %d: still in the holding room after %.0f s", r.number, gpGlobals->time - pBot.f_sgtc_ring_time);
      pBot.f_sgtc_ring_check = gpGlobals->time + 20.0f;
   }
   return TRUE;
}

//
// Using the rings: the route goes from the platform waypoint to the arrival.
// Press the ring button unless another bot did lately, then stand in the
// middle of the platform until the teleport (4.4 s after the press).
//
static qboolean SGTC_BotRingThink(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;

   if (pBot.sgtc_ring <= 0 || pBot.sgtc_ring > sgtc_num_rings)
   {
      pBot.sgtc_ring = 0;
      pBot.sgtc_ring_hold = FALSE;
      // carried into a holding room without having asked for the rings (it
      // stood on the platform when someone else pressed): wait there too
      for (int i = 0; i < sgtc_num_rings; i++)
         if (SGTC_InHoldingRoom(sgtc_rings[i], pEdict->v.origin))
         {
            pBot.sgtc_ring = i + 1;
            pBot.sgtc_ring_hold = TRUE;
            pBot.f_sgtc_ring_time = gpGlobals->time;
            pBot.f_sgtc_ring_check = 0;
            BotTrace(pBot, "sgtc rings %d: in the holding room, waiting for the second hop", sgtc_rings[i].number);
            return SGTC_BotRingHold(pBot, sgtc_rings[i]);
         }
      if (pBot.waypoint_goal == -1 || pBot.pBotEnemy != NULL)
         return FALSE;
      for (int i = 0; i < sgtc_num_rings; i++)
      {
         sgtc_ring_t &r = sgtc_rings[i];
         if (pBot.curr_waypoint_index == r.w_platform && (pEdict->v.origin - r.platform).Length() < 150.0f &&
             (pBot.waypoint_goal == r.w_dest || WaypointRouteFromTo(r.w_platform, pBot.waypoint_goal) == r.w_dest))
         {
            pBot.sgtc_ring = i + 1;
            pBot.f_sgtc_ring_time = gpGlobals->time;
            pBot.f_sgtc_ring_check = 0;
            pBot.sgtc_ring_settled = FALSE;
            BotTrace(pBot, "sgtc rings %d: use them to reach wpt=%d", r.number, pBot.waypoint_goal);
            break;
         }
      }
      if (!pBot.sgtc_ring)
         return FALSE;
   }

   sgtc_ring_t &r = sgtc_rings[pBot.sgtc_ring - 1];

   if (pBot.sgtc_ring_hold)
      return SGTC_BotRingHold(pBot, r);

   if ((pEdict->v.origin - r.platform).Length() > 600.0f)
   {
      if (SGTC_InHoldingRoom(r, pEdict->v.origin))
      {
         // the first of two hops: stay in the column for the second one
         pBot.sgtc_ring_hold = TRUE;
         pBot.f_sgtc_ring_time = gpGlobals->time;
         pBot.f_sgtc_ring_check = 0;
         BotTrace(pBot, "sgtc rings %d: in the holding room at %.0f %.0f %.0f, waiting for the second hop", r.number,
            pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z);
         return SGTC_BotRingHold(pBot, r);
      }
      BotTrace(pBot, "sgtc rings %d: travelled, now at %.0f %.0f %.0f", r.number, pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z);
      pBot.sgtc_ring = 0;
      pBot.curr_waypoint_index = -1;
      return FALSE;
   }
   if (pBot.f_sgtc_ring_time + 25.0f < gpGlobals->time || pBot.pBotEnemy != NULL)
   {
      BotTrace(pBot, "sgtc rings %d: give up", r.number);
      pBot.sgtc_ring = 0;
      pBot.waypoint_goal = -1;
      pBot.f_waypoint_goal_time = 0;
      return FALSE;
   }

   // nobody pressed lately (the rings take ~10 s to come back): press it
   SGTC_WatchRings();
   float &last_press = sgtc_ring_last_press[pBot.sgtc_ring - 1];
   if (last_press > gpGlobals->time)
      last_press = 0;
   Vector stand = r.num_spots ? r.spot[(int)(&pBot - bots) % r.num_spots] : r.platform;
   Vector target = stand;
   if (!FNullEnt(r.button) && last_press + 10.0f < gpGlobals->time)
   {
      Vector b = SGTC_EntCenter(r.button);
      target = b;
      if ((b - GetGunPosition(pEdict)).Length() < 64.0f)  // +use reach
      {
         Vector angles = UTIL_VecToAngles(b - GetGunPosition(pEdict));
         pEdict->v.idealpitch = -angles.x;
         pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
         qboolean clear = SGTC_RingBandClear(pEdict);
         if (pBot.f_sgtc_ring_next_use <= gpGlobals->time &&
             (clear || pBot.f_sgtc_ring_time + 20.0f < gpGlobals->time))
         {
            pBot.f_sgtc_ring_next_use = gpGlobals->time + 1.0f;
            pEdict->v.button |= IN_USE;
            UTIL_ConsolePrintf("[sgtc-stat] rings %d: %s presses (%s) t=%.1f\n", r.number, STRING(pEdict->v.netname),
               clear ? "clear" : "forced", gpGlobals->time);
            last_press = gpGlobals->time;
            BotTrace(pBot, "sgtc rings %d: press the button", r.number);
         }
      }
   }

   Vector to = target - pEdict->v.origin;
   to.z = 0;

   // on its spot: stand still until the rings come (the user saw bots
   // moving about the platform); only a push of 40 units sends it back
   if (target == stand && (to.Length() < 16.0f || (pBot.sgtc_ring_settled && to.Length() < 40.0f)))
   {
      pBot.sgtc_ring_settled = TRUE;
      pBot.f_move_speed = 0;
      pBot.f_strafe_direction = 0;
      pBot.b_not_maxspeed = TRUE;
      pEdict->v.button &= ~(IN_JUMP | IN_DUCK);
      return TRUE;
   }
   pBot.sgtc_ring_settled = FALSE;

   pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
   BotFixIdealYaw(pEdict);
   pBot.f_move_speed = to.Length() > 16.0f ? (to.Length() > 80.0f ? pBot.f_max_speed : 100.0f) : 0.0f;
   // BotThinkFinalizeMovement scales any move up to f_max_speed unless
   // b_not_maxspeed: at full speed bots orbited the platform center
   pBot.b_not_maxspeed = TRUE;
   pBot.f_strafe_direction = 0;

   // not getting there (a teammate in the way, the corner by the button,
   // 7 give-ups in 22 on sg_chateau): jump
   if (pBot.f_sgtc_ring_check <= gpGlobals->time)
   {
      if (pBot.f_sgtc_ring_check > 0 && to.Length() > 40.0f && (pEdict->v.origin - pBot.v_sgtc_ring_check_pos).Length() < 16.0f)
      {
         pEdict->v.button |= IN_JUMP;
         BotTrace(pBot, "sgtc rings %d: not moving, jump", r.number);
      }
      pBot.f_sgtc_ring_check = gpGlobals->time + 1.0f;
      pBot.v_sgtc_ring_check_pos = pEdict->v.origin;
   }
   return TRUE;
}

// the first door on this edge (w -> next, or a plain segment with w = -1) that
// is shut and not about to open (shut_only), or that is not open, -1 if none
static int SGTC_DoorOnEdge(int w, int next, const Vector &a, const Vector &b, qboolean shut_only)
{
   for (int i = 0; i < sgtc_num_doors; i++)
   {
      sgtc_door_t &d = sgtc_doors[i];
      if (!d.passage)
         continue;
      qboolean forced = w != -1 && ((w == d.w_a && next == d.w_b) || (w == d.w_b && next == d.w_a));
      if (!forced && !SGTC_DoorOnSegment(d, a, b))
         continue;
      int state = SGTC_DoorState(d);
      if (shut_only ? state == SGTC_DOOR_SHUT && d.f_lock <= gpGlobals->time : state != SGTC_DOOR_OPEN)
         return i;
   }
   return -1;
}

// ... along the route from waypoint 'from' to 'to', at most max_steps waypoints
static int SGTC_RouteDoor(int from, int to, int max_steps, qboolean shut_only)
{
   if (!sgtc_num_doors || from < 0 || from >= num_waypoints || to < 0 || to >= num_waypoints)
      return -1;
   int w = from;
   for (int step = 0; step < max_steps && w != to; step++)
   {
      int next = WaypointRouteFromTo(w, to);
      if (next == WAYPOINT_UNREACHABLE || next == w || next < 0 || next >= num_waypoints)
         break;
      int i = SGTC_DoorOnEdge(w, next, waypoints[w].origin, waypoints[next].origin, shut_only);
      if (i != -1)
         return i;
      w = next;
   }
   return -1;
}

// ... from where the bot stands, along its route
static int SGTC_DoorAhead(bot_t &pBot, int max_steps, qboolean shut_only)
{
   int w = pBot.curr_waypoint_index;
   if (w < 0 || w >= num_waypoints)
      return -1;
   int i = SGTC_DoorOnEdge(-1, -1, pBot.pEdict->v.origin, waypoints[w].origin, shut_only);
   if (i == -1 && pBot.waypoint_goal != -1)
      i = SGTC_RouteDoor(w, pBot.waypoint_goal, max_steps, shut_only);
   return i;
}

// the waypoint a door errand starts from: the current one, or when that lies
// past a door that isn't open (the route was taking it through), the nearest
// one in sight on the bot's side (sg_shunky: a bot at wpt 938 heading for 939
// found its button "behind a shut door" and dropped the goal)
static int SGTC_BotDoorFrom(bot_t &pBot)
{
   int w = pBot.curr_waypoint_index;
   const Vector &o = pBot.pEdict->v.origin;
   if (w < 0 || w >= num_waypoints || SGTC_DoorOnEdge(-1, -1, o, waypoints[w].origin, FALSE) == -1)
      return w;
   int best = -1;
   float best_dist = 400.0f;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
         continue;
      float dist = (waypoints[i].origin - o).Length();
      if (dist >= best_dist || SGTC_DoorOnEdge(-1, -1, o, waypoints[i].origin, FALSE) != -1)
         continue;
      TraceResult tr;
      UTIL_TraceLine(o, waypoints[i].origin, ignore_monsters, pBot.pEdict, &tr);
      if (tr.flFraction < 1.0f)
         continue;
      best_dist = dist;
      best = i;
   }
   return best != -1 ? best : w;
}

static qboolean SGTC_ButtonUsable(const sgtc_button_t &b, int side, const char **why)
{
   if (FNullEnt(b.ent) || strcmp(STRING(b.ent->v.target), b.target))
   {
      *why = "button used up";  // a one-shot one, retargeted to "nowhere"
      return FALSE;
   }
   if (b.needcard)
   {
      *why = "button needs a card";
      return FALSE;
   }
   if (b.team && b.team != side)
   {
      *why = "the other side's button";
      return FALSE;
   }
   if (b.touch && b.ent->v.solid == SOLID_NOT)
   {
      *why = "touch button not solid";
      return FALSE;
   }
   // never shut an open door (ctp_conquest's start-open blast doors kill,
   // dmg 1000; a toggle door's button shuts it again): every door the press
   // moves is shut and still, but for a harmless one another door fires
   for (int r = 0; r < b.num_reach; r++)
   {
      const sgtc_panel_t &p = sgtc_panels[b.reach[r]];
      if (!SGTC_PanelValid(p) || SGTC_PanelShut(p))
         continue;
      if (b.reach_opens[r] || p.ent->v.dmg > 0 || !SGTC_PanelStill(p))
      {
         *why = "a door it moves is open or moving";
         return FALSE;
      }
   }
   return TRUE;
}

// the button this bot can open the door with: its side's, pressed from a
// waypoint on its side of every shut door; the nearest by route (and that
// waypoint in *press_wpt)
static int SGTC_PickButton(bot_t &pBot, const sgtc_door_t &d, const char **why, int *press_wpt)
{
   int side = SGTC_GetPlayerTeam(pBot.pEdict), best = -1, from = SGTC_BotDoorFrom(pBot);
   float best_dist = WAYPOINT_MAX_DISTANCE;

   *why = "no button";
   if (from < 0 || from >= num_waypoints)
   {
      *why = "no waypoint";
      return -1;
   }
   for (int k = 0; k < d.num_buttons; k++)
   {
      const sgtc_button_t &b = sgtc_buttons[d.button[k]];
      if (!SGTC_ButtonUsable(b, side, why))
         continue;
      if (!b.num_wpt)
         *why = "button unreachable";
      for (int j = 0; j < b.num_wpt; j++)
      {
         float dist = WaypointDistanceFromTo(from, b.wpt[j]);
         if (dist >= WAYPOINT_MAX_DISTANCE)
         {
            *why = "button unreachable";
            continue;
         }
         if (SGTC_RouteDoor(from, b.wpt[j], 64, TRUE) != -1)
         {
            *why = "button behind a shut door";
            continue;
         }
         if (dist < best_dist)
         {
            best_dist = dist;
            best = d.button[k];
            if (press_wpt)
               *press_wpt = b.wpt[j];
         }
      }
   }
   return best;
}

// a goal behind a shut door, not about to open, that this bot can't open
static qboolean SGTC_DoorCutsOff(bot_t &pBot, int goal)
{
   if (!sgtc_num_doors || pBot.curr_waypoint_index == -1)
      return FALSE;
   int i = SGTC_RouteDoor(SGTC_BotDoorFrom(pBot), goal, 64, TRUE);
   const char *why;
   if (i == -1 || SGTC_PickButton(pBot, sgtc_doors[i], &why, NULL) != -1)
      return FALSE;
   BotTrace(pBot, "sgtc door %d: goal wpt=%d is behind it, shut (%s)", i, goal, why);
   return TRUE;
}

// after a press: the managers' delay, and a moment for the door to start
static float SGTC_ButtonLock(const sgtc_button_t &b, const sgtc_door_t &d)
{
   float delay = 0;
   for (int r = 0; r < b.num_reach; r++)
      for (int k = 0; k < d.num_panels; k++)
         if (b.reach[r] == d.panel[k] && b.reach_opens[r] && b.reach_delay[r] > delay)
            delay = b.reach_delay[r];
   return delay + 2.5f;
}

static void SGTC_DoorPressed(bot_t &pBot, sgtc_door_t &d, const sgtc_button_t &b)
{
   edict_t *pEdict = pBot.pEdict;
   d.presser = pEdict;
   d.f_pressed = gpGlobals->time;
   d.f_lock = gpGlobals->time + SGTC_ButtonLock(b, d);
   d.press_checked = FALSE;
   Vector c = SGTC_EntCenter(b.ent);
   const Vector &o = pEdict->v.origin;
   UTIL_ConsolePrintf("[sgtc-stat] door \"%s\": %s %s its button at %.0f %.0f %.0f (from %.0f %.0f %.0f, %.0f u from its box, view %.0f %.0f)\n",
      STRING(d.door->v.targetname), STRING(pEdict->v.netname), b.touch ? "walks into" : "presses", c.x, c.y, c.z,
      o.x, o.y, o.z, SGTC_UseDist(b.ent, o), pEdict->v.v_angle.x, pEdict->v.v_angle.y);
}

// "[sgtc-stat] door ... open/shut" when that changes (4 times a second)
static float sgtc_door_watch;
static void SGTC_WatchDoors(void)
{
   if (sgtc_door_watch > gpGlobals->time + 1.0f)  // a new map
      sgtc_door_watch = 0;
   if (sgtc_door_watch > gpGlobals->time)
      return;
   sgtc_door_watch = gpGlobals->time + 0.25f;
   for (int i = 0; i < sgtc_num_doors; i++)
   {
      sgtc_door_t &d = sgtc_doors[i];
      int state = SGTC_DoorState(d);
      // a press that moved nothing by the end of its lock (the managers'
      // delay + 2.5 s): logged once
      if (!d.press_checked)
      {
         if (state != SGTC_DOOR_SHUT || d.f_pressed > gpGlobals->time)
            d.press_checked = TRUE;
         else if (d.f_lock <= gpGlobals->time)
         {
            d.press_checked = TRUE;
            UTIL_ConsolePrintf("[sgtc-stat] door \"%s\": the press by %s did nothing\n", STRING(d.door->v.targetname),
               !FNullEnt(d.presser) ? STRING(d.presser->v.netname) : "?");
         }
      }
      if (!d.passage || state == SGTC_DOOR_MOVING || state == d.last_state)
         continue;
      if (d.last_state != -1)
      {
         char by[96] = "";
         if (!FNullEnt(d.presser) && d.f_pressed > 0 && gpGlobals->time - d.f_pressed < 30.0f)
            safevoid_snprintf(by, sizeof(by), ", %.1f s after %s pressed its button", gpGlobals->time - d.f_pressed,
               STRING(d.presser->v.netname));
         UTIL_ConsolePrintf("[sgtc-stat] door \"%s\" %s%s\n", STRING(d.door->v.targetname), SGTC_DoorStateName(state), by);
      }
      d.last_state = state;
   }
}

//
// In front of a door that is about to open after a press, or that moves and
// hurts or swings: wait clear of it (in the doorway itself: walk on).
//
static qboolean SGTC_BotDoorHold(bot_t &pBot, int i)
{
   edict_t *pEdict = pBot.pEdict;

   if (i != -1)
   {
      sgtc_door_t &d = sgtc_doors[i];
      int state = SGTC_DoorState(d);
      qboolean rotating = sgtc_panels[d.panel[0]].rotating;
      if ((state == SGTC_DOOR_MOVING && (d.door->v.dmg > 0 || rotating)) ||
          (state == SGTC_DOOR_SHUT && d.f_lock > gpGlobals->time))
      {
         const Vector &o = pEdict->v.origin;
         float dx = o.x < d.mins.x ? d.mins.x - o.x : (o.x > d.maxs.x ? o.x - d.maxs.x : 0.0f);
         float dy = o.y < d.mins.y ? d.mins.y - o.y : (o.y > d.maxs.y ? o.y - d.maxs.y : 0.0f);
         float dist = sqrtf(dx * dx + dy * dy), sweep = 0, from_hinge = 0;
         Vector away(0, 0, 0);
         if (rotating)
         {
            // a swinging panel reaches its farthest corner from the hinge, plus a hull
            const Vector &h = d.door->v.origin;
            float rx = fabs(d.mins.x - h.x) > fabs(d.maxs.x - h.x) ? fabs(d.mins.x - h.x) : fabs(d.maxs.x - h.x);
            float ry = fabs(d.mins.y - h.y) > fabs(d.maxs.y - h.y) ? fabs(d.mins.y - h.y) : fabs(d.maxs.y - h.y);
            sweep = sqrtf(rx * rx + ry * ry) + 20.0f;
            away = o - h;
            away.z = 0;
            from_hinge = away.Length();
         }
         if (dist >= 20.0f && (rotating ? from_hinge < sweep + 100.0f : dist < 120.0f))
         {
            if (pBot.f_sgtc_door_hold <= 0 || pBot.f_sgtc_door_hold > gpGlobals->time)
            {
               pBot.f_sgtc_door_hold = gpGlobals->time;
               BotTrace(pBot, "sgtc door %d: %s, wait clear of it", i, state == SGTC_DOOR_SHUT ? "about to open" : "moving");
            }
            if (gpGlobals->time - pBot.f_sgtc_door_hold > 15.0f)
               return FALSE;  // stays like that: go on
            pBot.f_move_speed = 0;
            pBot.b_not_maxspeed = TRUE;
            pBot.f_strafe_direction = 0;
            if (rotating && from_hinge < sweep && from_hinge > 1.0f)
            {
               pBot.v_sgtc_move = away.Normalize() * 200.0f;
               pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
            }
            return TRUE;
         }
      }
   }
   pBot.f_sgtc_door_hold = 0;
   return FALSE;
}

//
// A shut door on the route: go to a button that opens it first, press it
// (or walk into it), then carry on to the goal. Returns TRUE while it takes
// over the movement (at the button, or waiting clear of a door).
//
static qboolean SGTC_BotDoorThink(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;

   SGTC_WatchDoors();

   if (pBot.sgtc_door > sgtc_num_doors || pBot.sgtc_door_btn < 0 || pBot.sgtc_door_btn >= sgtc_num_buttons)
      pBot.sgtc_door = 0;
   if (pBot.f_sgtc_door_time > gpGlobals->time + 30.0f || pBot.f_sgtc_door_retry > gpGlobals->time + 30.0f)  // a new map
      pBot.f_sgtc_door_time = pBot.f_sgtc_door_retry = 0;
   if (pBot.f_sgtc_door_use > gpGlobals->time + 5.0f)  // a new map
      pBot.f_sgtc_door_use = 0;
   if (pBot.f_sgtc_door_fight > gpGlobals->time)  // a new map
      pBot.f_sgtc_door_fight = 0;

   if (pBot.sgtc_door <= 0)
   {
      pBot.f_sgtc_door_fight = 0;
      if (!sgtc_num_doors || pBot.waypoint_goal == -1 || pBot.curr_waypoint_index == -1 || pBot.pBotEnemy != NULL)
      {
         pBot.f_sgtc_door_hold = 0;
         return FALSE;
      }
      if (SGTC_BotDoorHold(pBot, SGTC_DoorAhead(pBot, 2, FALSE)))
         return TRUE;
      if (pBot.f_sgtc_door_retry > gpGlobals->time)
         return FALSE;
      pBot.f_sgtc_door_retry = gpGlobals->time + 0.5f;  // next look along the route

      int found = SGTC_DoorAhead(pBot, 64, TRUE);
      if (found == -1)
         return FALSE;
      sgtc_door_t &d = sgtc_doors[found];
      const char *why;
      int press_wpt = -1;
      int bi = SGTC_PickButton(pBot, d, &why, &press_wpt);
      if (bi == -1)
      {
         // nobody opens it from here: that goal is cut off for now
         BotTrace(pBot, "sgtc door %d \"%s\": shut on the way to wpt=%d, can't open it (%s): new goal", found,
            STRING(d.door->v.targetname), pBot.waypoint_goal, why);
         for (int i = EXCLUDE_POINTS_COUNT - 1; i > 0; i--)
            pBot.exclude_points[i] = pBot.exclude_points[i - 1];
         pBot.exclude_points[0] = pBot.waypoint_goal;
         pBot.waypoint_goal = -1;
         pBot.f_waypoint_goal_time = 0;
         pBot.f_sgtc_door_retry = gpGlobals->time + 3.0f;
         return FALSE;
      }
      sgtc_button_t &b = sgtc_buttons[bi];
      int from = SGTC_BotDoorFrom(pBot);
      if (from != pBot.curr_waypoint_index)
      {
         BotTrace(pBot, "sgtc door %d: wpt=%d is past it, from wpt=%d instead", found, pBot.curr_waypoint_index, from);
         pBot.curr_waypoint_index = from;
      }
      float dist = WaypointDistanceFromTo(from, press_wpt);
      pBot.sgtc_door = found + 1;
      pBot.sgtc_door_btn = bi;
      pBot.sgtc_door_wpt = press_wpt;
      pBot.sgtc_door_goal = pBot.waypoint_goal;
      pBot.f_sgtc_door_time = gpGlobals->time;
      pBot.f_sgtc_door_until = gpGlobals->time + (dist < 10500.0f ? 20.0f + dist / 150.0f : 90.0f);
      pBot.f_sgtc_door_use = 0;
      pBot.f_sgtc_door_fight = 0;
      pBot.waypoint_goal = press_wpt;
      pBot.f_waypoint_goal_time = gpGlobals->time + 120.0f;
      BotTrace(pBot, "sgtc door %d \"%s\": shut on the way to wpt=%d, going to its %s button wpt=%d (%.0f u)", found,
         STRING(d.door->v.targetname), pBot.sgtc_door_goal, b.touch ? "touch" : "use", press_wpt, dist);
      return FALSE;
   }

   sgtc_door_t &d = sgtc_doors[pBot.sgtc_door - 1];
   sgtc_button_t &b = sgtc_buttons[pBot.sgtc_door_btn];
   int state = SGTC_DoorState(d);
   qboolean mine = d.presser == pEdict && d.f_lock > gpGlobals->time;
   // a fight on the way doesn't count against the time it has
   float until = pBot.f_sgtc_door_until + (pBot.f_sgtc_door_fight > 0 ? gpGlobals->time - pBot.f_sgtc_door_fight : 0.0f);
   const char *why = NULL, *unusable = NULL;
   if (state != SGTC_DOOR_SHUT)
      why = SGTC_DoorStateName(state);
   else if (d.f_lock > gpGlobals->time && !mine)
      why = "pressed by someone else";
   else if (!mine && !SGTC_ButtonUsable(b, SGTC_GetPlayerTeam(pEdict), &unusable))
      why = unusable;
   else if (until < gpGlobals->time || until > gpGlobals->time + 100.0f)
      why = "gave up";
   if (why)
   {
      // done: back to the goal. Gave up: not that goal behind the door for a while
      qboolean failed = !strcmp(why, "gave up");
      BotTrace(pBot, "sgtc door %d: %s, %s wpt=%d", pBot.sgtc_door - 1, why, failed ? "drop the goal" : "back to", pBot.sgtc_door_goal);
      pBot.f_sgtc_door_retry = gpGlobals->time + (failed ? 10.0f : 2.0f);
      pBot.sgtc_door = 0;
      pBot.f_sgtc_door_fight = 0;
      pBot.waypoint_goal = pBot.sgtc_door_goal;
      if (failed && pBot.waypoint_goal != -1)
      {
         for (int i = EXCLUDE_POINTS_COUNT - 1; i > 0; i--)
            pBot.exclude_points[i] = pBot.exclude_points[i - 1];
         pBot.exclude_points[0] = pBot.waypoint_goal;
         pBot.waypoint_goal = -1;
      }
      pBot.f_waypoint_goal_time = pBot.waypoint_goal != -1 ? gpGlobals->time + 120.0f : 0.0f;
      return FALSE;
   }

   // an enemy on the way: fight it (jk_botti's combat code drops the goal
   // waypoint meanwhile), then go on to the button. It used to end the
   // errand: on sg_shunky, where the button is in the Goa'uld base 5900
   // units of waypoints from the Tau'ri spawn, each enemy sighted down the
   // long corridors sent the Tau'ri back towards their goal, and 2 s later
   // towards the button again: they went to and fro in the middle.
   if (pBot.pBotEnemy != NULL)
   {
      if (pBot.f_sgtc_door_fight <= 0)
      {
         pBot.f_sgtc_door_fight = gpGlobals->time;
         BotTrace(pBot, "sgtc door %d: enemy, fight it, then on to the button", pBot.sgtc_door - 1);
      }
      return FALSE;
   }
   if (pBot.f_sgtc_door_fight > 0)
   {
      pBot.f_sgtc_door_until += gpGlobals->time - pBot.f_sgtc_door_fight;
      pBot.f_sgtc_door_fight = 0;
   }

   const Vector &o = pEdict->v.origin;
   Vector btn = SGTC_EntCenter(b.ent);
   Vector to_btn = btn - o;
   Vector eye = GetGunPosition(pEdict);
   float reach = SGTC_UseDist(b.ent, o);
   int pw = pBot.sgtc_door_wpt >= 0 && pBot.sgtc_door_wpt < num_waypoints ? pBot.sgtc_door_wpt : b.w;
   qboolean close_by;
   if (b.touch)
   {
      float takeover = pw != -1 ? (waypoints[pw].origin - btn).Length() + 64.0f : 0.0f;  // its waypoint may be farther
      close_by = to_btn.Length() <= (takeover > 200.0f ? takeover : 200.0f);
   }
   else
      close_by = (reach < 100.0f && SGTC_UseSight(b.ent, eye, pEdict)) || (pw != -1 && (waypoints[pw].origin - o).Length() < 48.0f);
   if (!close_by)
   {
      // on the way: the navigation goes to the press waypoint. A fight, a
      // goal reached or a goal of its own replaced it: set it again
      if (pw != -1 && pBot.waypoint_goal != pw)
      {
         BotTrace(pBot, "sgtc door %d: on to its button again, wpt=%d (goal was wpt=%d)", pBot.sgtc_door - 1, pw, pBot.waypoint_goal);
         pBot.waypoint_goal = pw;
         pBot.f_waypoint_goal_time = gpGlobals->time + 120.0f;
      }
      return FALSE;
   }

   // near the button: face it, walk up to it in world space, press
   pBot.f_move_speed = 0;
   pBot.b_not_maxspeed = TRUE;
   pBot.f_strafe_direction = 0;
   if (mine)
      return TRUE;  // pressed: wait there until the door moves

   if (b.touch)
   {
      Vector angles = UTIL_VecToAngles(btn - eye);
      pEdict->v.ideal_yaw = angles.y;
      pEdict->v.idealpitch = -angles.x;
      pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
      BotFixIdealYaw(pEdict);
      Vector to2 = to_btn;
      to2.z = 0;
      // touch-activated (sg_strike3's spawn doors): walk into it, +use does
      // nothing. Then step back at once: touching it again once it is back
      // would shut the toggle door again.
      const Vector &a0 = pEdict->v.absmin, &a1 = pEdict->v.absmax, &b0 = b.ent->v.absmin, &b1 = b.ent->v.absmax;
      qboolean touching = a0.x <= b1.x + 2 && a1.x >= b0.x - 2 && a0.y <= b1.y + 2 && a1.y >= b0.y - 2 &&
                          a0.z <= b1.z + 2 && a1.z >= b0.z - 2;
      qboolean fired = b.ent->v.velocity.Length() > 0.1f || b.ent->v.avelocity.Length() > 0.1f;
      if (touching && pBot.f_sgtc_door_use <= 0)
         pBot.f_sgtc_door_use = gpGlobals->time;
      if (fired || (touching && gpGlobals->time - pBot.f_sgtc_door_use > 0.3f))
      {
         SGTC_DoorPressed(pBot, d, b);
         BotTrace(pBot, "sgtc door %d: walked into the button%s, back to wpt=%d", pBot.sgtc_door - 1, fired ? " (it moves)" : "",
            pBot.sgtc_door_goal);
         pBot.f_sgtc_door_fight = 0;
         if (to2.Length() > 1.0f)
         {
            pBot.v_sgtc_move = to2.Normalize() * -200.0f;
            pBot.f_sgtc_move_until = gpGlobals->time + 0.4f;
         }
         pBot.f_sgtc_door_use = 0;
         pBot.f_sgtc_door_retry = gpGlobals->time + 2.0f;
         pBot.sgtc_door = 0;
         pBot.waypoint_goal = pBot.sgtc_door_goal;
         pBot.f_waypoint_goal_time = pBot.waypoint_goal != -1 ? gpGlobals->time + 120.0f : 0.0f;
         return TRUE;
      }
      if (to2.Length() > 1.0f)
      {
         pBot.v_sgtc_move = to2.Normalize() * 150.0f;
         pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
      }
      return TRUE;
   }

   // +use: look along PlayerUse's eye-to-box vector, walk until the box is
   // within 40 units (not up against it: from sg_stargate's south plate the
   // box lay under the eyes), press within 60 (PlayerUse: 64)
   Vector aim = SGTC_UseAim(b.ent, eye);
   Vector angles = UTIL_VecToAngles(aim);
   pEdict->v.ideal_yaw = angles.y;
   pEdict->v.idealpitch = -angles.x;
   pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   BotFixIdealYaw(pEdict);
   BotFixIdealPitch(pEdict);
   Vector to2 = eye + aim - o;
   to2.z = 0;
   if (reach > 40.0f && to2.Length() > 1.0f)
   {
      pBot.v_sgtc_move = to2.Normalize() * 150.0f;
      pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
   }
   // PlayerUse looks along v_angle, and jk_botti levels it every frame
   // (BotCheckLogoSpraying zeroes v_angle.x after BotChangePitch, BotPointGun
   // then only adds one filtered step towards idealpitch): set the view
   // itself, with no turn left in progress for BotPointGun to add
   float dyaw = angles.y - pEdict->v.v_angle.y;
   while (dyaw > 180.0f)
      dyaw -= 360.0f;
   while (dyaw < -180.0f)
      dyaw += 360.0f;
   pEdict->v.v_angle.x = pEdict->v.idealpitch;
   pEdict->v.pitch_speed = 0;
   if (fabs(dyaw) < 20.0f)
   {
      pEdict->v.v_angle.y = pEdict->v.ideal_yaw;
      pEdict->v.yaw_speed = 0;
      if (pBot.f_sgtc_door_use <= gpGlobals->time && reach < 60.0f)
      {
         pBot.f_sgtc_door_use = gpGlobals->time + 1.5f;
         pEdict->v.button |= IN_USE;
         SGTC_DoorPressed(pBot, d, b);
         BotTrace(pBot, "sgtc door %d: press the button", pBot.sgtc_door - 1);
      }
   }
   return TRUE;
}

static qboolean SGTC_BotDialThink(bot_t &pBot)
{
   if (pBot.sgtc_gate <= 0 || pBot.sgtc_gate > sgtc_num_gates)
      return FALSE;

   sgtc_gate_t &g = sgtc_gates[pBot.sgtc_gate - 1];
   edict_t *pEdict = pBot.pEdict;

   if (g.open || pBot.f_sgtc_gate_time + 40.0f < gpGlobals->time || FNullEnt(g.dhd))
   {
      // done (or gave up): replan, which now leads into the open wormhole
      pBot.sgtc_gate = 0;
      pBot.waypoint_goal = -1;
      pBot.f_waypoint_goal_time = 0;
      return FALSE;
   }

   Vector dhd = SGTC_EntCenter(g.dhd);
   Vector to_dhd = dhd - pEdict->v.origin;
   if (to_dhd.Length() > 110 || pBot.pBotEnemy != NULL)
      return FALSE;

   Vector angles = UTIL_VecToAngles(dhd - GetGunPosition(pEdict));
   pEdict->v.ideal_yaw = angles.y;
   pEdict->v.idealpitch = -angles.x;
   pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   BotFixIdealYaw(pEdict);

   pBot.f_move_speed = to_dhd.Length2D() > 50 ? 120.0f : 0.0f;
   pBot.b_not_maxspeed = TRUE;  // or it's scaled back up to full speed
   pBot.f_strafe_direction = 0;

   // One gate at a time on the whole map. Dialing a DHD while the other gate
   // was open, opening or being dialed corrupted SGTC's hl.dll memory (crashes
   // on sg_stargate and sg_nox, 3 servers out of 3 vs none with no dialing).
   // So: no press while another gate was pressed in the last 30 s, while any
   // gate is open, or until 5 s after one closed (the map re-arms the DHDs).
   // One press per 1.2 s for the gate, whichever bot presses; nothing while
   // the DHD has no target (the gate is opening or closing).
   const char *target = STRING(g.dhd->v.target);
   float &last_press = sgtc_gate_last_press[g.number < 10 ? g.number : 0];
   if (last_press > gpGlobals->time)  // new map
      last_press = 0;
   qboolean busy = sgtc_gate_closed_time + 5.0f > gpGlobals->time && sgtc_gate_closed_time <= gpGlobals->time;
   // A gate without DHD (sg_conquest's PC): one press runs the whole sequence
   // (about 35 s), so the other gates wait 45 s after it, and it is never
   // pressed again within 45 s (a second press would start it over). A
   // blocked gate (dialed, iris closed) is as busy as an open one.
   for (int i = 0; i < sgtc_num_gates; i++)
   {
      int n = sgtc_gates[i].number < 10 ? sgtc_gates[i].number : 0;
      float lock = sgtc_gates[i].one_press ? 45.0f : 30.0f;
      if (sgtc_gates[i].open || sgtc_gates[i].blocked ||
          (n != g.number && sgtc_gate_last_press[n] + lock > gpGlobals->time && sgtc_gate_last_press[n] <= gpGlobals->time))
         busy = TRUE;
   }
   if (!busy && pBot.f_sgtc_next_use <= gpGlobals->time && last_press + (g.one_press ? 45.0f : 1.2f) <= gpGlobals->time &&
       target[0] && !FNullEnt(FIND_ENTITY_BY_TARGETNAME(NULL, target)))
   {
      pBot.f_sgtc_next_use = gpGlobals->time + 1.0f;
      last_press = gpGlobals->time;
      pEdict->v.button |= IN_USE;
      BotTrace(pBot, "sgtc gate %d: press DHD (target=%s)", g.number, STRING(g.dhd->v.target));
      if (g.one_press)
         UTIL_ConsolePrintf("[sgtc-stat] gate %d: %s presses its dialer \"%s\" (target \"%s\") t=%.1f\n", g.number,
            STRING(pEdict->v.netname), STRING(g.dhd->v.targetname), target, gpGlobals->time - sgtc_state.f_round_start);
   }
   return TRUE;
}


//
// Transits (patch_teleport, 2026-10-22): ways across the map that the
// waypoints only crossed with a hand-made LIFT_START -> LIFT_END pair, or not
// at all.
//
// - special_teleport_in (SGTC): on with spawnflags 2 "Start On", moves
//   players when "players" is 1, keeps the player's offset: arrival =
//   position - source info_marker ("message") + target info_marker
//   ("target"). ctp_conquest: the only exit of the Goa'uld start room (a
//   22-unit plane in the gate opening); sg_scorpus: the Goa'uld way down (a
//   4-unit plane). The stargate quadrants ("act*") and obelisks have their
//   own code.
// - trigger_teleport with one fixed info_teleport_destination and no map
//   logic around it (master, retargeted or switched: gates, rings, telepo),
//   not into a kill room, not a fall catcher (arrival back at a spawn, or far
//   up from a volume you fall into or under the map). sg_hina: the Goa'uld's
//   one-way route around the force field.
//   A forced path from the waypoint in (or next to) the volume to the one at
//   the arrival; bots walk straight in (the waypoints in an opening can't be
//   "touched" before the teleport); the always-on special volumes are
//   coverage hazards and no flood fill step crosses them; a damaging
//   trigger_hurt across the way in (sg_scorpus "notele", on once C4 zone 2
//   blew) cuts the link while it is on.
// - long lifts: a vertical func_door under a LIFT_START waypoint whose
//   LIFT_END is 300+ units up. sg_scorpus "lift": 1800 units at 150/s,
//   called 3 s after someone stands on it, back down 10 s after arriving; the
//   top gate "exiter" opens 2 s after the arrival and crushes (10000) when it
//   closes 5 s later. Bots wait off the platform until it is down, stand
//   still on it to the top (no watchdog, strafe, jump or nav timeout: the
//   pole in the shaft broke jk_botti's own lift handling), and walk out once
//   the exit door is open.
//
#define SGTC_MAX_TPS    16
#define SGTC_MAX_LIFTS  4
#define SGTC_TP_COST    100.0f  // route cost of a teleport link: the walk into the volume

typedef struct
{
   edict_t *ent;
   qboolean special;     // special_teleport_in: keeps the offset
   Vector mins, maxs;    // the volume
   Vector from;          // where bots come from (the waypoints leading in)
   Vector entry;         // standing position where that walk meets the volume
   Vector aim;           // walk at this: just inside the volume
   Vector arrival;       // standing position at the arrival
   qboolean walk_in;     // reached on foot (not a ladder top in the air)
   int w_src, w_dest;
   edict_t *guard[2];    // damaging trigger_hurt across the way in
   int num_guards;
   qboolean closed;      // a guard is on: the link is cut
} sgtc_tp_t;

typedef struct
{
   edict_t *door;        // the platform: a vertical func_door
   edict_t *exit;        // a door between the top and the landing (sg_scorpus "exiter"), or NULL
   Vector exit_rest;
   Vector mins, maxs;    // the platform's box (horizontally)
   Vector stand;         // the LIFT_START waypoint: standing on the platform, down
   Vector spot[6];       // standing spots with a clear ride up
   int num_spots;
   float top_z;          // the LIFT_END waypoint's height
   int w_start, w_end, w_land;
} sgtc_lift_t;

static sgtc_tp_t sgtc_tps[SGTC_MAX_TPS];
static int sgtc_num_tps;
static sgtc_lift_t sgtc_lifts[SGTC_MAX_LIFTS];
static int sgtc_num_lifts;
static edict_t *sgtc_tp_vol[SGTC_MAX_TPS];  // always-on special_teleport_in volumes (coverage)
static int sgtc_num_tp_vols;

// keys these entities keep in their private data, seen while the map loads
#define SGTC_MAX_TP_KV 128
static struct { edict_t *ent; int players; char message[64]; qboolean master; } sgtc_tp_kv[SGTC_MAX_TP_KV];
static int sgtc_num_tp_kv;

// per bot (index in bots[])
typedef struct
{
   float stamp;          // last think: a gap (dead, new map) resets the rest
   int tp;               // 1-based teleport being walked into
   float tp_start, tp_until, tp_retry;
   Vector tp_last;
   int lift;             // 1-based lift being used
   int lift_state;       // 1 on the platform (down or riding), 2 at the top
   int lift_dir;         // which way it last saw the platform move (0: not yet)
   float lift_time, lift_top_time, lift_retry, lift_wait_log;
   qboolean lift_out_logged;
} sgtc_transit_t;
static sgtc_transit_t sgtc_transit[32];

static void SGTC_ResetTransits(void)
{
   sgtc_num_tps = 0;
   sgtc_num_lifts = 0;
   sgtc_num_tp_vols = 0;
   sgtc_num_tp_kv = 0;
   memset(sgtc_transit, 0, sizeof(sgtc_transit));
}

static int SGTC_TpKv(edict_t *pent)
{
   for (int i = 0; i < sgtc_num_tp_kv; i++)
      if (sgtc_tp_kv[i].ent == pent)
         return i;
   return -1;
}

void SGTC_TransitKeyValue(edict_t *pent, KeyValueData *pkvd)
{
   if (!pkvd || !pkvd->szClassName || !pkvd->szKeyName || !pkvd->szValue)
      return;
   qboolean special = !stricmp(pkvd->szClassName, "special_teleport_in");
   if (special ? (stricmp(pkvd->szKeyName, "players") && stricmp(pkvd->szKeyName, "message")) :
                 (stricmp(pkvd->szClassName, "trigger_teleport") || stricmp(pkvd->szKeyName, "master")))
      return;
   int i = SGTC_TpKv(pent);
   if (i == -1)
   {
      if (sgtc_num_tp_kv >= SGTC_MAX_TP_KV)
         return;
      i = sgtc_num_tp_kv++;
      memset(&sgtc_tp_kv[i], 0, sizeof(sgtc_tp_kv[i]));
      sgtc_tp_kv[i].ent = pent;
   }
   if (!stricmp(pkvd->szKeyName, "players"))
      sgtc_tp_kv[i].players = atoi(pkvd->szValue);
   else if (!stricmp(pkvd->szKeyName, "message"))
      safe_strcopy(sgtc_tp_kv[i].message, sizeof(sgtc_tp_kv[i].message), pkvd->szValue);
   else
      sgtc_tp_kv[i].master = pkvd->szValue[0] != 0;
}

static void SGTC_EntBox(edict_t *pent, Vector &mins, Vector &maxs)
{
   mins = pent->v.origin + pent->v.mins;
   maxs = pent->v.origin + pent->v.maxs;
}

static qboolean SGTC_InBox(const Vector &p, const Vector &mins, const Vector &maxs, float dxy, float dz)
{
   return p.x >= mins.x - dxy && p.x <= maxs.x + dxy && p.y >= mins.y - dxy && p.y <= maxs.y + dxy &&
          p.z >= mins.z - dz && p.z <= maxs.z + dz;
}

static float SGTC_BoxDist2D(const Vector &p, const Vector &mins, const Vector &maxs)
{
   float dx = p.x < mins.x ? mins.x - p.x : (p.x > maxs.x ? p.x - maxs.x : 0.0f);
   float dy = p.y < mins.y ? mins.y - p.y : (p.y > maxs.y ? p.y - maxs.y : 0.0f);
   return sqrtf(dx * dx + dy * dy);
}

// where the walk from 'from' towards the middle of the box meets it (horizontally)
static Vector SGTC_BoxEntry(const Vector &from, const Vector &mins, const Vector &maxs)
{
   Vector c = (mins + maxs) * 0.5f;
   float f[2] = {from.x, from.y}, d[2] = {c.x - from.x, c.y - from.y};
   float lo[2] = {mins.x, mins.y}, hi[2] = {maxs.x, maxs.y};
   float t0 = 0.0f;
   for (int k = 0; k < 2; k++)
   {
      if (fabs(d[k]) < 0.001f)
         continue;
      float u0 = (lo[k] - f[k]) / d[k], u1 = (hi[k] - f[k]) / d[k];
      if (u1 < u0)
         u0 = u1;
      if (u0 > t0)
         t0 = u0;
   }
   if (t0 > 1.0f)
      t0 = 1.0f;
   return Vector(from.x + d[0] * t0, from.y + d[1] * t0, from.z);
}

// a damaging trigger_hurt (on or off) where a player standing at p would be
static edict_t *SGTC_HurtAt(const Vector &p)
{
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "trigger_hurt")))
   {
      Vector mins, maxs;
      SGTC_EntBox(pent, mins, maxs);
      if (pent->v.dmg > 0 && SGTC_InBox(p, mins, maxs, 16.0f, 36.0f))
         return pent;
   }
   return NULL;
}

// distance to the nearest spawn point; the lowest spawn height in *low_z
static float SGTC_SpawnDist(const Vector &p, float *low_z)
{
   static const char *spawn_class[2] = {"info_player_torri", "info_player_goauld"};
   float best = 99999.0f;
   if (low_z)
      *low_z = 99999.0f;
   for (int c = 0; c < 2; c++)
   {
      edict_t *pent = NULL;
      while (!FNullEnt(pent = FIND_ENTITY_BY_STRING(pent, "classname", spawn_class[c])))
      {
         float d = (pent->v.origin - p).Length();
         if (d < best)
            best = d;
         if (low_z && pent->v.origin.z < *low_z)
            *low_z = pent->v.origin.z;
      }
   }
   return best;
}

// the waypoint bots enter the volume from: one in it (with the player hull;
// a hand-placed LIFT_START first: ctp_conquest 146, sg_scorpus 588), else a
// floor waypoint up to 160 units away with a clear straight walk into it
static int SGTC_TpSource(const Vector &mins, const Vector &maxs, qboolean &inside)
{
   Vector c = (mins + maxs) * 0.5f;
   int best = -1;
   float best_d = 99999.0f;
   qboolean best_lift = FALSE;

   inside = TRUE;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LIFT_END))
         continue;
      if (!SGTC_InBox(waypoints[i].origin, mins, maxs, 16.0f, 36.0f))
         continue;
      qboolean lift = (waypoints[i].flags & W_FL_LIFT_START) != 0;
      float d = (waypoints[i].origin - c).Length();
      if ((lift && !best_lift) || (lift == best_lift && d < best_d))
      {
         best = i;
         best_d = d;
         best_lift = lift;
      }
   }
   if (best != -1)
      return best;

   inside = FALSE;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LIFT_END | W_FL_LADDER | W_FL_JUMP))
         continue;
      const Vector &o = waypoints[i].origin;
      if (o.z - 36.0f > maxs.z || o.z + 36.0f < mins.z)
         continue;  // walking in at its height, the hull wouldn't touch the volume
      Vector e = SGTC_BoxEntry(o, mins, maxs);
      Vector dir = e - o;
      float d = dir.Length();
      if (d < 1.0f || d > 160.0f || d >= best_d)
         continue;
      TraceResult tr;
      UTIL_TraceHull(o, e - dir * (17.0f / d), ignore_monsters, human_hull, NULL, &tr);
      if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
         continue;
      best = i;
      best_d = d;
   }
   return best;
}

// where the waypoints leading into w come from: the mean of those outside the volume
static Vector SGTC_TpApproach(int w, const Vector &mins, const Vector &maxs)
{
   Vector sum(0, 0, 0);
   int n = 0;
   for (int j = 0; j < num_waypoints; j++)
   {
      if (j == w || (waypoints[j].flags & (W_FL_DELETED | W_FL_AIMING)) || SGTC_InBox(waypoints[j].origin, mins, maxs, 16.0f, 36.0f))
         continue;
      int path_index = -1, p;
      while ((p = WaypointFindPath(path_index, j)) != -1)
         if (p == w)
         {
            sum = sum + waypoints[j].origin;
            n++;
            break;
         }
   }
   return n ? sum * (1.0f / n) : waypoints[w].origin;
}

// the arrival's waypoint: a hand-placed LIFT_END close by first, else the nearest (or a new one)
static int SGTC_TpArrivalWaypoint(const Vector &p)
{
   int best = -1;
   float best_d = 96.0f;
   for (int i = 0; i < num_waypoints; i++)
   {
      if ((waypoints[i].flags & (W_FL_DELETED | W_FL_LIFT_END)) != W_FL_LIFT_END)
         continue;
      float d = (waypoints[i].origin - p).Length();
      if (d < best_d && fabs(waypoints[i].origin.z - p.z) < 40.0f)
      {
         best = i;
         best_d = d;
      }
   }
   return best != -1 ? best : SGTC_WaypointAt(p, 96.0f, 0);
}

static qboolean SGTC_TpGuarded(const sgtc_tp_t &tp, const char **name)
{
   for (int g = 0; g < tp.num_guards; g++)
   {
      edict_t *h = tp.guard[g];
      if (!FNullEnt(h) && !h->free && !strcmp(STRING(h->v.classname), "trigger_hurt") && h->v.solid != SOLID_NOT)
      {
         if (name)
            *name = STRING(h->v.targetname);
         return TRUE;
      }
   }
   return FALSE;
}

static void SGTC_LinkTeleport(edict_t *pent, qboolean special, const Vector &src_marker, const Vector &dest, float low_spawn)
{
   Vector mins, maxs;
   SGTC_EntBox(pent, mins, maxs);
   char what[192];
   safevoid_snprintf(what, sizeof(what), "%s \"%s\" -> \"%s\" (%.0f %.0f %.0f - %.0f %.0f %.0f)",
      special ? "special_teleport_in" : "trigger_teleport", STRING(pent->v.targetname), STRING(pent->v.target),
      mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
   if (sgtc_num_tps >= SGTC_MAX_TPS)
   {
      UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: %s: too many teleports\n", what);
      return;
   }
   qboolean inside;
   int w_src = SGTC_TpSource(mins, maxs, inside);
   if (w_src == -1)
   {
      UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: %s: no waypoint in it or walking distance from it\n", what);
      return;
   }

   sgtc_tp_t &tp = sgtc_tps[sgtc_num_tps];
   memset(&tp, 0, sizeof(tp));
   tp.ent = pent;
   tp.special = special;
   tp.mins = mins;
   tp.maxs = maxs;

   // the way in: from the waypoints leading there, towards the middle of the
   // volume; on foot if there is floor just before it at the height of the
   // source waypoint (hand-placed ones float 36-60 units over it) or of the
   // way there, and the hull standing there reaches the volume
   tp.from = inside ? SGTC_TpApproach(w_src, mins, maxs) : waypoints[w_src].origin;
   Vector dir = (mins + maxs) * 0.5f - tp.from;
   dir.z = 0;
   dir = dir.Length() > 1.0f ? dir.Normalize() : Vector(1, 0, 0);
   Vector e = SGTC_BoxEntry(tp.from, mins, maxs), stand;
   Vector probe = e - dir * 20.0f;
   probe.z = waypoints[w_src].origin.z + 18.0f;
   tp.entry = Vector(e.x, e.y, waypoints[w_src].origin.z);
   qboolean floor = SGTC_GroundAt(probe, 18.0f + 100.0f, stand);
   if (!floor)
   {
      probe.z = tp.from.z + 18.0f;
      floor = SGTC_GroundAt(probe, 18.0f + 60.0f, stand);
   }
   if (floor && stand.z - 36.0f <= maxs.z && stand.z + 36.0f >= mins.z)
   {
      TraceResult tr;
      UTIL_TraceHull(stand, Vector(e.x - dir.x * 17.0f, e.y - dir.y * 17.0f, stand.z), ignore_monsters, human_hull, NULL, &tr);
      if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction >= 1.0f)
      {
         tp.walk_in = TRUE;
         tp.entry = Vector(e.x, e.y, stand.z);
      }
   }
   tp.aim = tp.entry + dir * 8.0f;

   // the arrival: the destination for a trigger_teleport (TeleportTouch puts
   // the origin 37 units over it), the offset kept for a special_teleport_in
   // (from where the hull first touches the volume)
   Vector arrive;
   if (special)
      arrive = (tp.walk_in ? tp.entry - dir * 16.0f : waypoints[w_src].origin) - src_marker + dest;
   else
      arrive = dest + Vector(0, 0, 37);
   Vector ground;
   if (SGTC_GroundAt(arrive + Vector(0, 0, 2), 600.0f, ground) || SGTC_GroundAt(arrive + Vector(0, 0, 40), 640.0f, ground))
      arrive = ground;
   tp.arrival = arrive;

   char why[128];
   why[0] = 0;
   float spawn_d = SGTC_SpawnDist(arrive, NULL);
   edict_t *hurt = SGTC_HurtAt(arrive);
   if (!FNullEnt(hurt))
      safevoid_snprintf(why, sizeof(why), "a kill room (trigger_hurt \"%s\", %.0f damage)", STRING(hurt->v.targetname), hurt->v.dmg);
   else if (!special && spawn_d < 256.0f)
      safevoid_snprintf(why, sizeof(why), "back at a spawn (%.0f units): a fall catcher", spawn_d);
   else if (!special && arrive.z - maxs.z > 512.0f && ((!tp.walk_in && !inside) || maxs.z < low_spawn - 640.0f))
      safevoid_snprintf(why, sizeof(why), "%.0f units up from a volume %s: a fall catcher", arrive.z - maxs.z,
         maxs.z < low_spawn - 640.0f ? "under the map" : "you fall into");
   if (why[0])
   {
      UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: %s: arrival %.0f %.0f %.0f is %s\n", what, arrive.x, arrive.y, arrive.z, why);
      return;
   }
   int w_dest = SGTC_TpArrivalWaypoint(arrive);
   if (w_dest == -1 || w_dest == w_src)
   {
      UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: %s: no waypoint at the arrival %.0f %.0f %.0f\n", what, arrive.x, arrive.y, arrive.z);
      return;
   }
   tp.w_src = w_src;
   tp.w_dest = w_dest;

   // damaging trigger_hurts across the way in: the link is cut while one is on
   edict_t *h = NULL;
   while (!FNullEnt(h = FIND_ENTITY_BY_CLASSNAME(h, "trigger_hurt")) && tp.num_guards < 2)
   {
      Vector hmins, hmaxs;
      SGTC_EntBox(h, hmins, hmaxs);
      if (h->v.dmg > 0 && SGTC_SegmentHitsBox(tp.from, tp.aim, hmins - Vector(16, 16, 36), hmaxs + Vector(16, 16, 36)))
         tp.guard[tp.num_guards++] = h;
   }

   // a waypoint in the volume: reaching it is the teleport, keep no other way on
   int cut = 0;
   if (inside)
   {
      int out[64], n = 0, path_index = -1, p;
      while ((p = WaypointFindPath(path_index, w_src)) != -1 && n < 64)
         if (p != w_dest)
            out[n++] = p;
      for (int k = 0; k < n; k++)
         WaypointCutPath(w_src, out[k]);
      cut = n;
   }
   WaypointForceLongPath(w_src, w_dest, SGTC_TP_COST);
   if (SGTC_TpGuarded(tp, NULL))
   {
      WaypointCutPath(w_src, w_dest);
      tp.closed = TRUE;
   }
   sgtc_num_tps++;
   UTIL_ConsolePrintf("[sgtc-stat] teleport %d: %s: wpt %d (%s, from %.0f %.0f %.0f) -> %.0f %.0f %.0f (wpt %d%s), cost %.0f, %d other paths out cut\n",
      sgtc_num_tps, what, w_src, inside ? (tp.walk_in ? "in it, walked into" : "in it, climbed to") : "walked into from there",
      tp.from.x, tp.from.y, tp.from.z, arrive.x, arrive.y, arrive.z, w_dest,
      (waypoints[w_dest].flags & W_FL_LIFT_END) ? ", hand-placed LIFT_END" : "", SGTC_TP_COST, cut);
   for (int g = 0; g < tp.num_guards; g++)
      UTIL_ConsolePrintf("[sgtc-stat]   way in guarded by trigger_hurt \"%s\" (%.0f damage, %s now%s)\n", STRING(tp.guard[g]->v.targetname),
         tp.guard[g]->v.dmg, tp.guard[g]->v.solid != SOLID_NOT ? "on" : "off", tp.closed ? ": link cut" : "");
}

// the platform right under the LIFT_START spot at that height, and still
static qboolean SGTC_LiftAt(const sgtc_lift_t &L, float z)
{
   if (FNullEnt(L.door) || L.door->v.velocity.Length() > 1.0f)
      return FALSE;
   TraceResult tr;
   Vector p(L.stand.x, L.stand.y, z);
   UTIL_TraceLine(p, p - Vector(0, 0, 72), ignore_monsters, NULL, &tr);
   return tr.flFraction < 1.0f && tr.pHit == L.door;
}

// the exit door up out of the way (sg_scorpus "exiter": 120 units up in
// 1.2 s; a standing player passes under it from 66), and not closing
static qboolean SGTC_LiftExitOpen(const sgtc_lift_t &L)
{
   if (FNullEnt(L.exit))
      return TRUE;
   Vector off = L.exit->v.origin - L.exit_rest;
   float moved = off.Length();
   if (DotProduct(L.exit->v.velocity, off) < 0.0f)
      return FALSE;
   return moved >= 80.0f || (moved >= 24.0f && L.exit->v.velocity.Length() < 1.0f);
}

static void SGTC_BuildLifts(void)
{
   for (int s = 0; s < num_waypoints && sgtc_num_lifts < SGTC_MAX_LIFTS; s++)
   {
      if ((waypoints[s].flags & (W_FL_DELETED | W_FL_LIFT_START)) != W_FL_LIFT_START)
         continue;
      Vector ws = waypoints[s].origin;
      int path_index = -1, e;
      while ((e = WaypointFindPath(path_index, s)) != -1 && sgtc_num_lifts < SGTC_MAX_LIFTS)
      {
         if ((waypoints[e].flags & (W_FL_DELETED | W_FL_LIFT_END)) != W_FL_LIFT_END || waypoints[e].origin.z - ws.z < 300.0f)
            continue;

         // the platform: a vertical func_door right under the LIFT_START
         edict_t *door = NULL, *pent = NULL;
         Vector mins, maxs;
         while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "func_door")))
         {
            SGTC_EntBox(pent, mins, maxs);
            if (fabs(pent->v.movedir.z) > 0.9f && ws.x >= mins.x && ws.x <= maxs.x && ws.y >= mins.y && ws.y <= maxs.y &&
                mins.z < ws.z - 36.0f && mins.z > ws.z - 36.0f - 64.0f)
            {
               door = pent;
               break;
            }
         }
         if (!door)
         {
            UTIL_ConsolePrintf("[sgtc-stat] lift skipped: wpt %d -> %d (%.0f units up): no vertical func_door under it (or not down now)\n",
               s, e, waypoints[e].origin.z - ws.z);
            continue;
         }

         sgtc_lift_t &L = sgtc_lifts[sgtc_num_lifts];
         memset(&L, 0, sizeof(L));
         L.door = door;
         L.mins = mins;
         L.maxs = maxs;
         L.stand = ws;
         L.top_z = waypoints[e].origin.z;
         L.w_start = s;
         L.w_end = e;
         L.w_land = -1;

         // the landing (the LIFT_END's way on) and a door across the way there
         int pi2 = -1, l;
         while ((l = WaypointFindPath(pi2, e)) != -1 && FNullEnt(L.exit))
         {
            if (l == s || (waypoints[l].flags & W_FL_DELETED))
               continue;
            if (L.w_land == -1)
               L.w_land = l;
            edict_t *x = NULL;
            while (!FNullEnt(x = FIND_ENTITY_BY_CLASSNAME(x, "func_door")))
            {
               if (x == door)
                  continue;
               Vector xmins, xmaxs;
               SGTC_EntBox(x, xmins, xmaxs);
               if (SGTC_SegmentHitsBox(waypoints[e].origin, waypoints[l].origin, xmins - Vector(16, 16, 36), xmaxs + Vector(16, 16, 36)))
               {
                  L.w_land = l;
                  L.exit = x;
                  L.exit_rest = x->v.origin;
                  break;
               }
            }
         }
         if (L.w_land == -1)
         {
            UTIL_ConsolePrintf("[sgtc-stat] lift skipped: wpt %d -> %d: no way off at the top\n", s, e);
            continue;
         }

         // spots on the platform with nothing in the way up (the pole, the
         // top gate over the platform's south edge): a bot there doesn't block it
         static const float around[7][2] = {{0, 0}, {32, 0}, {-32, 0}, {0, 32}, {0, -32}, {64, 0}, {-64, 0}};
         for (int k = 0; k < 7 && L.num_spots < 6; k++)
         {
            Vector p = ws + Vector(around[k][0], around[k][1], 0);
            if (p.x < mins.x + 16.0f || p.x > maxs.x - 16.0f || p.y < mins.y + 16.0f || p.y > maxs.y - 16.0f)
               continue;
            TraceResult tr;
            UTIL_TraceHull(p, p + Vector(0, 0, L.top_z - ws.z), ignore_monsters, human_hull, NULL, &tr);
            if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
               continue;
            L.spot[L.num_spots++] = p;
         }
         if (!L.num_spots)
            L.spot[L.num_spots++] = ws;

         sgtc_num_lifts++;
         UTIL_ConsolePrintf("[sgtc-stat] lift %d \"%s\": platform wpt %d (%.0f %.0f %.0f) -> wpt %d, %.0f units up, %d spots, exit door %s%s%s -> wpt %d (%.0f %.0f %.0f)\n",
            sgtc_num_lifts, STRING(door->v.targetname), s, ws.x, ws.y, ws.z, e, L.top_z - ws.z, L.num_spots,
            FNullEnt(L.exit) ? "none" : "\"", FNullEnt(L.exit) ? "" : STRING(L.exit->v.targetname), FNullEnt(L.exit) ? "" : "\"",
            L.w_land, waypoints[L.w_land].origin.x, waypoints[L.w_land].origin.y, waypoints[L.w_land].origin.z);
      }
   }
}

// once per map, after the coverage and the gate/ring/door paths
static void SGTC_BuildTransits(void)
{
   sgtc_num_tps = 0;
   sgtc_num_lifts = 0;
   memset(sgtc_transit, 0, sizeof(sgtc_transit));
   float low_spawn;
   SGTC_SpawnDist(Vector(0, 0, 0), &low_spawn);
   int logic = 0;

   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "special_teleport_in")))
   {
      const char *name = STRING(pent->v.targetname);
      if (!strncmp(name, "act", 3) || !strncmp(name, "obelisk", 7))
         continue;  // stargate quadrants, obelisk transports: their own code
      int k = SGTC_TpKv(pent);
      const char *message = (k != -1 && sgtc_tp_kv[k].message[0]) ? sgtc_tp_kv[k].message : STRING(pent->v.message);
      const char *target = STRING(pent->v.target);
      edict_t *src = message[0] ? FIND_ENTITY_BY_TARGETNAME(NULL, message) : NULL;
      edict_t *dst = target[0] ? FIND_ENTITY_BY_TARGETNAME(NULL, target) : NULL;
      const char *why = NULL;
      if (k == -1 || sgtc_tp_kv[k].players != 1)
         why = "doesn't move players";
      else if (!(pent->v.spawnflags & 2))
         why = "not \"Start On\"";
      else if (name[0] && !FNullEnt(FIND_ENTITY_BY_STRING(NULL, "target", name)))
         why = "switched by map logic";
      else if (FNullEnt(src) || FNullEnt(dst))
         why = "source or target info_marker missing";
      if (why)
      {
         Vector c = SGTC_EntCenter(pent);
         UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: special_teleport_in \"%s\" at %.0f %.0f %.0f: %s\n", name, c.x, c.y, c.z, why);
         continue;
      }
      SGTC_LinkTeleport(pent, TRUE, src->v.origin, dst->v.origin, low_spawn);
   }

   pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "trigger_teleport")))
   {
      const char *name = STRING(pent->v.targetname), *target = STRING(pent->v.target);
      if (!target[0] || (name[0] == 'g' && strstr(name, "tele")))
         continue;  // kawoosh zones (no destination), gates: their own code
      int k = SGTC_TpKv(pent);
      if ((k != -1 && sgtc_tp_kv[k].master) || (name[0] && !FNullEnt(FIND_ENTITY_BY_STRING(NULL, "target", name))))
      {
         logic++;  // rings (master), telepo / rt* (retargeted), switched
         continue;
      }
      if (pent->v.spawnflags & 2)
         continue;  // SF_TRIGGER_NOCLIENTS
      int n = 0;
      edict_t *dest = NULL, *d = NULL;
      while (!FNullEnt(d = FIND_ENTITY_BY_TARGETNAME(d, target)))
      {
         dest = d;
         n++;
      }
      if (n != 1)
      {
         Vector c = SGTC_EntCenter(pent);
         UTIL_ConsolePrintf("[sgtc-stat] teleport skipped: trigger_teleport \"%s\" at %.0f %.0f %.0f: %s \"%s\"\n", name, c.x, c.y, c.z,
            n ? "several destinations named" : "no destination", target);
         continue;
      }
      SGTC_LinkTeleport(pent, FALSE, Vector(0, 0, 0), dest->v.origin, low_spawn);
   }
   if (logic)
      UTIL_ConsolePrintf("[sgtc-stat] teleports: %d trigger_teleport(s) left to map logic (master, retargeted or switched)\n", logic);

   SGTC_BuildLifts();
   if (sgtc_num_tps)
      WaypointRebuildRoutes();
}

// every 0.5 s: a trigger_hurt across a teleport's way in switched on or off
static void SGTC_WatchTransits(void)
{
   qboolean changed = FALSE;
   for (int i = 0; i < sgtc_num_tps; i++)
   {
      sgtc_tp_t &tp = sgtc_tps[i];
      if (!tp.num_guards)
         continue;
      const char *name = "";
      qboolean guarded = SGTC_TpGuarded(tp, &name);
      if (guarded == tp.closed)
         continue;
      tp.closed = guarded;
      if (guarded)
         WaypointCutPath(tp.w_src, tp.w_dest);
      else
         WaypointForcePath(tp.w_src, tp.w_dest);
      changed = TRUE;
      UTIL_ConsolePrintf("[sgtc-stat] teleport %d: way in %s, wpt %d -> %d %s t=%.1f\n", i + 1,
         guarded ? "closed (trigger_hurt on)" : "open again", tp.w_src, tp.w_dest, guarded ? "cut" : "restored",
         gpGlobals->time - sgtc_state.f_round_start);
      if (guarded && name[0])
         UTIL_ConsolePrintf("[sgtc-stat]   by trigger_hurt \"%s\"\n", name);
   }
   if (changed)
      WaypointRebuildRoutes();
}

// always-on special_teleport_in that move players: coverage hazards, and
// no flood fill / approach step across them (sg_scorpus: a 4-unit plane)
static qboolean SGTC_SpecialOn(edict_t *pent)
{
   int k = SGTC_TpKv(pent);
   const char *name = STRING(pent->v.targetname);
   return k != -1 && sgtc_tp_kv[k].players == 1 && (pent->v.spawnflags & 2) && strncmp(name, "act", 3) && strncmp(name, "obelisk", 7);
}

static void SGTC_TransitHazards(void)
{
   sgtc_num_tp_vols = 0;
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "special_teleport_in")))
   {
      if (!SGTC_SpecialOn(pent) || sgtc_num_tp_vols >= SGTC_MAX_TPS)
         continue;
      sgtc_tp_vol[sgtc_num_tp_vols++] = pent;
      if (sgtc_cov_num_hazards < 128)
         sgtc_cov_hazard[sgtc_cov_num_hazards++] = pent;
   }
   if (sgtc_num_tp_vols)
      UTIL_ConsolePrintf("[sgtc-stat] coverage: %d always-on special_teleport_in volume(s) as hazards\n", sgtc_num_tp_vols);
}

static qboolean SGTC_CrossesTeleport(const Vector &a, const Vector &b)
{
   for (int i = 0; i < sgtc_num_tp_vols; i++)
   {
      Vector mins, maxs;
      SGTC_EntBox(sgtc_tp_vol[i], mins, maxs);
      if (SGTC_SegmentHitsBox(a, b, mins - Vector(16, 16, 36), maxs + Vector(16, 16, 36)))
         return TRUE;
   }
   return FALSE;
}

// in a linked teleport volume (with the player hull): no area goal there
static qboolean SGTC_InTeleport(const Vector &p)
{
   for (int i = 0; i < sgtc_num_tps; i++)
      if (SGTC_InBox(p, sgtc_tps[i].mins, sgtc_tps[i].maxs, 16.0f, 36.0f))
         return TRUE;
   return FALSE;
}

// stand still: no move, strafe, jump or duck, and no "nav timeout" meanwhile
static void SGTC_TransitHold(bot_t &pBot)
{
   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.f_strafe_time = gpGlobals->time + 0.5f;
   pBot.b_not_maxspeed = TRUE;
   pBot.pEdict->v.button &= ~(IN_JUMP | IN_DUCK);
   pBot.f_waypoint_time = gpGlobals->time;
}

// a straight move in world space (facing that way unless fighting)
static void SGTC_TransitWalk(bot_t &pBot, const Vector &target, float speed)
{
   edict_t *pEdict = pBot.pEdict;
   SGTC_TransitHold(pBot);
   Vector to = target - pEdict->v.origin;
   to.z = 0;
   if (to.Length() < 1.0f)
      return;
   if (pBot.pBotEnemy == NULL)
   {
      pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
      BotFixIdealYaw(pEdict);
   }
   pBot.v_sgtc_move = to.Normalize() * speed;
   pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
}

static qboolean SGTC_BotLiftThink(bot_t &pBot, sgtc_transit_t &s)
{
   edict_t *pEdict = pBot.pEdict;
   const Vector &o = pEdict->v.origin;
   float t = gpGlobals->time;
   const char *botname = STRING(pEdict->v.netname);

   if (!sgtc_num_lifts)
      return FALSE;
   if (s.lift > sgtc_num_lifts)
      s.lift = 0;

   if (!s.lift)
   {
      // on a moving lift, whatever the route: hold still (sg_scorpus: a
      // strafe off the platform is a fall down an 1800-unit shaft)
      for (int i = 0; i < sgtc_num_lifts; i++)
         if (!FNullEnt(sgtc_lifts[i].door) && pEdict->v.groundentity == sgtc_lifts[i].door &&
             sgtc_lifts[i].door->v.velocity.Length() > 1.0f)
         {
            s.lift = i + 1;
            s.lift_state = 1;
            s.lift_dir = 0;
            s.lift_time = t;
            s.lift_out_logged = FALSE;
            UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s rides along (%s) t=%.1f\n", i + 1, botname,
               sgtc_lifts[i].door->v.velocity.z > 0 ? "up" : "down", t);
            break;
         }
   }
   if (!s.lift)
   {
      int w = pBot.curr_waypoint_index, goal = pBot.waypoint_goal;
      if (w < 0 || pBot.pBotEnemy != NULL || s.lift_retry > t)
         return FALSE;
      for (int i = 0; i < sgtc_num_lifts; i++)
      {
         sgtc_lift_t &L = sgtc_lifts[i];
         qboolean via = (w == L.w_start && goal != -1 && (goal == L.w_end || WaypointRouteFromTo(L.w_start, goal) == L.w_end)) ||
                        (w == L.w_end && (pBot.v_prev_waypoint_origin - L.stand).Length() < 1.0f);
         if (!via || fabs(o.z - L.stand.z) > 64.0f)
            continue;
         qboolean on = SGTC_InBox(o, L.mins, L.maxs, 0.0f, 99999.0f);
         qboolean down = SGTC_LiftAt(L, L.stand.z);
         if (on && down)
         {
            s.lift = i + 1;
            s.lift_state = 1;
            s.lift_dir = 0;
            s.lift_time = t;
            s.lift_out_logged = FALSE;
            UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s boards t=%.1f\n", i + 1, botname, t);
            BotTrace(pBot, "sgtc lift %d: boards (goal wpt=%d)", i + 1, goal);
            break;
         }
         if (down)
            return FALSE;  // walk on: jk_botti heads for the LIFT_START

         // the lift is away: wait off the platform (in the shaft it would come down on the bot)
         Vector c = (L.mins + L.maxs) * 0.5f;
         if (on)
         {
            Vector back = pBot.v_prev_waypoint_origin;
            if (back == Vector(0, 0, 0) || SGTC_InBox(back, L.mins, L.maxs, 16.0f, 99999.0f))
            {
               Vector away = o - c;
               away.z = 0;
               back = o + (away.Length() > 1.0f ? away.Normalize() : Vector(0, -1, 0)) * 120.0f;
            }
            SGTC_TransitWalk(pBot, back, 200.0f);
            return TRUE;
         }
         if (SGTC_BoxDist2D(o, L.mins, L.maxs) > 160.0f)
            return FALSE;  // not there yet
         SGTC_TransitHold(pBot);
         Vector to = c - o;
         to.z = 0;
         if (to.Length() > 1.0f)
         {
            pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
            BotFixIdealYaw(pEdict);
         }
         if (s.lift_wait_log + 15.0f < t || s.lift_wait_log > t)
         {
            s.lift_wait_log = t;
            UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s waits off the platform at %.0f %.0f %.0f (lift %s) t=%.1f\n", i + 1, botname,
               o.x, o.y, o.z, L.door->v.velocity.Length() > 1.0f ? "moving" : "up", t);
         }
         return TRUE;
      }
      if (!s.lift)
         return FALSE;
   }

   int li = s.lift;
   sgtc_lift_t &L = sgtc_lifts[li - 1];
   if (s.lift_state == 1)
   {
      if (!SGTC_InBox(o, L.mins, L.maxs, 8.0f, 99999.0f) || o.z < L.stand.z - 100.0f)
      {
         UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s left the platform at %.0f %.0f %.0f t=%.1f\n", li, botname, o.x, o.y, o.z, t);
         s.lift = 0;
         return FALSE;
      }
      if (o.z > L.top_z - 60.0f && SGTC_LiftAt(L, L.top_z))
      {
         s.lift_state = 2;
         s.lift_top_time = t;
         UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s at the top after %.1f s\n", li, botname, t - s.lift_time);
      }
      else
      {
         if (L.door->v.velocity.z > 1.0f)
            s.lift_dir = 1;
         else if (L.door->v.velocity.z < -1.0f)
            s.lift_dir = -1;
         qboolean down = fabs(o.z - L.stand.z) < 40.0f && SGTC_LiftAt(L, L.stand.z);
         if (down && s.lift_dir < 0)
         {
            // it rode the platform back down (stepped on it up there, or it never got off)
            UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s back down on the platform, %.1f s after getting on\n", li, botname, t - s.lift_time);
            s.lift = 0;
            return FALSE;
         }
         if (down && !s.lift_dir && s.lift_time + 10.0f < t)
         {
            // nothing moved it (sg_scorpus: the platform's trigger fires 3 s after it is touched, then waits 30 s)
            UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s gave up, down and still for 10 s on the platform\n", li, botname);
            s.lift = 0;
            s.lift_retry = t + 20.0f;
            pBot.waypoint_goal = -1;
            pBot.f_waypoint_goal_time = 0;
            return FALSE;
         }
         // to its spot (nothing over it all the way up), riding or not, then still
         Vector spot = L.spot[(int)(&s - sgtc_transit) % L.num_spots];
         spot.z = o.z;
         if ((spot - o).Length() > 12.0f)
            SGTC_TransitWalk(pBot, spot, 120.0f);
         else
            SGTC_TransitHold(pBot);
         pBot.curr_waypoint_index = L.w_end;
         pBot.waypoint_origin = waypoints[L.w_end].origin;
         return TRUE;
      }
   }

   // at the top: off to the landing once the exit door is open
   int land = L.w_land;
   Vector to = waypoints[land].origin - o;
   to.z = 0;
   if (to.Length() < 40.0f || !SGTC_InBox(o, L.mins, L.maxs, 24.0f, 99999.0f))
   {
      UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s off at wpt %d, %.1f s after boarding\n", li, botname, land, t - s.lift_time);
      BotTrace(pBot, "sgtc lift %d: off at the top", li);
      s.lift = 0;
      pBot.curr_waypoint_index = land;
      pBot.waypoint_origin = waypoints[land].origin;
      pBot.f_waypoint_time = t;
      return FALSE;
   }
   if (o.z < L.top_z - 100.0f)
   {
      UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s went back down with it\n", li, botname);
      s.lift = 0;
      return FALSE;
   }
   if (!SGTC_LiftExitOpen(L))
   {
      if (s.lift_top_time + 9.0f < t)
      {
         UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s gave up, the exit door stayed shut 9 s\n", li, botname);
         s.lift = 0;
         s.lift_retry = t + 10.0f;
         return FALSE;
      }
      SGTC_TransitHold(pBot);
      return TRUE;
   }
   if (!s.lift_out_logged)
   {
      s.lift_out_logged = TRUE;
      UTIL_ConsolePrintf("[sgtc-stat] lift %d: %s walks out, exit door open %.1f s after arrival\n", li, botname, t - s.lift_top_time);
   }
   SGTC_TransitWalk(pBot, waypoints[land].origin, pBot.f_max_speed);
   return TRUE;
}

static qboolean SGTC_BotTeleportThink(bot_t &pBot, sgtc_transit_t &s)
{
   edict_t *pEdict = pBot.pEdict;
   const Vector &o = pEdict->v.origin;
   float t = gpGlobals->time;

   if (!sgtc_num_tps)
      return FALSE;
   if (s.tp > sgtc_num_tps)
      s.tp = 0;
   if (s.tp)
   {
      sgtc_tp_t &tp = sgtc_tps[s.tp - 1];
      if ((o - s.tp_last).Length() > 200.0f)
      {
         UTIL_ConsolePrintf("[sgtc-stat] teleport %d: %s went through in %.1f s, now at %.0f %.0f %.0f (expected %.0f %.0f %.0f)\n", s.tp,
            STRING(pEdict->v.netname), t - s.tp_start, o.x, o.y, o.z, tp.arrival.x, tp.arrival.y, tp.arrival.z);
         BotTrace(pBot, "sgtc teleport %d: through, now at %.0f %.0f %.0f", s.tp, o.x, o.y, o.z);
         s.tp = 0;
         pBot.curr_waypoint_index = -1;
         return FALSE;
      }
      if (s.tp_until < t || pBot.pBotEnemy != NULL || tp.closed)
      {
         UTIL_ConsolePrintf("[sgtc-stat] teleport %d: %s gave up (%s) at %.0f %.0f %.0f\n", s.tp, STRING(pEdict->v.netname),
            tp.closed ? "way in closed" : (pBot.pBotEnemy != NULL ? "enemy" : "not in after 4 s"), o.x, o.y, o.z);
         s.tp = 0;
         s.tp_retry = t + 5.0f;
         return FALSE;
      }
      s.tp_last = o;
      SGTC_TransitWalk(pBot, tp.aim, pBot.f_max_speed);
      return TRUE;
   }

   int w = pBot.curr_waypoint_index, goal = pBot.waypoint_goal;
   if (w < 0 || pBot.pBotEnemy != NULL || s.tp_retry > t || pBot.b_on_ladder)
      return FALSE;
   for (int i = 0; i < sgtc_num_tps; i++)
   {
      sgtc_tp_t &tp = sgtc_tps[i];
      if (tp.closed || !tp.walk_in)
         continue;  // a ladder top in the volume: jk_botti's climb gets there
      qboolean via = (w == tp.w_src && goal != -1 && (goal == tp.w_dest || WaypointRouteFromTo(tp.w_src, goal) == tp.w_dest)) ||
                     (w == tp.w_dest && (pBot.v_prev_waypoint_origin - waypoints[tp.w_src].origin).Length() < 1.0f);
      if (!via)
         continue;
      Vector d = tp.entry - o;
      if (fabs(d.z) > 48.0f || d.Make2D().Length() > 200.0f)
         continue;
      // a clear straight walk until the hull touches it (else the route gets nearer first)
      Vector dir = tp.aim - o;
      dir.z = 0;
      if (dir.Length() < 1.0f)
         continue;
      dir = dir.Normalize();
      TraceResult tr;
      int hull = (pEdict->v.flags & FL_DUCKING) ? head_hull : human_hull;
      Vector touch = Vector(tp.aim.x, tp.aim.y, o.z) - dir * 25.0f;
      UTIL_TraceHull(o, touch, ignore_monsters, hull, pEdict, &tr);
      if (tr.fStartSolid || tr.fAllSolid || tr.flFraction < 1.0f)
         continue;
      // and floor all the way (the straight walk skips the ledge checks)
      qboolean floor = TRUE;
      float len = (touch - o).Length();
      for (float k = 32.0f; k < len && floor; k += 32.0f)
      {
         Vector g;
         floor = SGTC_GroundAt(o + dir * k + Vector(0, 0, 18), 18.0f + 60.0f, g);
      }
      if (!floor)
         continue;
      s.tp = i + 1;
      s.tp_start = t;
      s.tp_until = t + 4.0f;
      s.tp_last = o;
      UTIL_ConsolePrintf("[sgtc-stat] teleport %d: %s walks in from %.0f %.0f %.0f (wpt %d -> %d) t=%.1f\n", i + 1,
         STRING(pEdict->v.netname), o.x, o.y, o.z, tp.w_src, tp.w_dest, t);
      BotTrace(pBot, "sgtc teleport %d: walk in (goal wpt=%d)", i + 1, goal);
      SGTC_TransitWalk(pBot, tp.aim, pBot.f_max_speed);
      return TRUE;
   }
   return FALSE;
}

static qboolean SGTC_BotTransitThink(bot_t &pBot)
{
   int bi = (int)(&pBot - bots);
   if (bi < 0 || bi >= 32 || (!sgtc_num_tps && !sgtc_num_lifts))
      return FALSE;
   sgtc_transit_t &s = sgtc_transit[bi];
   if (s.stamp > gpGlobals->time || s.stamp + 1.0f < gpGlobals->time)
      memset(&s, 0, sizeof(s));  // a new map, or it was dead
   s.stamp = gpGlobals->time;
   if (SGTC_BotLiftThink(pBot, s))
      return TRUE;
   return SGTC_BotTeleportThink(pBot, s);
}

static qboolean SGTC_SetGoal(bot_t &pBot, int index, qboolean dynamic, const char *why)
{
   if (index == -1)
      return FALSE;

   // goal cut off from here: dial a gate / go through it first
   int detour = SGTC_GateDetour(pBot, index, &why);
   if (detour != -1)
   {
      index = detour;
      dynamic = FALSE;
   }
   else if (!SGTC_Reachable(pBot, index))
      return FALSE;  // cut off and no gate to use: try something else
   else if (SGTC_DoorCutsOff(pBot, index))
      return FALSE;  // behind a shut door it can't open

   if (SGTC_TransportRefuses(pBot, index))
      return FALSE;  // over a button transporter this bot can't use

   pBot.wpt_goal_type = WPT_GOAL_LOCATION;
   pBot.waypoint_goal = index;
   pBot.pTrackSoundEdict = NULL;
   pBot.f_track_sound_time = -1;
   pBot.f_sgtc_goal_time = gpGlobals->time;
   pBot.sgtc_goal_dynamic = dynamic;

   BotTrace(pBot, "goal set: sgtc %s wpt=%d", why, index);
   return TRUE;
}

//
// Called when the bot has nothing better to do (no enemy, no sound to track):
// go play the map objective for its side instead of wandering at random.
//
// C4 (weapon id 14 in SGTC 2.0) in the bot's inventory
#define SGTC_WEAPON_C4 14
static qboolean SGTC_HasC4(edict_t *pEdict)
{
   return (pEdict->v.weapons & (1u << SGTC_WEAPON_C4)) != 0;
}

// the wall a C4 zone blows up (its target, a func_breakable) still stands
static qboolean SGTC_C4ZoneLive(int i)
{
   edict_t *zone = sgtc_obj.bombzone[i];
   if (FNullEnt(zone))
      return FALSE;
   const char *target = STRING(zone->v.target);
   if (!target[0])
      return TRUE;
   edict_t *pWall = FIND_ENTITY_BY_TARGETNAME(NULL, target);
   // not a wall (sg_strike3: a multisource): live until blown this round
   if (!FNullEnt(pWall) && strcmp(STRING(pWall->v.classname), "func_breakable") != 0)
      return i >= 8 || !sgtc_c4_zone_blown[i];
   return !FNullEnt(pWall) && pWall->v.solid != SOLID_NOT && pWall->v.health > 0;
}

// "[sgtc-stat] c4 zone N: wall down" when it goes (every 0.5 s)
static void SGTC_WatchC4Walls(void)
{
   static qboolean was_live[SGTC_MAX_OBJ];
   static float last_time = 0;
   if (gpGlobals->time < last_time)  // a new map
      memset(was_live, 0, sizeof(was_live));
   last_time = gpGlobals->time;
   for (int i = 0; i < sgtc_obj.num_bombzones; i++)
   {
      qboolean live = SGTC_C4ZoneLive(i);
      if (was_live[i] && !live)
         UTIL_ConsolePrintf("[sgtc-stat] c4 zone %d: wall \"%s\" down, t=%.1f\n", i, STRING(sgtc_obj.bombzone[i]->v.target), gpGlobals->time);
      was_live[i] = live;
   }
}

// the C4 zone a point is in (-1: none); dist = how far out of the nearest one
static int SGTC_C4ZoneAt(const Vector &p, float &dist)
{
   dist = 99999.0f;
   for (int i = 0; i < sgtc_obj.num_bombzones; i++)
   {
      edict_t *zone = sgtc_obj.bombzone[i];
      if (FNullEnt(zone))
         continue;
      Vector mins = zone->v.origin + zone->v.mins, maxs = zone->v.origin + zone->v.maxs;
      Vector out(p.x < mins.x ? mins.x - p.x : (p.x > maxs.x ? p.x - maxs.x : 0),
                 p.y < mins.y ? mins.y - p.y : (p.y > maxs.y ? p.y - maxs.y : 0),
                 p.z < mins.z ? mins.z - p.z : (p.z > maxs.z ? p.z - maxs.z : 0));
      if (out.Length() == 0)
      {
         dist = 0;
         return i;
      }
      if (out.Length() < dist)
         dist = out.Length();
   }
   return -1;
}

// "[sgtc-stat] c4 charge ..." when a charge appears and when it goes off
static void SGTC_WatchC4Charges(void)
{
   static struct { edict_t *ent; int serial; Vector pos; float t0; char owner[32]; } charge[16];
   static float last_time = 0;
   if (gpGlobals->time < last_time)  // a new map
      memset(charge, 0, sizeof(charge));
   last_time = gpGlobals->time;

   // gone (went off): the last position seen
   for (int k = 0; k < 16; k++)
   {
      if (!charge[k].ent)
         continue;
      edict_t *e = charge[k].ent;
      if (!e->free && e->serialnumber == charge[k].serial && !strcmp(STRING(e->v.classname), "monster_satchel"))
      {
         charge[k].pos = e->v.origin;
         continue;
      }
      float d;
      int z = SGTC_C4ZoneAt(charge[k].pos, d);
      if (z >= 0 && z < 8)
         sgtc_c4_zone_blown[z] = TRUE;
      UTIL_ConsolePrintf("[sgtc-stat] c4 charge of %s went off after %.1fs at %.0f %.0f %.0f: %s %d (%.0f out)\n", charge[k].owner,
         gpGlobals->time - charge[k].t0, charge[k].pos.x, charge[k].pos.y, charge[k].pos.z, z >= 0 ? "IN zone" : "outside, nearest", z, d);
      charge[k].ent = NULL;
   }

   // new ones
   edict_t *e = NULL;
   while (!FNullEnt(e = FIND_ENTITY_BY_CLASSNAME(e, "monster_satchel")))
   {
      int k, free_k = -1;
      for (k = 0; k < 16; k++)
      {
         if (charge[k].ent == e && charge[k].serial == e->serialnumber)
            break;
         if (!charge[k].ent && free_k < 0)
            free_k = k;
      }
      if (k < 16 || free_k < 0)
         continue;
      charge[free_k].ent = e;
      charge[free_k].serial = e->serialnumber;
      charge[free_k].pos = e->v.origin;
      charge[free_k].t0 = gpGlobals->time;
      edict_t *owner = e->v.owner;
      safe_strcopy(charge[free_k].owner, sizeof(charge[free_k].owner), FNullEnt(owner) ? "?" : STRING(owner->v.netname));
      for (int i = 0; i < 32; i++)
         if (bots[i].is_used && bots[i].pEdict == owner)
            bots[i].sgtc_c4_confirmed = TRUE;
      float d;
      int z = SGTC_C4ZoneAt(e->v.origin, d);
      UTIL_ConsolePrintf("[sgtc-stat] c4 charge of %s appears at %.0f %.0f %.0f: %s %d (%.0f out)\n", charge[free_k].owner,
         e->v.origin.x, e->v.origin.y, e->v.origin.z, z >= 0 ? "IN zone" : "outside, nearest", z, d);
   }
}

// the bot's rank among the bots of its side (slot order): a stable way to
// hand out roles so that a side spreads out
static int SGTC_BotSideRank(bot_t &pBot)
{
   int side = SGTC_GetPlayerTeam(pBot.pEdict), rank = 0;
   for (int i = 0; i < 32; i++)
      if (bots[i].is_used && &bots[i] < &pBot && !FNullEnt(bots[i].pEdict) && SGTC_GetPlayerTeam(bots[i].pEdict) == side)
         rank++;
   return rank;
}

// the ring set carriers take to the nuke: the one whose arrival is nearest
// to it (within 1200 units). Its departure platform waypoint, or its arrival
// one; -1 without rings.
static int SGTC_RingPlatformWaypoint(const Vector &nuke, qboolean arrival)
{
   int best = -1;
   float best_d = 1200.0f;
   for (int i = 0; i < sgtc_num_rings; i++)
   {
      float d = (sgtc_rings[i].dest - nuke).Length();
      if (d < best_d)
      {
         best_d = d;
         best = i;
      }
   }
   if (best == -1)
      return -1;
   return arrival ? sgtc_rings[best].w_dest : sgtc_rings[best].w_platform;
}

// the center of a side's spawn cluster nearest to p
static Vector SGTC_SpawnClusterNear(int side, const Vector &p)
{
   int n = sgtc_num_spawn_clusters[side], best = -1;
   float best_d = 0;
   for (int k = 0; k < n; k++)
   {
      float d = (sgtc_spawn_cluster[side][k].center - p).Length();
      if (best == -1 || d < best_d)
      {
         best = k;
         best_d = d;
      }
   }
   return best != -1 ? sgtc_spawn_cluster[side][best].center : sgtc_obj.spawn_center[side];
}

// the waypoint nearest to a spawn cluster's center (no ladder rung or jump spot)
static int SGTC_ClusterWaypoint(const Vector &c)
{
   int best = -1;
   float best_d = 600.0f;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP))
         continue;
      float d = (waypoints[i].origin - c).Length();
      if (d < best_d && fabs(waypoints[i].origin.z - c.z) < 150.0f)
      {
         best = i;
         best_d = d;
      }
   }
   return best;
}

// The enemy spawn area that matters for this bot: the cluster it can walk
// to with the shortest route, else the nearest one (sg_conquest: the
// Goa'uld can't walk to the SGC, so the planet's Tau'ri spawn). A side with
// one cluster: its average, as before.
static Vector SGTC_EnemySpawnFor(bot_t &pBot, int side)
{
   int n = sgtc_num_spawn_clusters[side];
   if (n <= 1)
      return sgtc_obj.spawn_center[side];
   int from = pBot.curr_waypoint_index, best = -1, nearest = 0;
   float best_d = 0, nearest_d = 0;
   for (int k = 0; k < n; k++)
   {
      const Vector &c = sgtc_spawn_cluster[side][k].center;
      float d = (c - pBot.pEdict->v.origin).Length();
      if (k == 0 || d < nearest_d)
      {
         nearest = k;
         nearest_d = d;
      }
      if (from < 0 || from >= num_waypoints)
         continue;
      int w = SGTC_ClusterWaypoint(c);
      if (w == -1)
         continue;
      float r = WaypointDistanceFromTo(from, w);
      if (r < WAYPOINT_MAX_DISTANCE && (best == -1 || r < best_d))
      {
         best = k;
         best_d = r;
      }
   }
   return sgtc_spawn_cluster[side][best != -1 ? best : nearest].center;
}

// the card carrier's goal: the waypoint to press the card button from
// (sg_conquest's Stolen_btn: wpt 374, inside the trigger_goal)
static int SGTC_CardWaypoint(bot_t &pBot)
{
   if (sgtc_goal_btn_w != -1 && sgtc_goal_btn_w < num_waypoints && !(waypoints[sgtc_goal_btn_w].flags & W_FL_DELETED))
      return sgtc_goal_btn_w;
   int w = SGTC_ButtonWaypoint(sgtc_obj.goal_button);
   if (sgtc_obj.paths_built && w != -1)
   {
      sgtc_goal_btn_w = w;
      Vector bc = SGTC_EntCenter(sgtc_obj.goal_button);
      UTIL_ConsolePrintf("[sgtc-stat] card button \"%s\": pressed from wpt %d (%.0f units from it)\n",
         STRING(sgtc_obj.goal_button->v.targetname), w, (waypoints[w].origin - bc).Length());
   }
   return w;
}

qboolean SGTC_BotFindObjectiveGoal(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   int side = SGTC_GetPlayerTeam(pEdict);
   int enemy_side = side == SGTC_TEAM_TAURI ? SGTC_TEAM_GOAULD : SGTC_TEAM_TAURI;
   qboolean attacker = sgtc_obj.attack_team && side == sgtc_obj.attack_team;
   int roll = RANDOM_LONG2(1, 100);
   int index = -1;

   if (side != SGTC_TEAM_TAURI && side != SGTC_TEAM_GOAULD)
      return FALSE;

   // the bomb is armed: get into a shelter before it goes off
   if (sgtc_state.f_bomb_armed_time > 0 && sgtc_obj.num_shelters)
   {
      edict_t *best = sgtc_obj.shelter[0];
      for (int i = 1; i < sgtc_obj.num_shelters; i++)
         if (SGTC_Dist(pEdict, SGTC_EntCenter(sgtc_obj.shelter[i])) < SGTC_Dist(pEdict, SGTC_EntCenter(best)))
            best = sgtc_obj.shelter[i];
      index = SGTC_WaypointNear(pBot, SGTC_EntCenter(best), 150);
      if (SGTC_SetGoal(pBot, index, FALSE, "shelter")) return TRUE;
   }

   // C4 zones: an attacker carrying C4 always goes to one whose wall still
   // stands and nobody is planting at: the nearest (a random one at each new
   // goal sent sg_strike3's planters back and forth between its two zones,
   // 4000 units apart). Before the key: on sg_stargate the key lies behind
   // the C4 wall, and the key branch below always returns
   if (sgtc_obj.num_bombzones && attacker && SGTC_HasC4(pEdict) && !sgtc_obj.key_carrier)
   {
      int pick = -1;
      float pick_d = 0;
      for (int i = 0; i < sgtc_obj.num_bombzones; i++)
         if (SGTC_C4ZoneLive(i) && sgtc_c4_planted[i] + 30.0f < gpGlobals->time)
         {
            float d = (SGTC_EntCenter(sgtc_obj.bombzone[i]) - pEdict->v.origin).Length();
            if (pick == -1 || d < pick_d)
            {
               pick = i;
               pick_d = d;
            }
         }
      if (pick != -1)
      {
         edict_t *zone = sgtc_obj.bombzone[pick];
         index = SGTC_WaypointNear(pBot, SGTC_EntCenter(zone), 80);
         if (SGTC_SetGoal(pBot, index, FALSE, "c4 plant")) return TRUE;
      }
   }

   // bomb key maps, and sg_conquest's card: its "nuke" is the button that
   // unlocks the goal in the SGC (the card entity is gone while carried)
   edict_t *site = sgtc_obj.nuke ? sgtc_obj.nuke : sgtc_obj.goal_button;
   qboolean card = !sgtc_obj.nuke && sgtc_obj.goal_button != NULL;
   if (sgtc_obj.nuke || sgtc_obj.key || (card && sgtc_obj.key_carrier))
   {
      Vector nuke = site ? SGTC_EntCenter(site) : Vector(0, 0, 0);
      edict_t *carrier = sgtc_obj.key_carrier;
      const char *why = attacker ? "bomb attack" : "bomb defense";

      if (carrier == pEdict && card)
      {
         index = SGTC_CardWaypoint(pBot);
         why = "card button";
      }
      else if (carrier == pEdict && sgtc_obj.nuke)
         index = SGTC_WaypointNear(pBot, nuke, 0);
      else if (attacker && card)
      {
         // the others fetch the card, or escort its carrier home
         if (carrier)
         {
            index = SGTC_WaypointNear(pBot, carrier->v.origin, 300);
            why = "card escort";
         }
         else if (sgtc_obj.key)
         {
            index = SGTC_WaypointNear(pBot, sgtc_obj.key->v.origin, roll <= 70 ? 0 : 500);
            why = "card fetch";
         }
      }
      else if (attacker)
      {
         if (carrier && sgtc_obj.nuke)  // escort the carrier to the bomb
            index = roll <= 50 ? SGTC_WaypointNear(pBot, carrier->v.origin, 300) : SGTC_WaypointNear(pBot, nuke, 400);
         else if (sgtc_obj.key && !carrier)
            index = roll <= 70 ? SGTC_WaypointNear(pBot, sgtc_obj.key->v.origin, 0) : (sgtc_obj.nuke ? SGTC_WaypointNear(pBot, nuke, 500) : -1);
         else if (sgtc_obj.nuke)
            index = SGTC_WaypointNear(pBot, nuke, 500);
      }
      else
      {
         // Defender roles by rank in the side, so that it spreads out instead
         // of everyone rolling the same dice: ranks 0 and 2 hunt (the key, or
         // the ring platform the attackers must take the key from), rank 1
         // guards the nuke, rank 3 the key. sg_stargate: the Goa'uld sat in
         // their ship (118 of 130 defence goals in it, 2 ring trips in 7
         // matches); the user found it dull.
         int role = SGTC_BotSideRank(pBot) % 4;
         int w_depart = SGTC_RingPlatformWaypoint(nuke, FALSE);
         int w_arrive = SGTC_RingPlatformWaypoint(nuke, TRUE);
         // not the platform in the enemy spawn (sg_chateau: SGTC punishes spawn camping)
         if (w_depart != -1 && sgtc_obj.spawn_center[enemy_side] != Vector(0, 0, 0) &&
             (waypoints[w_depart].origin - SGTC_SpawnClusterNear(enemy_side, waypoints[w_depart].origin)).Length() < 700.0f)
            w_depart = -1;
         if (carrier || sgtc_state.f_bomb_arming_time > 0)  // stop the carrier / the arming
         {
            if (role == 1 || !carrier)
               index = SGTC_WaypointNear(pBot, nuke, 200);
            else if (role == 3 && w_arrive != -1 && roll <= 60)
               index = SGTC_WaypointNear(pBot, waypoints[w_arrive].origin, 250);  // where the carrier lands
            else
               index = roll <= 75 ? SGTC_WaypointNear(pBot, carrier->v.origin, 0) : SGTC_WaypointNear(pBot, nuke, 200);
         }
         else if ((role == 0 || role == 2) && sgtc_obj.key)
            index = (roll <= 55 || w_depart == -1) ? SGTC_WaypointNear(pBot, sgtc_obj.key->v.origin, 600)
                                                   : SGTC_WaypointNear(pBot, waypoints[w_depart].origin, 350);
         else if (role == 3 && sgtc_obj.key)
            index = SGTC_WaypointNear(pBot, sgtc_obj.key->v.origin, 600);
         else if (site)
            index = SGTC_WaypointNear(pBot, nuke, 600);
      }

      if (SGTC_SetGoal(pBot, index, carrier != NULL, why)) return TRUE;
   }

   // reach-the-goal maps (attackers rush it, defenders hold the area); not
   // the attackers of a goal a card button unlocks (sg_conquest: it is their
   // own SGC, the carrier does it)
   if (sgtc_obj.num_goals && roll > 15 && !(attacker && sgtc_obj.goal_button))
   {
      edict_t *goal = sgtc_obj.goal[RANDOM_LONG2(0, sgtc_obj.num_goals - 1)];
      index = SGTC_WaypointNear(pBot, SGTC_EntCenter(goal), attacker ? 0 : 700);
      if (SGTC_SetGoal(pBot, index, FALSE, attacker ? "goal attack" : "goal defense")) return TRUE;
   }

   if (sgtc_obj.num_bombzones && roll > 15)
   {
      edict_t *zone = sgtc_obj.bombzone[RANDOM_LONG2(0, sgtc_obj.num_bombzones - 1)];
      index = SGTC_WaypointNear(pBot, SGTC_EntCenter(zone), attacker ? 100 : 700);
      if (SGTC_SetGoal(pBot, index, FALSE, attacker ? "c4 attack" : "c4 defense")) return TRUE;
   }

   // capture points: both sides fight over them
   if (sgtc_obj.num_captures && roll > 10)
   {
      edict_t *zone = sgtc_obj.capture[RANDOM_LONG2(0, sgtc_obj.num_captures - 1)];
      // a waypoint inside the zone (widened by the player hull), else the
      // nearest to its centre (the centre can be high above the floor)
      Vector mins = zone->v.origin + zone->v.mins - Vector(16, 16, 36);
      Vector maxs = zone->v.origin + zone->v.maxs + Vector(16, 16, 36);
      int inside[32], n = 0;
      for (int w = 0; w < num_waypoints && n < 32; w++)
      {
         const Vector &o = waypoints[w].origin;
         if (!(waypoints[w].flags & W_FL_DELETED) && o.x >= mins.x && o.x <= maxs.x && o.y >= mins.y && o.y <= maxs.y &&
             o.z >= mins.z && o.z <= maxs.z)
            inside[n++] = w;
      }
      index = n ? inside[RANDOM_LONG2(0, n - 1)] : SGTC_WaypointNear(pBot, SGTC_EntCenter(zone), 150);
      if (index == -1)
         index = SGTC_WaypointNear(pBot, SGTC_EntCenter(zone), 300);
      if (SGTC_SetGoal(pBot, index, FALSE, "capture")) return TRUE;
   }

   // no objective (or variety): head for the enemy side of the map, but stay
   // out of the spawn itself (SGTC punishes spawn camping)
   if (sgtc_obj.spawn_center[enemy_side] != Vector(0, 0, 0))
   {
      // the enemy spawn area on this side of the map (sg_conquest: the Tau'ri
      // spawn in the SGC and on the planet, their average is in mid-air)
      Vector enemy_spawn = SGTC_EnemySpawnFor(pBot, enemy_side);
      // one at random among all of them (no ladder rung or jump spot),
      // preferably at about the enemy spawn's height
      int pick = -1, n = 0, pick_any = -1, n_any = 0;
      for (int i = 0; i < num_waypoints; i++)
      {
         if (waypoints[i].flags & (W_FL_DELETED | W_FL_LADDER | W_FL_JUMP))
            continue;
         float d = (waypoints[i].origin - enemy_spawn).Length();
         if (d <= 600 || d >= 1600)
            continue;
         if (RANDOM_LONG2(1, ++n_any) == 1)
            pick_any = i;
         if (fabs(waypoints[i].origin.z - enemy_spawn.z) < 200.0f && RANDOM_LONG2(1, ++n) == 1)
            pick = i;
      }
      if (pick == -1)
         pick = pick_any;
      if (pick != -1 && SGTC_SetGoal(pBot, pick, FALSE, "contact")) return TRUE;
   }

   return FALSE;
}


//
// Per-frame objective actions, after navigation: the key carrier arms the bomb,
// goals that follow a moving player get refreshed.
//
//
// A straight walk from the bot to a C4 standing spot is clear, knee and
// chest high (the approach is a world-space move, without waypoints).
//
static qboolean SGTC_C4SpotClear(edict_t *pEdict, const Vector &spot)
{
   float feet = pEdict->v.origin.z + pEdict->v.mins.z, spot_feet = spot.z - 36.0f;
   for (int i = 1; i <= 2; i++)
   {
      TraceResult tr;
      UTIL_TraceLine(Vector(pEdict->v.origin.x, pEdict->v.origin.y, feet + 24.0f * i),
                     Vector(spot.x, spot.y, spot_feet + 24.0f * i), ignore_monsters, pEdict, &tr);
      if (tr.flFraction < 1.0f)
         return FALSE;
   }
   return TRUE;
}

//
// In a C4 zone whose wall still stands, with C4 and no enemy: select it,
// look down, fire once, then get away for a few seconds (it goes off by
// itself). Returns TRUE while it controls the bot.
//
static qboolean SGTC_BotC4Think(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;

   // getting away from the C4 just planted
   if (pBot.f_sgtc_c4_flee_until > gpGlobals->time)
   {
      if (pBot.f_sgtc_c4_flee_until > gpGlobals->time + 10.0f)
         pBot.f_sgtc_c4_flee_until = 0;  // a new map
      else if (!pBot.sgtc_c4_confirmed && pBot.f_sgtc_c4_attack + 4.0f < gpGlobals->time)
      {
         // no charge came out: free the zone, try again. The log says what
         // the game's own trace (60 units from the eyes along the aim) meets now
         TraceResult tr;
         Vector eyes = pEdict->v.origin + pEdict->v.view_ofs;
         UTIL_TraceLine(eyes, eyes + UTIL_AnglesToForward(pEdict->v.v_angle) * 60.0f, dont_ignore_monsters, pEdict, &tr);
         char hit[96];
         if (tr.flFraction < 1.0f && tr.pHit)
            safevoid_snprintf(hit, sizeof(hit), "%s at %.0f%s", STRING(tr.pHit->v.classname), tr.flFraction * 60.0f,
                              (tr.pHit->v.flags & FL_CONVEYOR) ? ", a conveyor: refused" : "");
         else
            safe_strcopy(hit, sizeof(hit), "nothing within 60");
         UTIL_ConsolePrintf("[sgtc-stat] c4: %s plant failed (no charge, weapon %d, %s, pitch %.0f, %s, eye trace: %s)\n",
            STRING(pEdict->v.netname), pBot.current_weapon.iId, pBot.sgtc_c4_spot_idx >= 0 ? "drop" : "throw",
            pEdict->v.v_angle.x, (pEdict->v.flags & FL_DUCKING) ? "crouched" : "standing", hit);
         pBot.f_sgtc_c4_flee_until = 0;
         if (pBot.sgtc_c4_zone_idx >= 0 && pBot.sgtc_c4_zone_idx < 8)
         {
            sgtc_c4_planted[pBot.sgtc_c4_zone_idx] = 0;
            if (pBot.sgtc_c4_spot_idx >= 0 && pBot.sgtc_c4_spot_idx < 16)
               sgtc_c4_spot_fails[pBot.sgtc_c4_zone_idx][pBot.sgtc_c4_spot_idx]++;
         }
      }
      else if (!pBot.sgtc_c4_confirmed && pBot.pBotEnemy == NULL)
      {
         // hold fire until the charge comes out (the weapon may still be
         // deploying, or something switched away from it); run once it's out.
         // It's placed like a tripmine: a 60-unit trace from the eyes, so a
         // drop at the feet needs a crouch to reach the floor (eyes ~30 above
         // it instead of ~64). A drop fires only once crouched and looking
         // well down: the trace then meets the floor a few units ahead of
         // the feet, inside the zone. A throw fires at once.
         qboolean c4_drop = pBot.sgtc_c4_spot_idx >= 0;
         pBot.f_move_speed = 0;
         pBot.f_strafe_direction = 0;
         pBot.b_not_maxspeed = TRUE;
         pEdict->v.idealpitch = pBot.f_sgtc_c4_pitch;
         pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
         // keep facing the zone (the navigation turns the bot meanwhile)
         int zi = pBot.sgtc_c4_zone_idx;
         if (zi >= 0 && zi < sgtc_obj.num_bombzones && !FNullEnt(sgtc_obj.bombzone[zi]))
         {
            Vector to = SGTC_EntCenter(sgtc_obj.bombzone[zi]) - pEdict->v.origin;
            to.z = 0;
            if (to.Length() > 1.0f)
            {
               pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
               BotFixIdealYaw(pEdict);
            }
         }
         if (c4_drop)
            pEdict->v.button |= IN_DUCK;
         if (pBot.current_weapon.iId != SGTC_WEAPON_C4)
         {
            if (pBot.f_sgtc_c4_next <= gpGlobals->time)
            {
               pBot.f_sgtc_c4_next = gpGlobals->time + 0.5f;
               UTIL_SelectItem(pEdict, "weapon_c4");
            }
         }
         else if (!c4_drop || ((pEdict->v.flags & FL_DUCKING) && pEdict->v.v_angle.x > pBot.f_sgtc_c4_pitch - 15.0f))
            pEdict->v.button |= IN_ATTACK;
         pBot.f_sgtc_c4_flee_until = gpGlobals->time + 4.0f;
         return TRUE;
      }
      else if (pBot.pBotEnemy == NULL)
      {
         Vector away = pEdict->v.origin - pBot.v_sgtc_c4_zone;
         away.z = 0;
         if (away.Length() < 1.0f)
            away = Vector(1, 0, 0);
         pBot.v_sgtc_move = away.Normalize() * pBot.f_max_speed;
         pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
         pBot.f_move_speed = 0;
         pBot.f_strafe_direction = 0;
         return TRUE;
      }
   }

   int side = SGTC_GetPlayerTeam(pEdict);
   if (!sgtc_obj.num_bombzones || !sgtc_obj.attack_team || side != sgtc_obj.attack_team ||
       !SGTC_HasC4(pEdict) || pBot.pBotEnemy != NULL)
      return FALSE;

   // the nearest live zone with a standing spot within 150 units (or, with
   // none found in it, its box within 150 units at about the same height)
   const Vector &o = pEdict->v.origin;
   int z, best_z = -1, best_k = -1;
   Vector target, best_target;
   float best = 150.0f;
   qboolean drop = FALSE, best_drop = FALSE;
   for (z = 0; z < sgtc_obj.num_bombzones; z++)
   {
      edict_t *zone = sgtc_obj.bombzone[z];
      if (FNullEnt(zone) || !SGTC_C4ZoneLive(z) || sgtc_c4_planted[z] + 30.0f > gpGlobals->time)
         continue;
      Vector mins = zone->v.origin + zone->v.mins, maxs = zone->v.origin + zone->v.maxs;

      if (!sgtc_c4_spots_done[z])
      {
         // a grid over the zone (16 units, wider on a huge zone): hull
         // standing places, feet inside. Then up to 16 of them spread over
         // the whole zone (farthest-point picks): the first 16 in grid order
         // all lay along one edge of sg_strike3's 384-unit sarcophagus zone,
         // 168+ units from its goal waypoint, beyond the 150 units a planter
         // looks for a spot
         static Vector cand[512];
         static float cand_d[512];
         int n = 0;
         float step = 16.0f;
         while (((maxs.x - mins.x) / step + 1.0f) * ((maxs.y - mins.y) / step + 1.0f) > 512.0f)
            step += 8.0f;
         sgtc_c4_spots_done[z] = TRUE;
         sgtc_c4_num_spots[z] = 0;
         memset(sgtc_c4_spot_fails[z], 0, sizeof(sgtc_c4_spot_fails[z]));
         for (float x = mins.x + 8.0f; x <= maxs.x - 8.0f && n < 512; x += step)
            for (float y = mins.y + 8.0f; y <= maxs.y - 8.0f && n < 512; y += step)
            {
               // standing on the floor, not on top of the wall itself
               TraceResult tr;
               Vector top(x, y, maxs.z + 36.0f);
               UTIL_TraceHull(top, top - Vector(0, 0, maxs.z - mins.z + 8.0f), ignore_monsters, human_hull, NULL, &tr);
               if (tr.fStartSolid || tr.fAllSolid || tr.flFraction >= 1.0f || tr.vecPlaneNormal.z < 0.7f ||
                   (!FNullEnt(tr.pHit) && !strcmp(STRING(tr.pHit->v.targetname), STRING(zone->v.target)) && STRING(zone->v.target)[0]))
                  continue;
               Vector g = tr.vecEndPos;
               if (g.z - 36.0f < mins.z - 2.0f || g.z - 36.0f > maxs.z - 8.0f)
                  continue;
               cand[n++] = g;
            }
         // farthest-point picks over the floor plan, from the first place
         for (int i = 0; i < n; i++)
            cand_d[i] = 1e9f;
         int pick = 0;
         while (n && sgtc_c4_num_spots[z] < 16)
         {
            Vector sp = cand[pick];
            sgtc_c4_spot[z][sgtc_c4_num_spots[z]++] = sp;
            float far_d = 0;
            int far_i = -1;
            for (int i = 0; i < n; i++)
            {
               Vector dv = cand[i] - sp;
               dv.z = 0;
               if (dv.Length() < cand_d[i])
                  cand_d[i] = dv.Length();
               if (cand_d[i] > far_d)
               {
                  far_d = cand_d[i];
                  far_i = i;
               }
            }
            if (far_i < 0 || far_d < 8.0f)
               break;  // every place is a spot already
            pick = far_i;
         }
         UTIL_ConsolePrintf("[sgtc-stat] c4 zone %d (wall \"%s\"): %d standing spots inside (spread over %d)\n", z, STRING(zone->v.target),
            sgtc_c4_num_spots[z], n);
      }

      if (sgtc_c4_num_spots[z])
      {
         for (int k = 0; k < sgtc_c4_num_spots[z]; k++)
         {
            Vector d = sgtc_c4_spot[z][k] - o;
            if (fabs(d.z) > 60.0f || d.Length() >= best || sgtc_c4_spot_fails[z][k] >= 2)
               continue;
            if (!SGTC_C4SpotClear(pEdict, sgtc_c4_spot[z][k]))
               continue;  // something in the way of the straight walk
            best = d.Length();
            best_z = z;
            best_k = k;
            best_target = sgtc_c4_spot[z][k];
            best_drop = TRUE;
         }
      }
      else if (o.z > mins.z - 60.0f && o.z < maxs.z + 60.0f)
      {
         target = Vector(o.x < mins.x ? mins.x : (o.x > maxs.x ? maxs.x : o.x),
                         o.y < mins.y ? mins.y : (o.y > maxs.y ? maxs.y : o.y), o.z);
         if ((target - o).Length() < best)
         {
            best = (target - o).Length();
            best_z = z;
            best_target = target;
            best_drop = FALSE;
         }
      }
   }
   if (best_z == -1)
      return FALSE;
   z = best_z;
   target = best_target;
   drop = best_drop;
   if (!drop)
      best_k = -1;
   float dist = (Vector(target.x, target.y, 0) - Vector(o.x, o.y, 0)).Length();
   Vector center = SGTC_EntCenter(sgtc_obj.bombzone[z]);

   // go there with the C4 in hand
   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.b_not_maxspeed = TRUE;
   if (dist > (drop ? 10.0f : 36.0f))
   {
      Vector to = target - o;
      to.z = 0;
      pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
      BotFixIdealYaw(pEdict);
      pBot.v_sgtc_move = to.Normalize() * (dist > 60.0f ? 200.0f : 100.0f);
      pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
      if (pBot.current_weapon.iId != SGTC_WEAPON_C4 && pBot.f_sgtc_c4_next <= gpGlobals->time)
      {
         pBot.f_sgtc_c4_next = gpGlobals->time + 0.5f;
         UTIL_SelectItem(pEdict, "weapon_c4");
      }
      return TRUE;
   }
   // standing in the zone: drop it at the feet; outside it: throw it about
   // level at the zone, it sticks to what's there. A positive v_angle pitch
   // looks DOWN (jk_botti's "85 degrees is downward"): the old -85 looked
   // straight up, so no drop ever met the floor, and the only charges were
   // those stuck to a wall the aim swept on its way up
   Vector to = center - o;
   to.z = 0;
   if (to.Length() > 1.0f)
   {
      pEdict->v.ideal_yaw = UTIL_VecToAngles(to).y;
      BotFixIdealYaw(pEdict);
   }
   if (drop)
   {
      pEdict->v.idealpitch = 85.0f;
      pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
      pEdict->v.button |= IN_DUCK;
   }
   else
   {
      // towards the zone's centre, at eye height kept inside the zone's height
      edict_t *zone = sgtc_obj.bombzone[z];
      Vector eyes = o + pEdict->v.view_ofs;
      float lo = zone->v.origin.z + zone->v.mins.z + 8.0f, hi = zone->v.origin.z + zone->v.maxs.z - 8.0f;
      float aim_z = eyes.z > hi ? hi : eyes.z;
      if (aim_z < lo)
         aim_z = lo;
      float pitch = -UTIL_VecToAngles(Vector(center.x, center.y, aim_z) - eyes).x;
      pEdict->v.idealpitch = pitch < -30.0f ? -30.0f : (pitch > 40.0f ? 40.0f : pitch);
      pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   }
   Vector nearest = target;

   if (pBot.f_sgtc_c4_next > gpGlobals->time + 5.0f)
      pBot.f_sgtc_c4_next = 0;  // a new map
   if (pBot.f_sgtc_c4_next > gpGlobals->time)
      return TRUE;
   pBot.f_sgtc_c4_next = gpGlobals->time + 0.5f;

   if (pBot.current_weapon.iId != SGTC_WEAPON_C4)
   {
      UTIL_SelectItem(pEdict, "weapon_c4");
      return TRUE;
   }

   if (!drop)
      pEdict->v.button |= IN_ATTACK;  // a drop fires from the hold above, crouched and looking down
   sgtc_c4_planted[z] = gpGlobals->time;
   pBot.f_sgtc_c4_flee_until = gpGlobals->time + 4.0f;
   pBot.f_sgtc_c4_attack = gpGlobals->time;
   pBot.sgtc_c4_zone_idx = z;
   pBot.sgtc_c4_spot_idx = best_k;
   pBot.sgtc_c4_confirmed = FALSE;
   pBot.f_sgtc_c4_pitch = pEdict->v.idealpitch;
   pBot.v_sgtc_c4_zone = nearest;
   pBot.waypoint_goal = -1;
   pBot.f_waypoint_goal_time = 0;
   UTIL_ConsolePrintf("[sgtc-stat] c4: %s plants in zone %d (wall \"%s\") at %.0f %.0f %.0f (%s)\n", STRING(pEdict->v.netname), z,
      STRING(sgtc_obj.bombzone[z]->v.target), pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z, drop ? "drop" : "throw");
   return TRUE;
}

//
// Standing spots around the nuke from which +use reaches it. PlayerUse looks
// for the entity within 64 units of the player's origin, so a standing origin
// within 56 of the nuke's origin: wherever a player hull finds ground on a
// ring 24-48 units out (the slab the nuke stands on, or the floor next to it).
//
static void SGTC_FindNukeSpots(const Vector &nuke_origin)
{
   if (sgtc_nuke_spots_done && (sgtc_nuke_spots_at - nuke_origin).Length() < 1.0f)
      return;
   sgtc_nuke_spots_done = TRUE;
   sgtc_nuke_spots_at = nuke_origin;
   sgtc_nuke_num_spots = 0;
   for (float r = 24.0f; r <= 48.0f && sgtc_nuke_num_spots < 16; r += 12.0f)
      for (int a = 0; a < 360 && sgtc_nuke_num_spots < 16; a += 30)
      {
         float rad = a * (M_PI / 180.0f);
         Vector p = nuke_origin + Vector(cosf(rad) * r, sinf(rad) * r, 60.0f);
         Vector g;
         // the hull starts 24 above the nuke's base (clear of a low ceiling), or
         // lower when that is inside something
         if (!SGTC_GroundAt(p, 120.0f, g) && !SGTC_GroundAt(p - Vector(0, 0, 36.0f), 84.0f, g))
            continue;
         if ((g - nuke_origin).Length() > 56.0f)
            continue;
         sgtc_nuke_spot[sgtc_nuke_num_spots++] = g;
      }
   UTIL_ConsolePrintf("[sgtc-stat] nuke at %.0f %.0f %.0f: %d use spots%s\n", nuke_origin.x, nuke_origin.y, nuke_origin.z,
      sgtc_nuke_num_spots, sgtc_nuke_num_spots ? "" : " (none: walk at it)");
   for (int k = 0; k < sgtc_nuke_num_spots && k < 4; k++)
      UTIL_ConsolePrintf("[sgtc-stat]   use spot %.0f %.0f %.0f\n", sgtc_nuke_spot[k].x, sgtc_nuke_spot[k].y, sgtc_nuke_spot[k].z);
}

//
// The card carrier at the button that unlocks the goal (sg_conquest's
// "Stolen_btn": needs the card, Tau'ri only, inside the trigger_goal): walk
// up to it, face it, press use once a second. +use only reaches a usable
// entity whose center is within 64 units of the player's origin and that it
// faces; a button fires on the press, not while use is held. Pressed again
// after it fired, a stay-pressed button does nothing.
//
static qboolean SGTC_BotPressCardButton(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   edict_t *pButton = sgtc_obj.goal_button;
   Vector c = SGTC_EntCenter(pButton);
   Vector to = c - pEdict->v.origin;
   float dist = to.Length();
   if (pBot.pBotEnemy != NULL && dist > 64.0f)
      return FALSE;  // fight first, unless it can press right now

   Vector angles = UTIL_VecToAngles(c - GetGunPosition(pEdict));
   pEdict->v.ideal_yaw = angles.y;
   pEdict->v.idealpitch = -angles.x;
   pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   BotFixIdealYaw(pEdict);
   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.b_not_maxspeed = TRUE;  // or it's scaled back up to full speed
   to.z = 0;
   float dist2d = to.Length();
   if (dist > 48.0f && dist2d > 8.0f)
   {
      pBot.v_sgtc_move = to.Normalize() * (dist2d > 48.0f ? 150.0f : 80.0f);
      pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
   }
   if (pBot.f_sgtc_next_use > gpGlobals->time + 5.0f)  // a new map
      pBot.f_sgtc_next_use = 0;
   if (dist <= 64.0f && pBot.f_sgtc_next_use <= gpGlobals->time)
   {
      pBot.f_sgtc_next_use = gpGlobals->time + 1.0f;
      pEdict->v.button |= IN_USE;
      BotTrace(pBot, "sgtc card: press \"%s\" (dist=%.0f)", STRING(pButton->v.targetname), dist);
      if (sgtc_goal_btn_log > gpGlobals->time)  // a new map
         sgtc_goal_btn_log = 0;
      if (sgtc_goal_btn_log + 10.0f <= gpGlobals->time)
      {
         sgtc_goal_btn_log = gpGlobals->time;
         UTIL_ConsolePrintf("[sgtc-stat] card carrier %s presses \"%s\" (dist %.0f, frame %.0f) t=%.1f\n", STRING(pEdict->v.netname),
            STRING(pButton->v.targetname), dist, pButton->v.frame, gpGlobals->time - sgtc_state.f_round_start);
      }
   }
   return TRUE;
}

void SGTC_BotObjectiveThink(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;

   // carrier/escort goals follow a moving player: re-plan every few seconds
   if (pBot.sgtc_goal_dynamic && pBot.f_sgtc_goal_time + 3.0f < gpGlobals->time && pBot.pBotEnemy == NULL)
   {
      pBot.sgtc_goal_dynamic = FALSE;
      pBot.waypoint_goal = -1;
      pBot.f_waypoint_goal_time = 0;
   }

   // travelling: the waypoint near a gate is usually in front of the event
   // horizon, walk the rest of the way into the wormhole
   if (pBot.f_sgtc_enter_until > gpGlobals->time && pBot.pBotEnemy == NULL)
   {
      Vector to_gate = pBot.v_sgtc_enter - pEdict->v.origin;
      if (to_gate.Length() < 250.0f)
      {
         // the gate it walks into: shut (closed, or its arrival a kill zone),
         // give up and re-plan; its vortex on, wait (stepping out of it)
         for (int gi = 0; gi < sgtc_num_gates; gi++)
         {
            sgtc_gate_t &g = sgtc_gates[gi];
            if ((g.entrance - pBot.v_sgtc_enter).Length() > 1.0f || (g.open && !g.kawoosh))
               continue;
            if (!g.kawoosh)
            {
               BotTrace(pBot, "sgtc gate %d: %s, stop walking in", g.number, g.blocked ? "arrival blocked (iris)" : "closed");
               pBot.f_sgtc_enter_until = 0;
               pBot.waypoint_goal = -1;
               pBot.f_waypoint_goal_time = 0;
               return;
            }
            pBot.f_move_speed = 0;
            pBot.f_strafe_direction = 0;
            pBot.b_not_maxspeed = TRUE;
            pBot.sgtc_stuck_count = 0;
            pBot.v_sgtc_stuck_pos = pEdict->v.origin;
            Vector back = to_gate * -1.0f;
            back.z = 0;
            if (SGTC_InGateZone(pEdict->v.origin) && back.Length() > 1.0f)
            {
               pBot.v_sgtc_move = back.Normalize() * pBot.f_max_speed;
               pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
            }
            return;
         }
         Vector angles = UTIL_VecToAngles(to_gate);
         pEdict->v.ideal_yaw = angles.y;
         BotFixIdealYaw(pEdict);
         pBot.f_move_speed = pBot.f_max_speed;
         pBot.f_strafe_direction = 0;
         pBot.sgtc_stuck_count = 0;
         pBot.f_sgtc_push_time = gpGlobals->time;
         if (pBot.f_sgtc_push_trace <= gpGlobals->time)
         {
            pBot.f_sgtc_push_trace = gpGlobals->time + 1.0f;
            BotTrace(pBot, "sgtc gate push: dist=%.0f org=%.0f %.0f %.0f vel=%.0f %.0f %.0f yaw=%.0f ideal=%.0f speed=%.0f",
               to_gate.Length(), pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z,
               pEdict->v.velocity.x, pEdict->v.velocity.y, pEdict->v.velocity.z,
               pEdict->v.v_angle.y, pEdict->v.ideal_yaw, pBot.f_move_speed);
         }
         return;
      }
      // pushed into the wormhole a moment ago and now far away: teleported
      if (pBot.f_sgtc_push_time + 2.0f > gpGlobals->time && to_gate.Length() > 600.0f)
      {
         pBot.f_sgtc_push_time = 0;
         BotTrace(pBot, "sgtc gate: travelled, now at %.0f %.0f %.0f", pEdict->v.origin.x, pEdict->v.origin.y, pEdict->v.origin.z);
         pBot.f_sgtc_enter_until = 0;
         pBot.waypoint_goal = -1;
         pBot.f_waypoint_goal_time = 0;
         pBot.curr_waypoint_index = -1;
      }
   }

   // teleported (rings, map teleporters, a gate): find the nearest waypoint again
   if ((pEdict->v.origin - pBot.v_sgtc_prev_origin).Length() > 250.0f && pBot.v_sgtc_prev_origin != Vector(0, 0, 0))
      pBot.curr_waypoint_index = -1;
   pBot.v_sgtc_prev_origin = pEdict->v.origin;

   // rings up around it and not waiting there for its own departure (the
   // teleport comes at +4.4 s): the rings come back down about a second
   // after the arrivals land, on the band where the map's destinations are
   // (54-60 units out, nearly every "killed by func_train"). Inside the ring
   // wall, go to the middle at once, in world space (turning first took that
   // second); outside it, step away.
   Vector ring_offset;
   int ring_set;
   if (SGTC_UnderActiveRings(pEdict, ring_offset, ring_set))
   {
      // the platform's floor plate ("obeliskpush1", a func_door rising at 3
      // units/s during the teleport on sg_chateau) rises into the feet of
      // those just set down on it: stuck inside it, they can't move until
      // the rings come down on them. Lift the bot onto it.
      TraceResult tr;
      int hull = (pEdict->v.flags & FL_DUCKING) ? head_hull : human_hull;
      UTIL_TraceHull(pEdict->v.origin, pEdict->v.origin + Vector(0, 0, 1), ignore_monsters, hull, pEdict, &tr);
      if (tr.fStartSolid && !FNullEnt(tr.pHit))
      {
         for (float up = 2.0f; up <= 12.0f; up += 2.0f)
         {
            Vector o = pEdict->v.origin + Vector(0, 0, up);
            UTIL_TraceHull(o, o + Vector(0, 0, 1), ignore_monsters, hull, pEdict, &tr);
            if (!tr.fStartSolid && !tr.fAllSolid)
            {
               SET_ORIGIN(pEdict, o);
               break;
            }
         }
      }

      qboolean leaving = (pBot.sgtc_ring == ring_set + 1 && sgtc_ring_last_press[ring_set] + 5.0f > gpGlobals->time);
      if (!leaving)
      {
         float d = ring_offset.Length();
         pBot.f_strafe_direction = 0;
         pEdict->v.button &= ~(IN_JUMP | IN_DUCK);
         pBot.sgtc_stuck_count = 0;
         pBot.v_sgtc_stuck_pos = pEdict->v.origin;
         pBot.f_move_speed = 0;
         pBot.b_not_maxspeed = TRUE;
         if (d >= 20.0f)
         {
            pBot.v_sgtc_move = (d < 70.0f ? ring_offset * -1.0f : ring_offset).Normalize() * (d < 70.0f ? 200.0f : pBot.f_max_speed);
            pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
         }
         return;
      }
   }

   // button transporters (rt rings, obelisks, telepo pads): press, wait, ride
   if (SGTC_BotTransportThink(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   // using the rings: press their button, wait on the platform
   if (SGTC_BotRingThink(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   // walking into an always-on teleport, waiting for / riding a long lift
   if (SGTC_BotTransitThink(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   // planting C4 in a C4 zone, or getting away from it
   if (SGTC_BotC4Think(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   // a closed door on the route: go press its button first
   if (SGTC_BotDoorThink(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   // the key off the waypoints (sg_stargate: in a niche 123 units from the
   // nearest one, behind the C4 wall): an attacker that sees it close by
   // walks straight to it
   if (sgtc_obj.key && sgtc_obj.attack_team && !sgtc_obj.key_carrier && pBot.pBotEnemy == NULL &&
       SGTC_GetPlayerTeam(pEdict) == sgtc_obj.attack_team)
   {
      Vector to_key = sgtc_obj.key->v.origin - pEdict->v.origin;
      if (to_key.Length() < 300.0f && fabs(to_key.z) < 80.0f)
      {
         TraceResult tr;
         UTIL_TraceLine(GetGunPosition(pEdict), sgtc_obj.key->v.origin, ignore_monsters, pEdict, &tr);
         if (tr.flFraction >= 1.0f || tr.pHit == sgtc_obj.key)
         {
            to_key.z = 0;
            if (to_key.Length() > 1.0f)
            {
               pEdict->v.ideal_yaw = UTIL_VecToAngles(to_key).y;
               BotFixIdealYaw(pEdict);
               pBot.v_sgtc_move = to_key.Normalize() * 200.0f;
               pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
            }
            pBot.f_move_speed = 0;
            pBot.f_strafe_direction = 0;
            pBot.b_not_maxspeed = TRUE;
            pBot.sgtc_stuck_count = 0;
            pBot.v_sgtc_stuck_pos = pEdict->v.origin;
            return;
         }
      }
   }

   // dialing a DHD: stand there and press, no stuck check meanwhile
   if (SGTC_BotDialThink(pBot))
   {
      pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;
      return;
   }

   qboolean arming = sgtc_obj.key_carrier == pEdict && sgtc_obj.nuke && sgtc_state.f_bomb_armed_time <= 0 &&
                     (SGTC_EntCenter(sgtc_obj.nuke) - pEdict->v.origin).Length() <= 160;
   // the card carrier at the button that unlocks the goal (sg_conquest)
   qboolean pressing = sgtc_obj.key_carrier == pEdict && !sgtc_obj.nuke && sgtc_obj.goal_button &&
                       (SGTC_EntCenter(sgtc_obj.goal_button) - pEdict->v.origin).Length() <= 110;

   // progress watchdog: jk_botti can oscillate on ledges, ladders and narrow
   // ledges for minutes. Not moving while wanting to: jump/strafe, then re-plan.
   if (pBot.f_sgtc_stuck_check <= gpGlobals->time)
   {
      pBot.f_sgtc_stuck_check = gpGlobals->time + 2.5f;

      if (!arming && !pressing && pBot.pBotEnemy == NULL && (pEdict->v.origin - pBot.v_sgtc_stuck_pos).Length() < 40.0f)
         pBot.sgtc_stuck_count++;
      else
         pBot.sgtc_stuck_count = 0;
      pBot.v_sgtc_stuck_pos = pEdict->v.origin;

      if (pBot.sgtc_stuck_count == 2 && pBot.b_on_ladder)
         BotTrace(pBot, "sgtc unstuck: on a ladder, no jump wpt=%d goal=%d", pBot.curr_waypoint_index, pBot.waypoint_goal);
      else if (pBot.sgtc_stuck_count == 2)
      {
         BotTrace(pBot, "sgtc unstuck: jump wpt=%d goal=%d", pBot.curr_waypoint_index, pBot.waypoint_goal);
         pEdict->v.button |= IN_JUMP;
         // strafe, but not toward a drop
         Vector right = UTIL_AnglesToRight(Vector(0, pEdict->v.v_angle.y, 0));
         float side = RANDOM_LONG2(0, 1) ? 1.0f : -1.0f;
         if (SGTC_LedgeAt(pEdict, right * side, 48.0f))
            side = -side;
         if (SGTC_LedgeAt(pEdict, right * side, 48.0f))
            side = 0;
         pBot.f_strafe_direction = side;
         pBot.f_strafe_time = gpGlobals->time + 1.0f;
      }
      else if (pBot.sgtc_stuck_count >= 3)
      {
         BotTrace(pBot, "sgtc unstuck: replan wpt=%d goal=%d", pBot.curr_waypoint_index, pBot.waypoint_goal);
         for (int i = EXCLUDE_POINTS_COUNT - 1; i > 0; i--)
            pBot.exclude_points[i] = pBot.exclude_points[i - 1];
         pBot.exclude_points[0] = pBot.waypoint_goal;
         pBot.waypoint_goal = -1;
         pBot.f_waypoint_goal_time = 0;
         pBot.curr_waypoint_index = -1;
         pBot.sgtc_stuck_count = 0;
      }
   }

   if (pressing && SGTC_BotPressCardButton(pBot))
      return;

   if (!arming)
      return;

   if (pBot.pBotEnemy != NULL && sgtc_state.f_bomb_arming_time <= 0)
      return;

   // +use only finds an entity within 64 units of the player's origin. On
   // sg_chateau the nuke stands on a 40-unit slab: walking at it ended against
   // the rim, 79 units away, holding use for 60-80 s (Tamara_Johansen,
   // Walter_Harriman at -576 756 788). Go to a standing spot in use range
   // instead, jumping up when it is higher than a step.
   const Vector &nuke_origin = sgtc_obj.nuke->v.origin;
   Vector nuke = SGTC_EntCenter(sgtc_obj.nuke);
   SGTC_FindNukeSpots(nuke_origin);
   Vector target = nuke_origin;
   float best = 99999.0f;
   for (int k = 0; k < sgtc_nuke_num_spots; k++)
   {
      float d = (sgtc_nuke_spot[k] - pEdict->v.origin).Length();
      if (d < best)
      {
         best = d;
         target = sgtc_nuke_spot[k];
      }
   }
   Vector to = target - pEdict->v.origin;
   float dz = to.z;
   to.z = 0;
   float dist = to.Length();

   // face the nuke: PlayerUse takes the usable entity the player faces best
   Vector angles = UTIL_VecToAngles(nuke - GetGunPosition(pEdict));
   pEdict->v.ideal_yaw = angles.y;
   pEdict->v.idealpitch = -angles.x;
   pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   BotFixIdealYaw(pEdict);

   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.b_not_maxspeed = TRUE;  // or it's scaled back up to full speed
   if (pBot.f_sgtc_nuke_jump > gpGlobals->time + 5.0f)  // a new map
      pBot.f_sgtc_nuke_jump = 0;
   qboolean in_reach = (nuke_origin - pEdict->v.origin).Length() <= 56.0f;
   if (!in_reach && dist > 8.0f)
   {
      // a move in world space: the bot keeps facing the nuke meanwhile
      pBot.v_sgtc_move = to.Normalize() * (dist > 48.0f ? 150.0f : 80.0f);
      pBot.f_sgtc_move_until = gpGlobals->time + 0.15f;
      // the spot is up a ledge higher than a step: jump at it, crouching in
      // the air (BotThink ducks while f_duck_time runs)
      if (dz > 18.0f && dist < 64.0f && (pEdict->v.flags & FL_ONGROUND) && pBot.f_sgtc_nuke_jump <= gpGlobals->time)
      {
         pBot.f_sgtc_nuke_jump = gpGlobals->time + 1.2f;
         pEdict->v.button |= IN_JUMP;
         pBot.f_duck_time = gpGlobals->time + 0.6f;
         BotTrace(pBot, "sgtc nuke: jump up to the use spot (dz=%.0f dist=%.0f)", dz, dist);
      }
   }
   pEdict->v.button |= IN_USE;
}


// sound worth tracking: made by a living player of the other side
qboolean SGTC_IsEnemySound(edict_t *pListener, edict_t *pSource)
{
   if (!SGTC_IsPlayerAlive(pSource) || pSource == pListener)
      return FALSE;

   int side = SGTC_GetPlayerTeam(pSource);
   return side != SGTC_TEAM_NONE && side != SGTC_GetPlayerTeam(pListener);
}


//
// Button-triggered transporters (patch_transport.py, 2026-10-22)
// --------------------------------------------------------------
// The "rt" transport rings (sg_tokra2 rt1-rt4, sg_strike3 rt1/rt2), the
// obelisks (sg_hina, sg_quietvillage) and the "telepo" pads (sg_target2a,
// sg_land3). Unlike the rg rings above, a press only sends its own end, and
// the far end is cleared first: a 5000 hurt on the rt far platform at
// +7.8 s, on the obelisk's other pad at +3.6 s. Found by entity names: at
// map start their teleports point at "nowhere".
//
// Each end has w_dep, the waypoint(s) where a bot starts using it, and
// w_arr, where arrivals pick the route up; a forced path w_dep(end) ->
// w_arr(other end) puts it in the route matrix. SGTC_BotTransportThink makes
// a bot whose route takes that path press the activator and wait on a
// departure spot; SGTC_TransportSafety (every frame, every bot) steps bots
// off a platform before the far-side kill or the rings, and keeps everyone
// inside a ring wall still until the rings are back at rest.
//
// Timings, seconds after the press (the maps' multi_managers):
//   rt:      the pressed platform's rings rise +3.3, its teleports send to
//            the holding cells "rtstorage*" +5.8; the far rings rise +5.8,
//            far hurt +7.8..8.2, the cells empty onto the far platform +8.3;
//            both activators work again +12.8
//   obelisk: other pad hurt +3.6..3.7, teleport +3.7, buttons back +5.5
//   telepo:  the pads point at the other side's pads from +3.0 to +4.0
//
extern PATH paths[MAX_WAYPOINTS];
int sgtc_transport = 1;  // "sgtc_transport 0": bots leave these transporters alone (read at map start)

#define SGTC_BT_MAX          8
#define SGTC_BT_RINGS        1
#define SGTC_BT_OBELISK      2
#define SGTC_BT_PADS         3

#define SGTC_BT_GO           0  // to the activator, or waiting there
#define SGTC_BT_WAIT         1  // on a departure spot until the teleport
#define SGTC_BT_HOLD         2  // in an rt holding cell, between the two hops

#define SGTC_BT_RT_RINGS     3.3f
#define SGTC_BT_RT_DEPART    5.8f
#define SGTC_BT_RT_FAR_RINGS 5.8f
#define SGTC_BT_RT_ARRIVE    8.3f
#define SGTC_BT_RT_REARM     12.8f
#define SGTC_BT_OB_DEPART    3.7f
#define SGTC_BT_OB_REARM     5.5f
#define SGTC_BT_PAD_ON       3.0f
#define SGTC_BT_PAD_OFF      4.0f

static const char *sgtc_bt_kind_name[4] = {"?", "rt rings", "obelisk", "telepo pads"};

typedef struct
{
   char name[16];           // "rt1", "obelisk2", "telepo", "telepoa"
   int kind;
   int partner;             // the other end, -1 = none
   Vector center;           // standing position (origin height) in the middle
   Vector vmin, vmax;       // the platform's teleport volume (pads: their union)
   float r_in, r_out;       // rt: ring band from the platform's edge to the rings' edge
   edict_t *act[2];         // activators: func_button, or a trigger_multiple on the platform
   int act_team[2];         // their m_team: 0 anyone, 1 Tau'ri only, 2 Goa'uld only
   int num_act;
   qboolean act_touch;      // act[0] is a trigger_multiple: walking onto the platform presses
   Vector press;            // where to stand to press act[0] (rt button, obelisk)
   Vector spot[8];          // departure spots (rt: one per teleport quadrant)
   int num_spots;
   edict_t *train[6];       // rt: the rings
   Vector train_rest[6];
   float train_moved[6];
   int num_trains;
   edict_t *pad[8];         // telepo: the teleport columns
   int num_pads;
   char pad_target0[32];    // ... their target at rest ("desti", missing)
   Vector arrive;           // where arrivals from this end land (at the other end)
   Vector offset;           // obelisk special_teleport_in: arrival = position + offset
   qboolean keep_offset;
   int w_dep[2];
   int num_dep;
   int w_arr;
   qboolean linked, can_link;
   float cost;              // route cost of the forced path
   qboolean armed_prev, rings_prev;
   float frame_prev[2];
   float t_send;            // last cycle this end sent
   float t_recv;            // last cycle this end received
   float t_unk;             // last cycle of this pair whose sending end isn't known (yet)
   float f_bot_press;       // a bot pressed it / walked onto it
   int bot_presser;
} sgtc_bt_t;

static sgtc_bt_t sgtc_bts[SGTC_BT_MAX];
static int sgtc_num_bts;
static Vector sgtc_bt_cells[16];  // rt holding cells: the "rtstorage*" destinations
static int sgtc_bt_num_cells;

// waypoints the per-map work chose: used while they still lie where
// expected, else the geometric pick in SGTC_BtWaypoints
static const struct { const char *map, *end; int dep[2]; int arr; } sgtc_bt_hints[] =
{
   {"sg_strike3", "rt1", {613, -1}, -1},      // press spots, 30 units west of the buttons
   {"sg_strike3", "rt2", {614, -1}, -1},
   {"sg_hina", "obelisk1", {218, -1}, 218},   // pad centres
   {"sg_hina", "obelisk2", {217, -1}, 217},
   {"sg_target2a", "telepo", {637, 638}, 637},   // front row of the bottom pads
   {"sg_target2a", "telepoa", {402, 401}, 402},  // either side of the top pads
   {NULL, NULL, {-1, -1}, -1}
};

// keyvalues the engine doesn't keep in entvars: m_team of the activators
// (hl.dll's CBaseButton use refuses players whose team name isn't "Torri"
// for m_team 1, "Goa'uld" for 2) and the two markers of a special_teleport_in
#define SGTC_BT_MAX_KV 96
static struct { edict_t *ent; int team; char message[32], target[32]; } sgtc_bt_kv[SGTC_BT_MAX_KV];
static int sgtc_bt_num_kv;

static int SGTC_BtKvFind(edict_t *pent)
{
   for (int i = 0; i < sgtc_bt_num_kv; i++)
      if (sgtc_bt_kv[i].ent == pent)
         return i;
   return -1;
}

static void SGTC_TransportKeyValue(edict_t *pent, KeyValueData *pkvd)
{
   const char *cls = pkvd->szClassName, *key = pkvd->szKeyName;
   if (!cls || !key || !pkvd->szValue || !pent)
      return;
   qboolean team = !stricmp(key, "m_team") &&
                   (!stricmp(cls, "func_button") || !stricmp(cls, "func_rot_button") || !stricmp(cls, "trigger_multiple"));
   qboolean marker = !stricmp(cls, "special_teleport_in") && (!stricmp(key, "message") || !stricmp(key, "target"));
   if (!team && !marker)
      return;
   int i = SGTC_BtKvFind(pent);
   if (i < 0)
   {
      if (sgtc_bt_num_kv >= SGTC_BT_MAX_KV)
         return;
      i = sgtc_bt_num_kv++;
      memset(&sgtc_bt_kv[i], 0, sizeof(sgtc_bt_kv[i]));
      sgtc_bt_kv[i].ent = pent;
   }
   if (team)
      sgtc_bt_kv[i].team = atoi(pkvd->szValue);
   else if (!stricmp(key, "message"))
      safe_strcopy(sgtc_bt_kv[i].message, sizeof(sgtc_bt_kv[i].message), pkvd->szValue);
   else
      safe_strcopy(sgtc_bt_kv[i].target, sizeof(sgtc_bt_kv[i].target), pkvd->szValue);
}

static int SGTC_BtTeamOf(edict_t *pent)
{
   int i = SGTC_BtKvFind(pent);
   return i >= 0 ? sgtc_bt_kv[i].team : 0;
}

static void SGTC_ResetTransports(void)
{
   sgtc_num_bts = 0;
   sgtc_bt_num_cells = 0;
   sgtc_bt_num_kv = 0;
}

static void SGTC_TransportBotReset(bot_t &pBot)
{
   pBot.sgtc_bt = 0;
   pBot.sgtc_bt_state = SGTC_BT_GO;
   pBot.sgtc_bt_spot = -1;
   pBot.sgtc_bt_act = 0;
   pBot.sgtc_bt_tries = 0;
   pBot.sgtc_bt_arrived = 0;
   pBot.sgtc_bt_safe_act = 0;
   pBot.f_sgtc_bt_time = 0;
   pBot.f_sgtc_bt_wait = 0;
   pBot.f_sgtc_bt_t0 = 0;
   pBot.f_sgtc_bt_next_use = 0;
   pBot.f_sgtc_bt_arrived_time = 0;
   pBot.f_sgtc_bt_safe_until = 0;
   pBot.f_sgtc_bt_check = 0;
   pBot.v_sgtc_bt_last = Vector(0, 0, 0);
}

// entities this code drives, for any other teleport linker to leave alone:
// the rt platforms and holding cells, the obelisk pads, the telepo pads
qboolean SGTC_TransportOwns(edict_t *pent)
{
   if (FNullEnt(pent))
      return FALSE;
   const char *n = STRING(pent->v.targetname);
   if (!strncmp(n, "obeliskteleportout", 18) || !strncmp(n, "telepo", 6) || !strncmp(n, "rtstorage", 9))
      return TRUE;
   return n[0] == 'r' && n[1] == 't' && n[2] >= '1' && n[2] <= '9' && strstr(n, "_teleportout") != NULL;
}

//
// geometry helpers
//
static void SGTC_BtBox(edict_t *pent, Vector &mn, Vector &mx)
{
   if (pent->v.maxs != pent->v.mins)
   {
      mn = pent->v.origin + pent->v.mins;
      mx = pent->v.origin + pent->v.maxs;
   }
   else
   {
      mn = pent->v.absmin;
      mx = pent->v.absmax;
   }
}

static void SGTC_BtBoxAdd(Vector &mn, Vector &mx, const Vector &a, const Vector &b)
{
   if (a.x < mn.x) mn.x = a.x;
   if (a.y < mn.y) mn.y = a.y;
   if (a.z < mn.z) mn.z = a.z;
   if (b.x > mx.x) mx.x = b.x;
   if (b.y > mx.y) mx.y = b.y;
   if (b.z > mx.z) mx.z = b.z;
}

// distance from p to the box (0 inside)
static float SGTC_BtBoxDist(const Vector &p, const Vector &mn, const Vector &mx)
{
   float dx = p.x < mn.x ? mn.x - p.x : (p.x > mx.x ? p.x - mx.x : 0.0f);
   float dy = p.y < mn.y ? mn.y - p.y : (p.y > mx.y ? p.y - mx.y : 0.0f);
   float dz = p.z < mn.z ? mn.z - p.z : (p.z > mx.z ? p.z - mx.z : 0.0f);
   return sqrtf(dx * dx + dy * dy + dz * dz);
}

// horizontal distance from c to the segment a-b
static float SGTC_BtSegDist2D(const Vector &a, const Vector &b, const Vector &c)
{
   float abx = b.x - a.x, aby = b.y - a.y, len2 = abx * abx + aby * aby;
   float t = len2 > 0.0f ? ((c.x - a.x) * abx + (c.y - a.y) * aby) / len2 : 0.0f;
   if (t < 0.0f) t = 0.0f;
   if (t > 1.0f) t = 1.0f;
   float dx = a.x + abx * t - c.x, dy = a.y + aby * t - c.y;
   return sqrtf(dx * dx + dy * dy);
}

// on the pad (obelisk) / pads (telepo): inside the volume widened by 'margin' (negative: narrowed)
static qboolean SGTC_BtOnPad(const sgtc_bt_t &e, const Vector &o, float margin)
{
   return o.x > e.vmin.x - margin && o.x < e.vmax.x + margin && o.y > e.vmin.y - margin && o.y < e.vmax.y + margin &&
          fabs(o.z - e.center.z) < 100.0f;
}

static void SGTC_BtAddSpot(sgtc_bt_t &e, const Vector &p)
{
   Vector g;
   if (e.num_spots >= 8 || !SGTC_GroundAt(p + Vector(0, 0, 18), 18.0f + 40.0f, g))
      return;
   TraceResult tr;
   UTIL_TraceHull(g, g + Vector(0, 0, 1), ignore_monsters, human_hull, NULL, &tr);
   if (tr.fStartSolid || tr.fAllSolid)
      return;
   e.spot[e.num_spots++] = g;
}

// where to stand to press the button act[0]: in +use reach (64 units from
// the origin to its box), seeing its middle; rt: outside the ring band, the
// nearest to the platform; obelisk: on the pad
static qboolean SGTC_BtFindPressSpot(sgtc_bt_t &e)
{
   edict_t *pAct = e.act[0];
   if (FNullEnt(pAct))
      return FALSE;
   Vector mn, mx;
   SGTC_BtBox(pAct, mn, mx);
   Vector bc = (mn + mx) * 0.5f;
   float best = 99999.0f;
   for (float dx = -88.0f; dx <= 88.0f; dx += 8.0f)
      for (float dy = -88.0f; dy <= 88.0f; dy += 8.0f)
      {
         Vector g;
         if (!SGTC_GroundAt(Vector(bc.x + dx, bc.y + dy, bc.z + 24.0f), 160.0f, g))
            continue;
         float reach = SGTC_BtBoxDist(g, mn, mx);
         if (reach > 50.0f || reach < 4.0f)
            continue;
         TraceResult tr;
         UTIL_TraceLine(g + Vector(0, 0, 28), bc, ignore_monsters, NULL, &tr);
         if (tr.flFraction < 1.0f && tr.pHit != pAct)
            continue;
         float score;
         if (e.kind == SGTC_BT_RINGS)
         {
            float r = (g - e.center).Make2D().Length();
            if (r < e.r_out + 32.0f || fabs(g.z - e.center.z) > 64.0f)
               continue;
            score = r + 3.0f * fabs(reach - 30.0f);
         }
         else
         {
            if (!SGTC_BtOnPad(e, g, -12.0f))
               continue;
            score = fabs(reach - 40.0f);
         }
         if (score < best)
         {
            best = score;
            e.press = g;
         }
      }
   return best < 99999.0f;
}

//
// finding the ends
//
static void SGTC_BtFindRings(void)
{
   for (int n = 1; n <= 9 && sgtc_num_bts < SGTC_BT_MAX; n++)
   {
      sgtc_bt_t e;
      memset(&e, 0, sizeof(e));
      safevoid_snprintf(e.name, sizeof(e.name), "rt%d", n);
      e.kind = SGTC_BT_RINGS;
      e.partner = -1;
      e.w_arr = -1;
      char buf[48];

      // the platform: rt<N>_teleportout_1..4, one per quadrant
      int num = 0;
      for (int k = 1; k <= 8; k++)
      {
         safevoid_snprintf(buf, sizeof(buf), "rt%d_teleportout_%d", n, k);
         edict_t *pent = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (FNullEnt(pent))
            continue;
         Vector mn, mx;
         SGTC_BtBox(pent, mn, mx);
         if (!num)
         {
            e.vmin = mn;
            e.vmax = mx;
         }
         else
            SGTC_BtBoxAdd(e.vmin, e.vmax, mn, mx);
         num++;
      }
      if (!num)
         continue;
      Vector c((e.vmin.x + e.vmax.x) * 0.5f, (e.vmin.y + e.vmax.y) * 0.5f, e.vmin.z + 54.0f);
      if (!SGTC_GroundAt(c, 100.0f, e.center))
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no floor in the middle of its platform, skipped\n", e.name);
         continue;
      }
      float wx = (e.vmax.x - e.vmin.x) * 0.5f, wy = (e.vmax.y - e.vmin.y) * 0.5f;
      e.r_in = wx < wy ? wx : wy;

      // the rings rt<N>_ring1..5: the band reaches their outer edge
      float r_out = 0;
      for (int k = 1; k <= 9 && e.num_trains < 6; k++)
      {
         safevoid_snprintf(buf, sizeof(buf), "rt%d_ring%d", n, k);
         edict_t *pent = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (FNullEnt(pent))
            continue;
         Vector mn, mx;
         SGTC_BtBox(pent, mn, mx);
         float h = (mx.x - mn.x) > (mx.y - mn.y) ? (mx.x - mn.x) * 0.5f : (mx.y - mn.y) * 0.5f;
         if (h > r_out)
            r_out = h;
         e.train[e.num_trains] = pent;
         e.train_rest[e.num_trains] = pent->v.origin;
         e.train_moved[e.num_trains] = gpGlobals->time;
         e.num_trains++;
      }
      e.r_out = r_out > e.r_in ? r_out : e.r_in + 30.0f;

      // the activator rt<N>_button: a func_button, or a trigger_multiple on the platform
      safevoid_snprintf(buf, sizeof(buf), "rt%d_button", n);
      edict_t *pAct = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
      if (FNullEnt(pAct))
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no %s, skipped\n", e.name, buf);
         continue;
      }
      e.act[0] = pAct;
      e.act_team[0] = SGTC_BtTeamOf(pAct);
      e.num_act = 1;
      e.act_touch = !strcmp(STRING(pAct->v.classname), "trigger_multiple") || !strcmp(STRING(pAct->v.classname), "trigger_once");

      // arrivals from here land on rt<N>_teleportin_1..4, on the other platform
      Vector sum(0, 0, 0);
      int na = 0;
      for (int k = 1; k <= 8; k++)
      {
         safevoid_snprintf(buf, sizeof(buf), "rt%d_teleportin_%d", n, k);
         edict_t *pent = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (FNullEnt(pent))
            continue;
         sum = sum + pent->v.origin;
         na++;
      }
      if (!na)
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no rt%d_teleportin_*, skipped\n", e.name, n);
         continue;
      }
      e.arrive = sum * (1.0f / na);

      // departure spots: one per teleport quadrant (the quadrant's holding
      // cell sets down on the same quadrant at the far end, so bots leaving
      // together don't land in each other), inside the ring wall
      float q = (e.r_in - 24.0f) / 1.414f;
      if (q > 24.0f) q = 24.0f;
      if (q < 8.0f) q = 8.0f;
      for (int k = 0; k < 4; k++)
         SGTC_BtAddSpot(e, e.center + Vector((k & 1) ? q : -q, (k & 2) ? q : -q, 0));
      if (!e.num_spots)
         SGTC_BtAddSpot(e, e.center);

      if (!e.act_touch && !SGTC_BtFindPressSpot(e))
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no spot to press %s from, skipped\n", e.name, STRING(pAct->v.targetname));
         continue;
      }
      sgtc_bts[sgtc_num_bts++] = e;
   }

   // the other end: the platform the arrivals land on
   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      if (e.kind != SGTC_BT_RINGS)
         continue;
      float best = 160.0f;
      for (int j = 0; j < sgtc_num_bts; j++)
      {
         if (j == i || sgtc_bts[j].kind != SGTC_BT_RINGS)
            continue;
         float d = (sgtc_bts[j].center - e.arrive).Length();
         if (d < best)
         {
            best = d;
            e.partner = j;
         }
      }
   }

   // the holding cells between the two hops
   edict_t *pent = NULL;
   while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "info_teleport_destination")) && sgtc_bt_num_cells < 16)
      if (!strncmp(STRING(pent->v.targetname), "rtstorage", 9))
         sgtc_bt_cells[sgtc_bt_num_cells++] = pent->v.origin;
}

static void SGTC_BtFindObelisks(void)
{
   int first = sgtc_num_bts;
   for (int n = 1; n <= 4 && sgtc_num_bts < SGTC_BT_MAX; n++)
   {
      sgtc_bt_t e;
      memset(&e, 0, sizeof(e));
      safevoid_snprintf(e.name, sizeof(e.name), "obelisk%d", n);
      e.kind = SGTC_BT_OBELISK;
      e.partner = -1;
      e.w_arr = -1;
      char buf[48];

      // the pad: a special_teleport_in (sg_hina, keeps the offset between its
      // two markers) or a trigger_teleport (sg_quietvillage, retargeted to
      // obeliskteleportin<other>), toggled on for 0.1 s by the button
      safevoid_snprintf(buf, sizeof(buf), "obeliskteleportout%d", n);
      edict_t *pPad = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
      safevoid_snprintf(buf, sizeof(buf), "obeliskbutton%d", n);
      edict_t *pAct = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
      if (FNullEnt(pPad) || FNullEnt(pAct))
         continue;
      SGTC_BtBox(pPad, e.vmin, e.vmax);
      Vector c((e.vmin.x + e.vmax.x) * 0.5f, (e.vmin.y + e.vmax.y) * 0.5f, e.vmin.z + 54.0f);
      if (!SGTC_GroundAt(c, 100.0f, e.center))
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no floor in the middle of its pad, skipped\n", e.name);
         continue;
      }
      float wx = (e.vmax.x - e.vmin.x) * 0.5f, wy = (e.vmax.y - e.vmin.y) * 0.5f;
      e.r_in = e.r_out = wx > wy ? wx : wy;
      e.act[0] = pAct;
      e.act_team[0] = SGTC_BtTeamOf(pAct);
      e.num_act = 1;

      if (!strcmp(STRING(pPad->v.classname), "special_teleport_in"))
      {
         int kv = SGTC_BtKvFind(pPad);
         const char *m1 = (kv >= 0 && sgtc_bt_kv[kv].message[0]) ? sgtc_bt_kv[kv].message : STRING(pPad->v.message);
         const char *m2 = (kv >= 0 && sgtc_bt_kv[kv].target[0]) ? sgtc_bt_kv[kv].target : STRING(pPad->v.target);
         edict_t *a = m1[0] ? FIND_ENTITY_BY_TARGETNAME(NULL, m1) : NULL;
         edict_t *b = m2[0] ? FIND_ENTITY_BY_TARGETNAME(NULL, m2) : NULL;
         if (!FNullEnt(a) && !FNullEnt(b))
         {
            e.offset = b->v.origin - a->v.origin;
            e.keep_offset = TRUE;
         }
      }
      else
      {
         safevoid_snprintf(buf, sizeof(buf), "obeliskteleportin%d", (n & 1) ? n + 1 : n - 1);
         edict_t *pDest = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (!FNullEnt(pDest))
            e.arrive = pDest->v.origin;
      }

      if (!SGTC_BtFindPressSpot(e))
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no spot on the pad to press %s from, skipped\n", e.name, STRING(pAct->v.targetname));
         continue;
      }
      // the presser's spot, then others on the pad
      Vector in = e.center - e.press;
      in.z = 0;
      in = in.Length() > 1.0f ? in.Normalize() : Vector(1, 0, 0);
      Vector side(-in.y, in.x, 0);
      Vector cand[6] = {e.press, e.press + side * 44.0f, e.press - side * 44.0f, e.press + in * 48.0f,
                        e.press + in * 48.0f + side * 44.0f, e.press + in * 48.0f - side * 44.0f};
      for (int k = 0; k < 6; k++)
         if (SGTC_BtOnPad(e, cand[k], -12.0f))
            SGTC_BtAddSpot(e, cand[k]);
      if (!e.num_spots)
         continue;
      sgtc_bts[sgtc_num_bts++] = e;
   }

   // pairs 1-2, 3-4
   for (int i = first; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      int n = atoi(e.name + 7);
      char other[16];
      safevoid_snprintf(other, sizeof(other), "obelisk%d", (n & 1) ? n + 1 : n - 1);
      for (int j = first; j < sgtc_num_bts; j++)
         if (!strcmp(sgtc_bts[j].name, other))
            e.partner = j;
      if (e.partner < 0)
         continue;
      if (!e.keep_offset && e.arrive == Vector(0, 0, 0))
      {
         e.offset = sgtc_bts[e.partner].center - e.center;  // no marker found: centre to centre
         e.keep_offset = TRUE;
      }
      if (e.keep_offset)
         e.arrive = e.press + e.offset;
   }
}

static void SGTC_BtFindPads(void)
{
   static const char *pad_fmt[2] = {"telepo%d", "telepo%da"};
   static const char *dest_fmt[2] = {"desti%da", "desti%d"};  // where each side's pads send
   int first = sgtc_num_bts;
   for (int s = 0; s < 2 && sgtc_num_bts < SGTC_BT_MAX; s++)
   {
      sgtc_bt_t e;
      memset(&e, 0, sizeof(e));
      safe_strcopy(e.name, sizeof(e.name), s ? "telepoa" : "telepo");
      e.kind = SGTC_BT_PADS;
      e.partner = -1;
      e.w_arr = -1;
      char buf[48];
      Vector sum(0, 0, 0), asum(0, 0, 0);
      int na = 0;
      for (int k = 1; k <= 8; k++)
      {
         safevoid_snprintf(buf, sizeof(buf), pad_fmt[s], k);
         edict_t *pPad = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (FNullEnt(pPad) || strcmp(STRING(pPad->v.classname), "trigger_teleport"))
            continue;
         Vector mn, mx;
         SGTC_BtBox(pPad, mn, mx);
         if (!e.num_pads)
         {
            e.vmin = mn;
            e.vmax = mx;
         }
         else
            SGTC_BtBoxAdd(e.vmin, e.vmax, mn, mx);
         e.pad[e.num_pads++] = pPad;
         sum = sum + (mn + mx) * 0.5f;
         // standing on the pad's centre
         SGTC_BtAddSpot(e, Vector((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, mn.z + 2.0f));
         safevoid_snprintf(buf, sizeof(buf), dest_fmt[s], k);
         edict_t *pDest = FIND_ENTITY_BY_TARGETNAME(NULL, buf);
         if (!FNullEnt(pDest))
         {
            asum = asum + pDest->v.origin;
            na++;
         }
      }
      if (e.num_pads < 2 || !na || !e.num_spots)
         continue;
      Vector c = sum * (1.0f / e.num_pads);
      e.center = Vector(c.x, c.y, e.spot[0].z);
      e.arrive = asum * (1.0f / na);
      safe_strcopy(e.pad_target0, sizeof(e.pad_target0), STRING(e.pad[0]->v.target));
      float r = 0;
      for (int k = 0; k < e.num_spots; k++)
      {
         float d = (e.spot[k] - e.center).Make2D().Length();
         if (d > r)
            r = d;
      }
      e.r_in = e.r_out = r + 24.0f;

      // the button(s) in the middle of the pads (sg_target2a: one for anyone
      // and one for the Goa'uld, in the same place); anyone's first
      edict_t *pent = NULL;
      while (!FNullEnt(pent = FIND_ENTITY_BY_CLASSNAME(pent, "func_button")) && e.num_act < 2)
      {
         Vector mn, mx;
         SGTC_BtBox(pent, mn, mx);
         Vector bc = (mn + mx) * 0.5f;
         if ((bc - c).Make2D().Length() > 48.0f || fabs(bc.z - e.center.z) > 200.0f)
            continue;
         int team = SGTC_BtTeamOf(pent);
         if (team == 0 && e.num_act == 1 && e.act_team[0] != 0)
         {
            e.act[1] = e.act[0];
            e.act_team[1] = e.act_team[0];
            e.act[0] = pent;
            e.act_team[0] = 0;
         }
         else
         {
            e.act[e.num_act] = pent;
            e.act_team[e.num_act] = team;
         }
         e.num_act++;
      }
      if (!e.num_act)
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: no button among its pads, skipped\n", e.name);
         continue;
      }
      sgtc_bts[sgtc_num_bts++] = e;
   }
   if (sgtc_num_bts - first == 2)
   {
      sgtc_bts[first].partner = first + 1;
      sgtc_bts[first + 1].partner = first;
   }
   else
      sgtc_num_bts = first;  // one side only: of no use
}

//
// waypoints and links
//
static int SGTC_BtWaypointNear(const Vector &pos, float radius, const Vector &avoid, float avoid_r)
{
   int best = -1;
   float best_d = radius;
   for (int i = 0; i < num_waypoints; i++)
   {
      if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP))
         continue;
      const Vector &w = waypoints[i].origin;
      if (fabs(w.z - pos.z) > 48.0f)
         continue;
      if (avoid_r > 0 && (w - avoid).Make2D().Length() < avoid_r)
         continue;
      float d = (w - pos).Make2D().Length();
      if (d < best_d)
      {
         best_d = d;
         best = i;
      }
   }
   return best;
}

static qboolean SGTC_BtHintOk(const sgtc_bt_t &e, int w, qboolean dep)
{
   if (w < 0 || w >= num_waypoints || (waypoints[w].flags & W_FL_DELETED))
      return FALSE;
   Vector d = waypoints[w].origin - e.center;
   if (fabs(d.z) > 64.0f || d.Make2D().Length() > 420.0f)
      return FALSE;
   return !(dep && e.kind == SGTC_BT_RINGS && d.Make2D().Length() < e.r_out + 32.0f);  // not on the ring band
}

static void SGTC_BtWaypoints(sgtc_bt_t &e)
{
   const char *map = STRING(gpGlobals->mapname);
   e.num_dep = 0;
   e.w_arr = -1;
   for (int h = 0; sgtc_bt_hints[h].map; h++)
   {
      if (stricmp(map, sgtc_bt_hints[h].map) || strcmp(e.name, sgtc_bt_hints[h].end))
         continue;
      for (int k = 0; k < 2; k++)
      {
         int w = sgtc_bt_hints[h].dep[k];
         if (w < 0)
            continue;
         if (SGTC_BtHintOk(e, w, TRUE))
            e.w_dep[e.num_dep++] = w;
         else
            UTIL_ConsolePrintf("[sgtc-stat] transport %s: map hint wpt %d not where expected, ignored\n", e.name, w);
      }
      int w = sgtc_bt_hints[h].arr;
      if (w >= 0)
      {
         if (SGTC_BtHintOk(e, w, FALSE))
            e.w_arr = w;
         else
            UTIL_ConsolePrintf("[sgtc-stat] transport %s: map hint wpt %d not where expected, ignored\n", e.name, w);
      }
   }

   // a hinted departure waypoint in +use reach of the button is the press
   // spot (sg_strike3: 30 units west of the buttons, well off the ring band)
   if (e.kind == SGTC_BT_RINGS && !e.act_touch && e.num_dep && !FNullEnt(e.act[0]))
   {
      Vector mn, mx;
      SGTC_BtBox(e.act[0], mn, mx);
      if (SGTC_BtBoxDist(waypoints[e.w_dep[0]].origin, mn, mx) <= 50.0f)
         e.press = waypoints[e.w_dep[0]].origin;
   }

   if (e.kind == SGTC_BT_RINGS)
   {
      // arrivals: the middle of the platform (in-paths removed in SGTC_BuildTransports)
      if (e.w_arr == -1)
         e.w_arr = SGTC_WaypointAt(e.center, 40.0f, 0);
      if (!e.num_dep)
      {
         // off the ring band: in front of the button, or the nearest to a trigger platform
         int w = e.act_touch ? SGTC_BtWaypointNear(e.center, 300.0f, e.center, e.r_out + 32.0f)
                             : SGTC_BtWaypointNear(e.press, 80.0f, e.center, e.r_out + 32.0f);
         if (w == -1 && !e.act_touch)
            w = WaypointAddOne(e.press, 0);
         if (w != -1)
            e.w_dep[e.num_dep++] = w;
      }
   }
   else if (e.kind == SGTC_BT_OBELISK)
   {
      if (!e.num_dep)
      {
         int w = SGTC_BtWaypointNear(e.center, 80.0f, e.center, 0);
         if (w == -1)
            w = WaypointAddOne(e.center, 0);
         if (w != -1)
            e.w_dep[e.num_dep++] = w;
      }
      if (e.w_arr == -1 && e.num_dep)
         e.w_arr = e.w_dep[0];
   }
   else
   {
      // the two nearest to the button, off the pads
      while (e.num_dep < 2)
      {
         int best = -1;
         float best_d = 170.0f;
         for (int i = 0; i < num_waypoints; i++)
         {
            if ((waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_LADDER | W_FL_JUMP)) ||
                fabs(waypoints[i].origin.z - e.center.z) > 48.0f || (e.num_dep && e.w_dep[0] == i))
               continue;
            qboolean on_pad = FALSE;
            for (int k = 0; k < e.num_spots && !on_pad; k++)
               on_pad = (waypoints[i].origin - e.spot[k]).Make2D().Length() < 28.0f;
            float d = (waypoints[i].origin - e.center).Make2D().Length();
            if (!on_pad && d < best_d)
            {
               best_d = d;
               best = i;
            }
         }
         if (best == -1)
            break;
         e.w_dep[e.num_dep++] = best;
      }
      if (e.w_arr == -1 && e.num_dep)
         e.w_arr = (e.num_dep > 1 && (waypoints[e.w_dep[1]].origin - e.center).Length() < (waypoints[e.w_dep[0]].origin - e.center).Length())
                   ? e.w_dep[1] : e.w_dep[0];
   }
}

// paths that cross an rt ring band: the repair pass links waypoints across
// the platform, and walking over sg_tokra2's rt1/rt3 fires their
// trigger_multiple; whoever is on the band when the rings rise is crushed
static int SGTC_BtUnpathBand(const sgtc_bt_t &e)
{
   int n = 0;
   float R = e.r_out + 20.0f;
   for (int a = 0; a < num_waypoints; a++)
   {
      if (waypoints[a].flags & W_FL_DELETED)
         continue;
      PATH &pa = paths[a];
      for (int k = 0; k < pa.last_idx_used; k++)
      {
         int b = pa.index[k];
         if (b < 0 || b >= num_waypoints || a == e.w_arr || b == e.w_arr)
            continue;
         const Vector &A = waypoints[a].origin, &B = waypoints[b].origin;
         if (fabs(A.z - e.center.z) > 80.0f && fabs(B.z - e.center.z) > 80.0f)
            continue;
         if (SGTC_BtSegDist2D(A, B, e.center) < R)
         {
            pa.index[k] = -1;
            n++;
         }
      }
   }
   return n;
}

static const char *SGTC_BtDeps(const sgtc_bt_t &e, char *buf, int size)
{
   if (!e.num_dep)
      safevoid_snprintf(buf, size, "none");
   else if (e.num_dep == 1)
      safevoid_snprintf(buf, size, "%d", e.w_dep[0]);
   else
      safevoid_snprintf(buf, size, "%d,%d", e.w_dep[0], e.w_dep[1]);
   return buf;
}

static void SGTC_BtLink(int i, qboolean rebuild)
{
   sgtc_bt_t &e = sgtc_bts[i];
   sgtc_bt_t &p = sgtc_bts[e.partner];
   for (int k = 0; k < e.num_dep; k++)
      WaypointForceLongPath(e.w_dep[k], p.w_arr, e.cost);
   e.linked = TRUE;
   char deps[24];
   UTIL_ConsolePrintf("[sgtc-stat] transport %s: linked wpt %s -> %d (%s), cost %.0f\n", e.name, SGTC_BtDeps(e, deps, sizeof(deps)),
      p.w_arr, p.name, e.cost);
   if (rebuild)
      WaypointRebuildRoutes();
}

static int SGTC_BtActState(const sgtc_bt_t &e);

static void SGTC_BtLog(const sgtc_bt_t &e)
{
   const sgtc_bt_t *p = e.partner >= 0 ? &sgtc_bts[e.partner] : NULL;
   static const char *team_name[3] = {"anyone", "Tau'ri only", "Goa'uld only"};
   int team = e.act_team[0] >= 0 && e.act_team[0] <= 2 ? e.act_team[0] : 0;
   const char *target = FNullEnt(e.act[0]) ? "" : STRING(e.act[0]->v.target);
   UTIL_ConsolePrintf("[sgtc-stat] transport %s (%s, other end %s): platform %.0f %.0f %.0f, %s %.0f-%.0f, activator \"%s\" %s (%s%s), target \"%s\"%s\n",
      e.name, sgtc_bt_kind_name[e.kind], p ? p->name : "none", e.center.x, e.center.y, e.center.z,
      e.kind == SGTC_BT_RINGS ? "ring band" : "radius", e.kind == SGTC_BT_RINGS ? e.r_in : 0.0f, e.r_out,
      FNullEnt(e.act[0]) ? "?" : STRING(e.act[0]->v.targetname), FNullEnt(e.act[0]) ? "?" : STRING(e.act[0]->v.classname),
      team_name[team], e.num_act > 1 ? ", +1 for one side" : "", target,
      e.kind != SGTC_BT_PADS && SGTC_BtActState(e) == 0 ? " (disabled)" : "");
   char deps[24], press[48];
   if (e.kind == SGTC_BT_PADS)
      safe_strcopy(press, sizeof(press), "from the pads");
   else if (e.act_touch)
      safe_strcopy(press, sizeof(press), "by walking onto the platform");
   else
      safevoid_snprintf(press, sizeof(press), "at %.0f %.0f %.0f", e.press.x, e.press.y, e.press.z);
   UTIL_ConsolePrintf("[sgtc-stat]   %s: press %s, %d spots, %d rings, %d pads, arrivals land at %.0f %.0f %.0f%s, wpt dep %s arr %d\n",
      e.name, press, e.num_spots, e.num_trains, e.num_pads, e.arrive.x, e.arrive.y, e.arrive.z,
      e.keep_offset ? " (keeps the offset)" : "", SGTC_BtDeps(e, deps, sizeof(deps)), e.w_arr);
}

static void SGTC_BuildTransports(void)
{
   sgtc_num_bts = 0;
   sgtc_bt_num_cells = 0;
   if (!sgtc_transport)
   {
      UTIL_ConsolePrintf("[sgtc-stat] transports: off (sgtc_transport 0)\n");
      return;
   }
   SGTC_BtFindRings();
   SGTC_BtFindObelisks();
   SGTC_BtFindPads();
   if (!sgtc_num_bts)
      return;

   for (int i = 0; i < sgtc_num_bts; i++)
      SGTC_BtWaypoints(sgtc_bts[i]);

   int removed = 0, links = 0;
   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      if (e.kind != SGTC_BT_RINGS || e.w_arr < 0)
         continue;
      // only arrivals start in the middle of an rt platform: no path into it
      for (int a = 0; a < num_waypoints; a++)
      {
         PATH &pa = paths[a];
         for (int k = 0; k < pa.last_idx_used; k++)
            if (pa.index[k] == e.w_arr)
            {
               pa.index[k] = -1;
               removed++;
            }
      }
      removed += SGTC_BtUnpathBand(e);
   }

   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      // the route matrix needs a cost: the trip plus some waiting, at ~250 units/s
      e.cost = 250.0f * ((e.kind == SGTC_BT_RINGS ? SGTC_BT_RT_ARRIVE : e.kind == SGTC_BT_OBELISK ? SGTC_BT_OB_DEPART : SGTC_BT_PAD_ON) + 3.0f);
      e.armed_prev = SGTC_BtActState(e) == 2;
      for (int k = 0; k < e.num_act; k++)
         e.frame_prev[k] = FNullEnt(e.act[k]) ? 0 : e.act[k]->v.frame;
      SGTC_BtLog(e);
      e.can_link = e.partner >= 0 && e.num_dep && sgtc_bts[e.partner].w_arr >= 0;
      if (!e.can_link)
      {
         UTIL_ConsolePrintf("[sgtc-stat]   %s: not linked (%s)\n", e.name, e.partner < 0 ? "no other end" : !e.num_dep ? "no departure waypoint" : "no arrival waypoint at the other end");
         continue;
      }
      if (e.kind != SGTC_BT_PADS && SGTC_BtActState(e) == 0)
      {
         UTIL_ConsolePrintf("[sgtc-stat]   %s: link deferred until its activator gets a target\n", e.name);
         continue;
      }
      SGTC_BtLink(i, FALSE);
      links++;
   }
   UTIL_ConsolePrintf("[sgtc-stat] transports: %d ends, %d linked, %d holding cells, %d paths into platforms or across ring bands removed\n",
      sgtc_num_bts, links, sgtc_bt_num_cells, removed);
   WaypointRebuildRoutes();
}

//
// cycles
//
static float SGTC_BtSince(float t)
{
   return (t > 0 && gpGlobals->time >= t) ? gpGlobals->time - t : -1.0f;
}

static float SGTC_BtDepart(const sgtc_bt_t &e)
{
   return e.kind == SGTC_BT_RINGS ? SGTC_BT_RT_DEPART : e.kind == SGTC_BT_OBELISK ? SGTC_BT_OB_DEPART : SGTC_BT_PAD_ON;
}

static float SGTC_BtRearm(const sgtc_bt_t &e)
{
   return e.kind == SGTC_BT_RINGS ? SGTC_BT_RT_REARM : e.kind == SGTC_BT_OBELISK ? SGTC_BT_OB_REARM : SGTC_BT_PAD_OFF + 0.5f;
}

// 0: no target (disabled), 1: a missing target (busy: a cycle runs), 2: armed
static int SGTC_BtActState(const sgtc_bt_t &e)
{
   if (!e.num_act || FNullEnt(e.act[0]))
      return 0;
   const char *t = STRING(e.act[0]->v.target);
   if (!t[0])
      return 0;
   return FNullEnt(FIND_ENTITY_BY_TARGETNAME(NULL, t)) ? 1 : 2;
}

static qboolean SGTC_BtCycleRecent(const sgtc_bt_t &e, float dt)
{
   float s = SGTC_BtSince(e.t_send), r = SGTC_BtSince(e.t_recv), u = SGTC_BtSince(e.t_unk);
   return (s >= 0 && s < dt) || (r >= 0 && r < dt) || (u >= 0 && u < dt);
}

static qboolean SGTC_BtRingsActive(const sgtc_bt_t &e)
{
   for (int t = 0; t < e.num_trains; t++)
   {
      edict_t *pTrain = e.train[t];
      if (!FNullEnt(pTrain) && (pTrain->v.velocity.Length() > 1.0f || (pTrain->v.origin - e.train_rest[t]).Length() > 4.0f))
         return TRUE;
   }
   return FALSE;
}

// a living player at the activator (in the trigger, or in +use reach of the button)
static edict_t *SGTC_BtSomeoneAt(const sgtc_bt_t &e)
{
   for (int p = 1; p <= gpGlobals->maxClients; p++)
   {
      edict_t *pPlayer = INDEXENT(p);
      if (FNullEnt(pPlayer) || pPlayer->free || !IsAlive(pPlayer))
         continue;
      for (int k = 0; k < e.num_act; k++)
      {
         if (FNullEnt(e.act[k]))
            continue;
         Vector mn, mx;
         SGTC_BtBox(e.act[k], mn, mx);
         float d = SGTC_BtBoxDist(pPlayer->v.origin, mn, mx);
         if (e.act_touch ? d < 24.0f : d < 72.0f)
            return pPlayer;
      }
   }
   return NULL;
}

// which end of the pair sent: a bot pressed it (or walked onto it) a moment
// ago, else someone is at one activator only; -1 when it can't be told
static int SGTC_BtSender(int i, edict_t **who)
{
   int ends[2] = {i, sgtc_bts[i].partner};
   int found = -1, n = 0;
   *who = NULL;
   for (int k = 0; k < 2; k++)
   {
      if (ends[k] < 0)
         continue;
      float s = SGTC_BtSince(sgtc_bts[ends[k]].f_bot_press);
      if (s >= 0 && s < 1.5f)
      {
         found = ends[k];
         n++;
      }
   }
   if (n == 1)
      return found;
   found = -1;
   n = 0;
   for (int k = 0; k < 2; k++)
   {
      edict_t *pWho;
      if (ends[k] >= 0 && (pWho = SGTC_BtSomeoneAt(sgtc_bts[ends[k]])) != NULL)
      {
         found = ends[k];
         *who = pWho;
         n++;
      }
   }
   return n == 1 ? found : -1;
}

static void SGTC_BtCycleAt(int i, int src, float t0, const char *how, edict_t *who)
{
   char name[40];
   safe_strcopy(name, sizeof(name), FNullEnt(who) ? "?" : STRING(who->v.netname));
   if (src >= 0)
   {
      sgtc_bt_t &s = sgtc_bts[src];
      s.t_send = t0;
      s.t_unk = 0;
      if (s.partner >= 0)
      {
         sgtc_bts[s.partner].t_recv = t0;
         sgtc_bts[s.partner].t_unk = 0;
      }
      float b = SGTC_BtSince(s.f_bot_press);
      if (FNullEnt(who) && b >= 0 && s.f_bot_press - t0 < 0.5f && t0 - s.f_bot_press < 2.0f &&
          s.bot_presser >= 0 && s.bot_presser < 32 && bots[s.bot_presser].is_used && !FNullEnt(bots[s.bot_presser].pEdict))
         safe_strcopy(name, sizeof(name), STRING(bots[s.bot_presser].pEdict->v.netname));
      UTIL_ConsolePrintf("[sgtc-stat] transport %s -> %s: cycle t=%.1f (%s, by %s)\n", s.name,
         s.partner >= 0 ? sgtc_bts[s.partner].name : "?", t0, how, name);
   }
   else
   {
      sgtc_bt_t &e = sgtc_bts[i];
      e.t_unk = t0;
      if (e.partner >= 0)
         sgtc_bts[e.partner].t_unk = t0;
      UTIL_ConsolePrintf("[sgtc-stat] transport %s/%s: cycle t=%.1f (%s), sending end not known yet\n", e.name,
         e.partner >= 0 ? sgtc_bts[e.partner].name : "?", t0, how);
   }
}

//
// Every frame: rest of the rings, cycle starts and their sending end,
// deferred links. "sgtc_ringdebug 1": a btdbg line 4 times a second while
// a cycle runs.
//
static void SGTC_WatchTransports(void)
{
   static float last = -1, next_dbg = 0;
   float now = gpGlobals->time;
   if (last == now)
      return;
   last = now;
   if (!sgtc_num_bts)
      return;

   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];

      // rings: rest = still for 8 s (the snapshot can be mid-motion)
      qboolean rings = FALSE;
      for (int t = 0; t < e.num_trains; t++)
      {
         edict_t *pTrain = e.train[t];
         if (FNullEnt(pTrain))
            continue;
         if (pTrain->v.velocity.Length() > 1.0f || e.train_moved[t] > now)
            e.train_moved[t] = now;
         else if (e.train_moved[t] + 8.0f < now && (pTrain->v.origin - e.train_rest[t]).Length() > 4.0f)
         {
            UTIL_ConsolePrintf("[sgtc-stat] transport %s: ring \"%s\" rest corrected from %.0f to %.0f\n", e.name,
               STRING(pTrain->v.targetname), e.train_rest[t].z, pTrain->v.origin.z);
            e.train_rest[t] = pTrain->v.origin;
         }
         if (pTrain->v.velocity.Length() > 1.0f || (pTrain->v.origin - e.train_rest[t]).Length() > 4.0f)
            rings = TRUE;
      }
      if (e.kind == SGTC_BT_RINGS && rings && !e.rings_prev)
      {
         // the sending end's rings rise first (+3.3, the far ones at +5.8)
         float u = SGTC_BtSince(e.t_unk);
         if (u >= 0 && u < SGTC_BT_RT_FAR_RINGS - 0.6f)
            SGTC_BtCycleAt(i, i, e.t_unk, "its rings rose first", NULL);
         else if (!SGTC_BtCycleRecent(e, SGTC_BT_RT_REARM))
            SGTC_BtCycleAt(i, i, now - SGTC_BT_RT_RINGS, "rings rose, press not seen", NULL);
      }
      e.rings_prev = rings;

      if (e.kind == SGTC_BT_PADS)
      {
         // a press flips the button's texture frame (both ways for a toggle button)
         for (int k = 0; k < e.num_act; k++)
         {
            if (FNullEnt(e.act[k]) || e.act[k]->v.frame == e.frame_prev[k])
               continue;
            qboolean press = (e.act[k]->v.spawnflags & 32) || e.act[k]->v.frame > e.frame_prev[k];
            e.frame_prev[k] = e.act[k]->v.frame;
            float s = SGTC_BtSince(e.t_send);
            if (press && (s < 0 || s > 1.0f))
            {
               edict_t *who = SGTC_BtSomeoneAt(e);
               SGTC_BtCycleAt(i, i, now, "button", who);
            }
         }
         // +3 s later the pads point at the other side
         if (e.num_pads && !FNullEnt(e.pad[0]) && strcmp(STRING(e.pad[0]->v.target), e.pad_target0))
         {
            float s = SGTC_BtSince(e.t_send);
            if (s < 0 || s > SGTC_BT_PAD_OFF + 0.5f)
               SGTC_BtCycleAt(i, i, now - SGTC_BT_PAD_ON, "pads on, press not seen", NULL);
         }
         continue;
      }

      // rt, obelisk: a press points both activators at a missing entity until the cycle is over
      int state = SGTC_BtActState(e);
      if (e.armed_prev && state == 1 && !SGTC_BtCycleRecent(e, 1.0f))
      {
         edict_t *who;
         int src = SGTC_BtSender(i, &who);
         SGTC_BtCycleAt(i, src, now, "activator", who);
      }
      e.armed_prev = (state == 2);

      // an activator with no target at map start got one (sg_strike3's rt1:
      // both C4 zones blew, or someone came down from rt2)
      if (!e.linked && e.can_link && state != 0)
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: activator enabled (target \"%s\") t=%.1f\n", e.name, STRING(e.act[0]->v.target), now);
         SGTC_BtLink(i, TRUE);
      }
   }

   if (!sgtc_ringdebug)
      return;
   if (next_dbg > now + 1.0f)
      next_dbg = 0;
   if (next_dbg > now)
      return;
   next_dbg = now + 0.25f;
   char line[1024];
   int n = snprintf(line, sizeof(line), "[sgtc-stat] btdbg t=%.2f", now);
   qboolean any = FALSE;
   for (int i = 0; i < sgtc_num_bts && n < (int)sizeof(line) - 96; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      float s = SGTC_BtSince(e.t_send), r = SGTC_BtSince(e.t_recv), u = SGTC_BtSince(e.t_unk), lim = SGTC_BtRearm(e) + 2.0f;
      const char *what = (s >= 0 && s < lim) ? "send" : (r >= 0 && r < lim) ? "recv" : (u >= 0 && u < lim) ? "unk" : NULL;
      if (!what && !e.rings_prev)
         continue;
      any = TRUE;
      n += snprintf(line + n, sizeof(line) - n, " %s[%s+%.1f act=%d rings=%d]", e.name, what ? what : "-",
         what ? (what[0] == 's' ? s : what[0] == 'r' ? r : u) : 0.0f, SGTC_BtActState(e), e.rings_prev);
   }
   if (!any)
      return;
   for (int p = 1; p <= gpGlobals->maxClients && n < (int)sizeof(line) - 96; p++)
   {
      edict_t *pPlayer = INDEXENT(p);
      if (FNullEnt(pPlayer) || pPlayer->free || !IsAlive(pPlayer))
         continue;
      for (int i = 0; i < sgtc_num_bts && n < (int)sizeof(line) - 96; i++)
      {
         Vector d = pPlayer->v.origin - sgtc_bts[i].center;
         if (fabs(d.z) < 120.0f && d.Make2D().Length() < sgtc_bts[i].r_out + 100.0f)
         {
            bot_t *pOther = UTIL_GetBotPointer(pPlayer);
            n += snprintf(line + n, sizeof(line) - n, " %.12s@%s(r=%.0f dz=%.0f%s)", STRING(pPlayer->v.netname), sgtc_bts[i].name,
               d.Make2D().Length(), d.z, pOther && pOther->sgtc_bt ? (pOther->sgtc_bt_state == SGTC_BT_WAIT ? " wait" : " go") : "");
         }
      }
   }
   UTIL_ConsolePrintf("%s\n", line);
}

//
// arrivals
//
static qboolean SGTC_BtInCell(const Vector &o)
{
   for (int k = 0; k < sgtc_bt_num_cells; k++)
   {
      Vector d = o - sgtc_bt_cells[k];
      if (fabs(d.z) < 80.0f && d.Make2D().Length() < 64.0f)
         return TRUE;
   }
   return FALSE;
}

// the position jumped: a holding cell, an arrival at an end, or something else
static void SGTC_BtTeleported(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   const Vector &o = pEdict->v.origin;
   float now = gpGlobals->time;
   const char *name = STRING(pEdict->v.netname);
   int using_end = (pBot.sgtc_bt > 0 && pBot.sgtc_bt <= sgtc_num_bts) ? pBot.sgtc_bt - 1 : -1;

   if (SGTC_BtInCell(o))
   {
      if (using_end >= 0)
         pBot.sgtc_bt_state = SGTC_BT_HOLD;
      UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s in a holding cell at %.0f %.0f %.0f%s\n",
         using_end >= 0 ? sgtc_bts[using_end].name : "rt", name, o.x, o.y, o.z, using_end >= 0 ? "" : " (rode along)");
      return;
   }
   for (int x = 0; x < sgtc_num_bts; x++)
   {
      sgtc_bt_t &e = sgtc_bts[x];
      if (e.partner < 0)
         continue;
      Vector d = o - e.center;
      if (fabs(d.z) > 100.0f || d.Make2D().Length() > e.r_out + 80.0f)
         continue;
      sgtc_bt_t &f = sgtc_bts[e.partner];
      float sent = SGTC_BtSince(f.t_send);
      float when = f.kind == SGTC_BT_RINGS ? SGTC_BT_RT_ARRIVE : SGTC_BtDepart(f);
      if (using_end != e.partner && !(sent >= when - 1.0f && sent < when + 2.5f))
         continue;  // a respawn near it, not an arrival
      UTIL_ConsolePrintf("[sgtc-stat] transport %s -> %s: %s arrived at %.0f %.0f %.0f, %.1f s after the press%s\n",
         f.name, e.name, name, o.x, o.y, o.z, sent, using_end == e.partner ? "" : " (rode along)");
      pBot.sgtc_bt_arrived = x + 1;
      pBot.f_sgtc_bt_arrived_time = now;
      if (using_end >= 0)
      {
         pBot.sgtc_bt = 0;
         pBot.sgtc_bt_state = SGTC_BT_GO;
         pBot.sgtc_bt_spot = -1;
      }
      pBot.curr_waypoint_index = -1;
      return;
   }
   if (using_end >= 0)
   {
      UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s carried elsewhere (%.0f %.0f %.0f), stops using it\n",
         sgtc_bts[using_end].name, name, o.x, o.y, o.z);
      pBot.sgtc_bt = 0;
      pBot.sgtc_bt_state = SGTC_BT_GO;
      pBot.sgtc_bt_spot = -1;
   }
}

// just arrived inside another player (two on one spot) or a wall: the
// nearest free spot (inside the ring wall for rt)
static void SGTC_BtUnstick(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   TraceResult tr;
   int hull = (pEdict->v.flags & FL_DUCKING) ? head_hull : human_hull;
   UTIL_TraceHull(pEdict->v.origin, pEdict->v.origin + Vector(0, 0, 1), dont_ignore_monsters, hull, pEdict, &tr);
   if (!tr.fStartSolid)
      return;
   char what[40];
   safe_strcopy(what, sizeof(what), FNullEnt(tr.pHit) ? "the world" :
      (tr.pHit->v.flags & (FL_CLIENT | FL_FAKECLIENT)) ? STRING(tr.pHit->v.netname) : STRING(tr.pHit->v.classname));
   const sgtc_bt_t &e = sgtc_bts[pBot.sgtc_bt_arrived - 1];
   static const float dist[4] = {18.0f, 30.0f, 42.0f, 56.0f};
   for (int k = 0; k < 4; k++)
      for (int a = 0; a < 8; a++)
      {
         float ang = a * (float)(M_PI / 4.0);
         Vector p = pEdict->v.origin + Vector(cosf(ang) * dist[k], sinf(ang) * dist[k], 0);
         if (e.kind == SGTC_BT_RINGS && (p - e.center).Make2D().Length() > e.r_in - 22.0f)
            continue;
         for (float up = 0; up <= 12.0f; up += 6.0f)
         {
            Vector q = p + Vector(0, 0, up);
            UTIL_TraceHull(q, q + Vector(0, 0, 1), dont_ignore_monsters, hull, pEdict, &tr);
            if (tr.fStartSolid || tr.fAllSolid)
               continue;
            SET_ORIGIN(pEdict, q);
            pBot.v_sgtc_bt_last = q;
            UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s arrived inside %s, moved %.0f units\n", e.name,
               STRING(pEdict->v.netname), what, dist[k]);
            return;
         }
      }
}

//
// Every frame, every living bot (SGTC_StartFrame): get off a platform about
// to be cleared, keep still inside a ring wall while the rings move. The
// move is a world-space one (v_sgtc_move); SGTC_BotTransportThink keeps the
// navigation from moving the bot meanwhile.
//
static void SGTC_TransportSafety(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   const Vector &o = pEdict->v.origin;
   float now = gpGlobals->time;

   if (pBot.f_sgtc_bt_arrived_time > now + 1.0f || pBot.f_sgtc_bt_safe_until > now + 1.0f)  // the previous map's
      SGTC_TransportBotReset(pBot);
   if (!sgtc_num_bts)
   {
      pBot.v_sgtc_bt_last = o;
      return;
   }
   if (pBot.v_sgtc_bt_last != Vector(0, 0, 0) && (o - pBot.v_sgtc_bt_last).Length() > 250.0f)
      SGTC_BtTeleported(pBot);
   pBot.v_sgtc_bt_last = pEdict->v.origin;
   if (!sgtc_transport)
      return;

   float since_arrival = (pBot.sgtc_bt_arrived > 0 && pBot.sgtc_bt_arrived <= sgtc_num_bts) ? SGTC_BtSince(pBot.f_sgtc_bt_arrived_time) : -1.0f;
   if (since_arrival >= 0 && since_arrival < 2.0f)
      SGTC_BtUnstick(pBot);

   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      if (e.kind == SGTC_BT_PADS)
         continue;
      Vector d = o - e.center;
      if (fabs(d.z) > 100.0f)
         continue;
      d.z = 0;
      float r = d.Length();
      if (r > e.r_out + 150.0f)
         continue;

      // set down here by the cycle in question (not by an earlier one)
      qboolean here = pBot.sgtc_bt_arrived == i + 1 && since_arrival >= 0;
      qboolean arrived_recv = here && e.t_recv > 0 && pBot.f_sgtc_bt_arrived_time >= e.t_recv;
      qboolean arrived_unk = here && e.t_unk > 0 && pBot.f_sgtc_bt_arrived_time >= e.t_unk;
      qboolean departing = pBot.sgtc_bt == i + 1 && pBot.sgtc_bt_state == SGTC_BT_WAIT;
      float t_recv = SGTC_BtSince(e.t_recv), t_send = SGTC_BtSince(e.t_send), t_unk = SGTC_BtSince(e.t_unk);
      int act = 0;  // 1 stand still, 2 step out, 3 step in
      const char *why = NULL;

      if (e.kind == SGTC_BT_RINGS)
      {
         float inner = e.r_in - 20.0f, outer = e.r_out + 28.0f;
         int band = r < inner ? 1 : r < outer ? ((r - inner < outer - r) ? 3 : 2) : 0;  // inside: still; on the band: the nearer side
         if (t_recv >= 0 && t_recv < SGTC_BT_RT_ARRIVE + 0.5f && !arrived_recv)
         {
            // the far end: its rings rise at +5.8 and its hurt clears it at +7.8
            if (t_recv < SGTC_BT_RT_FAR_RINGS - 0.3f)
            {
               if (r < outer)
               {
                  act = 2;
                  why = "far end of a cycle";
               }
            }
            else
               act = band;
         }
         else if (t_send >= 0 && t_send < SGTC_BT_RT_DEPART + 0.5f)
         {
            // the sending end: those not leaving get off before the rings rise
            if (t_send < SGTC_BT_RT_RINGS - 0.4f)
            {
               if (!departing && r < outer)
               {
                  act = 2;
                  why = "sending end, not leaving";
               }
            }
            else
               act = band;
         }
         else if (t_unk >= 0 && t_unk < SGTC_BT_RT_ARRIVE + 0.5f && !arrived_unk)
         {
            if (t_unk < SGTC_BT_RT_RINGS - 0.4f)
            {
               if (!departing && r < outer)
               {
                  act = 2;
                  why = "a cycle from an unknown end";
               }
            }
            else
               act = band;
         }
         else if (SGTC_BtRingsActive(e))
            act = band;  // arrivals stay inside until the rings are back down
      }
      else if (e.kind == SGTC_BT_OBELISK && SGTC_BtOnPad(e, o, 24.0f))
      {
         // the other pad's press clears this pad at +3.6 s
         if (t_recv >= 0 && t_recv < SGTC_BT_OB_DEPART + 0.3f && !arrived_recv)
         {
            act = 2;
            why = "far pad of a cycle";
         }
         else if (t_unk >= 0 && t_unk < SGTC_BT_OB_DEPART + 0.3f && !arrived_unk)
         {
            act = 2;
            why = "a cycle from an unknown pad";
         }
      }
      if (!act)
         continue;

      Vector dir = d;
      if (e.kind == SGTC_BT_OBELISK && !FNullEnt(e.act[0]))
      {
         // away from the obelisk (its button), across the pad
         Vector mn, mx;
         SGTC_BtBox(e.act[0], mn, mx);
         dir = o - (mn + mx) * 0.5f;
         dir.z = 0;
      }
      if (dir.Length() < 1.0f)
         dir = (e.num_dep ? waypoints[e.w_dep[0]].origin : e.center + Vector(1, 0, 0)) - e.center;
      dir.z = 0;
      dir = dir.Length() > 0.1f ? dir.Normalize() : Vector(1, 0, 0);
      if (act == 2)
      {
         pBot.v_sgtc_move = dir * (pBot.f_max_speed > 150.0f ? pBot.f_max_speed : 250.0f);
         pBot.f_sgtc_move_until = now + 0.15f;
      }
      else if (act == 3)
      {
         pBot.v_sgtc_move = dir * -200.0f;
         pBot.f_sgtc_move_until = now + 0.15f;
      }
      else
         pBot.f_sgtc_move_until = 0;
      pBot.f_sgtc_bt_safe_until = now + 0.15f;
      pBot.sgtc_bt_safe_act = act;
      if (why && (pBot.f_sgtc_bt_safe_log > now || pBot.f_sgtc_bt_safe_log + 4.0f < now))
      {
         pBot.f_sgtc_bt_safe_log = now;
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s steps off (%s, %.0f units from the middle)\n", e.name, STRING(pEdict->v.netname), why, r);
      }
      return;
   }
}

//
// using one
//
static int SGTC_BtDepIndex(const sgtc_bt_t &e, int w)
{
   for (int k = 0; k < e.num_dep; k++)
      if (e.w_dep[k] == w)
         return k;
   return -1;
}

// an activator this side may use (anyone's first), -1 none
static int SGTC_BtActFor(const sgtc_bt_t &e, int team)
{
   int best = -1;
   for (int k = 0; k < e.num_act; k++)
   {
      if (FNullEnt(e.act[k]))
         continue;
      if (e.act_team[k] == 0)
         return k;
      if (e.act_team[k] == team && best < 0)
         best = k;
   }
   return best;
}

static void SGTC_BtStand(bot_t &pBot, qboolean stop_move)
{
   pBot.f_move_speed = 0;
   pBot.f_strafe_direction = 0;
   pBot.b_not_maxspeed = TRUE;  // or BotThinkFinalizeMovement scales it back up
   pBot.pEdict->v.button &= ~(IN_JUMP | IN_DUCK);
   if (stop_move)
      pBot.f_sgtc_move_until = 0;
}

// world-space move towards target (no turning first); TRUE when there
static qboolean SGTC_BtMoveTo(bot_t &pBot, const Vector &target, float tolerance)
{
   edict_t *pEdict = pBot.pEdict;
   float now = gpGlobals->time;
   Vector to = target - pEdict->v.origin;
   to.z = 0;
   float d = to.Length();
   SGTC_BtStand(pBot, FALSE);
   if (d <= tolerance)
   {
      pBot.f_sgtc_move_until = 0;
      return TRUE;
   }
   float speed = d > 80.0f ? (pBot.f_max_speed > 100.0f ? pBot.f_max_speed : 250.0f) : (d > 24.0f ? 150.0f : 80.0f);
   pBot.v_sgtc_move = to * (speed / d);
   pBot.f_sgtc_move_until = now + 0.15f;
   // not getting there (a teammate in the way): jump
   if (pBot.f_sgtc_bt_check > now + 5.0f)
      pBot.f_sgtc_bt_check = 0;
   if (pBot.f_sgtc_bt_check <= now)
   {
      if (pBot.f_sgtc_bt_check > 0 && d > 40.0f && (pEdict->v.origin - pBot.v_sgtc_bt_check_pos).Length() < 16.0f)
         pEdict->v.button |= IN_JUMP;
      pBot.f_sgtc_bt_check = now + 1.0f;
      pBot.v_sgtc_bt_check_pos = pEdict->v.origin;
   }
   return FALSE;
}

// a departure spot nobody stands on or has taken, the nearest
static int SGTC_BtFreeSpot(const sgtc_bt_t &e, bot_t &pBot)
{
   int best = -1;
   float best_d = 99999.0f;
   for (int k = 0; k < e.num_spots; k++)
   {
      qboolean taken = FALSE;
      for (int p = 1; p <= gpGlobals->maxClients && !taken; p++)
      {
         edict_t *pPlayer = INDEXENT(p);
         if (FNullEnt(pPlayer) || pPlayer == pBot.pEdict || pPlayer->free || !IsAlive(pPlayer))
            continue;
         Vector d = pPlayer->v.origin - e.spot[k];
         bot_t *pOther = UTIL_GetBotPointer(pPlayer);
         if ((fabs(d.z) < 60.0f && d.Make2D().Length() < 26.0f) ||
             (pOther && pOther->sgtc_bt == pBot.sgtc_bt && pOther->sgtc_bt_spot == k))
            taken = TRUE;
      }
      float d = (e.spot[k] - pBot.pEdict->v.origin).Length();
      if (!taken && d < best_d)
      {
         best_d = d;
         best = k;
      }
   }
   return best;
}

// nobody but pSelf where a press would hurt: rt, on this end's ring band
// (riders inside it are fine) or anywhere on the other platform; obelisk, on
// the other pad
static qboolean SGTC_BtClear(const sgtc_bt_t &e, edict_t *pSelf)
{
   if (e.kind == SGTC_BT_PADS)
      return TRUE;
   const sgtc_bt_t *p = e.partner >= 0 ? &sgtc_bts[e.partner] : NULL;
   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);
      if (FNullEnt(pPlayer) || pPlayer == pSelf || pPlayer->free || !IsAlive(pPlayer))
         continue;
      const Vector &o = pPlayer->v.origin;
      if (e.kind == SGTC_BT_RINGS)
      {
         Vector d = o - e.center;
         float r = d.Make2D().Length();
         if (fabs(d.z) < 100.0f && r >= e.r_in - 20.0f && r < e.r_out + 28.0f)
            return FALSE;
         if (p)
         {
            d = o - p->center;
            if (fabs(d.z) < 100.0f && d.Make2D().Length() < p->r_out + 28.0f)
               return FALSE;
         }
      }
      else if (p && SGTC_BtOnPad(*p, o, 24.0f))
         return FALSE;
   }
   return TRUE;
}

// where to wait for the activator: the press spot, the edge of a trigger
// platform (its departure waypoint), a telepo pad
static Vector SGTC_BtWaitPoint(bot_t &pBot, const sgtc_bt_t &e)
{
   if (e.kind == SGTC_BT_PADS)
   {
      if (pBot.sgtc_bt_spot < 0 || pBot.sgtc_bt_spot >= e.num_spots)
         pBot.sgtc_bt_spot = SGTC_BtFreeSpot(e, pBot);
      return e.spot[pBot.sgtc_bt_spot >= 0 ? pBot.sgtc_bt_spot : (int)(&pBot - bots) % e.num_spots];
   }
   if (e.act_touch && e.num_dep)
      return waypoints[e.w_dep[0]].origin;
   return e.press;
}

static qboolean SGTC_BtGiveUp(bot_t &pBot, const char *why, qboolean drop_goal)
{
   int i = pBot.sgtc_bt - 1;
   float now = gpGlobals->time;
   UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s gives up (%s) after %.0f s%s\n", (i >= 0 && i < sgtc_num_bts) ? sgtc_bts[i].name : "?",
      STRING(pBot.pEdict->v.netname), why, now - pBot.f_sgtc_bt_time, drop_goal ? ", goal dropped" : "");
   BotTrace(pBot, "sgtc transport: give up (%s)", why);
   pBot.sgtc_bt = 0;
   pBot.sgtc_bt_state = SGTC_BT_GO;
   pBot.sgtc_bt_spot = -1;
   if (drop_goal)
   {
      pBot.sgtc_bt_refuse = i + 1;
      pBot.f_sgtc_bt_refuse_until = now + 20.0f;
      pBot.waypoint_goal = -1;
      pBot.f_waypoint_goal_time = 0;
   }
   return FALSE;
}

// the route takes a forced path of an end: start using it
static qboolean SGTC_BtStart(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   float now = gpGlobals->time;
   int cur = pBot.curr_waypoint_index, goal = pBot.waypoint_goal;
   if (goal < 0 || cur < 0 || cur == goal || cur >= num_waypoints || pBot.pBotEnemy != NULL)
      return FALSE;
   const Vector &o = pEdict->v.origin;
   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      if (!e.linked || e.partner < 0)
         continue;
      sgtc_bt_t &p = sgtc_bts[e.partner];
      int k = SGTC_BtDepIndex(e, cur);
      if (k >= 0)
      {
         if (WaypointRouteFromTo(cur, goal) != p.w_arr)
            continue;
      }
      else
      {
         // already past its departure waypoint, heading over the forced path
         if (cur != p.w_arr || (o - p.center).Length() < 400.0f)
            continue;
         float best = 99999.0f;
         for (int j = 0; j < e.num_dep; j++)
         {
            float d = (o - waypoints[e.w_dep[j]].origin).Length();
            if (d < best)
            {
               best = d;
               k = j;
            }
         }
         if (best > 200.0f)
            continue;
      }
      // close enough to take over from the navigation
      if ((o - waypoints[e.w_dep[k]].origin).Length() > 160.0f && (o - e.center).Length() > e.r_out + 100.0f)
         continue;

      int team = SGTC_GetPlayerTeam(pEdict);
      int a = SGTC_BtActFor(e, team);
      const char *why = NULL;
      if (a < 0)
         why = "its activator is for the other side";
      else if (e.kind != SGTC_BT_PADS && SGTC_BtActState(e) == 0)
         why = "its activator is disabled";
      else if (pBot.sgtc_bt_refuse == i + 1 && pBot.f_sgtc_bt_refuse_until > now && pBot.f_sgtc_bt_refuse_until < now + 60.0f)
         why = "gave up on it lately";
      if (why)
      {
         if (pBot.f_sgtc_bt_safe_log > now || pBot.f_sgtc_bt_safe_log + 4.0f < now)
         {
            pBot.f_sgtc_bt_safe_log = now;
            UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s can't use it (%s), drops goal wpt %d\n", e.name, STRING(pEdict->v.netname), why, goal);
         }
         for (int x = EXCLUDE_POINTS_COUNT - 1; x > 0; x--)
            pBot.exclude_points[x] = pBot.exclude_points[x - 1];
         pBot.exclude_points[0] = goal;
         pBot.waypoint_goal = -1;
         pBot.f_waypoint_goal_time = 0;
         return FALSE;
      }
      pBot.sgtc_bt = i + 1;
      pBot.sgtc_bt_state = SGTC_BT_GO;
      pBot.sgtc_bt_spot = -1;
      pBot.sgtc_bt_act = a;
      pBot.sgtc_bt_tries = 0;
      pBot.f_sgtc_bt_time = now;
      pBot.f_sgtc_bt_wait = 0;
      pBot.f_sgtc_bt_t0 = 0;
      pBot.f_waypoint_goal_time = now + 120.0f;  // waiting for a cycle takes a while
      UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s uses it to reach wpt %d (from wpt %d)\n", e.name, STRING(pEdict->v.netname), goal, cur);
      BotTrace(pBot, "sgtc transport %s: use it to reach wpt=%d", e.name, goal);
      return TRUE;
   }
   return FALSE;
}

static qboolean SGTC_BtUse(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   float now = gpGlobals->time;
   int i = pBot.sgtc_bt - 1;
   if (i < 0 || i >= sgtc_num_bts)
   {
      pBot.sgtc_bt = 0;
      return FALSE;
   }
   sgtc_bt_t &e = sgtc_bts[i];
   const Vector &o = pEdict->v.origin;
   int idx = (int)(&pBot - bots);

   if (now - pBot.f_sgtc_bt_time > 45.0f)
      return SGTC_BtGiveUp(pBot, "took too long", TRUE);
   if (pBot.sgtc_bt_state == SGTC_BT_GO && pBot.pBotEnemy != NULL)
      return SGTC_BtGiveUp(pBot, "an enemy", FALSE);
   if ((o - e.center).Length() > e.r_out + 700.0f)
      return SGTC_BtGiveUp(pBot, "too far from it", TRUE);

   if (pBot.sgtc_bt_state == SGTC_BT_WAIT)
   {
      // the cycle it waits for: the one going on when it took its spot, or
      // the next one from this end (after its own press / step)
      if (pBot.f_sgtc_bt_t0 <= 0 && e.t_send > 0 && e.t_send >= pBot.f_sgtc_bt_wait - 0.5f && e.t_send <= now)
         pBot.f_sgtc_bt_t0 = e.t_send;
      float sent = pBot.f_sgtc_bt_t0 > 0 ? now - pBot.f_sgtc_bt_t0 : -1.0f;
      float limit = e.kind == SGTC_BT_PADS ? SGTC_BT_PAD_OFF + 0.5f : SGTC_BtDepart(e) + 1.5f;
      float pending = e.kind == SGTC_BT_PADS ? SGTC_BT_PAD_ON + 1.0f : 2.5f;
      if (sent >= 0 ? sent > limit : now - pBot.f_sgtc_bt_wait > pending)
      {
         if (++pBot.sgtc_bt_tries >= 3)
            return SGTC_BtGiveUp(pBot, "missed the departure 3 times", TRUE);
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s missed the departure (%s), tries again\n", e.name, STRING(pEdict->v.netname),
            sent >= 0 ? "no teleport" : "no cycle started");
         pBot.sgtc_bt_state = SGTC_BT_GO;
         pBot.sgtc_bt_spot = -1;
         pBot.f_sgtc_bt_wait = 0;
         return TRUE;
      }
      if (pBot.sgtc_bt_spot < 0 || pBot.sgtc_bt_spot >= e.num_spots)
         pBot.sgtc_bt_spot = 0;
      if (SGTC_BtMoveTo(pBot, e.spot[pBot.sgtc_bt_spot], 6.0f))
         SGTC_BtStand(pBot, TRUE);
      return TRUE;
   }
   if (pBot.sgtc_bt_state == SGTC_BT_HOLD)
   {
      SGTC_BtStand(pBot, TRUE);
      return TRUE;
   }

   // GO: a cycle from this end just started (its own press or a teammate's): take a spot if there is time
   float sent = SGTC_BtSince(e.t_send);
   if (sent >= 0 && sent < SGTC_BtRearm(e))
   {
      float limit = e.kind == SGTC_BT_RINGS ? SGTC_BT_RT_RINGS - 0.4f : e.kind == SGTC_BT_OBELISK ? SGTC_BT_OB_DEPART - 0.4f : SGTC_BT_PAD_ON - 0.2f;
      int k = SGTC_BtFreeSpot(e, pBot);
      if (k >= 0 && sent + (e.spot[k] - o).Make2D().Length() / (pBot.f_max_speed > 100.0f ? pBot.f_max_speed : 100.0f) + 0.3f < limit)
      {
         pBot.sgtc_bt_spot = k;
         pBot.sgtc_bt_state = SGTC_BT_WAIT;
         pBot.f_sgtc_bt_t0 = e.t_send;
         pBot.f_sgtc_bt_wait = now;
         BotTrace(pBot, "sgtc transport %s: spot %d for the departure at +%.1f", e.name, k, SGTC_BtDepart(e));
         SGTC_BtMoveTo(pBot, e.spot[k], 6.0f);
         return TRUE;
      }
   }

   int state = SGTC_BtActState(e);
   if (e.kind != SGTC_BT_PADS && state == 0)
      return SGTC_BtGiveUp(pBot, "its activator got disabled", TRUE);
   // busy: a cycle runs (rt, obelisk: either way; telepo: pads of this side sent lately)
   qboolean busy = e.kind == SGTC_BT_PADS ? (sent >= 0 && sent < SGTC_BT_PAD_OFF + 0.5f) : state != 2;
   Vector target = SGTC_BtWaitPoint(pBot, e);
   if (!SGTC_BtMoveTo(pBot, target, e.kind == SGTC_BT_PADS ? 6.0f : 12.0f))
   {
      pBot.f_sgtc_bt_wait = 0;
      return TRUE;
   }
   SGTC_BtStand(pBot, TRUE);
   if (busy)
   {
      pBot.f_sgtc_bt_wait = 0;  // a cycle it couldn't join: wait for the next one
      return TRUE;
   }
   if (pBot.f_sgtc_bt_wait <= 0 || pBot.f_sgtc_bt_wait > now)
      pBot.f_sgtc_bt_wait = now;
   qboolean clear = SGTC_BtClear(e, pEdict);
   if (!clear && now - pBot.f_sgtc_bt_wait < 10.0f)
      return TRUE;

   if (e.act_touch)
   {
      // an rt trigger_multiple on the platform: walking onto it is the press
      int k = SGTC_BtFreeSpot(e, pBot);
      if (k < 0)
         return TRUE;
      e.f_bot_press = now;
      e.bot_presser = idx;
      pBot.sgtc_bt_spot = k;
      pBot.sgtc_bt_state = SGTC_BT_WAIT;
      pBot.f_sgtc_bt_t0 = 0;
      pBot.f_sgtc_bt_wait = now;
      UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s steps onto the platform (%s) t=%.1f\n", e.name, STRING(pEdict->v.netname),
         clear ? "clear" : "forced", now);
      SGTC_BtMoveTo(pBot, e.spot[k], 6.0f);
      return TRUE;
   }

   // face the button, then +use (the key must come up between presses).
   // PlayerUse takes, within 64 units, the usable entity whose box lies the
   // most straight ahead (the eye-to-box vector, clamped to the box): aim at
   // the nearest point of the box, not its middle (sg_strike3's rt buttons
   // are 312-unit boxes; sg_target2a has a Goa'uld-only button above the
   // one for anyone)
   edict_t *pAct = e.act[pBot.sgtc_bt_act >= 0 && pBot.sgtc_bt_act < e.num_act ? pBot.sgtc_bt_act : 0];
   if (FNullEnt(pAct))
      return SGTC_BtGiveUp(pBot, "no button", TRUE);
   Vector mn, mx;
   SGTC_BtBox(pAct, mn, mx);
   Vector eye = GetGunPosition(pEdict);
   Vector aim(eye.x < mn.x ? mn.x : (eye.x > mx.x ? mx.x : eye.x), eye.y < mn.y ? mn.y : (eye.y > mx.y ? mx.y : eye.y),
              eye.z < mn.z ? mn.z : (eye.z > mx.z ? mx.z : eye.z));
   if ((aim - eye).Length() < 1.0f)
      aim = (mn + mx) * 0.5f;
   Vector angles = UTIL_VecToAngles(aim - eye);
   pEdict->v.ideal_yaw = angles.y;
   pEdict->v.idealpitch = -angles.x;
   pBot.f_sgtc_pitch_hold = gpGlobals->time + 0.3f;
   BotFixIdealYaw(pEdict);
   float dyaw = pEdict->v.v_angle.y - angles.y, dpitch = pEdict->v.v_angle.x + angles.x;
   while (dyaw > 180.0f) dyaw -= 360.0f;
   while (dyaw < -180.0f) dyaw += 360.0f;
   while (dpitch > 180.0f) dpitch -= 360.0f;
   while (dpitch < -180.0f) dpitch += 360.0f;
   if (pBot.f_sgtc_bt_next_use > now + 5.0f)
      pBot.f_sgtc_bt_next_use = 0;
   if (pBot.f_sgtc_bt_next_use > now || fabs(dyaw) > 25.0f || (fabs(dpitch) > 12.0f && now - pBot.f_sgtc_bt_wait < 4.0f))
      return TRUE;
   pBot.f_sgtc_bt_next_use = now + 1.2f;
   pEdict->v.button |= IN_USE;
   e.f_bot_press = now;
   e.bot_presser = idx;
   UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s presses \"%s\" (%s) t=%.1f\n", e.name, STRING(pEdict->v.netname),
      STRING(pAct->v.targetname)[0] ? STRING(pAct->v.targetname) : STRING(pAct->v.target), clear ? "clear" : "forced", now);
   BotTrace(pBot, "sgtc transport %s: press", e.name);
   if (e.kind == SGTC_BT_PADS)
   {
      // it stands on its pad already: wait there for +3 s
      pBot.sgtc_bt_state = SGTC_BT_WAIT;
      pBot.f_sgtc_bt_t0 = 0;
      pBot.f_sgtc_bt_wait = now;
   }
   return TRUE;
}

//
// Called from SGTC_BotObjectiveThink, after the navigation; TRUE while it
// controls the bot.
//
static qboolean SGTC_BotTransportThink(bot_t &pBot)
{
   edict_t *pEdict = pBot.pEdict;
   float now = gpGlobals->time;

   if (pBot.f_sgtc_bt_time > now + 1.0f)  // the previous map's
      SGTC_TransportBotReset(pBot);
   if (!sgtc_transport || !sgtc_num_bts)
   {
      pBot.sgtc_bt = 0;
      return FALSE;
   }

   // between the two hops of an rt trip: the cell sends it on at +8.3 s
   if (SGTC_BtInCell(pEdict->v.origin))
   {
      SGTC_BtStand(pBot, TRUE);
      return TRUE;
   }

   // the safety code is moving it (or keeping it still)
   if (pBot.f_sgtc_bt_safe_until > now)
   {
      if (pBot.sgtc_bt && pBot.sgtc_bt_state == SGTC_BT_WAIT && pBot.sgtc_bt_safe_act != 1)
      {
         UTIL_ConsolePrintf("[sgtc-stat] transport %s: %s off its spot, waits for the next cycle\n",
            pBot.sgtc_bt <= sgtc_num_bts ? sgtc_bts[pBot.sgtc_bt - 1].name : "?", STRING(pEdict->v.netname));
         pBot.sgtc_bt_state = SGTC_BT_GO;
         pBot.sgtc_bt_spot = -1;
         pBot.f_sgtc_bt_wait = 0;
      }
      SGTC_BtStand(pBot, FALSE);
      return TRUE;
   }

   if (!pBot.sgtc_bt && !SGTC_BtStart(pBot))
      return FALSE;
   return SGTC_BtUse(pBot);
}

// SGTC_SetGoal: a route over the forced path of an end this bot can't use
// (a button for the other side, an activator with no target, one it gave
// up on lately) counts as unreachable
static qboolean SGTC_TransportRefuses(bot_t &pBot, int goal)
{
   if (!sgtc_num_bts || goal < 0 || pBot.curr_waypoint_index < 0)
      return FALSE;
   int team = SGTC_GetPlayerTeam(pBot.pEdict);
   float now = gpGlobals->time;
   qboolean refused[SGTC_BT_MAX], any = FALSE;
   for (int i = 0; i < sgtc_num_bts; i++)
   {
      sgtc_bt_t &e = sgtc_bts[i];
      refused[i] = e.linked && e.partner >= 0 &&
                   (SGTC_BtActFor(e, team) < 0 || (e.kind != SGTC_BT_PADS && SGTC_BtActState(e) == 0) ||
                    (pBot.sgtc_bt_refuse == i + 1 && pBot.f_sgtc_bt_refuse_until > now && pBot.f_sgtc_bt_refuse_until < now + 60.0f));
      if (refused[i])
         any = TRUE;
   }
   if (!any)
      return FALSE;
   int w = pBot.curr_waypoint_index;
   for (int step = 0; step < 512 && w != goal; step++)
   {
      int next = WaypointRouteFromTo(w, goal);
      if (next < 0 || next == WAYPOINT_UNREACHABLE || next == w || next >= num_waypoints)
         break;
      for (int i = 0; i < sgtc_num_bts; i++)
         if (refused[i] && next == sgtc_bts[sgtc_bts[i].partner].w_arr && SGTC_BtDepIndex(sgtc_bts[i], w) >= 0)
         {
            BotTrace(pBot, "sgtc transport %s: route to wpt=%d refused", sgtc_bts[i].name, goal);
            return TRUE;
         }
      w = next;
   }
   return FALSE;
}
