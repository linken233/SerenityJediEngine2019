/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26
//Force Disable Settings
#define FORCE_ALLOFF				262143  //All Force Powers Off
#define FORCE_ALLON					0
#define FORCE_JUMPONLY				262141
#define FORCE_NEUTRALSONLY			16353

void CG_LoadBar(void);

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	trap->UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t		*item;
	char	upperKey[1024];

	item = &bg_itemlist[itemNum];

	if (!item->classname || !item->classname[0])
	{
	//	CG_LoadingString( "Unknown item" );
		return;
	}

	strcpy(upperKey, item->classname);
	CG_LoadingString( CG_GetStringEdString("SP_INGAME",Q_strupr(upperKey)) );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			personality[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

/*
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];
	char			*skin;
	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = Q_strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );

		loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap->R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}
*/
	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
//	Q_CleanStr( personality );

	/*
	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap->S_RegisterSound( va( "sound/player/announce/%s.wav", personality ));
	}
	*/

	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
overlays UI_DrawConnectScreen
*/
#define UI_INFOFONT (UI_BIGFONT)
void CG_DrawInformation(void)
{
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value, valueNOFP;
	qhandle_t	levelshot;
	qhandle_t	levelshot2;
	char		buf[1024];
	int			iPropHeight = 18;	// I know, this is total crap, but as a post release asian-hack....  -Ste

	info = CG_ConfigString(CS_SERVERINFO);
	sysInfo = CG_ConfigString(CS_SYSTEMINFO);

	s = Info_ValueForKey(info, "mapname");
	levelshot = trap->R_RegisterShaderNoMip(va("levelshots/%s", s));
	levelshot2 = trap->R_RegisterShaderNoMip(va("levelshots/%s2", s));

	if (!levelshot)
	{
		levelshot = trap->R_RegisterShaderNoMip("menu/art/unknownmap_mp");
	}
	if (!levelshot2)
	{
		levelshot2 = trap->R_RegisterShaderNoMip("menu/art/unknownmap");
	}

	trap->R_SetColor(NULL);


	if (cg.loadLCARSStage >= 4)
	{
		CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot2);
	}
	else
	{
		CG_DrawPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot);
	}

	CG_LoadBar();

	y = 180 - 32;

	// don't print server lines if playing a local game
	trap->Cvar_VariableStringBuffer("sv_running", buf, sizeof(buf));
	if (!atoi(buf))
	{
		// server hostname
		Q_strncpyz(buf, Info_ValueForKey(info, "sv_hostname"), sizeof(buf));
		Q_CleanStr(buf);
		CG_DrawProportionalString(320, y, buf, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += iPropHeight;

		// pure server
		s = Info_ValueForKey(sysInfo, "sv_pure");
		if (s[0] == '1') {
			const char *psPure = CG_GetStringEdString("MP_INGAME", "PURE_SERVER");
			CG_DrawProportionalString(320, y, psPure, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}

		// server-specific message of the day
		s = CG_ConfigString(CS_MOTD);
		if (s[0]) {
			CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}

		{	// display global MOTD at bottom (mirrors ui_main UI_DrawConnectScreen
			char motdString[1024];
			trap->Cvar_VariableStringBuffer("cl_motdString", motdString, sizeof(motdString));

			if (motdString[0])
			{
				CG_DrawProportionalString(320, 425, motdString, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			}
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString(CS_MESSAGE);
	if (s[0]) {
		CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += iPropHeight;
	}

	// cheats warning
	s = Info_ValueForKey(sysInfo, "sv_cheats");
	if (s[0] == '1')
	{
		CG_DrawProportionalString(320, y, CG_GetStringEdString("MP_INGAME", "CHEATSAREENABLED"), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
		y += iPropHeight;
	}

	// game type
	s = BG_GetGametypeString(cgs.gametype);
	CG_DrawProportionalString(320, y, s, UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
	y += iPropHeight;

	if (cgs.gametype != GT_SIEGE)
	{
		if (cgs.gametype != GT_SINGLE_PLAYER)
		{
			value = atoi(Info_ValueForKey(info, "timelimit"));
			if (value)
			{
				CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "TIMELIMIT"), value), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
				y += iPropHeight;
			}
		}

		if (cgs.gametype < GT_CTF && cgs.gametype != GT_SINGLE_PLAYER)
		{
			value = atoi(Info_ValueForKey(info, "fraglimit"));
			if (value) {
				CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "FRAGLIMIT"), value), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
				y += iPropHeight;
			}

			if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
			{
				value = atoi(Info_ValueForKey(info, "duel_fraglimit"));
				if (value) {
					CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "WINLIMIT"), value), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
					y += iPropHeight;
				}
			}
		}
	}

	if (cgs.gametype >= GT_CTF) {
		value = atoi(Info_ValueForKey(info, "capturelimit"));
		if (value) {
			CG_DrawProportionalString(320, y, va("%s %i", CG_GetStringEdString("MP_INGAME", "CAPTURELIMIT"), value), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
	}

	if (cgs.gametype >= GT_TEAM)
	{
		value = atoi(Info_ValueForKey(info, "g_forceBasedTeams"));
		if (value) {
			CG_DrawProportionalString(320, y, CG_GetStringEdString("MP_INGAME", "FORCEBASEDTEAMS"), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
	}

	if (cgs.gametype != GT_SIEGE)
	{
		valueNOFP = atoi(Info_ValueForKey(info, "g_forcePowerDisable"));

		value = atoi(Info_ValueForKey(info, "g_maxForceRank"));
		if (value && valueNOFP != FORCE_ALLOFF && (value < NUM_FORCE_MASTERY_LEVELS)) {
			char fmStr[1024];

			trap->SE_GetStringTextString("MP_INGAME_MAXFORCERANK", fmStr, sizeof(fmStr));

			CG_DrawProportionalString(320, y, va("%s %s", fmStr, CG_GetStringEdString("MP_INGAME", forceMasteryLevels[value])), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (valueNOFP != FORCE_ALLOFF)
		{
			char fmStr[1024];
			trap->SE_GetStringTextString("MP_INGAME_MAXFORCERANK", fmStr, sizeof(fmStr));

			CG_DrawProportionalString(320, y, va("%s %s", fmStr, (char *)CG_GetStringEdString("MP_INGAME", forceMasteryLevels[7])), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}

		if (valueNOFP == FORCE_ALLOFF)
		{
			CG_DrawProportionalString(320, y, va("%s", (char *)CG_GetStringEdString("MP_INGAME", "NOFPSET")), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (valueNOFP == FORCE_JUMPONLY)
		{
			CG_DrawProportionalString(320, y, va("%s", (char *)CG_GetStringEdString("MP_INGAME", "NOFPSET")), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			(320, y, "Force Jump Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (valueNOFP == FORCE_NEUTRALSONLY)
		{
			CG_DrawProportionalString(320, y, "Neutral Force Powers Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (valueNOFP)
		{
			CG_DrawProportionalString(320, y, "Custom Force Setup", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}

		if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
		{
			value = atoi(Info_ValueForKey(info, "g_duelWeaponDisable"));
		}
		else
		{
			value = atoi(Info_ValueForKey(info, "g_weaponDisable"));
		}
		if (cgs.gametype != GT_JEDIMASTER && value == WP_SABERSONLY)
		{
			CG_DrawProportionalString(320, y, va("%s", (char *)CG_GetStringEdString("MP_INGAME", "SABERONLYSET")), UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (value == WP_MELEEONLY)
		{
			CG_DrawProportionalString(320, y, "Melee Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (value == WP_MELEESABERS)
		{
			CG_DrawProportionalString(320, y, "Sabers & Melee Only", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (value == WP_NOEXPLOS)
		{
			CG_DrawProportionalString(320, y, "No Explosives", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
		else if (value)
		{
			CG_DrawProportionalString(320, y, "Custom Weapon Setup", UI_CENTER | UI_INFOFONT | UI_DROPSHADOW, colorWhite);
			y += iPropHeight;
		}
	}
}

/*
===================
CG_LoadBar
===================
*/

#define LOADBAR_CLIP_WIDTH		256
#define LOADBAR_CLIP_HEIGHT		64

void CG_LoadBar(void)
{
	const int numticks = 9, tickwidth = 40, tickheight = 8;
	const int tickpadx = 20, tickpady = 12;
	const int capwidth = 8;
	const int barwidth = numticks*tickwidth+tickpadx*2+capwidth*2, barleft = ((640-barwidth)/2);
	const int barheight = tickheight + tickpady*2, bartop = 480-barheight;
	const int capleft = barleft+tickpadx, tickleft = capleft+capwidth, ticktop = bartop+tickpady;
	int			x, y;

	trap->R_SetColor( colorWhite );
	// Draw background
	CG_DrawPic(barleft, bartop, barwidth, barheight, cgs.media.loadBarLEDSurround);

	// Draw left cap (backwards)
	CG_DrawPic(tickleft, ticktop, -capwidth, tickheight, cgs.media.loadBarLEDCap);

	// Draw bar
	CG_DrawPic(tickleft, ticktop, tickwidth*cg.loadLCARSStage, tickheight, cgs.media.loadBarLED);

	// Draw right cap
	CG_DrawPic(tickleft+tickwidth*cg.loadLCARSStage, ticktop, capwidth, tickheight, cgs.media.loadBarLEDCap);

	y = 50;
	x = (640 - LOADBAR_CLIP_WIDTH) / 2;

	// Draw eoc cap
	CG_DrawPic(x, y, LOADBAR_CLIP_WIDTH, LOADBAR_CLIP_HEIGHT, cgs.media.load_SerenitySaberSystems);
}

