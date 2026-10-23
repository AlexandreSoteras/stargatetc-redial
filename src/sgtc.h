//
// JK_Botti - be more human!
//
// sgtc.h - StargateTC (game dir "stargatetc") support
//

#ifndef SGTC_H
#define SGTC_H

// weapon id placeholder until the real id arrives in the WeaponList message
#define SGTC_UNRESOLVED_WEAPON_ID 0x7fff

#define SGTC_TEAM_NONE   0
#define SGTC_TEAM_TAURI  1
#define SGTC_TEAM_GOAULD 2
#define SGTC_TEAM_UNAS   3

extern bot_weapon_select_t sgtc_weapon_select[NUM_OF_WEAPON_SELECTS];
extern bot_fire_delay_t sgtc_fire_delay[NUM_OF_WEAPON_SELECTS];

void SGTC_OnWeaponList(const bot_weapon_t &weapon);
int SGTC_GetPlayerTeam(edict_t *pEdict);
const char *SGTC_TeamName(int team);
qboolean SGTC_BotJoinThink(bot_t &pBot);
void SGTC_BotSpawned(bot_t &pBot);
void SGTC_BotLoadoutThink(bot_t &pBot);
void SGTC_ClampSpeed(bot_t &pBot);
void SGTC_AvoidLedges(bot_t &pBot, const float *viewangles, float &forwardmove, float &sidemove);
void SGTC_LadderGuard(bot_t &pBot, unsigned short &buttons);
int SGTC_RouteNext(bot_t &pBot);
void SGTC_KeepRemovedPlayers(void);
qboolean SGTC_PathSafe(const Vector &a, const Vector &b);
void SGTC_DumpPlayers(void (*print)(const char *));

typedef struct
{
   qboolean key_taken;          // bomb key carried by someone
   float f_bomb_arming_time;    // > 0 while the bomb is being armed
   float f_bomb_armed_time;     // > 0 once the bomb is armed
   float f_round_start;
   int wins[3];                 // [0] draws, [1] Tau'ri, [2] Goa'uld
} sgtc_state_t;

extern sgtc_state_t sgtc_state;
void SGTC_BotClient_TextMsg(void *p, int bot_index);
void SGTC_BotClient_TextMsgEnd(void *p, int bot_index);
void SGTC_BotClient_Concuss(void *p, int bot_index);
void SGTC_BotClient_ConcussEnd(void *p, int bot_index);
void SGTC_OnSound(const char *sample);
void SGTC_RoundReset(void);
void SGTC_MapReset(void);
void SGTC_KeyValue(edict_t *pent, KeyValueData *pkvd);
void SGTC_TransitKeyValue(edict_t *pent, KeyValueData *pkvd);
void SGTC_StartFrame(void);
void SGTC_GuardVtables(const char *where);
void SGTC_DropRemovedWeapons(const char *where);
void SGTC_MapObjectivesReset(void);
qboolean SGTC_BotFindObjectiveGoal(bot_t &pBot);
void SGTC_BotObjectiveThink(bot_t &pBot);
qboolean SGTC_IsEnemySound(edict_t *pListener, edict_t *pSource);
qboolean SGTC_FaceLadder(edict_t *pEdict);

extern int sgtc_trace;
extern int sgtc_autojoin;
extern int sgtc_rejoin;
extern int sgtc_humanreset;
void SGTC_HumanPicksClass(edict_t *pEntity);
void SGTC_CheckCorpses(void);
extern int sgtc_loadout;
extern int sgtc_wptdump;
void SGTC_WptDumpCommand(void);
void SGTC_Probe(const Vector &a, const Vector &b);
extern int sgtc_dial;
extern int sgtc_defend_travel;
extern int sgtc_stats;
extern int sgtc_ringdebug;
extern int sgtc_transport;
qboolean SGTC_TransportOwns(edict_t *pent);
void SGTC_TraceBegin(int msg_type, edict_t *pBotEdict);
void SGTC_TraceValue(const char *str, int value);
void SGTC_TraceEnd(void);
void SGTC_TraceCommand(edict_t *pEdict, const char *a1, const char *a2, const char *a3);

#endif // SGTC_H
