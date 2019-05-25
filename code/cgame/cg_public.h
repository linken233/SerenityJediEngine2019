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

#ifndef _CG_PUBLIC_H
#define _CG_PUBLIC_H

#define NUM_EXPLOSION_SHADERS	8
#define NUM_EXPLOSION_FRAMES	3

#define	CMD_BACKUP			64
#define	CMD_MASK			(CMD_BACKUP - 1)
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP


#define	MAX_ENTITIES_IN_SNAPSHOT	512

#define	SNAPFLAG_RATE_DELAYED		1		// the server withheld a packet to save bandwidth
#define	SNAPFLAG_DROPPED_COMMANDS	2		// the server lost some cmds coming from the client

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
struct snapshot_s
{
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, SNAPFLAG_DROPPED_COMMANDS

	int				serverTime;		// server time the message is valid for (in msec)

	byte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	int				cmdNum;			// the next cmdNum the server is expecting
									// client side prediction should start with this cmd
	playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numConfigstringChanges;	// configstrings that have changed since the last
	int				configstringNum;		// acknowledged snapshot_t (which is usually NOT the previous snapshot!)

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
};

typedef snapshot_s snapshot_t;


/*
==================================================================

functions imported from the main executable

==================================================================
*/

#define	CGAME_IMPORT_API_VERSION	4

typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_RMG_INIT,
	CG_CM_REGISTER_TERRAIN,
	CG_RE_INIT_RENDERER_TERRAIN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_MARKFRAGMENTS,
	CG_CM_SNAPPVS,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,
	CG_S_ADDLOOPINGSOUND,
	CG_S_STOPSOUNDS,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_REGISTERSHADERNOMIP,
	CG_R_REGISTERFONT,
	CG_R_FONTSTRLENPIXELS,
	CG_R_FONTSTRLENCHARS,
	CG_R_FONTHEIGHTPIXELS,
	CG_R_FONTDRAWSTRING,
	CG_LANGUAGE_ISASIAN,
	CG_LANGUAGE_USESSPACES,
	CG_ANYLANGUAGE_READFROMSTRING,
	CG_R_SETREFRACTIONPROP,
	CG_R_CLEARSCENE,
	CG_R_ADDREFENTITYTOSCENE,

	CG_R_INPVS,

	CG_R_GETLIGHTING,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_R_DRAWROTATEPIC,
	CG_R_DRAWROTATEPIC2,
	CG_R_SETRANGEFOG,
	CG_R_LA_GOGGLES,
	CG_R_SCISSOR,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,

	CG_GETDEFAULTSTATE,

	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_SETUSERCMDANGLES,
	CG_S_UPDATEAMBIENTSET,
	CG_S_ADDLOCALSET,
	CG_AS_PARSESETS,
	CG_AS_ADDENTRY,
	CG_AS_GETBMODELSOUND,
	CG_S_GETSAMPLELENGTH,
	COM_SETORGANGLES,
/*
Ghoul2 Insert Start
*/
	CG_G2_LISTBONES,
	CG_G2_LISTSURFACES,
	CG_G2_HAVEWEGHOULMODELS,
	CG_G2_SETMODELS,
/*
Ghoul2 Insert End
*/

	CG_R_GET_LIGHT_STYLE,
	CG_R_SET_LIGHT_STYLE,
	CG_R_GET_BMODEL_VERTS,
	CG_R_WORLD_EFFECT_COMMAND,

	CG_CIN_PLAYCINEMATIC,
	CG_CIN_STOPCINEMATIC,
	CG_CIN_RUNCINEMATIC,
	CG_CIN_DRAWCINEMATIC,
	CG_CIN_SETEXTENTS,
	CG_Z_MALLOC,
	CG_Z_FREE,
	CG_UI_MENU_RESET,
	CG_UI_MENU_NEW,
	CG_UI_SETACTIVE_MENU,
	CG_UI_MENU_OPENBYNAME,
	CG_UI_PARSE_INT,
	CG_UI_PARSE_STRING,
	CG_UI_PARSE_FLOAT,
	CG_UI_STARTPARSESESSION,
	CG_UI_ENDPARSESESSION,
	CG_UI_PARSEEXT,
	CG_UI_MENUPAINT_ALL,
	CG_UI_MENUCLOSE_ALL,
	CG_UI_STRING_INIT,
	CG_UI_GETMENUINFO,
	CG_SP_GETSTRINGTEXTSTRING,
	CG_UI_GETITEMTEXT,
	CG_UI_GETITEMINFO,

	// special
	CG_R_DRAWSCREENSHOT,

	CG_SP_REGISTER,
	CG_SP_GETSTRINGTEXT,
	CG_ANYLANGUAGE_READFROMSTRING2,

	CG_OPENJK_MENU_PAINT,
	CG_OPENJK_GETMENU_BYNAME,
} cgameImport_t;

