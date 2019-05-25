/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//NPC_stats.cpp
#include "b_local.h"
#include "b_public.h"
#include "anims.h"
#include "ghoul2/G2.h"

extern qboolean NPCsPrecached;

extern qboolean WP_SaberParseParms(const char* SaberName, saberInfo_t* saber);
extern void WP_RemoveSaber(saberInfo_t* sabers, int saberNum);
void NPC_PrecacheByClassName(const char* type);

stringID_table_t TeamTable[] =
{
	ENUM2STRING(NPCTEAM_FREE),			// caution, some code checks a team_t via "if (!team_t_varname)" so I guess this should stay as entry 0, great or what? -slc
	ENUM2STRING(NPCTEAM_ENEMY),
	ENUM2STRING(NPCTEAM_PLAYER),
	ENUM2STRING(NPCTEAM_NEUTRAL),	// most droids are team_neutral, there are some exceptions like Probe,Seeker,Interrogator
	ENUM2STRING(NPCTEAM_SOLO),
	{"",	-1}
};

// this list was made using the model directories, this MUST be in the same order as the CLASS_ enum in teams.h
stringID_table_t ClassTable[] =
{
	ENUM2STRING(CLASS_NONE),				// hopefully this will never be used by an npc), just covering all bases
	ENUM2STRING(CLASS_ATST),				// technically droid...
	ENUM2STRING(CLASS_BARTENDER),
	ENUM2STRING(CLASS_BESPIN_COP),
	ENUM2STRING(CLASS_CLAW),
	ENUM2STRING(CLASS_COMMANDO),
	ENUM2STRING(CLASS_DESANN),
	ENUM2STRING(CLASS_FISH),
	ENUM2STRING(CLASS_FLIER2),
	ENUM2STRING(CLASS_GALAK),
	ENUM2STRING(CLASS_GLIDER),
	ENUM2STRING(CLASS_GONK),				// droid
	ENUM2STRING(CLASS_GRAN),
	ENUM2STRING(CLASS_HOWLER),
	ENUM2STRING(CLASS_RANCOR),
	ENUM2STRING(CLASS_SAND_CREATURE),
	ENUM2STRING(CLASS_WAMPA),
	ENUM2STRING(CLASS_IMPERIAL),
	ENUM2STRING(CLASS_IMPWORKER),
	ENUM2STRING(CLASS_INTERROGATOR),		// droid 
	ENUM2STRING(CLASS_JAN),
	ENUM2STRING(CLASS_JEDI),
	ENUM2STRING(CLASS_KYLE),
	ENUM2STRING(CLASS_LANDO),
	ENUM2STRING(CLASS_LIZARD),
	ENUM2STRING(CLASS_LUKE),
	ENUM2STRING(CLASS_MARK1),			// droid
	ENUM2STRING(CLASS_MARK2),			// droid
	ENUM2STRING(CLASS_GALAKMECH),		// droid
	ENUM2STRING(CLASS_MINEMONSTER),
	ENUM2STRING(CLASS_MONMOTHA),
	ENUM2STRING(CLASS_MORGANKATARN),
	ENUM2STRING(CLASS_MOUSE),			// droid
	ENUM2STRING(CLASS_MURJJ),
	ENUM2STRING(CLASS_PRISONER),
	ENUM2STRING(CLASS_PROBE),			// droid
	ENUM2STRING(CLASS_PROTOCOL),			// droid
	ENUM2STRING(CLASS_R2D2),				// droid
	ENUM2STRING(CLASS_R5D2),				// droid
	ENUM2STRING(CLASS_REBEL),
	ENUM2STRING(CLASS_REBORN),
	ENUM2STRING(CLASS_REELO),
	ENUM2STRING(CLASS_REMOTE),
	ENUM2STRING(CLASS_RODIAN),
	ENUM2STRING(CLASS_SEEKER),			// droid
	ENUM2STRING(CLASS_SENTRY),
	ENUM2STRING(CLASS_SHADOWTROOPER),
	ENUM2STRING(CLASS_SABOTEUR),
	ENUM2STRING(CLASS_STORMTROOPER),
	ENUM2STRING(CLASS_SWAMP),
	ENUM2STRING(CLASS_SWAMPTROOPER),
	ENUM2STRING(CLASS_NOGHRI),
	ENUM2STRING(CLASS_TAVION),
	ENUM2STRING(CLASS_ALORA),
	ENUM2STRING(CLASS_TRANDOSHAN),
	ENUM2STRING(CLASS_UGNAUGHT),
	ENUM2STRING(CLASS_JAWA),
	ENUM2STRING(CLASS_WEEQUAY),
	ENUM2STRING(CLASS_TUSKEN),
	ENUM2STRING(CLASS_BOBAFETT),
	ENUM2STRING(CLASS_ROCKETTROOPER),
	ENUM2STRING(CLASS_SABER_DROID),
	ENUM2STRING(CLASS_PLAYER),
	ENUM2STRING(CLASS_ASSASSIN_DROID),
	ENUM2STRING(CLASS_HAZARD_TROOPER),
	ENUM2STRING(CLASS_VEHICLE),
	ENUM2STRING(CLASS_SBD),
	ENUM2STRING(CLASS_BATTLEDROID),
	ENUM2STRING(CLASS_DROIDEKA),
	ENUM2STRING(CLASS_MANDO),
	ENUM2STRING(CLASS_WOOKIE),
	ENUM2STRING(CLASS_CLONETROOPER),
	ENUM2STRING(CLASS_STORMCOMMANDO),
	ENUM2STRING(CLASS_VADER),
	ENUM2STRING(CLASS_SITHLORD),
	{ "CLASS_GALAK_MECH", CLASS_GALAKMECH },
	{ "",	-1 }
};

stringID_table_t BSTable[] =
{
	ENUM2STRING(BS_DEFAULT),//# default behavior for that NPC
	ENUM2STRING(BS_ADVANCE_FIGHT),//# Advance to captureGoal and shoot enemies if you can
	ENUM2STRING(BS_SLEEP),//# Play awake script when startled by sound
	ENUM2STRING(BS_FOLLOW_LEADER),//# Follow your leader and shoot any enemies you come across
	ENUM2STRING(BS_JUMP),//# Face navgoal and jump to it.
	ENUM2STRING(BS_SEARCH),//# Using current waypoint as a base), search the immediate branches of waypoints for enemies
	ENUM2STRING(BS_WANDER),//# Wander down random waypoint paths
	ENUM2STRING(BS_NOCLIP),//# Moves through walls), etc.
	ENUM2STRING(BS_REMOVE),//# Waits for player to leave PVS then removes itself
	ENUM2STRING(BS_CINEMATIC),//# Does nothing but face it's angles and move to a goal if it has one
	ENUM2STRING(BS_FLEE),//# Run toward the nav goal, avoiding danger
	//the rest are internal only
	// ... but we should list them anyway, otherwise workshop behavior will screw up
	ENUM2STRING(BS_WAIT),
	ENUM2STRING(BS_STAND_GUARD),
	ENUM2STRING(BS_PATROL),
	ENUM2STRING(BS_INVESTIGATE),
	ENUM2STRING(BS_HUNT_AND_KILL),
	ENUM2STRING(BS_FOLLOW_OVERRIDE),//# Follow your leader and shoot any enemies you come across
	{"",				-1},
};

#define stringIDExpand(str, strEnum)	{str, strEnum}, ENUM2STRING(strEnum)

stringID_table_t BSETTable[] =
{
	ENUM2STRING(BSET_SPAWN),//# script to use when first spawned
	ENUM2STRING(BSET_USE),//# script to use when used
	ENUM2STRING(BSET_AWAKE),//# script to use when awoken/startled
	ENUM2STRING(BSET_ANGER),//# script to use when aquire an enemy
	ENUM2STRING(BSET_ATTACK),//# script to run when you attack
	ENUM2STRING(BSET_VICTORY),//# script to run when you kill someone
	ENUM2STRING(BSET_LOSTENEMY),//# script to run when you can't find your enemy
	ENUM2STRING(BSET_PAIN),//# script to use when take pain
	ENUM2STRING(BSET_FLEE),//# script to use when take pain below 50% of health
	ENUM2STRING(BSET_DEATH),//# script to use when killed
	ENUM2STRING(BSET_DELAYED),//# script to run when self->delayScriptTime is reached
	ENUM2STRING(BSET_BLOCKED),//# script to run when blocked by a friendly NPC or player
	ENUM2STRING(BSET_BUMPED),//# script to run when bumped into a friendly NPC or player (can set bumpRadius)
	ENUM2STRING(BSET_STUCK),//# script to run when blocked by a wall
	ENUM2STRING(BSET_FFIRE),//# script to run when player shoots their own teammates
	ENUM2STRING(BSET_FFDEATH),//# script to run when player kills a teammate
	stringIDExpand("", BSET_INVALID),
	{"",				-1},
};

