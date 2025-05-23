/*	SCCS Id: @(#)topten.c	3.4	2000/01/21	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "dlb.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif
#ifdef XLOGFILE
#include "artifact.h" /* we need artilist so inherited arti can be in xlogfile */
#endif
#ifdef UNIX /* filename chmod() */
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef VMS
 /* We don't want to rewrite the whole file, because that entails	 */
 /* creating a new version which requires that the old one be deletable. */
# define UPDATE_RECORD_IN_PLACE
#endif

/*
 * Updating in place can leave junk at the end of the file in some
 * circumstances (if it shrinks and the O.S. doesn't have a straightforward
 * way to truncate it).  The trailing junk is harmless and the code
 * which reads the scores will ignore it.
 */
#ifdef UPDATE_RECORD_IN_PLACE
static long final_fpos;
#endif

#define done_stopprint program_state.stopprint

#define newttentry() (struct toptenentry *) alloc(sizeof(struct toptenentry))
#define dealloc_ttentry(ttent) free((genericptr_t) (ttent))
#define NAMSZ	20
#define DTHSZ	100
#define ROLESZ   3
#define PERSMAX	 3		/* entries per name/uid per char. allowed */
#define POINTSMIN	1	/* must be > 0 */
#define ENTRYMAX	2000	/* must be >= 10 */

#if !defined(MICRO) && !defined(MAC) && !defined(WIN32)
/*#define PERS_IS_UID*/		/* delete for PERSMAX per name; now per uid */
#endif
struct toptenentry {
	struct toptenentry *tt_next;
#ifdef UPDATE_RECORD_IN_PLACE
	long fpos;
#endif
	long points;
	int deathdnum, deathlev;
	int maxlvl, hp, maxhp, deaths;
	int ver_major, ver_minor, patchlevel;
	long deathdate, birthdate;
	int uid;
	char plrole[ROLESZ+1];
	char plrace[ROLESZ+1];
	char plgend[ROLESZ+1];
	char plalign[ROLESZ+1];
	char name[NAMSZ+1];
	char death[DTHSZ+1];
} *tt_head;

STATIC_DCL void FDECL(topten_print, (const char *));
STATIC_DCL void FDECL(topten_print_bold, (const char *));
STATIC_DCL xchar FDECL(observable_depth, (d_level *));
STATIC_DCL void NDECL(outheader);
STATIC_DCL void FDECL(outentry, (int,struct toptenentry *,BOOLEAN_P));
STATIC_DCL void FDECL(readentry, (FILE *,struct toptenentry *));
STATIC_DCL void FDECL(writeentry, (FILE *,struct toptenentry *));
#ifdef XLOGFILE
STATIC_DCL void FDECL(munge_xlstring, (char *dest, char *src, int n));
STATIC_DCL void FDECL(write_xlentry, (FILE *,struct toptenentry *));
#endif
STATIC_DCL void FDECL(free_ttlist, (struct toptenentry *));
STATIC_DCL int FDECL(classmon, (char *,BOOLEAN_P));
STATIC_DCL int FDECL(score_wanted,
		(BOOLEAN_P, int,struct toptenentry *,int,const char **,int));
#ifdef RECORD_CONDUCT
/*STATIC_DCL long FDECL(encodeconduct, (void));*/
#endif
#ifdef RECORD_ACHIEVE
STATIC_DCL long FDECL(encodeachieve, (void));
STATIC_DCL void FDECL(writeachieveX, (char *));
#endif
STATIC_DCL long FDECL(encode_xlogflags, (void));
#ifdef NO_SCAN_BRACK
STATIC_DCL void FDECL(nsb_mung_line,(char*));
STATIC_DCL void FDECL(nsb_unmung_line,(char*));
#endif

/* must fit with end.c; used in rip.c */
NEARDATA const char * const killed_by_prefix[] = {
	"killed by ", "betrayed by ", "choked on ", "poisoned by ", "died of ", "drowned in ",
	"burned by ", "dissolved in ", "crushed to death by ", "petrified by ", "turned to gold by ",
	"turned to salt by ", "vitrified by ", "turned to slime by ", "exploded by ", "died by ",
	"disintegrated by ", "killed by ", 	"" /*APOCALYPSE*/, "" /*PANICKED*/, "" /*TRICKED*/,
	"" /*QUIT*/, "" /*ESCAPED*/, ""  /*ASCENDED*/
};

static winid toptenwin = WIN_ERR;

#ifdef RECORD_START_END_TIME
static time_t deathtime = 0L;
#endif

STATIC_OVL void
topten_print(x)
const char *x;
{
	if (toptenwin == WIN_ERR)
	    raw_print(x);
	else
	    putstr(toptenwin, ATR_NONE, x);
}

STATIC_OVL void
topten_print_bold(x)
const char *x;
{
	if (toptenwin == WIN_ERR)
	    raw_print_bold(x);
	else
	    putstr(toptenwin, ATR_BOLD, x);
}

STATIC_OVL xchar
observable_depth(lev)
d_level *lev;
{
#if 0	/* if we ever randomize the order of the elemental planes, we
	   must use a constant external representation in the record file */
	if (In_endgame(lev)) {
	    if (Is_astralevel(lev))	 return -5;
	    else if (Is_waterlevel(lev)) return -4;
	    else if (Is_firelevel(lev))	 return -3;
	    else if (Is_airlevel(lev))	 return -2;
	    else if (Is_earthlevel(lev)) return -1;
	    else			 return 0;	/* ? */
	} else
#endif
	    return depth(lev);
}

STATIC_OVL void
readentry(rfile,tt)
FILE *rfile;
struct toptenentry *tt;
{
#ifdef NO_SCAN_BRACK /* Version_ Pts DgnLevs_ Hp___ Died__Born id */
	static const char fmt[] = "%d %d %d %ld %d %d %d %d %d %d %ld %ld %d%*c";
	static const char fmt32[] = "%c%c %s %s%*c";
	static const char fmt33[] = "%s %s %s %s %s %s%*c";
#else
	static const char fmt[] = "%d.%d.%d %ld %d %d %d %d %d %d %ld %ld %d ";
	static const char fmt32[] = "%c%c %[^,],%[^\n]%*c";
	static const char fmt33[] = "%s %s %s %s %[^,],%[^\n]%*c";
#endif

#ifdef UPDATE_RECORD_IN_PLACE
	/* note: fscanf() below must read the record's terminating newline */
	final_fpos = tt->fpos = ftell(rfile);
#endif
#define TTFIELDS 13
	if(fscanf(rfile, fmt,
			&tt->ver_major, &tt->ver_minor, &tt->patchlevel,
			&tt->points, &tt->deathdnum, &tt->deathlev,
			&tt->maxlvl, &tt->hp, &tt->maxhp, &tt->deaths,
			&tt->deathdate, &tt->birthdate,
			&tt->uid) != TTFIELDS)
#undef TTFIELDS
		tt->points = 0;
	else {
		/* Check for backwards compatibility */
		if (tt->ver_major < 3 ||
				(tt->ver_major == 3 && tt->ver_minor < 3)) {
			int i;

		    if (fscanf(rfile, fmt32,
				tt->plrole, tt->plgend,
				tt->name, tt->death) != 4)
			tt->points = 0;
		    tt->plrole[1] = '\0';
		    if ((i = str2role(tt->plrole)) >= 0)
			Strcpy(tt->plrole, roles[i].filecode);
		    Strcpy(tt->plrace, "?");
		    Strcpy(tt->plgend, (tt->plgend[0] == 'M') ? "Mal" : "Fem");
		    Strcpy(tt->plalign, "?");
		} else if (fscanf(rfile, fmt33,
				tt->plrole, tt->plrace, tt->plgend,
				tt->plalign, tt->name, tt->death) != 6)
			tt->points = 0;
#ifdef NO_SCAN_BRACK
		if(tt->points > 0) {
			nsb_unmung_line(tt->name);
			nsb_unmung_line(tt->death);
		}
#endif
	}