#ifdef JK2_MODE
typedef enum {
	CG_PRINT_JK2,
	CG_ERROR_JK2,
	CG_MILLISECONDS_JK2,
	CG_CVAR_REGISTER_JK2,
	CG_CVAR_UPDATE_JK2,
	CG_CVAR_SET_JK2,
	CG_ARGC_JK2,
	CG_ARGV_JK2,
	CG_ARGS_JK2,
	CG_FS_FOPENFILE_JK2,
	CG_FS_READ_JK2,
	CG_FS_WRITE_JK2,
	CG_FS_FCLOSEFILE_JK2,
	CG_SENDCONSOLECOMMAND_JK2,
	CG_ADDCOMMAND_JK2,
	CG_SENDCLIENTCOMMAND_JK2,
	CG_UPDATESCREEN_JK2,
	CG_CM_LOADMAP_JK2,
	CG_CM_NUMINLINEMODELS_JK2,
	CG_CM_INLINEMODEL_JK2,
	CG_CM_TEMPBOXMODEL_JK2,
	CG_CM_POINTCONTENTS_JK2,
	CG_CM_TRANSFORMEDPOINTCONTENTS_JK2,
	CG_CM_BOXTRACE_JK2,
	CG_CM_TRANSFORMEDBOXTRACE_JK2,
	CG_CM_MARKFRAGMENTS_JK2,
	CG_CM_SNAPPVS_JK2,
	CG_S_STARTSOUND_JK2,
	CG_S_STARTLOCALSOUND_JK2,
	CG_S_CLEARLOOPINGSOUNDS_JK2,
	CG_S_ADDLOOPINGSOUND_JK2,
	CG_S_UPDATEENTITYPOSITION_JK2,
	CG_S_RESPATIALIZE_JK2,
	CG_S_REGISTERSOUND_JK2,
	CG_S_STARTBACKGROUNDTRACK_JK2,
	CG_R_LOADWORLDMAP_JK2,
	CG_R_REGISTERMODEL_JK2,
	CG_R_REGISTERSKIN_JK2,
	CG_R_REGISTERSHADER_JK2,
	CG_R_REGISTERSHADERNOMIP_JK2,
	CG_R_REGISTERFONT_JK2,
	CG_R_FONTSTRLENPIXELS_JK2,
	CG_R_FONTSTRLENCHARS_JK2,
	CG_R_FONTHEIGHTPIXELS_JK2,
	CG_R_FONTDRAWSTRING_JK2,
	CG_LANGUAGE_ISASIAN_JK2,
	CG_LANGUAGE_USESSPACES_JK2,
	CG_ANYLANGUAGE_READFROMSTRING_JK2,
	CG_R_CLEARSCENE_JK2,
	CG_R_ADDREFENTITYTOSCENE_JK2,
	CG_R_GETLIGHTING_JK2,
	CG_R_ADDPOLYTOSCENE_JK2,
	CG_R_ADDLIGHTTOSCENE_JK2,
	CG_R_RENDERSCENE_JK2,
	CG_R_SETCOLOR_JK2,
	CG_R_DRAWSTRETCHPIC_JK2,
	CG_R_DRAWSCREENSHOT_JK2,
	CG_R_MODELBOUNDS_JK2,
	CG_R_LERPTAG_JK2,
	CG_R_DRAWROTATEPIC_JK2,
	CG_R_DRAWROTATEPIC2_JK2,
	CG_R_LA_GOGGLES_JK2,
	CG_R_SCISSOR_JK2,
	CG_GETGLCONFIG_JK2,
	CG_GETGAMESTATE_JK2,
	CG_GETCURRENTSNAPSHOTNUMBER_JK2,
	CG_GETSNAPSHOT_JK2,
	CG_GETSERVERCOMMAND_JK2,
	CG_GETCURRENTCMDNUMBER_JK2,
	CG_GETUSERCMD_JK2,
	CG_SETUSERCMDVALUE_JK2,
	CG_SETUSERCMDANGLES_JK2,
	CG_S_UPDATEAMBIENTSET_JK2,
	CG_S_ADDLOCALSET_JK2,
	CG_AS_PARSESETS_JK2,
	CG_AS_ADDENTRY_JK2,
	CG_AS_GETBMODELSOUND_JK2,
	CG_S_GETSAMPLELENGTH_JK2,
	COM_SETORGANGLES_JK2,
/*
Ghoul2 Insert Start
*/
	CG_G2_LISTBONES_JK2,
	CG_G2_LISTSURFACES_JK2,
	CG_G2_HAVEWEGHOULMODELS_JK2,
	CG_G2_SETMODELS_JK2,
/*
Ghoul2 Insert End
*/

	CG_R_GET_LIGHT_STYLE_JK2,
	CG_R_SET_LIGHT_STYLE_JK2,
	CG_R_GET_BMODEL_VERTS_JK2,
	CG_R_WORLD_EFFECT_COMMAND_JK2,

	CG_CIN_PLAYCINEMATIC_JK2,
	CG_CIN_STOPCINEMATIC_JK2,
	CG_CIN_RUNCINEMATIC_JK2,
	CG_CIN_DRAWCINEMATIC_JK2,
	CG_CIN_SETEXTENTS_JK2,
	CG_Z_MALLOC_JK2,
	CG_Z_FREE_JK2,
	CG_UI_MENU_RESET_JK2,
	CG_UI_MENU_NEW_JK2,
	CG_UI_PARSE_INT_JK2,
	CG_UI_PARSE_STRING_JK2,
	CG_UI_PARSE_FLOAT_JK2,
	CG_UI_STARTPARSESESSION_JK2,
	CG_UI_ENDPARSESESSION_JK2,
	CG_UI_PARSEEXT_JK2,
	CG_UI_MENUPAINT_ALL_JK2,
	CG_UI_STRING_INIT_JK2,
	CG_UI_GETMENUINFO_JK2,
	CG_SP_REGISTER_JK2,
	CG_SP_GETSTRINGTEXTSTRING_JK2,
	CG_SP_GETSTRINGTEXT_JK2,
	CG_UI_GETITEMTEXT_JK2,
	CG_ANYLANGUAGE_READFROMSTRING2_JK2,

	CG_OPENJK_MENU_PAINT_JK2,
	CG_OPENJK_GETMENU_BYNAME_JK2,
} cgameJK2Import_t;
#endif

//----------------------------------------------

#endif // _CG_PUBLIC_H