extern stringID_table_t WPTable[];
extern stringID_table_t FPTable[];

char* TeamNames[TEAM_NUM_TEAMS] =
{
	"",
	"player",
	"enemy",
	"neutral"
};

// this list was made using the model directories, this MUST be in the same order as the CLASS_ enum in teams.h
char* ClassNames[CLASS_NUM_CLASSES] =
{
	"",				// class none
	"atst",
	"bartender",
	"bespin_cop",
	"claw",
	"commando",
	"desann",
	"fish",
	"flier2",
	"galak",
	"glider",
	"gonk",
	"gran",
	"howler",
	"imperial",
	"impworker",
	"interrogator",
	"jan",
	"jedi",
	"kyle",
	"lando",
	"lizard",
	"luke",
	"mark1",
	"mark2",
	"galak_mech",
	"minemonster",
	"monmotha",
	"morgankatarn",
	"mouse",
	"murjj",
	"prisoner",
	"probe",
	"protocol",
	"r2d2",
	"r5d2",
	"rebel",
	"reborn",
	"reelo",
	"remote",
	"rodian",
	"seeker",
	"sentry",
	"shadowtrooper",
	"stormtrooper",
	"swamp",
	"swamptrooper",
	"tavion",
	"alora",
	"trandoshan",
	"ugnaught",
	"weequay",
	"bobafett",
	"rockettrooper",
	"saberdroid",
	"assassindroid",
	"hazardtrooper",
	"vehicle",
	"rancor",
	"sandcreature",
	"wampa",
};


/*
NPC_ReactionTime
*/
//FIXME use grandom in here
int NPC_ReactionTime(void)
{
	return 200 * (6 - NPCS.NPCInfo->stats.reactions);
}

extern qboolean BG_ParseLiteral(const char** data, const char* string);

#define MAX_NPC_DATA_SIZE 0x100000
char	NPCParms[MAX_NPC_DATA_SIZE];

static rank_t TranslateRankName(const char* name)
{
	if (!Q_stricmp(name, "civilian"))
	{
		return RANK_CIVILIAN;
	}

	if (!Q_stricmp(name, "crewman"))
	{
		return RANK_CREWMAN;
	}

	if (!Q_stricmp(name, "ensign"))
	{
		return RANK_ENSIGN;
	}

	if (!Q_stricmp(name, "ltjg"))
	{
		return RANK_LT_JG;
	}

	if (!Q_stricmp(name, "lt"))
	{
		return RANK_LT;
	}

	if (!Q_stricmp(name, "ltcomm"))
	{
		return RANK_LT_COMM;
	}

	if (!Q_stricmp(name, "commander"))
	{
		return RANK_COMMANDER;
	}

	if (!Q_stricmp(name, "captain"))
	{
		return RANK_CAPTAIN;
	}

	return RANK_CIVILIAN;

}

static int MoveTypeNameToEnum(const char* name)
{
	if (!Q_stricmp("runjump", name))
	{
		return MT_RUNJUMP;
	}
	else if (!Q_stricmp("walk", name))
	{
		return EF_WALK;
	}
	else if (!Q_stricmp("flyswim", name))
	{
		return EF2_FLYING;
	}
	else if (!Q_stricmp("static", name))
	{
		return MT_STATIC;
	}

	return MT_STATIC;
}

extern saber_colors_t TranslateSaberColor(const char* name);


#ifdef CONVENIENT_ANIMATION_FILE_DEBUG_THING
void SpewDebugStuffToFile(animation_t * anims)
{
	char BGPAFtext[40000];
	fileHandle_t f;
	int i = 0;

	trap->FS_Open("file_of_debug_stuff_SP.txt", &f, FS_WRITE);

	if (!f)
	{
		return;
	}

	BGPAFtext[0] = 0;

	while (i < MAX_ANIMATIONS)
	{
		strcat(BGPAFtext, va("%i %i\n", i, anims[i].frameLerp));
		i++;
	}

	trap->FS_Write(BGPAFtext, strlen(BGPAFtext), f);
	trap->FS_Close(f);
}
#endif

qboolean G_ParseAnimFileSet(const char* filename, const char* animCFG, int* animFileIndex)
{
	*animFileIndex = BG_ParseAnimationFile(filename, NULL, qfalse);
	//if it's humanoid we should have it cached and return it, if it is not it will be loaded (unless it's also cached already)

	if (*animFileIndex == -1)
	{
		return qfalse;
	}
	return qtrue;
}

void NPC_PrecacheAnimationCFG(const char* NPC_type)
{
#if 0 
	char	filename[MAX_QPATH];
	const char* token;
	const char* value;
	const char* p;
	int		junk;

	if (!Q_stricmp("random", NPC_type))
	{//sorry, can't precache a random just yet
		return;
	}

	p = NPCParms;
	COM_BeginParseSession(NPCFile);

	// look for the right NPC
	while (p)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
			return;

		if (!Q_stricmp(token, NPC_type))
		{
			break;
		}

		SkipBracedSection(&p, 0);
	}

	if (!p)
	{
		return;
	}

	if (BG_ParseLiteral(&p, "{"))
	{
		return;
	}

	// parse the NPC info block
	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (!token[0])
		{
			Com_Printf(S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPC_type);
			return;
		}

		if (!Q_stricmp(token, "}"))
		{
			break;
		}

		// legsmodel
		if (!Q_stricmp(token, "legsmodel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			//must copy data out of this pointer into a different part of memory because the funcs we're about to call will call COM_ParseExt
			Q_strncpyz(filename, value, sizeof(filename));
			G_ParseAnimFileSet(filename, filename, &junk);
			return;
		}

		// playerModel
		if (!Q_stricmp(token, "playerModel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
		}
	}
#endif
}

extern int NPC_WeaponsForTeam(team_t team, int spawnflags, const char* NPC_type);
void NPC_PrecacheWeapons(team_t playerTeam, int spawnflags, char* NPCtype)
{
	int weapons = NPC_WeaponsForTeam(playerTeam, spawnflags, NPCtype);
	int curWeap;

	for (curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++)
	{
		if (weapons & (1 << curWeap))
		{
			RegisterItem(BG_FindItemForWeapon((weapon_t)curWeap));
		}
	}

#if 0 
	int weapons = NPC_WeaponsForTeam(playerTeam, spawnflags, NPCtype);
	gitem_t* item;
	for (int curWeap = WP_SABER; curWeap < WP_NUM_WEAPONS; curWeap++)
	{
		if ((weapons & (1 << curWeap)))
		{
			item = FindItemForWeapon(((weapon_t)(curWeap)));	//precache the weapon
			CG_RegisterItemSounds((item - bg_itemlist));
			CG_RegisterItemVisuals((item - bg_itemlist));
			//precache the in-hand/in-world ghoul2 weapon model

			char weaponModel[64];

			strcpy(weaponModel, weaponData[curWeap].weaponMdl);
			if (char* spot = strstr(weaponModel, ".md3")) {
				*spot = 0;
				spot = strstr(weaponModel, "_w");//i'm using the in view weapon array instead of scanning the item list, so put the _w back on
				if (!spot) {
					strcat(weaponModel, "_w");
				}
				strcat(weaponModel, ".glm");	//and change to ghoul2
			}
			trap->G2API_PrecacheGhoul2Model(weaponModel); // correct way is item->world_model
		}
	}
#endif
}