	/* check old score entries for Y2K problem and fix whenever found */
	if (tt->points > 0) {
		if (tt->birthdate < 19000000L) tt->birthdate += 19000000L;
		if (tt->deathdate < 19000000L) tt->deathdate += 19000000L;
	}
}

STATIC_OVL void
writeentry(rfile,tt)
FILE *rfile;
struct toptenentry *tt;
{
#ifdef NO_SCAN_BRACK
	nsb_mung_line(tt->name);
	nsb_mung_line(tt->death);
	                   /* Version_ Pts DgnLevs_ Hp___ Died__Born id */
	(void) fprintf(rfile,"%d %d %d %ld %d %d %d %d %d %d %ld %ld %d ",
#else
	(void) fprintf(rfile,"%d.%d.%d %ld %d %d %d %d %d %d %ld %ld %d ",
#endif
		tt->ver_major, tt->ver_minor, tt->patchlevel,
		tt->points, tt->deathdnum, tt->deathlev,
		tt->maxlvl, tt->hp, tt->maxhp, tt->deaths,
		tt->deathdate, tt->birthdate, tt->uid);
	if (tt->ver_major < 3 ||
			(tt->ver_major == 3 && tt->ver_minor < 3))
#ifdef NO_SCAN_BRACK
		(void) fprintf(rfile,"%c%c %s %s\n",
#else
		(void) fprintf(rfile,"%c%c %s,%s\n",
#endif
			tt->plrole[0], tt->plgend[0],
			onlyspace(tt->name) ? "_" : tt->name, tt->death);
	else
#ifdef NO_SCAN_BRACK
		(void) fprintf(rfile,"%s %s %s %s %s %s\n",
#else
		(void) fprintf(rfile,"%s %s %s %s %s,%s\n",
#endif
			tt->plrole, tt->plrace, tt->plgend, tt->plalign,
			onlyspace(tt->name) ? "_" : tt->name, tt->death);

#ifdef NO_SCAN_BRACK
	nsb_unmung_line(tt->name);
	nsb_unmung_line(tt->death);
#endif
}

#ifdef XLOGFILE
#define SEP ":"
#define SEPC ':'

/* copy a maximum of n-1 characters from src to dest, changing ':' and '\n'
 * to '_'; always null-terminate. */
STATIC_OVL void
munge_xlstring(dest, src, n)
char *dest;
char *src;
int n;
{
  int i;

  for(i = 0; i < (n - 1) && src[i] != '\0'; i++) {
    if(src[i] == SEPC || src[i] == '\n')
      dest[i] = '_';
    else
      dest[i] = src[i];
  }

  dest[i] = '\0';

  return;
}

STATIC_OVL void
write_xlentry(rfile,tt)
FILE *rfile;
struct toptenentry *tt;
{

  char buf[DTHSZ+1];

  /* Log all of the data found in the regular logfile */
  (void)fprintf(rfile,
                "version=NDNH-%d.%d.%d"
                SEP "points=%ld"
                SEP "deathdnum=%d"
                SEP "deathlev=%d"
                SEP "maxlvl=%d"
                SEP "hp=%d"
                SEP "maxhp=%d"
                SEP "deaths=%d"
                SEP "deathdate=%ld"
                SEP "birthdate=%ld"
                SEP "uid=%d",
                tt->ver_major, tt->ver_minor, tt->patchlevel,
                tt->points, tt->deathdnum, tt->deathlev,
                tt->maxlvl, tt->hp, tt->maxhp, tt->deaths,
                tt->deathdate, tt->birthdate, tt->uid);

  (void)fprintf(rfile,
                SEP "role=%s"
                SEP "race=%s"
                SEP "gender=%s"
                SEP "align=%s",
                tt->plrole, tt->plrace, tt->plgend, tt->plalign);

  if (Race_if(PM_ENT) || Race_if(PM_HALF_DRAGON) || Race_if(PM_CLOCKWORK_AUTOMATON)) {
    (void)fprintf(rfile, SEP "species=%s", base_species_name());
  }

   munge_xlstring(buf, plname, DTHSZ + 1);
  (void)fprintf(rfile, SEP "name=%s", buf);

   munge_xlstring(buf, tt->death, DTHSZ + 1);
  (void)fprintf(rfile, SEP "death=%s", buf);

  (void)fprintf(rfile, SEP "flags=0x%lx", encode_xlogflags());

#ifdef RECORD_CONDUCT
  (void)fprintf(rfile, SEP "conduct=0x%lx", encodeconduct());
#endif

#ifdef RECORD_TURNS
  (void)fprintf(rfile, SEP "turns=%ld", moves);
#endif

#ifdef RECORD_ACHIEVE
  (void)fprintf(rfile, SEP "achieve=0x%lx", encodeachieve());
  {
	long dnethachievements = 0L;
	int i,keys=0;
	for(i=0;i<9;i++){
		if((achieve.get_keys >> i) & 1) keys++;
	}
	  if(achieve.killed_lucifer)     dnethachievements |= 1L << 0;//1
	  if(achieve.killed_asmodeus)    dnethachievements |= 1L << 1;//2
	  if(achieve.killed_demogorgon)  dnethachievements |= 1L << 2;//4
	  if(keys >= 1)					 dnethachievements |= 1L << 3;//8
	  if(keys >= 3)					 dnethachievements |= 1L << 4;//10
	  if(keys == 9)  				 dnethachievements |= 1L << 5;//20
  (void)fprintf(rfile, SEP "dnetachieve=0x%lx", dnethachievements);
  }
  {
	  char achieveXbuff[25*ACHIEVE_NUMBER] = {0};//The longest trophy name is 23 characters, +2 for the separator and null, times however many trophies.
	  writeachieveX(achieveXbuff);
  (void)fprintf(rfile, SEP "achieveX=%s", achieveXbuff);
  }
#endif

#ifdef RECORD_REALTIME
  (void)fprintf(rfile, SEP "realtime=%ld", (long)realtime_data.realtime);
#endif

#ifdef RECORD_START_END_TIME
  (void)fprintf(rfile, SEP "starttime=%ld", (long)u.ubirthday);
  (void)fprintf(rfile, SEP "endtime=%ld", (long)deathtime);
#endif

#ifdef RECORD_GENDER0
  (void)fprintf(rfile, SEP "gender0=%s", genders[flags.initgend].filecode);
#endif

#ifdef RECORD_ALIGN0
  (void)fprintf(rfile, SEP "align0=%s", 
          aligns[1 - galign(u.ugodbase[UGOD_ORIGINAL])].filecode);
#endif

  if (Race_if(PM_ENT) || Race_if(PM_HALF_DRAGON) || Race_if(PM_CLOCKWORK_AUTOMATON)) {
    (void)fprintf(rfile, SEP "species0=%s", species[flags.initspecies].name);
  }

  if (flags.descendant) {
    (void)fprintf(rfile, SEP "inherited=%s", artilist[u.inherited].name);
  }

  (void)fprintf(rfile, "\n");

}

#undef SEP
#undef SEPC
#endif /* XLOGFILE */


void
get_current_ttentry_data(t0, how)
struct toptenentry *t0;
int how;
{
	int uid = getuid();
	t0->ver_major = VERSION_MAJOR;
	t0->ver_minor = VERSION_MINOR;
	t0->patchlevel = PATCHLEVEL;
	t0->points = u.urexp;
	t0->deathdnum = u.uz.dnum;
	t0->deathlev = observable_depth(&u.uz);
	t0->maxlvl = deepest_lev_reached(TRUE);
	t0->hp = u.uhp;
	t0->maxhp = u.uhpmax;
	t0->deaths = u.umortality;
	t0->uid = uid;
	// (void) strncpy(t0->plrole, urole.filecode, ROLESZ);
	(void) strncpy(t0->plrole, code_of(urole.malenum), ROLESZ);
	t0->plrole[ROLESZ] = '\0';
	(void) strncpy(t0->plrace, urace.filecode, ROLESZ);
	t0->plrace[ROLESZ] = '\0';
	(void) strncpy(t0->plgend, genders[flags.female].filecode, ROLESZ);
	t0->plgend[ROLESZ] = '\0';
	(void) strncpy(t0->plalign, get_alignment_code(), ROLESZ);
	t0->plalign[ROLESZ] = '\0';
	(void) strncpy(t0->name, plname, NAMSZ);
	t0->name[NAMSZ] = '\0';
	t0->death[0] = '\0';
	if (how == -1) {
	    Strcat(t0->death, "hangup");
	} else {
	    switch (killer_format) {
		default: impossible("bad killer format?");
		case KILLED_BY_AN:
			Strcat(t0->death, killed_by_prefix[how]);
			(void) strncat(t0->death, an(killer),
						DTHSZ-strlen(t0->death));
			break;
		case KILLED_BY:
			Strcat(t0->death, killed_by_prefix[how]);
			(void) strncat(t0->death, killer,
						DTHSZ-strlen(t0->death));
			break;
		case NO_KILLER_PREFIX:
			(void) strncat(t0->death, killer, DTHSZ);
			break;
	    }
	}
	t0->birthdate = yyyymmdd(u.ubirthday);

#ifdef RECORD_START_END_TIME
  /* Make sure that deathdate and deathtime refer to the same time; it
   * wouldn't be good to have deathtime refer to the day after deathdate. */

#if defined(BSD) && !defined(POSIX_TYPES)
        (void) time((long *)&deathtime);
#else
        (void) time(&deathtime);
#endif

        t0->deathdate = yyyymmdd(deathtime);
#else
        t0->deathdate = yyyymmdd((time_t)0L);
#endif /* RECORD_START_END_TIME */
}

/* record into file whenever user does HUP */
void
mk_HUPfile(char *fname)
{
  if (fname[0]) {
    char new_dump_fn[512];
    Sprintf(new_dump_fn, "%s", dump_format_str(fname));

    FILE *dump_fp = fopen(new_dump_fn, "a");
    if (!dump_fp) {
    } else {
	struct toptenentry t0;
#ifdef UNIX
	mode_t dumpmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	chmod(new_dump_fn, dumpmode);
#endif
	get_current_ttentry_data(&t0, -1);
	write_xlentry(dump_fp, &t0);
	fclose(dump_fp);
    }
  }
}


void
write_HUP_file()
{
#ifdef HUPLIST_FN
    mk_HUPfile(HUPLIST_FN);
#endif
}


STATIC_OVL void
free_ttlist(tt)
struct toptenentry *tt;
{
	struct toptenentry *ttnext;

	while (tt->points > 0) {
		ttnext = tt->tt_next;
		dealloc_ttentry(tt);
		tt = ttnext;
	}
	dealloc_ttentry(tt);
}

void
topten(how)
int how;
{
	int rank, rank0 = -1, rank1 = 0;
	int occ_cnt = PERSMAX;
	register struct toptenentry *t0, *tprev;
	struct toptenentry *t1;
	FILE *rfile;
	register int flg = 0;
	boolean t0_used;
#ifdef LOGFILE
	FILE *lfile;
#endif /* LOGFILE */
#ifdef XLOGFILE
	FILE *xlfile;
#endif /* XLOGFILE */

/* Under DICE 3.0, this crashes the system consistently, apparently due to
 * corruption of *rfile somewhere.  Until I figure this out, just cut out
 * topten support entirely - at least then the game exits cleanly.  --AC
 */
#ifdef _DCC
	return;
#endif

/* If we are in the midst of a panic, cut out topten entirely.
 * topten uses alloc() several times, which will lead to
 * problems if the panic was the result of an alloc() failure.
 */
	if (program_state.panicking)
		return;

	if (flags.toptenwin) {
	    toptenwin = create_nhwindow(NHW_TEXT);
	}

#if defined(UNIX) || defined(VMS) || defined(__EMX__)
#define HUP	if (!program_state.done_hup)
#else
#define HUP
#endif

#ifdef TOS
	restore_colors();	/* make sure the screen is black on white */
#endif
	/* create a new 'topten' entry */
	t0_used = FALSE;
	t0 = newttentry();
	/* deepest_lev_reached() is in terms of depth(), and reporting the
	 * deepest level reached in the dungeon death occurred in doesn't
	 * seem right, so we have to report the death level in depth() terms
	 * as well (which also seems reasonable since that's all the player
	 * sees on the screen anyway)
	 */

	get_current_ttentry_data(t0,how);

	t0->tt_next = 0;
#ifdef UPDATE_RECORD_IN_PLACE
	t0->fpos = -1L;
#endif

#ifdef LOGFILE		/* used for debugging (who dies of what, where) */
	if (lock_file(LOGFILE, SCOREPREFIX, 10)) {
	    if(!(lfile = fopen_datafile(LOGFILE, "a", SCOREPREFIX))) {
		HUP raw_print("Cannot open log file!");
	    } else {
		writeentry(lfile, t0);
		(void) fclose(lfile);
	    }
	    unlock_file(LOGFILE);
	}
#endif /* LOGFILE */

#ifdef XLOGFILE
         if(lock_file(XLOGFILE, SCOREPREFIX, 10)) {
             if(!(xlfile = fopen_datafile(XLOGFILE, "a", SCOREPREFIX))) {
                  HUP raw_print("Cannot open extended log file!");
             } else {
                  write_xlentry(xlfile, t0);
                  (void) fclose(xlfile);
             }
             unlock_file(XLOGFILE);
         }
#endif /* XLOGFILE */

	if (wizard || discover) {
	    if (how != PANICKED) HUP {
		char pbuf[BUFSZ];
		topten_print("");
		Sprintf(pbuf,
	      "Since you were in %s mode, the score list will not be checked.",
		    wizard ? "wizard" : "discover");
		topten_print(pbuf);
#ifdef DUMP_LOG
		if (dump_fn[0]) {
		  dump("", pbuf);
		  dump("", "");
		}
#endif
	    }
	    goto showwin;
	}

	if (!lock_file(RECORD, SCOREPREFIX, 60))
		goto destroywin;

#ifdef UPDATE_RECORD_IN_PLACE
	rfile = fopen_datafile(RECORD, "r+", SCOREPREFIX);
#else
	rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
#endif

	if (!rfile) {
		HUP raw_print("Cannot open record file!");
		unlock_file(RECORD);
		goto destroywin;
	}

	HUP topten_print("");
#ifdef DUMP_LOG
	dump("", "");
#endif

	/* assure minimum number of points */
	if(t0->points < POINTSMIN) t0->points = 0;

	t1 = tt_head = newttentry();
	tprev = 0;
	/* rank0: -1 undefined, 0 not_on_list, n n_th on list */
	for(rank = 1; ; ) {
	    readentry(rfile, t1);
	    if (t1->points < POINTSMIN) t1->points = 0;
	    if(rank0 < 0 && t1->points < t0->points) {
		rank0 = rank++;
		if(tprev == 0)
			tt_head = t0;
		else
			tprev->tt_next = t0;
		t0->tt_next = t1;
#ifdef UPDATE_RECORD_IN_PLACE
		t0->fpos = t1->fpos;	/* insert here */
#endif
		t0_used = TRUE;
		occ_cnt--;
		flg++;		/* ask for a rewrite */
	    } else tprev = t1;

	    if(t1->points == 0) break;
	    if(
#ifdef PERS_IS_UID
		t1->uid == t0->uid &&
#else
		strncmp(t1->name, t0->name, NAMSZ) == 0 &&
#endif
		!strncmp(t1->plrole, t0->plrole, ROLESZ) &&
		--occ_cnt <= 0) {
		    if(rank0 < 0) {
			rank0 = 0;
			rank1 = rank;
			HUP {
			    char pbuf[BUFSZ];
			    Sprintf(pbuf,
			  "You didn't beat your previous score of %ld points.",
				    t1->points);
			    topten_print(pbuf);
			    topten_print("");
#ifdef DUMP_LOG
			    dump("", pbuf);
			    dump("", "");
#endif
			}
		    }
		    if(occ_cnt < 0) {
			flg++;
			continue;
		    }
		}
	    if(rank <= ENTRYMAX) {
		t1->tt_next = newttentry();
		t1 = t1->tt_next;
		rank++;
	    }
	    if(rank > ENTRYMAX) {
		t1->points = 0;
		break;
	    }
	}
	if(flg) {	/* rewrite record file */
#ifdef UPDATE_RECORD_IN_PLACE
		(void) fseek(rfile, (t0->fpos >= 0 ?
				     t0->fpos : final_fpos), SEEK_SET);
#else
		(void) fclose(rfile);
		if(!(rfile = fopen_datafile(RECORD, "w", SCOREPREFIX))){
			HUP raw_print("Cannot write record file");
			unlock_file(RECORD);
			free_ttlist(tt_head);
			goto destroywin;
		}
#endif	/* UPDATE_RECORD_IN_PLACE */
		if(rank0 > 0){
		    if(rank0 <= 10) {
			if(!done_stopprint) 
			topten_print("You made the top ten list!");
#ifdef DUMP_LOG
			dump("", "You made the top ten list!");
#endif
		    } else {
			char pbuf[BUFSZ];
			Sprintf(pbuf,
			  "You reached the %d%s place on the top %d list.",
				rank0, ordin(rank0), ENTRYMAX);
			if(!done_stopprint) topten_print(pbuf);
#ifdef DUMP_LOG
			dump("", pbuf);
#endif
		    }
		    if(!done_stopprint) topten_print("");
#ifdef DUMP_LOG
		    dump("", "");
#endif
		}
	}
	if(rank0 == 0) rank0 = rank1;
	if(rank0 <= 0) rank0 = rank;
	if(!done_stopprint) outheader();
	t1 = tt_head;
	for(rank = 1; t1->points != 0; rank++, t1 = t1->tt_next) {
	    if(flg
#ifdef UPDATE_RECORD_IN_PLACE
		    && rank >= rank0
#endif
		) writeentry(rfile, t1);
	    /* if (done_stopprint) continue; */
	    if (rank > flags.end_top &&
		    (rank < rank0 - flags.end_around ||
		     rank > rank0 + flags.end_around) &&
		    (!flags.end_own ||
#ifdef PERS_IS_UID
					t1->uid != t0->uid
#else
					strncmp(t1->name, t0->name, NAMSZ)
#endif
		)) continue;
	    if (rank == rank0 - flags.end_around &&
		    rank0 > flags.end_top + flags.end_around + 1 &&
		    !flags.end_own) {
		if(!done_stopprint) topten_print("");
#ifdef DUMP_LOG
		dump("", "");
#endif
	    }
	    if(rank != rank0)
		outentry(rank, t1, FALSE);
	    else if(!rank1)
		outentry(rank, t1, TRUE);
	    else {
		outentry(rank, t1, TRUE);
		outentry(0, t0, TRUE);
	    }
	}
	if(rank0 >= rank) if(!done_stopprint)
		outentry(0, t0, TRUE);
#ifdef UPDATE_RECORD_IN_PLACE
	if (flg) {
# ifdef TRUNCATE_FILE
	    /* if a reasonable way to truncate a file exists, use it */
	    truncate_file(rfile);
# else
	    /* use sentinel record rather than relying on truncation */
	    t1->points = 0L;	/* terminates file when read back in */
	    t1->ver_major = t1->ver_minor = t1->patchlevel = 0;
	    t1->uid = t1->deathdnum = t1->deathlev = 0;
	    t1->maxlvl = t1->hp = t1->maxhp = t1->deaths = 0;
	    t1->plrole[0] = t1->plrace[0] = t1->plgend[0] = t1->plalign[0] = '-';
	    t1->plrole[1] = t1->plrace[1] = t1->plgend[1] = t1->plalign[1] = 0;
	    t1->birthdate = t1->deathdate = yyyymmdd((time_t)0L);
	    Strcpy(t1->name, "@");
	    Strcpy(t1->death, "<eod>\n");
	    writeentry(rfile, t1);
	    (void) fflush(rfile);
# endif	/* TRUNCATE_FILE */
	}
#endif	/* UPDATE_RECORD_IN_PLACE */
	(void) fclose(rfile);
	unlock_file(RECORD);
	free_ttlist(tt_head);

  showwin:
	if (flags.toptenwin && !done_stopprint) display_nhwindow(toptenwin, 1);
  destroywin:
	if (!t0_used) dealloc_ttentry(t0);
	if (flags.toptenwin) {
	    destroy_nhwindow(toptenwin);
	    toptenwin=WIN_ERR;
	}
}

STATIC_OVL void
outheader()
{
	char linebuf[BUFSZ];
	register char *bp;

	Strcpy(linebuf, " No  Points     Name");
	bp = eos(linebuf);
	while(bp < linebuf + COLNO - 9) *bp++ = ' ';
	Strcpy(bp, "Hp [max]");
	if(!done_stopprint) topten_print(linebuf);
#ifdef DUMP_LOG
	dump("", linebuf);
#endif
}

/* so>0: standout line; so=0: ordinary line */
STATIC_OVL void
outentry(rank, t1, so)
struct toptenentry *t1;
int rank;
boolean so;
{
	boolean second_line = TRUE;
	char linebuf[BUFSZ];
	char *bp, hpbuf[24], linebuf3[BUFSZ];
	int hppos, lngr;


	linebuf[0] = '\0';
	if (rank) Sprintf(eos(linebuf), "%3d", rank);
	else Strcat(linebuf, "   ");

	Sprintf(eos(linebuf), " %10ld  %.10s", t1->points, t1->name);
	Sprintf(eos(linebuf), "-%s", t1->plrole);
	if (t1->plrace[0] != '?')
		Sprintf(eos(linebuf), "-%s", t1->plrace);
	/* Printing of gender and alignment is intentional.  It has been
	 * part of the NetHack Geek Code, and illustrates a proper way to
	 * specify a character from the command line.
	 */
	Sprintf(eos(linebuf), "-%s", t1->plgend);
	if (t1->plalign[0] != '?')
		Sprintf(eos(linebuf), "-%s ", t1->plalign);
	else
		Strcat(linebuf, " ");
	if (!strncmp("escaped", t1->death, 7)) {
	    Sprintf(eos(linebuf), "escaped the dungeon %s[max level %d]",
		    !strncmp(" (", t1->death + 7, 2) ? t1->death + 7 + 2 : "",
		    t1->maxlvl);
	    /* fixup for closing paren in "escaped... with...Amulet)[max..." */
	    if ((bp = index(linebuf, ')')) != 0)
		*bp = (t1->deathdnum == astral_level.dnum) ? '\0' : ' ';
	    second_line = FALSE;
	} else if (!strncmp("ascended", t1->death, 8)) {
		if (!strcmp("ascended", t1->death)) {
			Sprintf(eos(linebuf), "ascended to demigod%s-hood",
				(t1->plgend[0] == 'F') ? "dess" : "");
		}
		else {
			Sprintf(eos(linebuf), "%s", t1->death);
		}
		second_line = FALSE;
	} else {
	    if (!strncmp(t1->death, "quit", 4)) {
		Strcat(linebuf, "quit");
		second_line = FALSE;
	    } else if (!strncmp(t1->death, "died of st", 10)) {
		Strcat(linebuf, "starved to death");
		second_line = FALSE;
	    } else if (!strncmp(t1->death, "choked", 6)) {
		Sprintf(eos(linebuf), "choked on h%s food",
			(t1->plgend[0] == 'F') ? "er" : "is");
	    } else if (!strncmp(t1->death, "poisoned", 8)) {
		Strcat(linebuf, "was poisoned");
	    } else if (!strncmp(t1->death, "crushed", 7)) {
		Strcat(linebuf, "was crushed to death");
	    } else if (!strncmp(t1->death, "petrified by ", 13)) {
		Strcat(linebuf, "turned to stone");
	    } else Strcat(linebuf, "died");

	    if (t1->deathdnum == astral_level.dnum) {
		const char *arg, *fmt = " on the Plane of %s";

		switch (t1->deathlev) {
		case -5:
			fmt = " on the %s Plane";
			arg = "Astral";	break;
		case -4:
			arg = "Water";	break;
		case -3:
			arg = "Fire";	break;
		case -2:
			arg = "Air";	break;
		case -1:
			arg = "Earth";	break;
		default:
			arg = "Void";	break;
		}
		Sprintf(eos(linebuf), fmt, arg);
	    } else {
		Sprintf(eos(linebuf), " in %s", dungeons[t1->deathdnum].dname);
		if (t1->deathdnum != knox_level.dnum)
		    Sprintf(eos(linebuf), " on level %d", t1->deathlev);
		if (t1->deathlev != t1->maxlvl)
		    Sprintf(eos(linebuf), " [max %d]", t1->maxlvl);
	    }

	    /* kludge for "quit while already on Charon's boat" */
	    if (!strncmp(t1->death, "quit ", 5))
		Strcat(linebuf, t1->death + 4);
	}
	Strcat(linebuf, ".");

	/* Quit, starved, ascended, and escaped contain no second line */
	if (second_line)
	    Sprintf(eos(linebuf), "  %c%s.", highc(*(t1->death)), t1->death+1);

	lngr = (int)strlen(linebuf);
	if (t1->hp <= 0) hpbuf[0] = '-', hpbuf[1] = '\0';
	else Sprintf(hpbuf, "%d", t1->hp);
	/* beginning of hp column after padding (not actually padded yet) */
	hppos = COLNO - (sizeof("  Hp [max]")-1); /* sizeof(str) includes \0 */
	while (lngr >= hppos) {
	    for(bp = eos(linebuf);
		    !(*bp == ' ' && (bp-linebuf < hppos));
		    bp--)
		;
	    /* special case: word is too long, wrap in the middle */
	    if (linebuf+15 >= bp) bp = linebuf + hppos - 1;
	    /* special case: if about to wrap in the middle of maximum
	       dungeon depth reached, wrap in front of it instead */
	    if (bp > linebuf + 5 && !strncmp(bp - 5, " [max", 5)) bp -= 5;
	    if (*bp != ' ')
		Strcpy(linebuf3, bp);
	    else
		Strcpy(linebuf3, bp+1);
	    *bp = 0;
	    if (so) {
		while (bp < linebuf + (COLNO-1)) *bp++ = ' ';
		*bp = 0;
		if(!done_stopprint) topten_print_bold(linebuf);
#ifdef DUMP_LOG
		dump("*", linebuf[0]==' '? linebuf+1: linebuf);
#endif
	    } else {
		if(!done_stopprint) topten_print(linebuf);
#ifdef DUMP_LOG
		dump(" ", linebuf[0]==' '? linebuf+1: linebuf);
#endif
	    }
	    Sprintf(linebuf, "%15s %s", "", linebuf3);
	    lngr = strlen(linebuf);
	}
	/* beginning of hp column not including padding */
	hppos = COLNO - 7 - (int)strlen(hpbuf);
	bp = eos(linebuf);

	if (bp <= linebuf + hppos) {
	    /* pad any necessary blanks to the hit point entry */
	    while (bp < linebuf + hppos) *bp++ = ' ';
	    Strcpy(bp, hpbuf);
	    Sprintf(eos(bp), " %s[%d]",
		    (t1->maxhp < 10) ? "  " : (t1->maxhp < 100) ? " " : "",
		    t1->maxhp);
	}

	if (so) {
	    bp = eos(linebuf);
	    if (so >= COLNO) so = COLNO-1;
	    while (bp < linebuf + so) *bp++ = ' ';
	    *bp = 0;
	    if(!done_stopprint) topten_print_bold(linebuf);
	} else
	    if(!done_stopprint) topten_print(linebuf);
#ifdef DUMP_LOG
	dump(" ", linebuf[0]==' '? linebuf+1: linebuf);
#endif
}

STATIC_OVL int
score_wanted(current_ver, rank, t1, playerct, players, uid)
boolean current_ver;
int rank;
struct toptenentry *t1;
int playerct;
const char **players;
int uid;
{
	int i;

	if (current_ver && (t1->ver_major != VERSION_MAJOR ||
			    t1->ver_minor != VERSION_MINOR ||
			    t1->patchlevel != PATCHLEVEL))
		return 0;

#ifdef PERS_IS_UID
	if (!playerct && t1->uid == uid)
		return 1;
#endif

	for (i = 0; i < playerct; i++) {
	    if (players[i][0] == '-' && index("pr", players[i][1]) &&
                players[i][2] == 0 && i + 1 < playerct) {
		char *arg = (char *)players[i + 1];
		if ((players[i][1] == 'p' &&
		     str2role(arg) == str2role(t1->plrole)) ||
		    (players[i][1] == 'r' &&
		     str2race(arg) == str2race(t1->plrace)))
		    return 1;
		i++;
	    } else if (strcmp(players[i], "all") == 0 ||
		    strncmp(t1->name, players[i], NAMSZ) == 0 ||
		    (players[i][0] == '-' &&
		     players[i][1] == t1->plrole[0] &&
		     players[i][2] == 0) ||
		    (digit(players[i][0]) && rank <= atoi(players[i])))
		return 1;
	}
	return 0;
}

long
encode_xlogflags(void)
{
       long e = 0L;

       if (wizard)              e |= 0x001L; /* wizard mode */
       if (discover)            e |= 0x002L; /* explore mode */
       if (killer_flags & 0x1)  e |= 0x004L; /* died, (with the Amulet) */
       if (killer_flags & 0x2)  e |= 0x008L; /* died, (in celestial disgrace) */
       if (killer_flags & 0x4)  e |= 0x010L; /* died, (with a fake Amulet) */
       if (has_loaded_bones)    e |= 0x020L; /* has loaded bones */
       /*
       if(!u.uconduct.unvegetarian)    e |= 0x004L;
       if(!u.uconduct.gnostic)         e |= 0x008L;
       if(!u.uconduct.weaphit)         e |= 0x010L;
       if(!u.uconduct.killer)          e |= 0x020L;
       if(!u.uconduct.literate)        e |= 0x040L;
       if(!u.uconduct.polypiles)       e |= 0x080L;
       if(!u.uconduct.polyselfs)       e |= 0x100L;
       if(!u.uconduct.wishes)          e |= 0x200L;
       if(!u.uconduct.wisharti)        e |= 0x400L;
       if(!num_genocides())            e |= 0x800L;
       if(!u.uconduct.shopID)          e |= 0x1000L;
       if(!u.uconduct.IDs)             e |= 0x2000L;
       */
       return e;
}


#ifdef RECORD_CONDUCT
long
encodeconduct(void)
{
       long e = 0L;

       if(!u.uconduct.food)            e |= 0x0001L;
       if(!u.uconduct.unvegan)         e |= 0x0002L;
       if(!u.uconduct.unvegetarian)    e |= 0x0004L;
       if(!u.uconduct.gnostic)         e |= 0x0008L;
       if(!u.uconduct.weaphit)         e |= 0x0010L;
       if(!u.uconduct.killer)          e |= 0x0020L;
       if(!u.uconduct.literate)        e |= 0x0040L;
       if(!u.uconduct.polypiles)       e |= 0x0080L;
       if(!u.uconduct.polyselfs)       e |= 0x0100L;
       if(!u.uconduct.wishes)          e |= 0x0200L;
       if(!u.uconduct.wisharti)        e |= 0x0400L;
       if(!num_genocides())            e |= 0x0800L;
       if(!u.uconduct.shopID)          e |= 0x1000L;
       if(!u.uconduct.IDs)             e |= 0x2000L;

       return e;
}
#endif

#ifdef RECORD_ACHIEVE
long
encodeachieve(void)
{
  /* Achievement bitfield:
   * bit  meaning
   *  0   obtained the Bell of Opening
   *  1   entered gehennom (by any means)
   *  2   obtained the Candelabrum of Invocation
   *  3   obtained the Book of the Dead
   *  4   performed the invocation ritual
   *  5   obtained the amulet
   *  6   entered elemental planes
   *  7   entered astral plane
   *  8   ascended (not "escaped in celestial disgrace!")
   *  9   obtained the luckstone from the Mines
   *  10  obtained the sokoban prize
   *  11  killed the challenge boss
   *  DEPRICATED: 12  killed lucifer
   *  DEPRICATED: 13  killed asmodeus
   *  DEPRICATED: 14  killed demogorgon
   *  DEPRICATED: 15  obtained 3 keys
   *  DEPRICATED: 16  obtained 9 keys
   */

  long r;
  r = 0L;

  if(achieve.get_bell)           r |= 1L << 0;
  if(achieve.enter_gehennom)     r |= 1L << 1;
  if(achieve.get_candelabrum)    r |= 1L << 2;
  if(achieve.get_book)           r |= 1L << 3;
  if(achieve.perform_invocation) r |= 1L << 4;
  if(achieve.get_amulet)         r |= 1L << 5;
  if(In_endgame(&u.uz) || In_void(&u.uz))          r |= 1L << 6;
  if(Is_astralevel(&u.uz) || In_void(&u.uz))       r |= 1L << 7;
  if(achieve.ascended)           r |= 1L << 8;
  if(achieve.get_luckstone)      r |= 1L << 9;
  if(achieve.finish_sokoban)     r |= 1L << 10;
  if(achieve.killed_challenge)   r |= 1L << 11;
  
  return r;
}
#endif


#ifdef RECORD_ACHIEVE
#define	CHECK_ACHIEVE(aflag, string) \
	if(achieve.trophies&aflag){\
		Sprintf(eos(achieveXbuff), "%s%s", seperator, string);\
		seperator[0] = ',';\
		achievesWritten++;\
	}

void
writeachieveX(achieveXbuff)
char *achieveXbuff;
{
	//achieveXbuff is 25*ACHIEVE_NUMBER chars long. 25 is > than the average length of a trophy name, but keep an eye on that too.
	char seperator[2] = "";
	int achievesWritten = 0;
	CHECK_ACHIEVE(ARC_QUEST,"archeologist_quest")
	CHECK_ACHIEVE(CAV_QUEST,"caveman_quest")
	CHECK_ACHIEVE(CON_QUEST,"convict_quest")
	CHECK_ACHIEVE(KNI_QUEST,"knight_quest")
	CHECK_ACHIEVE(HEA_QUEST,"healer_quest")
	CHECK_ACHIEVE(ANA_QUEST,"anachrononaut_quest")
	CHECK_ACHIEVE(AND_QUEST,"android_quest")
	CHECK_ACHIEVE(ANA_ASC,"anachrononaut_ascension")
	CHECK_ACHIEVE(BIN_QUEST,"binder_quest")
	CHECK_ACHIEVE(BIN_ASC,"binder_ascension")
	CHECK_ACHIEVE(PIR_QUEST,"pirate_quest")
	CHECK_ACHIEVE(BRD_QUEST,"bard_quest")
	CHECK_ACHIEVE(NOB_QUEST,"base_noble_quest")
	CHECK_ACHIEVE(MAD_QUEST,"madman_quest")
	CHECK_ACHIEVE(MONK_QUEST,"monk_quest")
	CHECK_ACHIEVE(UH_QUEST,"undead_hunter_quest")
	CHECK_ACHIEVE(UH_ASC,"undead_hunter_ascension")
	CHECK_ACHIEVE(HDR_NOB_QUEST,"hedrow_noble_quest")
	CHECK_ACHIEVE(HDR_SHR_QUEST,"hedrow_shared_quest")
	CHECK_ACHIEVE(DRO_NOB_QUEST,"drow_noble_quest")
	CHECK_ACHIEVE(DRO_SHR_QUEST,"drow_shared_quest")
	CHECK_ACHIEVE(DRO_HEA_QUEST,"drow_healer_quest")
	CHECK_ACHIEVE(DWA_NOB_QUEST,"dwarf_noble_quest")
	CHECK_ACHIEVE(DWA_KNI_QUEST,"dwarf_knight_quest")
	CHECK_ACHIEVE(GNO_RAN_QUEST,"gnome_ranger_quest")
	CHECK_ACHIEVE(ELF_SHR_QUEST,"elf_shared_quest")
	CHECK_ACHIEVE(FEM_DRA_NOB_QUEST,"painted_quest")
	CHECK_ACHIEVE(CLOCK_ASC,"clockwork_ascension")
	CHECK_ACHIEVE(CHIRO_ASC,"chiropteran_ascension")
	CHECK_ACHIEVE(YUKI_ASC,"yuki_onna_ascension")
	CHECK_ACHIEVE(HALF_ASC,"half_dragon_ascension")
	CHECK_ACHIEVE(LAW_QUEST,"law_quest")
	CHECK_ACHIEVE(NEU_QUEST,"neutral_quest")
	CHECK_ACHIEVE(CHA_QUEST,"chaos_temple_quest")
	CHECK_ACHIEVE(MITH_QUEST,"mithardir_quest")
	CHECK_ACHIEVE(MORD_QUEST,"mordor_quest")
	CHECK_ACHIEVE(SECOND_THOUGHTS,"second_thoughts")
	CHECK_ACHIEVE(LAMASHTU_KILL,"lamashtu_kill")
	CHECK_ACHIEVE(BAALPHEGOR_KILL,"baalphegor_kill")
	CHECK_ACHIEVE(ANGEL_VAULT,"angel_hell_vault")
	CHECK_ACHIEVE(ANCIENT_VAULT,"ancient_hell_vault")
	CHECK_ACHIEVE(TANNINIM_VAULT,"tanninim_hell_vault")
	CHECK_ACHIEVE(DEMON_VAULT,"demon_hell_vault")
	CHECK_ACHIEVE(DEVIL_VAULT,"devil_hell_vault")
	CHECK_ACHIEVE(UNKNOWN_WISH,"unknown_god_wish")
	CHECK_ACHIEVE(CASTLE_WISH,"castle_wish")
	CHECK_ACHIEVE(ILLUMIAN,"illuminated")
	CHECK_ACHIEVE(RESCUE,"exodus")
	CHECK_ACHIEVE(FULL_LOADOUT,"fully_upgraded")
	CHECK_ACHIEVE(NIGHTMAREHUNTER,"hunter_of_nightmares")
	CHECK_ACHIEVE(SPEED_PHASE,"two_keys")
	CHECK_ACHIEVE(QUITE_MAD,"quite_mad")
	CHECK_ACHIEVE(TOTAL_DRUNK,"booze_hound")
	CHECK_ACHIEVE(BOKRUG_QUEST,"bokrug_ascension")
	CHECK_ACHIEVE(IEA_UPGRADES,"iea_upgraded")
	if(achievesWritten > 0) Sprintf(eos(achieveXbuff), ",");
	if(achieve.get_kroo)   Sprintf(eos(achieveXbuff), "%s,", "get_kroo");
	if(achieve.get_raggo)   Sprintf(eos(achieveXbuff), "%s,", "get_raggo");
	if(achieve.get_poplar)   Sprintf(eos(achieveXbuff), "%s,", "get_poplar");
	if(achieve.get_abominable)   Sprintf(eos(achieveXbuff), "%s,", "get_abominable");
	if(achieve.get_gilly)   Sprintf(eos(achieveXbuff), "%s,", "get_gilly");
	if(achieve.did_unknown)   Sprintf(eos(achieveXbuff), "%s,", "did_unknown");
	if(achieve.killed_illurien)   Sprintf(eos(achieveXbuff), "%s,", "killed_illurien");
	if(achieve.get_skey && achieve.get_ckey)   Sprintf(eos(achieveXbuff), "%s,", "pain_duo");
	if(achieve.used_smith) Sprintf(eos(achieveXbuff), "%s,", "used_smith");
	if(achieve.max_punch) Sprintf(eos(achieveXbuff), "%s,", "max_punch");
	if(achieve.garnet_spear) Sprintf(eos(achieveXbuff), "%s,", "garnet_spear");
	if(achieve.inked_up) Sprintf(eos(achieveXbuff), "%s,", "inked_up");
	if(achieve.new_races) Sprintf(eos(achieveXbuff), "%s,", "new_races");
}
#undef CHECK_ACHIEVE
#endif

/*
 * print selected parts of score list.
 * argc >= 2, with argv[0] untrustworthy (directory names, et al.),
 * and argv[1] starting with "-s".
 */
void
prscore(argc,argv)
int argc;
char **argv;
{
	const char **players;
	int playerct, rank;
	boolean current_ver = TRUE, init_done = FALSE;
	register struct toptenentry *t1;
	FILE *rfile;
	boolean match_found = FALSE;
	register int i;
	char pbuf[BUFSZ];
	int uid = -1;
#ifndef PERS_IS_UID
	const char *player0;
#endif

	if (argc < 2 || strncmp(argv[1], "-s", 2)) {
		raw_printf("prscore: bad arguments (%d)", argc);
		return;
	}

	rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
	if (!rfile) {
		raw_print("Cannot open record file!");
		return;
	}

#ifdef	AMIGA
	{
	    extern winid amii_rawprwin;
	    init_nhwindows(&argc, argv);
	    amii_rawprwin = create_nhwindow(NHW_TEXT);
	}
#endif

	/* If the score list isn't after a game, we never went through
	 * initialization. */
	if (wiz1_level.dlevel == 0) {
		dlb_init();
		init_dungeons();
		init_done = TRUE;
	}

	if (!argv[1][2]){	/* plain "-s" */
		argc--;
		argv++;
	} else	argv[1] += 2;

	if (argc > 1 && !strcmp(argv[1], "-v")) {
		current_ver = FALSE;
		argc--;
		argv++;
	}

	if (argc <= 1) {
#ifdef PERS_IS_UID
		uid = getuid();
		playerct = 0;
		players = (const char **)0;
#else
		player0 = plname;
		if (!*player0)
# ifdef AMIGA
			player0 = "all";	/* single user system */
# else
			player0 = "hackplayer";
# endif
		playerct = 1;
		players = &player0;
#endif
	} else {
		playerct = --argc;
		players = (const char **)++argv;
	}
	raw_print("");

	t1 = tt_head = newttentry();
	for (rank = 1; ; rank++) {
	    readentry(rfile, t1);
	    if (t1->points == 0) break;
	    if (!match_found &&
		    score_wanted(current_ver, rank, t1, playerct, players, uid))
		match_found = TRUE;
	    t1->tt_next = newttentry();
	    t1 = t1->tt_next;
	}

	(void) fclose(rfile);
	if (init_done) {
	    free_dungeons();
	    dlb_cleanup();
	}

	if (match_found) {
	    outheader();
	    t1 = tt_head;
	    for (rank = 1; t1->points != 0; rank++, t1 = t1->tt_next) {
		if (score_wanted(current_ver, rank, t1, playerct, players, uid))
		    (void) outentry(rank, t1, 0);
	    }
	} else {
	    Sprintf(pbuf, "Cannot find any %sentries for ",
				current_ver ? "current " : "");
	    if (playerct < 1) Strcat(pbuf, "you.");
	    else {
		if (playerct > 1) Strcat(pbuf, "any of ");
		for (i = 0; i < playerct; i++) {
		    /* stop printing players if there are too many to fit */
		    if (strlen(pbuf) + strlen(players[i]) + 2 >= BUFSZ) {
			if (strlen(pbuf) < BUFSZ-4) Strcat(pbuf, "...");
			else Strcpy(pbuf+strlen(pbuf)-4, "...");
			break;
		    }
		    Strcat(pbuf, players[i]);
		    if (i < playerct-1) {
			if (players[i][0] == '-' &&
			    index("pr", players[i][1]) && players[i][2] == 0)
			    Strcat(pbuf, " ");
			else Strcat(pbuf, ":");
		    }
		}
	    }
	    raw_print(pbuf);
	    raw_printf("Usage: %s -s [-v] <playertypes> [maxrank] [playernames]",

			 hname);
	    raw_printf("Player types are: [-p role] [-r race]");
	}
	free_ttlist(tt_head);
#ifdef	AMIGA
	{
	    extern winid amii_rawprwin;
	    display_nhwindow(amii_rawprwin, 1);
	    destroy_nhwindow(amii_rawprwin);
	    amii_rawprwin = WIN_ERR;
	}
#endif
}

STATIC_OVL int
classmon(plch, fem)
	char *plch;
	boolean fem;
{
	int i;

	/* Look for this role in the role table */
	for (i = 0; roles[i].name.m; i++)
	    if (!strncmp(plch, roles[i].filecode, ROLESZ)) {
		if (fem && roles[i].femalenum != NON_PM)
		    return roles[i].femalenum;
		else if (roles[i].malenum != NON_PM)
		    return roles[i].malenum;
		else
		    return PM_HUMAN;
	    }
	if (!strcmp(plch, "Elf")) return PM_ELF;
	if (!strcmp(plch, "Dro")) return PM_DROW_MATRON;
	if (!strcmp(plch, "Hdr")) return PM_HEDROW_WARRIOR;
	if (!strcmp(plch, "Dna")) return PM_DWARF_LORD;
	if (!strcmp(plch, "Dnb")) return PM_DWARF_LORD;
	if (!strcmp(plch, "Ndr")) return PM_DROW_MATRON;
	if (!strcmp(plch, "Nhd")) return PM_HEDROW_WARRIOR;
	/* this might be from a 3.2.x score for former Elf class */
	if (!strcmp(plch, "E")) return PM_RANGER;

	impossible("What weird role is this? (%s)", plch);
	return (PM_HUMAN_MUMMY);
}

/*
 * Get a random player name and class from the high score list,
 * and attach them to an object (for statues or morgue corpses).
 */
struct obj *
tt_oname(otmp)
struct obj *otmp;
{
	int rank;
	register int i;
	register struct toptenentry *tt;
	FILE *rfile;
	struct toptenentry tt_buf;

	if (!otmp) return((struct obj *) 0);

	rfile = fopen_datafile(RECORD, "r", SCOREPREFIX);
	if (!rfile) {
		impossible("Cannot open record file!");
		return (struct obj *)0;
	}

	tt = &tt_buf;
	rank = rnd(1000);
pickentry:
	for(i = rank; i; i--) {
	    readentry(rfile, tt);
	    if(tt->points == 0) break;
	}

	if(tt->points == 0) {
		if(rank > 1) {
			rank = 1;
			rewind(rfile);
			goto pickentry;
		}
		otmp = (struct obj *) 0;
	} else {
		/* reset timer in case corpse started out as lizard or troll */
		if (otmp->otyp == CORPSE) stop_all_timers(otmp->timed);
		otmp->corpsenm = classmon(tt->plrole, (tt->plgend[0] == 'F'));
		otmp->owt = weight(otmp);
		otmp = oname(otmp, tt->name);
		if (otmp->otyp == CORPSE) start_corpse_timeout(otmp);
	}

	(void) fclose(rfile);
	return otmp;
}

#ifdef NO_SCAN_BRACK
/* Lattice scanf isn't up to reading the scorefile.  What */
/* follows deals with that; I admit it's ugly. (KL) */
/* Now generally available (KL) */
STATIC_OVL void
nsb_mung_line(p)
	char *p;
{
	while ((p = index(p, ' ')) != 0) *p = '|';
}

STATIC_OVL void
nsb_unmung_line(p)
	char *p;
{
	while ((p = index(p, '|')) != 0) *p = ' ';
}
#endif /* NO_SCAN_BRACK */

/*topten.c*/