void NPC_Precache(gentity_t * spawner)
{
	npcteam_t			playerTeam = NPCTEAM_FREE;
	const char* token;
	const char* value;
	const char* p;
	char* patch;
	char	sound[MAX_QPATH];
	qboolean	md3Model = qfalse;
	char	playerModel[MAX_QPATH];
	char	customSkin[MAX_QPATH];
	char	sessionName[MAX_QPATH + 15];

	if (!Q_stricmp("random", spawner->NPC_type))
	{//sorry, can't precache a random just yet
		return;
	}
	strcpy(customSkin, "default");

	p = NPCParms;
	Com_sprintf(sessionName, sizeof(sessionName), "NPC_Precache(%s)", spawner->NPC_type);
	COM_BeginParseSession(sessionName);

	// look for the right NPC
	while (p)
	{
		token = COM_ParseExt(&p, qtrue);
		if (token[0] == 0)
			return;

		if (!Q_stricmp(token, spawner->NPC_type))
		{
			break;
		}

		SkipBracedSection(&p, 0);
	}

	if (!p)
	{
		return;
	}

	if (BG_ParseLiteral(&p, "{"))
	{
		return;
	}

	// parse the NPC info block
	while (1)
	{
		token = COM_ParseExt(&p, qtrue);
		if (!token[0])
		{
			Com_Printf(S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", spawner->NPC_type);
			return;
		}

		if (!Q_stricmp(token, "}"))
		{
			break;
		}

		// headmodel
		if (!Q_stricmp(token, "headmodel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}

			if (!Q_stricmp("none", value))
			{
			}
			else
			{
				//Q_strncpyz( ri.headModelName, value, sizeof(ri.headModelName), qtrue);
			}
			md3Model = qtrue;
			continue;
		}

		// torsomodel
		if (!Q_stricmp(token, "torsomodel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}

			if (!Q_stricmp("none", value))
			{
			}
			else
			{
				//Q_strncpyz( ri.torsoModelName, value, sizeof(ri.torsoModelName), qtrue);
			}
			md3Model = qtrue;
			continue;
		}

		// legsmodel
		if (!Q_stricmp(token, "legsmodel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			Q_strncpyz(playerModel, value, sizeof(playerModel));
			md3Model = qtrue;
			continue;
		}

		// playerModel
		if (!Q_stricmp(token, "playerModel"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			Q_strncpyz(playerModel, value, sizeof(playerModel));
			md3Model = qfalse;
			continue;
		}

		// customSkin
		if (!Q_stricmp(token, "customSkin"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			Q_strncpyz(customSkin, value, sizeof(customSkin));
			continue;
		}

		// playerTeam
		if (!Q_stricmp(token, "playerTeam"))
		{
			char tk[4096];

			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			Com_sprintf(tk, sizeof(tk), "NPC%s", token);
			playerTeam = (team_t)GetIDForString(TeamTable, tk);
			continue;
		}


		// snd
		if (!Q_stricmp(token, "snd"))
		{
			if (COM_ParseString(&p, &value)) {
				continue;
			}
			if (!(spawner->r.svFlags& SVF_NO_BASIC_SOUNDS))
			{
				Q_strncpyz(sound, value, sizeof(sound));
				patch = strstr(sound, "/");
				if (patch)
				{
					*patch = 0;
				}
				spawner->s.csSounds_Std = G_SoundIndex(va("*$%s", sound));
			}
			continue;
		}

		// sndcombat
		if (!Q_stricmp(token, "sndcombat"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			if (!(spawner->r.svFlags& SVF_NO_COMBAT_SOUNDS))
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz(sound, value, sizeof(sound));
				patch = strstr(sound, "/");
				if (patch)
				{
					*patch = 0;
				}
				spawner->s.csSounds_Combat = G_SoundIndex(va("*$%s", sound));
			}
			continue;
		}

		// sndextra
		if (!Q_stricmp(token, "sndextra")) {
			if (COM_ParseString(&p, &value)) {
				continue;
			}
			if (!(spawner->r.svFlags& SVF_NO_EXTRA_SOUNDS))
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz(sound, value, sizeof(sound));
				patch = strstr(sound, "/");
				if (patch)
				{
					*patch = 0;
				}
				spawner->s.csSounds_Extra = G_SoundIndex(va("*$%s", sound));
			}
			continue;
		}

		// sndjedi
		if (!Q_stricmp(token, "sndjedi"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			if (!(spawner->r.svFlags& SVF_NO_EXTRA_SOUNDS))
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz(sound, value, sizeof(sound));
				patch = strstr(sound, "/");
				if (patch)
				{
					*patch = 0;
				}
				spawner->s.csSounds_Jedi = G_SoundIndex(va("*$%s", sound));
			}
			continue;
		}

		// sndcallout
		if (!Q_stricmp(token, "sndcallout"))
		{
			if (COM_ParseString(&p, &value))
			{
				continue;
			}
			if (!(spawner->r.svFlags& SVF_NO_EXTRA_SOUNDS))
			{
				//FIXME: store this in some sound field or parse in the soundTable like the animTable...
				Q_strncpyz(sound, value, sizeof(sound));
				patch = strstr(sound, "/");
				if (patch)
				{
					*patch = 0;
				}
				spawner->s.csSounds_Custom = G_SoundIndex(va("*$%s", sound));
			}
			continue;
		}

		if (!Q_stricmp(token, "weapon"))
		{
			int curWeap;

			if (COM_ParseString(&p, &value))
			{
				continue;
			}

			curWeap = GetIDForString(WPTable, value);

			if (curWeap > WP_NONE&& curWeap < WP_NUM_WEAPONS)
			{
				RegisterItem(BG_FindItemForWeapon((weapon_t)curWeap));
			}
			continue;
		}
	}

	// If we're not a vehicle, then an error here would be valid...
	if (!spawner->client || spawner->client->NPC_class != CLASS_VEHICLE)
	{
		//adding in md3 NPC support
		char modelName[MAX_QPATH];

		if (md3Model)
		{
			//Com_sprintf(modelName, sizeof(modelName), "models/players/%s/lower.md3", playerModel);
		}
		else
		{ //if we have a model/skin then index them so they'll be registered immediately
		  //when the client gets a configstring update.
			Com_sprintf(modelName, sizeof(modelName), "models/players/%s/model.glm", playerModel);
			if (customSkin[0])
			{ //append it after a *
				strcat(modelName, va("*%s", customSkin));
			}
			G_ModelIndex(modelName);
		}
	}

	//precache this NPC's possible weapons
	NPC_PrecacheWeapons((team_t)playerTeam, spawner->spawnflags, spawner->NPC_type);

	// Anything else special about them
	NPC_PrecacheByClassName(spawner->NPC_type);
}

void NPC_BuildRandom(gentity_t * NPC)
{
}

#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100
extern void SetupGameGhoul2Model(gentity_t * ent, char* modelname, char* skinName);
qboolean NPC_ParseParms(const char* NPCName, gentity_t * NPC)
{
	const char* token;
	const char* value;
	const char* p;
	int		n;
	float	f;
	char* patch;
	char	sound[MAX_QPATH];
	char	playerModel[MAX_QPATH];
	char	customSkin[MAX_QPATH];
	char	sessionName[MAX_QPATH + 17];
	renderInfo_t* ri = &NPC->client->renderInfo;
	gNPCstats_t* stats = NULL;
	qboolean	md3Model = qtrue;
	char	surfOff[1024];
	char	surfOn[1024];
	qboolean parsingPlayer = qfalse;
	vec3_t playerMins;
	vec3_t playerMaxs;
	int npcSaber1 = 0;
	int npcSaber2 = 0;

	VectorSet(playerMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(playerMaxs, 15, 15, DEFAULT_MAXS_2);

	strcpy(customSkin, "default");
	if (!NPCName || !NPCName[0])
	{
		NPCName = "Player";
	}

	if (!NPC->s.number && NPC->client != NULL)
	{//player, only want certain data
		parsingPlayer = qtrue;
	}

	if (NPC->NPC)
	{
		stats = &NPC->NPC->stats;
		// fill in defaults
		stats->aggression = 5;
		stats->aim = 5;
		stats->earshot = 2048;
		stats->evasion = 5;
		stats->hfov = 140;
		stats->intelligence = 5;
		stats->move = 5;
		stats->reactions = 5;
		stats->vfov = 120;
		stats->vigilance = 0.1f;
		stats->visrange = 2048;
		stats->health = 0;
		stats->yawSpeed = 190;
		stats->walkSpeed = 90;
		stats->runSpeed = 300;
		stats->acceleration = 25;//Increase/descrease speed this much per frame (20fps)
	}
	else
	{
		stats = NULL;
	}
	memset((char*)surfOff, 0, sizeof(surfOff));
	memset((char*)surfOn, 0, sizeof(surfOn));

	ri->headYawRangeLeft = 80;
	ri->headYawRangeRight = 80;
	ri->headPitchRangeUp = 45;
	ri->headPitchRangeDown = 45;
	ri->torsoYawRangeLeft = 60;
	ri->torsoYawRangeRight = 60;
	ri->torsoPitchRangeUp = 30;
	ri->torsoPitchRangeDown = 50;

	VectorCopy(playerMins, NPC->r.mins);
	VectorCopy(playerMaxs, NPC->r.maxs);
	NPC->client->ps.crouchheight = CROUCH_MAXS_2;
	NPC->client->ps.standheight = DEFAULT_MAXS_2;

	NPC->client->dismemberProbHead = 100;
	NPC->client->dismemberProbArms = 100;
	NPC->client->dismemberProbHands = 100;
	NPC->client->dismemberProbWaist = 100;
	NPC->client->dismemberProbLegs = 100;

	NPC->client->ps.customRGBA[0] = 255;
	NPC->client->ps.customRGBA[1] = 255;
	NPC->client->ps.customRGBA[2] = 255;
	NPC->client->ps.customRGBA[3] = 255;

	if (!Q_stricmp("random", NPCName))
	{//Randomly assemble a starfleet guy
		NPC_BuildRandom(NPCS.NPC);
	}
	else
	{
		int fp;

		p = NPCParms;
		Com_sprintf(sessionName, sizeof(sessionName), "NPC_ParseParms(%s)", NPCName);
		COM_BeginParseSession(sessionName);

		// look for the right NPC
		while (p)
		{
			token = COM_ParseExt(&p, qtrue);
			if (token[0] == 0)
			{
				return qfalse;
			}

			if (!Q_stricmp(token, NPCName))
			{
				break;
			}

			SkipBracedSection(&p, 0);
		}
		if (!p)
		{
			return qfalse;
		}

		if (BG_ParseLiteral(&p, "{"))
		{
			return qfalse;
		}

		// parse the NPC info block
		while (1)
		{
			token = COM_ParseExt(&p, qtrue);
			if (!token[0])
			{
				Com_Printf(S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", NPCName);
				return qfalse;
			}

			if (!Q_stricmp(token, "}"))
			{
				break;
			}
			//===MODEL PROPERTIES===========================================================
					// custom color
			if (!Q_stricmp(token, "customRGBA"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!Q_stricmp(value, "random"))
				{
					NPC->client->ps.customRGBA[0] = Q_irand(0, 255);
					NPC->client->ps.customRGBA[1] = Q_irand(0, 255);
					NPC->client->ps.customRGBA[2] = Q_irand(0, 255);
					NPC->client->ps.customRGBA[3] = 255;
				}
				else if (!Q_stricmp(value, "random1"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 5))
					{
					default:
					case 0:
						NPC->client->ps.customRGBA[0] = 127;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1:
						NPC->client->ps.customRGBA[0] = 177;
						NPC->client->ps.customRGBA[1] = 29;
						NPC->client->ps.customRGBA[2] = 13;
						break;
					case 2:
						NPC->client->ps.customRGBA[0] = 47;
						NPC->client->ps.customRGBA[1] = 90;
						NPC->client->ps.customRGBA[2] = 40;
						break;
					case 3:
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 4:
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 5:
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 199;
						NPC->client->ps.customRGBA[2] = 14;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_hf"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 7))
					{
					default:
					case 0://red1
						NPC->client->ps.customRGBA[0] = 165;
						NPC->client->ps.customRGBA[1] = 48;
						NPC->client->ps.customRGBA[2] = 21;
						break;
					case 1://yellow1
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 230;
						NPC->client->ps.customRGBA[2] = 132;
						break;
					case 2://bluegray
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 3://pink
						NPC->client->ps.customRGBA[0] = 233;
						NPC->client->ps.customRGBA[1] = 183;
						NPC->client->ps.customRGBA[2] = 208;
						break;
					case 4://lt blue
						NPC->client->ps.customRGBA[0] = 161;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 240;
						break;
					case 5://blue
						NPC->client->ps.customRGBA[0] = 101;
						NPC->client->ps.customRGBA[1] = 159;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://orange
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 157;
						NPC->client->ps.customRGBA[2] = 114;
						break;
					case 7://violet
						NPC->client->ps.customRGBA[0] = 216;
						NPC->client->ps.customRGBA[1] = 160;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_hm"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 7))
					{
					default:
					case 0://yellow
						NPC->client->ps.customRGBA[0] = 252;
						NPC->client->ps.customRGBA[1] = 243;
						NPC->client->ps.customRGBA[2] = 180;
						break;
					case 1://blue
						NPC->client->ps.customRGBA[0] = 69;
						NPC->client->ps.customRGBA[1] = 109;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 2://gold
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 197;
						NPC->client->ps.customRGBA[2] = 73;
						break;
					case 3://orange
						NPC->client->ps.customRGBA[0] = 178;
						NPC->client->ps.customRGBA[1] = 78;
						NPC->client->ps.customRGBA[2] = 18;
						break;
					case 4://bluegreen
						NPC->client->ps.customRGBA[0] = 112;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 161;
						break;
					case 5://blue2
						NPC->client->ps.customRGBA[0] = 123;
						NPC->client->ps.customRGBA[1] = 182;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://green2
						NPC->client->ps.customRGBA[0] = 0;
						NPC->client->ps.customRGBA[1] = 88;
						NPC->client->ps.customRGBA[2] = 105;
						break;
					case 7://violet
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 0;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_kdm"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 8))
					{
					default:
					case 0://blue
						NPC->client->ps.customRGBA[0] = 85;
						NPC->client->ps.customRGBA[1] = 120;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1://violet
						NPC->client->ps.customRGBA[0] = 173;
						NPC->client->ps.customRGBA[1] = 142;
						NPC->client->ps.customRGBA[2] = 219;
						break;
					case 2://brown1
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 197;
						NPC->client->ps.customRGBA[2] = 73;
						break;
					case 3://orange
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 4://gold
						NPC->client->ps.customRGBA[0] = 254;
						NPC->client->ps.customRGBA[1] = 199;
						NPC->client->ps.customRGBA[2] = 14;
						break;
					case 5://blue2
						NPC->client->ps.customRGBA[0] = 68;
						NPC->client->ps.customRGBA[1] = 194;
						NPC->client->ps.customRGBA[2] = 217;
						break;
					case 6://red1
						NPC->client->ps.customRGBA[0] = 170;
						NPC->client->ps.customRGBA[1] = 3;
						NPC->client->ps.customRGBA[2] = 30;
						break;
					case 7://yellow1
						NPC->client->ps.customRGBA[0] = 225;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 144;
						break;
					case 8://violet2
						NPC->client->ps.customRGBA[0] = 167;
						NPC->client->ps.customRGBA[1] = 202;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_rm"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 8))
					{
					default:
					case 0://blue
						NPC->client->ps.customRGBA[0] = 127;
						NPC->client->ps.customRGBA[1] = 153;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 1://green1
						NPC->client->ps.customRGBA[0] = 208;
						NPC->client->ps.customRGBA[1] = 249;
						NPC->client->ps.customRGBA[2] = 85;
						break;
					case 2://blue2
						NPC->client->ps.customRGBA[0] = 181;
						NPC->client->ps.customRGBA[1] = 207;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 3://gold
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 83;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					case 4://gold
						NPC->client->ps.customRGBA[0] = 224;
						NPC->client->ps.customRGBA[1] = 171;
						NPC->client->ps.customRGBA[2] = 44;
						break;
					case 5://green2
						NPC->client->ps.customRGBA[0] = 49;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 131;
						break;
					case 6://red1
						NPC->client->ps.customRGBA[0] = 163;
						NPC->client->ps.customRGBA[1] = 79;
						NPC->client->ps.customRGBA[2] = 17;
						break;
					case 7://violet2
						NPC->client->ps.customRGBA[0] = 148;
						NPC->client->ps.customRGBA[1] = 104;
						NPC->client->ps.customRGBA[2] = 228;
						break;
					case 8://green3
						NPC->client->ps.customRGBA[0] = 138;
						NPC->client->ps.customRGBA[1] = 136;
						NPC->client->ps.customRGBA[2] = 0;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_tf"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 5))
					{
					default:
					case 0://green1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 235;
						NPC->client->ps.customRGBA[2] = 100;
						break;
					case 1://blue1
						NPC->client->ps.customRGBA[0] = 62;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 2://red1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 110;
						NPC->client->ps.customRGBA[2] = 120;
						break;
					case 3://purple
						NPC->client->ps.customRGBA[0] = 180;
						NPC->client->ps.customRGBA[1] = 150;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 4://flesh
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 200;
						NPC->client->ps.customRGBA[2] = 212;
						break;
					case 5://base
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 255;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					}
				}
				else if (!Q_stricmp(value, "jedi_zf"))
				{
					NPC->client->ps.customRGBA[3] = 255;
					switch (Q_irand(0, 7))
					{
					default:
					case 0://red1
						NPC->client->ps.customRGBA[0] = 204;
						NPC->client->ps.customRGBA[1] = 19;
						NPC->client->ps.customRGBA[2] = 21;
						break;
					case 1://orange1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 107;
						NPC->client->ps.customRGBA[2] = 40;
						break;
					case 2://pink1
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 148;
						NPC->client->ps.customRGBA[2] = 155;
						break;
					case 3://gold
						NPC->client->ps.customRGBA[0] = 255;
						NPC->client->ps.customRGBA[1] = 164;
						NPC->client->ps.customRGBA[2] = 59;
						break;
					case 4://violet1
						NPC->client->ps.customRGBA[0] = 216;
						NPC->client->ps.customRGBA[1] = 160;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 5://blue1
						NPC->client->ps.customRGBA[0] = 101;
						NPC->client->ps.customRGBA[1] = 159;
						NPC->client->ps.customRGBA[2] = 255;
						break;
					case 6://blue2
						NPC->client->ps.customRGBA[0] = 161;
						NPC->client->ps.customRGBA[1] = 226;
						NPC->client->ps.customRGBA[2] = 240;
						break;
					case 7://blue3
						NPC->client->ps.customRGBA[0] = 37;
						NPC->client->ps.customRGBA[1] = 155;
						NPC->client->ps.customRGBA[2] = 181;
						break;
					}
				}
				else
				{
					NPC->client->ps.customRGBA[0] = atoi(value);

					if (COM_ParseInt(&p, &n))
					{
						continue;
					}
					NPC->client->ps.customRGBA[1] = n;

					if (COM_ParseInt(&p, &n))
					{
						continue;
					}
					NPC->client->ps.customRGBA[2] = n;

					if (COM_ParseInt(&p, &n))
					{
						continue;
					}
					NPC->client->ps.customRGBA[3] = n;
				}
				continue;
			}

			// headmodel
			if (!Q_stricmp(token, "headmodel"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}

				if (!Q_stricmp("none", value))
				{
					//Zero the head clamp range so the torso & legs don't lag behind
					ri->headYawRangeLeft =
						ri->headYawRangeRight =
						ri->headPitchRangeUp =
						ri->headPitchRangeDown = 0;
				}
				continue;
			}

			// torsomodel
			if (!Q_stricmp(token, "torsomodel"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}

				if (!Q_stricmp("none", value))
				{
					//Zero the torso clamp range so the legs don't lag behind
					ri->torsoYawRangeLeft =
						ri->torsoYawRangeRight =
						ri->torsoPitchRangeUp =
						ri->torsoPitchRangeDown = 0;
				}
				continue;
			}

			// legsmodel
			if (!Q_stricmp(token, "legsmodel"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				//md3 models use their legsmodel as their player model
				Q_strncpyz(playerModel, value, sizeof(playerModel));
				continue;
			}

			// playerModel
			if (!Q_stricmp(token, "playerModel"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				Q_strncpyz(playerModel, value, sizeof(playerModel));
				md3Model = qfalse;
				continue;
			}

			// customSkin
			if (!Q_stricmp(token, "customSkin"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				Q_strncpyz(customSkin, value, sizeof(customSkin));
				continue;
			}

			// surfOff
			if (!Q_stricmp(token, "surfOff"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (surfOff[0])
				{
					Q_strcat((char*)surfOff, sizeof(surfOff), ",");
					Q_strcat((char*)surfOff, sizeof(surfOff), value);
				}
				else
				{
					Q_strncpyz(surfOff, value, sizeof(surfOff));
				}
				continue;
			}

			// surfOn
			if (!Q_stricmp(token, "surfOn"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (surfOn[0])
				{
					Q_strcat((char*)surfOn, sizeof(surfOn), ",");
					Q_strcat((char*)surfOn, sizeof(surfOn), value);
				}
				else
				{
					Q_strncpyz(surfOn, value, sizeof(surfOn));
				}
				continue;
			}

			//headYawRangeLeft
			if (!Q_stricmp(token, "headYawRangeLeft"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->headYawRangeLeft = n;
				continue;
			}

			//headYawRangeRight
			if (!Q_stricmp(token, "headYawRangeRight"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->headYawRangeRight = n;
				continue;
			}

			//headPitchRangeUp
			if (!Q_stricmp(token, "headPitchRangeUp"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->headPitchRangeUp = n;
				continue;
			}

			//headPitchRangeDown
			if (!Q_stricmp(token, "headPitchRangeDown"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->headPitchRangeDown = n;
				continue;
			}

			//torsoYawRangeLeft
			if (!Q_stricmp(token, "torsoYawRangeLeft"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->torsoYawRangeLeft = n;
				continue;
			}

			//torsoYawRangeRight
			if (!Q_stricmp(token, "torsoYawRangeRight"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->torsoYawRangeRight = n;
				continue;
			}

			//torsoPitchRangeUp
			if (!Q_stricmp(token, "torsoPitchRangeUp"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->torsoPitchRangeUp = n;
				continue;
			}

			//torsoPitchRangeDown
			if (!Q_stricmp(token, "torsoPitchRangeDown"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				ri->torsoPitchRangeDown = n;
				continue;
			}

			// Uniform XYZ scale
			if (!Q_stricmp(token, "scale"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (n != 100)
				{
					NPC->client->ps.iModelScale = n; //so the client knows
					if (n >= 1024)
					{
						Com_Printf("WARNING: MP does not support scaling up to or over 1024%\n");
						n = 1023;
					}

					NPC->modelScale[0] = NPC->modelScale[1] = NPC->modelScale[2] = n / 100.0f;
				}
				continue;
			}

			//X scale
			if (!Q_stricmp(token, "scaleX"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
					//NPC->s.modelScale[0] = n/100.0f;
				}
				continue;
			}

			//Y scale
			if (!Q_stricmp(token, "scaleY"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
					//NPC->s.modelScale[1] = n/100.0f;
				}
				continue;
			}

			//Z scale
			if (!Q_stricmp(token, "scaleZ"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (n != 100)
				{
					Com_Printf("MP doesn't support xyz scaling, use 'scale'.\n");
					//	NPC->s.modelScale[2] = n/100.0f;
				}
				continue;
			}

			//===AI STATS=====================================================================
			if (!parsingPlayer)
			{
				// aggression
				if (!Q_stricmp(token, "aggression")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->aggression = n;
					}
					continue;
				}

				// aim
				if (!Q_stricmp(token, "aim")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->aim = n;
					}
					continue;
				}

				// earshot
				if (!Q_stricmp(token, "earshot")) {
					if (COM_ParseFloat(&p, &f)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (f < 0.0f)
					{
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->earshot = f;
					}
					continue;
				}

				// evasion
				if (!Q_stricmp(token, "evasion"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5)
					{
						Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->evasion = n;
					}
					continue;
				}

				// hfov
				if (!Q_stricmp(token, "hfov")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 30 || n > 180) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->hfov = n;// / 2;	//FIXME: Why was this being done?!
					}
					continue;
				}

				// intelligence
				if (!Q_stricmp(token, "intelligence")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->intelligence = n;
					}
					continue;
				}

				// move
				if (!Q_stricmp(token, "move")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->move = n;
					}
					continue;
				}

				// reactions
				if (!Q_stricmp(token, "reactions")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 1 || n > 5) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->reactions = n;
					}
					continue;
				}

				// shootDistance
				if (!Q_stricmp(token, "shootDistance")) {
					if (COM_ParseFloat(&p, &f)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (f < 0.0f)
					{
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->shootDistance = f;
					}
					continue;
				}

				// vfov
				if (!Q_stricmp(token, "vfov")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 30 || n > 180) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->vfov = n / 2;
					}
					continue;
				}

				// vigilance
				if (!Q_stricmp(token, "vigilance")) {
					if (COM_ParseFloat(&p, &f)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (f < 0.0f)
					{
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->vigilance = f;
					}
					continue;
				}

				// visrange
				if (!Q_stricmp(token, "visrange")) {
					if (COM_ParseFloat(&p, &f)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (f < 0.0f)
					{
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->visrange = f;
					}
					continue;
				}

				// rank
				if (!Q_stricmp(token, "rank"))
				{
					if (COM_ParseString(&p, &value))
					{
						continue;
					}
					if (NPC->NPC)
					{
						NPC->NPC->rank = TranslateRankName(value);
					}
					continue;
				}
			}

			// health
			if (!Q_stricmp(token, "health"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					stats->health = n;
				}
				else if (parsingPlayer)
				{
					NPC->client->ps.stats[STAT_MAX_HEALTH] = NPC->client->pers.maxHealth = n;
				}
				continue;
			}

			// fullName
			if (!Q_stricmp(token, "fullName"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				NPC->fullName = G_NewString(value);
				continue;
			}

			// playerTeam
			if (!Q_stricmp(token, "playerTeam"))
			{
				char tk[4096]; //rww - hackilicious!

				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				Com_sprintf(tk, sizeof(tk), "NPC%s", token);
				NPC->client->playerTeam = NPC->s.teamowner = (team_t)GetIDForString(TeamTable, tk);
				continue;
			}

			// enemyTeam
			if (!Q_stricmp(token, "enemyTeam"))
			{
				char tk[4096]; //rww - hackilicious!

				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				Com_sprintf(tk, sizeof(tk), "NPC%s", token);
				NPC->client->enemyTeam = (npcteam_t)GetIDForString(TeamTable, tk);
				continue;
			}

			// class
			if (!Q_stricmp(token, "class"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				NPC->client->NPC_class = (class_t)GetIDForString(ClassTable, value);
				NPC->s.NPC_class = NPC->client->NPC_class; //we actually only need this value now, but at the moment I don't feel like changing the 200+ references to client->NPC_class.

				// No md3's for vehicles.
				if (NPC->client->NPC_class == CLASS_VEHICLE)
				{
					if (!NPC->m_pVehicle)
					{//you didn't spawn this guy right!
						Com_Printf(S_COLOR_RED "ERROR: Tried to spawn a vehicle NPC (%s) without using NPC_Vehicle or 'NPC spawn vehicle <vehiclename>'!!!  Bad, bad, bad!  Shame on you!\n", NPCName);
						return qfalse;
					}
					md3Model = qfalse;
				}

				continue;
			}

			// dismemberment probability for head
			if (!Q_stricmp(token, "dismemberProbHead")) {
				if (COM_ParseInt(&p, &n)) {
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					//	NPC->client->dismemberProbHead = n;
						//rwwFIXMEFIXME: support for this?
				}
				continue;
			}

			// dismemberment probability for arms
			if (!Q_stricmp(token, "dismemberProbArms")) {
				if (COM_ParseInt(&p, &n)) {
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					//	NPC->client->dismemberProbArms = n;
				}
				continue;
			}

			// dismemberment probability for hands
			if (!Q_stricmp(token, "dismemberProbHands")) {
				if (COM_ParseInt(&p, &n)) {
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					//	NPC->client->dismemberProbHands = n;
				}
				continue;
			}

			// dismemberment probability for waist
			if (!Q_stricmp(token, "dismemberProbWaist")) {
				if (COM_ParseInt(&p, &n)) {
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					//	NPC->client->dismemberProbWaist = n;
				}
				continue;
			}

			// dismemberment probability for legs
			if (!Q_stricmp(token, "dismemberProbLegs")) {
				if (COM_ParseInt(&p, &n)) {
					SkipRestOfLine(&p);
					continue;
				}
				if (n < 0)
				{
					Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
					continue;
				}
				if (NPC->NPC)
				{
					//	NPC->client->dismemberProbLegs = n;
				}
				continue;
			}

			//===MOVEMENT STATS============================================================

			if (!Q_stricmp(token, "width"))
			{
				if (COM_ParseInt(&p, &n))
				{
					continue;
				}

				NPC->r.mins[0] = NPC->r.mins[1] = -n;
				NPC->r.maxs[0] = NPC->r.maxs[1] = n;
				continue;
			}

			if (!Q_stricmp(token, "height"))
			{
				if (COM_ParseInt(&p, &n))
				{
					continue;
				}
				if (NPC->client->NPC_class == CLASS_VEHICLE
					&& NPC->m_pVehicle
					&& NPC->m_pVehicle->m_pVehicleInfo
					&& NPC->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
				{//a flying vehicle's origin must be centered in bbox and it should spawn on the ground
					//trace_t		tr;
					//vec3_t		bottom;
					//float		adjust = 32.0f;
					NPC->r.maxs[2] = NPC->client->ps.standheight = (n / 2.0f);
					NPC->r.mins[2] = -NPC->r.maxs[2];
					NPC->s.origin[2] += (DEFAULT_MINS_2 - NPC->r.mins[2]) + 0.125f;
					VectorCopy(NPC->s.origin, NPC->client->ps.origin);
					VectorCopy(NPC->s.origin, NPC->r.currentOrigin);
					G_SetOrigin(NPC, NPC->s.origin);
					trap->LinkEntity((sharedEntity_t*)NPC);
				}
				else
				{
					NPC->r.mins[2] = DEFAULT_MINS_2;//Cannot change
					NPC->r.maxs[2] = NPC->client->ps.standheight = n + DEFAULT_MINS_2;
				}
				NPC->radius = n;
				continue;
			}

			if (!Q_stricmp(token, "crouchheight"))
			{
				if (COM_ParseInt(&p, &n))
				{
					continue;
				}

				NPC->client->ps.crouchheight = n + DEFAULT_MINS_2;
				continue;
			}

			if (!parsingPlayer)
			{
				if (!Q_stricmp(token, "movetype"))
				{
					if (COM_ParseString(&p, &value))
					{
						continue;
					}
					if (Q_stricmp("flyswim", value) == 0)
					{
						NPC->client->ps.eFlags2 |= EF2_FLYING;
					}
					NPC->client->moveType = (movetype_t)MoveTypeNameToEnum(value);
					continue;
				}

				// yawSpeed
				if (!Q_stricmp(token, "yawSpeed")) {
					if (COM_ParseInt(&p, &n)) {
						SkipRestOfLine(&p);
						continue;
					}
					if (n <= 0) {
						Com_Printf("bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->yawSpeed = ((float)(n));
					}
					continue;
				}

				// walkSpeed
				if (!Q_stricmp(token, "walkSpeed"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 0)
					{
						Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->walkSpeed = n;
					}
					continue;
				}

				//runSpeed
				if (!Q_stricmp(token, "runSpeed"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 0)
					{
						Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->runSpeed = n;
					}
					continue;
				}

				//acceleration
				if (!Q_stricmp(token, "acceleration"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (n < 0)
					{
						Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						stats->acceleration = n;
					}
					continue;
				}
				//sex - skip in MP
				if (!Q_stricmp(token, "sex"))
				{
					if (COM_ParseString(&p, &value))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (value[0] == 'm')
					{//male
						stats->sex = SEX_MALE;
					}
					else if (value[0] == 'n')
					{//neutral
						stats->sex = SEX_NEUTRAL;
					}
					else if (value[0] == 'a')
					{//asexual?
						stats->sex = SEX_NEUTRAL;
					}
					else if (value[0] == 'f')
					{//female
						stats->sex = SEX_FEMALE;
					}
					else if (value[0] == 's')
					{//shemale?
						stats->sex = SEX_SHEMALE;
					}
					else if (value[0] == 'h')
					{//hermaphrodite?
						stats->sex = SEX_SHEMALE;
					}
					else if (value[0] == 't')
					{//transsexual/transvestite?
						stats->sex = SEX_SHEMALE;
					}
					continue;
				}
				//===MISC===============================================================================
								// default behavior
				if (!Q_stricmp(token, "behavior"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (n < BS_DEFAULT || n >= NUM_BSTATES)
					{
						Com_Printf(S_COLOR_YELLOW"WARNING: bad %s in NPC '%s'\n", token, NPCName);
						continue;
					}
					if (NPC->NPC)
					{
						NPC->NPC->defaultBehavior = (bState_t)(n);
					}
					continue;
				}
			}

			// snd
			if (!Q_stricmp(token, "snd"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!(NPC->r.svFlags& SVF_NO_BASIC_SOUNDS))
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz(sound, value, sizeof(sound));
					patch = strstr(sound, "/");
					if (patch)
					{
						*patch = 0;
					}
					//	ci->customBasicSoundDir = G_NewString( sound )
				}
				continue;
			}

			// sndcombat
			if (!Q_stricmp(token, "sndcombat"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!(NPC->r.svFlags& SVF_NO_COMBAT_SOUNDS))
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz(sound, value, sizeof(sound));
					patch = strstr(sound, "/");
					if (patch)
					{
						*patch = 0;
					}
					//	ci->customCombatSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndextra
			if (!Q_stricmp(token, "sndextra"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!(NPC->r.svFlags& SVF_NO_EXTRA_SOUNDS))
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz(sound, value, sizeof(sound));
					patch = strstr(sound, "/");
					if (patch)
					{
						*patch = 0;
					}
					//	ci->customExtraSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndjedi
			if (!Q_stricmp(token, "sndjedi"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!(NPC->r.svFlags& SVF_NO_EXTRA_SOUNDS))
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz(sound, value, sizeof(sound));
					patch = strstr(sound, "/");
					if (patch)
					{
						*patch = 0;
					}
					//ci->customJediSoundDir = G_NewString( sound );
				}
				continue;
			}

			// sndcallout
			if (!Q_stricmp(token, "sndcallout"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (!(NPC->r.svFlags& SVF_NO_EXTRA_SOUNDS))
				{
					//FIXME: store this in some sound field or parse in the soundTable like the animTable...
					Q_strncpyz(sound, value, sizeof(sound));
					patch = strstr(sound, "/");
					if (patch)
					{
						*patch = 0;
					}
					//ci->customJediSoundDir = G_NewString(sound);
				}
				continue;
			}

			//New NPC/jedi stats:
			//starting weapon
			if (!Q_stricmp(token, "weapon"))
			{
				int weap;

				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				//FIXME: need to precache the weapon, too?  (in above func)
				weap = GetIDForString(WPTable, value);

				if (weap == WP_STUN_BATON)
				{//  EEEWWWWW!!!!!
					weap = WP_BLASTER;
				}
				if (weap >= WP_NONE && weap <= WP_NUM_WEAPONS)
				{
					NPC->client->ps.weapon = weap;
					NPC->client->ps.stats[STAT_WEAPONS] |= (1 << NPC->client->ps.weapon);
					if (weap > WP_NONE)
					{
						NPC->client->ps.ammo[weaponData[NPC->client->ps.weapon].ammoIndex] = 100;//FIXME: max ammo!
					}
				}
				continue;
			}

			if (!parsingPlayer)
			{
				//altFire
				if (!Q_stricmp(token, "altFire"))
				{
					if (COM_ParseInt(&p, &n))
					{
						SkipRestOfLine(&p);
						continue;
					}
					if (NPC->NPC)
					{
						if (n != 0)
						{
							NPC->NPC->scriptFlags |= SCF_ALT_FIRE;
						}
					}
					continue;
				}
				//Other unique behaviors/numbers that are currently hardcoded?
			}

			//force powers
			fp = GetIDForString(FPTable, token);
			if (fp >= FP_FIRST && fp < NUM_FORCE_POWERS)
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//FIXME: need to precache the fx, too?  (in above func)
				//cap
				if (n > 5)
				{
					n = 5;
				}
				else if (n < 0)
				{
					n = 0;
				}
				if (n)
				{//set
					NPC->client->ps.fd.forcePowersKnown |= (1 << fp);
				}
				else
				{//clear
					NPC->client->ps.fd.forcePowersKnown &= ~(1 << fp);
				}
				NPC->client->ps.fd.forcePowerLevel[fp] = n;
				continue;
			}

			//max force power
			if (!Q_stricmp(token, "forcePowerMax"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				NPC->client->ps.fd.forcePowerMax = n;
				continue;
			}

			//force regen rate - default is 100ms
			if (!Q_stricmp(token, "forceRegenRate"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//NPC->client->ps.forcePowerRegenRate = n;
				continue;
			}

			//force regen amount - default is 1 (points per second)
			if (!Q_stricmp(token, "forceRegenAmount"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//NPC->client->ps.forcePowerRegenAmount = n;
				continue;
			}

			//have a sabers.cfg and just name your saber in your NPCs.cfg/ICARUS script
			//saber name
			if (!Q_stricmp(token, "saber"))
			{
				char* saberName;

				if (COM_ParseString(&p, &value))
				{
					continue;
				}

				saberName = (char*)BG_TempAlloc(4096);//G_NewString( value );
				strcpy(saberName, value);

				WP_SaberParseParms(saberName, &NPC->client->saber[0]);
				npcSaber1 = G_ModelIndex(va("@%s", saberName));

				BG_TempFree(4096);
				continue;
			}

			//second saber name
			if (!Q_stricmp(token, "saber2"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}

				if (!(NPC->client->saber[0].saberFlags& SFL_TWO_HANDED))
				{//can't use a second saber if first one is a two-handed saber...?
					char* saberName = (char*)BG_TempAlloc(4096);//G_NewString( value );
					strcpy(saberName, value);

					WP_SaberParseParms(saberName, &NPC->client->saber[1]);
					if ((NPC->client->saber[1].saberFlags& SFL_TWO_HANDED))
					{//tsk tsk, can't use a twoHanded saber as second saber
						WP_RemoveSaber(NPC->client->saber, 1);
					}
					else
					{
						//NPC->client->ps.dualSabers = qtrue;
						npcSaber2 = G_ModelIndex(va("@%s", saberName));
					}
					BG_TempFree(4096);
				}
				continue;
			}

			// saberColor
			if (!Q_stricmp(token, "saberColor"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					saber_colors_t color = TranslateSaberColor(value);
					for (n = 0; n < MAX_BLADES; n++)
					{
						NPC->client->saber[0].blade[n].color = color;
						NPC->s.boltToPlayer = NPC->s.boltToPlayer & 0x38;//(111000)
						NPC->s.boltToPlayer += (color + 1);
					}
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor2"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[1].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor3"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[2].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor4"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[3].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor5"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[4].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor6"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[5].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor7"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[6].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saberColor8"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[0].blade[7].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					saber_colors_t color = TranslateSaberColor(value);
					for (n = 0; n < MAX_BLADES; n++)
					{
						NPC->client->saber[1].blade[n].color = color;
						NPC->s.boltToPlayer = NPC->s.boltToPlayer & 0x7;//(000111)
						NPC->s.boltToPlayer += ((color + 1) << 3);
					}
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color2"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[1].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color3"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[2].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color4"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[3].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color5"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[4].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color6"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[5].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color7"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[6].color = TranslateSaberColor(value);
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Color8"))
			{
				if (COM_ParseString(&p, &value))
				{
					continue;
				}
				if (NPC->client)
				{
					NPC->client->saber[1].blade[7].color = TranslateSaberColor(value);
				}
				continue;
			}

			//saber length
			if (!Q_stricmp(token, "saberLength"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}

				for (n = 0; n < MAX_BLADES; n++)
				{
					NPC->client->saber[0].blade[n].lengthMax = f;
				}
				continue;
			}

			if (!Q_stricmp(token, "saberLength2"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[1].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength3"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[2].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength4"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[3].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength5"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[4].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength6"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[5].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength7"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[6].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saberLength8"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[0].blade[7].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				for (n = 0; n < MAX_BLADES; n++)
				{
					NPC->client->saber[1].blade[n].lengthMax = f;
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Length2"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[1].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length3"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[2].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length4"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[3].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length5"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[4].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length6"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[5].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length7"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[6].lengthMax = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Length8"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 4.0f)
				{
					f = 4.0f;
				}
				NPC->client->saber[1].blade[7].lengthMax = f;
				continue;
			}

			//saber radius
			if (!Q_stricmp(token, "saberRadius"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				for (n = 0; n < MAX_BLADES; n++)
				{
					NPC->client->saber[0].blade[n].radius = f;
				}
				continue;
			}

			if (!Q_stricmp(token, "saberRadius2"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[1].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius3"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[2].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius4"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[3].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius5"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[4].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius6"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[5].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius7"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[6].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saberRadius8"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[0].blade[7].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				for (n = 0; n < MAX_BLADES; n++)
				{
					NPC->client->saber[1].blade[n].radius = f;
				}
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius2"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[1].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius3"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[2].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius4"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[3].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius5"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[4].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius6"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[5].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius7"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[6].radius = f;
				continue;
			}

			if (!Q_stricmp(token, "saber2Radius8"))
			{
				if (COM_ParseFloat(&p, &f))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (f < 0.25f)
				{
					f = 0.25f;
				}
				NPC->client->saber[1].blade[7].radius = f;
				continue;
			}

			//ADD:
			//saber sounds (on, off, loop)
			//loop sound (like Vader's breathing or droid bleeps, etc.)

			//starting saber style
			if (!Q_stricmp(token, "saberStyle"))
			{
				if (COM_ParseInt(&p, &n))
				{
					SkipRestOfLine(&p);
					continue;
				}
				//cap
				if (n < 0)
				{
					n = 0;
				}
				else if (n > 5)
				{
					n = 5;
				}
				NPC->client->ps.fd.saberAnimLevel = n;
				continue;
			}

			if (!parsingPlayer)
			{
				Com_Printf("WARNING: unknown keyword '%s' while parsing '%s'\n", token, NPCName);
			}
			SkipRestOfLine(&p);
		}
	}

	/*
	Ghoul2 Insert Start
	*/
	if (!md3Model)
	{
		qboolean setTypeBack = qfalse;

		if (Q_stricmp("player", playerModel) == 0)
		{//set up the playerModel to be the playerModel of the entity with the player script_targetname
			gentity_t* player = G_Find(NULL, FOFS(script_targetname), "player");
			char* s;

			if (!player)
			{//no players have spawned in yet.  As such, wing it by using jan
				int scan, rgb[3];
				char* s2;
				Q_strncpyz(playerModel, g_spmodel.string, sizeof(playerModel));
				s2 = va("%s", g_spmodelrgb.string);
				scan = sscanf(s2, "%i %i %i", &rgb[0], &rgb[1], &rgb[2]);
				*s2 = '\0';
				if (scan != 3)
				{
					rgb[0] = 255;
					rgb[1] = 255;
					rgb[2] = 255;
				}
				if ((rgb[0] + rgb[1] + rgb[2]) < 100) // too dark
					rgb[0] = rgb[1] = rgb[2] = 255;
				NPC->client->ps.customRGBA[0] = rgb[0];
				NPC->client->ps.customRGBA[1] = rgb[1];
				NPC->client->ps.customRGBA[2] = rgb[2];
			}
			else
			{
				char	userinfo[MAX_INFO_STRING];
				trap->GetUserinfo(player->s.number, userinfo, sizeof(userinfo));
				Q_strncpyz(playerModel, Info_ValueForKey(userinfo, "model"), sizeof(playerModel));
				//copy over the customRGB data
				NPC->client->ps.customRGBA[0] = player->client->ps.customRGBA[0];
				NPC->client->ps.customRGBA[1] = player->client->ps.customRGBA[1];
				NPC->client->ps.customRGBA[2] = player->client->ps.customRGBA[2];
			}

			//parse custom skin if there is one.
			s = Q_strrchr(playerModel, '/');
			if (s)
			{//model has custom skin
				s++;
				strcpy(customSkin, s);
				s--;
				*s = '\0';
			}
		}

		if (npcSaber1 == 0)
		{ //use "kyle" for a default then
			npcSaber1 = G_ModelIndex("@Kyle");
			WP_SaberParseParms(DEFAULT_SABER, &NPC->client->saber[0]);
		}

		NPC->s.npcSaber1 = npcSaber1;
		NPC->s.npcSaber2 = npcSaber2;

		if (!customSkin[0])
		{
			strcpy(customSkin, "default");
		}

		if (NPC->client&& NPC->client->NPC_class == CLASS_VEHICLE)
		{ //vehicles want their names fed in as models
			//we put the $ in front to indicate a name and not a model
			strcpy(playerModel, va("$%s", NPCName));
		}
		SetupGameGhoul2Model(NPC, playerModel, customSkin);

		if (!NPC->NPC_type)
		{ //just do this for now so NPC_Precache can see the name.
			NPC->NPC_type = (char*)NPCName;
			setTypeBack = qtrue;
		}

		NPC_Precache(NPC); //this will just soundindex some values for sounds on the client,

		if (setTypeBack)
		{ //don't want this being set if we aren't ready yet.
			NPC->NPC_type = NULL;
		}
	}
	else
	{
		NPC->s.modelindex = G_ModelIndex(va("models/players/%s/lower.md3", playerModel));
	}

	return qtrue;
}

char npcParseBuffer[MAX_NPC_DATA_SIZE];

void NPC_LoadParms(void)
{
	int			len, totallen, npcExtFNLen, fileCnt, i;
	char* holdChar, * marker;
	char		npcExtensionListBuf[16384];			//	The list of file names read in
	fileHandle_t f;
	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = NPCParms + totallen;
	*marker = 0;

	//now load in the extra .npc extensions
	fileCnt = trap->FS_GetFileList("ext_data/mpnpcs", ".npc", npcExtensionListBuf, sizeof(npcExtensionListBuf));

	holdChar = npcExtensionListBuf;
	for (i = 0; i < fileCnt; i++, holdChar += npcExtFNLen + 1)
	{
		npcExtFNLen = strlen(holdChar);

		len = trap->FS_Open(va("ext_data/mpnpcs/%s", holdChar), &f, FS_READ);

		if (len == -1)
		{
			Com_Printf("error reading file\n");
		}
		else
		{
			if (totallen + len >= MAX_NPC_DATA_SIZE)
			{
				trap->FS_Close(f);
				trap->Error(ERR_DROP, "NPC_LoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)");
			}
			trap->FS_Read(npcParseBuffer, len, f);
			npcParseBuffer[len] = 0;

			len = COM_Compress(npcParseBuffer);

			strcat(marker, npcParseBuffer);
			strcat(marker, "\n");
			len++;
			trap->FS_Close(f);

			totallen += len;
			marker = NPCParms + totallen;
		}
	}
}

extern void NPC_ShadowTrooper_Precache(void);
extern void NPC_Gonk_Precache(void);
extern void NPC_Mouse_Precache(void);
extern void NPC_Seeker_Precache(void);
extern void NPC_Remote_Precache(void);
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t * self);
extern void NPC_MineMonster_Precache(void);
extern void NPC_Howler_Precache(void);
extern void NPC_Rancor_Precache(void);
extern void NPC_MutantRancor_Precache(void);
extern void NPC_Wampa_Precache(void);
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_Protocol_Precache(void);
extern void Boba_Precache(void);
extern void RT_Precache(void);
extern void SandCreature_Precache(void);
extern void NPC_TavionScepter_Precache(void);
extern void NPC_TavionSithSword_Precache(void);
extern void NPC_Rosh_Dark_Precache(void);
extern void NPC_Tusken_Precache(void);
extern void NPC_Saboteur_Precache(void);
extern void NPC_CultistDestroyer_Precache(void);
extern void NPC_GalakMech_Precache(void);

void NPC_Jawa_Precache(void)
{
	for (int i = 1; i < 7; i++)
	{
		G_SoundIndex(va("sound/chars/jawa/misc/chatter%d.wav", i));
	}
	G_SoundIndex("sound/chars/jawa/misc/ooh-tee-nee.wav");
}

void NPC_Tusken_Precache(void)
{
	int i;
	for (i = 1; i < 5; i++)
	{
		G_SoundIndex(va("sound/weapons/tusken_staff/stickhit%d.wav", i));
	}
}

void NPC_PrecacheByClassName(const char* type)
{//Precashes the extra fx/effects/etc used by special NPCs 
	if (!type || !type[0])
	{
		return;
	}

	if (!Q_stricmp("gonk", type))
	{
		NPC_Gonk_Precache();
	}
	else if (!Q_stricmp("mouse", type))
	{
		NPC_Mouse_Precache();
	}
	else if (!Q_stricmpn("r2d2", type, 4))
	{
		NPC_R2D2_Precache();
	}
	else if (!Q_stricmp("atst", type))
	{
		NPC_ATST_Precache();
	}
	else if (!Q_stricmpn("r5d2", type, 4))
	{
		NPC_R5D2_Precache();
	}
	else if (!Q_stricmp("mark1", type))
	{
		NPC_Mark1_Precache();
	}
	else if (!Q_stricmp("mark2", type))
	{
		NPC_Mark2_Precache();
	}
	else if (!Q_stricmp("interrogator", type))
	{
		NPC_Interrogator_Precache(NULL);
	}
	else if (!Q_stricmp("probe", type))
	{
		NPC_Probe_Precache();
	}
	else if (!Q_stricmp("seeker", type))
	{
		NPC_Seeker_Precache();
	}
	else if (!Q_stricmpn("remote", type, 6))
	{
		NPC_Remote_Precache();
	}
	else if (!Q_stricmpn("shadowtrooper", type, 13))
	{
		NPC_ShadowTrooper_Precache();
	}
	else if (!Q_stricmp("minemonster", type))
	{
		NPC_MineMonster_Precache();
	}
	else if (!Q_stricmp("howler", type))
	{
		NPC_Howler_Precache();
	}
	else if (!Q_stricmp("sentry", type))
	{
		NPC_Sentry_Precache();
	}
	else if (!Q_stricmpn("protocol", type, 8))
	{
		NPC_Protocol_Precache();
	}
	else if (!Q_stricmp("galak_mech", type))
	{
		NPC_GalakMech_Precache();
	}
	else if (!Q_stricmp("wampa", type))
	{
		NPC_Wampa_Precache();
	}
	else if (!Q_stricmp("rancor", type))
	{
		NPC_Rancor_Precache();
	}
	else if (!Q_stricmp("mutant_rancor", type))
	{
		NPC_Rancor_Precache();
	}
	else if (!Q_stricmp("sand_creature", type))
	{
		SandCreature_Precache();
	}
	else if (!Q_stricmp("boba_fett", type))
	{
		Boba_Precache();
	}
	else if (!Q_stricmp("rockettrooper2", type))
	{
		RT_Precache();
	}
	else if (!Q_stricmp("rockettrooper2Officer", type))
	{
		RT_Precache();
	}
	else if (!Q_stricmp("tavion_scepter", type))
	{
		NPC_TavionScepter_Precache();
	}
	else if (!Q_stricmp("tavion_sith_sword", type))
	{
		NPC_TavionSithSword_Precache();
	}
	else if (!Q_stricmp("rosh_dark", type))
	{
		NPC_Rosh_Dark_Precache();
	}
	else if (!Q_stricmpn("tusken", type, 6))
	{
		NPC_Tusken_Precache();
	}
	else if (!Q_stricmpn("saboteur", type, 8))
	{
		NPC_Saboteur_Precache();
	}
	else if (!Q_stricmp("cultist_destroyer", type))
	{
		NPC_CultistDestroyer_Precache();
	}
	else if (!Q_stricmpn("jawa", type, 4))
	{
		NPC_Jawa_Precache();
	}
}