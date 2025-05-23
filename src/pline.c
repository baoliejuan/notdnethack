/*	SCCS Id: @(#)pline.c	3.4	1999/11/28	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#define NEED_VARARGS /* Uses ... */	/* comment line for pre-compiled headers */
#include <math.h>
#include "hack.h"
#include "hashmap.h"

#ifdef OVLB

static boolean no_repeat = FALSE;

static char *FDECL(You_buf, (int));

#if defined(DUMP_LOG) && defined(DUMPMSGS)
char msgs[DUMPMSGS][BUFSZ];
int lastmsg = -1;
#endif

void
msgpline_add(typ, pattern)
     int typ;
     char *pattern;
{
    int errnum;
    char errbuf[80];
    const char *err = (char *)0;
    struct _plinemsg *tmp = (struct _plinemsg *) alloc(sizeof(struct _plinemsg));
    if (!tmp) return;
    tmp->msgtype = typ;
    tmp->is_regexp = iflags.msgtype_regex;
    if (tmp->is_regexp) {
	errnum = regcomp(&tmp->match, pattern, REG_EXTENDED | REG_NOSUB);
	if (errnum != 0) {
	    regerror(errnum, &tmp->match, errbuf, sizeof(errbuf));
	    err = errbuf;
	}
	if (err) {
	    raw_printf("\nMSGTYPE regex error: %s\n", err);
	    wait_synch();
	    free(tmp);
	    return;
	}
    } else {
	tmp->pattern = strdup(pattern);
    }
    tmp->next = pline_msg;
    pline_msg = tmp;
}

void
msgpline_free()
{
    struct _plinemsg *tmp = pline_msg;
    struct _plinemsg *tmp2;
    while (tmp) {
	if (tmp->is_regexp) {
	    (void) regfree(&tmp->match);
	} else {
	    free(tmp->pattern);
	}
	tmp2 = tmp;
	tmp = tmp->next;
	free(tmp2);
    }
    pline_msg = NULL;
}

int
msgpline_type(msg)
     const char *msg;
{
    struct _plinemsg *tmp = pline_msg;
    while (tmp) {
	if (tmp->is_regexp) {
	    if (regexec(&tmp->match, msg, 0, NULL, 0) == 0) return tmp->msgtype;
	} else {
	    if (pmatch(tmp->pattern, msg)) return tmp->msgtype;
	}
	tmp = tmp->next;
    }
    return MSGTYP_NORMAL;
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */
const char * FDECL(replace, (const char *, const char *, const char *));

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vpline, (const char *, va_list));

void
pline VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vpline(line, VA_ARGS);
	VA_END();
}

char prevmsg[BUFSZ];

# ifdef USE_STDARG
static void
vpline(const char *line, va_list the_args) {
# else
static void
vpline(line, the_args) const char *line; va_list the_args; {
# endif

#else	/* USE_STDARG | USE_VARARG */

#define vpline pline

void
pline VA_DECL(const char *, line)
#endif	/* USE_STDARG | USE_VARARG */

	char pbuf[BUFSZ];
	int typ;
/* Do NOT use VA_START and VA_END in here... see above */

	if (!line || !*line) return;
	if (index(line, '%')) {
	    Vsprintf(pbuf,line,VA_ARGS);
	    line = pbuf;
	}
	if(Role_if(PM_PIRATE)){/*Ben Collver's fixes*/
//		line = piratesay(line);
	}
#if defined(DUMP_LOG) && defined(DUMPMSGS)
	if (DUMPMSGS > 0 && !program_state.gameover) {
	  lastmsg = (lastmsg + 1) % DUMPMSGS;
	  strncpy(msgs[lastmsg], line, BUFSZ);
	}
#endif
	typ = msgpline_type(line);
	if (!iflags.window_inited) {
	    raw_print(line);
	    return;
	}
#ifndef MAC
	if (no_repeat && !strcmp(line, toplines))
	    return;
#endif /* MAC */
	if (vision_full_recalc) vision_recalc(0);
	if (u.ux) flush_screen(1);		/* %% */
	if (typ == MSGTYP_NOSHOW) return;
	if (typ == MSGTYP_NOREP && !strcmp(line, prevmsg)) return;
	putstr(WIN_MESSAGE, 0, line);
	strncpy(prevmsg, line, BUFSZ);
	if (typ == MSGTYP_STOP) display_nhwindow(WIN_MESSAGE, TRUE); /* --more-- */
}

/*VARARGS1*/
void
Norep VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, const char *);
	no_repeat = TRUE;
	vpline(line, VA_ARGS);
	no_repeat = FALSE;
	VA_END();
	return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char *
You_buf(siz)
int siz;
{
	if (siz > you_buf_siz) {
		if (you_buf) free((genericptr_t) you_buf);
		you_buf_siz = siz + 10;
		you_buf = (char *) alloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

void
free_youbuf()
{
	if (you_buf) free((genericptr_t) you_buf),  you_buf = (char *)0;
	you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 Strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void
You VA_DECL(const char *, line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
Your VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "Your ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_feel VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You feel ", line), VA_ARGS);
	VA_END();
}


/*VARARGS1*/
void
You_cant VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "You can't ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
pline_The VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "The ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
There VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	vpline(YouMessage(tmp, "There ", line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
You_hear VA_DECL(const char *,line)
	char *tmp;
	VA_START(line);
	VA_INIT(line, const char *);
	if (Underwater)
		YouPrefix(tmp, "You barely hear ", line);
	else if (u.usleep)
		YouPrefix(tmp, "You dream that you hear ", line);
	else
		YouPrefix(tmp, "You hear ", line);
	vpline(strcat(tmp, line), VA_ARGS);
	VA_END();
}

/*VARARGS1*/
void
verbalize VA_DECL(const char *,line)
	char *tmp;
	if (!flags.soundok) return;
	VA_START(line);
	VA_INIT(line, const char *);
	tmp = You_buf((int)strlen(line) + sizeof "\"\"");
	Strcpy(tmp, "\"");
	Strcat(tmp, line);
	Strcat(tmp, "\"");
	vpline(tmp, VA_ARGS);
	VA_END();
}

/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

#if defined(USE_STDARG) || defined(USE_VARARGS)
static void FDECL(vraw_printf,(const char *,va_list));

void
raw_printf VA_DECL(const char *, line)
	VA_START(line);
	VA_INIT(line, char *);
	vraw_printf(line, VA_ARGS);
	VA_END();
}

# ifdef USE_STDARG
static void
vraw_printf(const char *line, va_list the_args) {
# else
static void
vraw_printf(line, the_args) const char *line; va_list the_args; {
# endif

#else  /* USE_STDARG | USE_VARARG */

void
raw_printf VA_DECL(const char *, line)
#endif
/* Do NOT use VA_START and VA_END in here... see above */

	if(!index(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    Vsprintf(pbuf,line,VA_ARGS);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void
impossible VA_DECL(const char *, s)
	char pbuf[2*BUFSZ];
	VA_START(s);
	VA_INIT(s, const char *);
	if (program_state.in_impossible)
		panic("impossible called impossible");
	program_state.in_impossible = 1;
	Vsprintf(pbuf,s,VA_ARGS);
	pbuf[BUFSZ-1] = '\0';
	paniclog("impossible", pbuf);
	if (iflags.debug_fuzzer)
		panic("%s", pbuf);
	pline("%s", pbuf);
	pline("Program in disorder - you should probably S)ave and reload the game.");
	program_state.in_impossible = 0;
	VA_END();
}

const char * const hallu_alignments[] = {
	"trinitarian",
	"sectarian",
	
	"radishes",
	"binary",
	
	"evil",
	"really evil",
	"omnicidal",
	"mad",
	"crazy",
	"loud",
	"hungry",
	"greedy",
	
	"cuddly",
	"funky",
	"chill",
	"relaxed",
	"drunk",
	"curious",
	"bi-curious",
	"magnificent",
	"cool",
	
	"chilly",
	"standoffish",
	
	"Jello",
	"Spam",
	"tastes like orange Tang",
	"doesn't taste like orange Tang",
	"butterscotch", 
	"cinnamon",
	
	"currently not available",
	"gone swimming",
	"not listening",
	"smashing things",
	"thinking",
	
	"jumble",
	"crossword",

	"yellow",
	"purple",
	"orange",
	"blue"
};

const char *
align_str(alignment)
    aligntyp alignment;
{
	if (Hallucination) {
		return hallu_alignments[rn2(SIZE(hallu_alignments))];
	}
	if(Role_if(PM_EXILE) && Is_astralevel(&u.uz)){
		switch ((int)alignment) {
		case A_CHAOTIC: return "unaligned";
		case A_NEUTRAL: return "gnostic";
		case A_LAWFUL:	return "mundane";
		case A_NONE:	return "unaligned";
		case A_VOID:	return "gnostic";
//		case A_UNKNOWN:	return "unknown";
		}
    } else if(Role_if(PM_EXILE) && In_quest(&u.uz)){
		switch ((int)alignment) {
		case A_CHAOTIC: return "chaotic";
		case A_NEUTRAL: return "neutral";
		case A_LAWFUL:	return "lawful";
		case A_NONE:	return "mundane";
		case A_VOID:	return "gnostic";
//		case A_UNKNOWN:	return "unknown";
		}
    } else {
		switch ((int)alignment) {
		case A_CHAOTIC: return "chaotic";
		case A_NEUTRAL: return "neutral";
		case A_LAWFUL:	return "lawful";
		case A_NONE:	return "unaligned";
		case A_VOID:	return "gnostic";
//		case A_UNKNOWN:	return "unknown";
		}
	}
    return "unknown";
}

const char *
align_str_proper(alignment)
    aligntyp alignment;
{
	if(Role_if(PM_EXILE) && Is_astralevel(&u.uz)){
		switch ((int)alignment) {
		case A_CHAOTIC: return "Unaligned";
		case A_NEUTRAL: return "Gnostic";
		case A_LAWFUL:	return "Mundane";
		case A_NONE:	return "Unaligned";
		case A_VOID:	return "Gnostic";
//		case A_UNKNOWN:	return "Unknown";
		}
    } else if(Role_if(PM_EXILE) && In_quest(&u.uz)){
		switch ((int)alignment) {
		case A_CHAOTIC: return "Chaotic";
		case A_NEUTRAL: return "Neutral";
		case A_LAWFUL:	return "Lawful";
		case A_NONE:	return "Mundane";
		case A_VOID:	return "Gnostic";
//		case A_UNKNOWN:	return "Unknown";
		}
    } else {
		switch ((int)alignment) {
		case A_CHAOTIC: return "Chaotic";
		case A_NEUTRAL: return "Neutral";
		case A_LAWFUL:	return "Lawful";
		case A_NONE:	return "Unaligned";
		case A_VOID:	return "Gnostic";
//		case A_UNKNOWN:	return "Unknown";
		}
	}
    return "Unknown";
}

#define mslotdrtotal(slot)	\
	mon_slot_dr(mtmp, (struct monst *) 0, slot, &base, &armac, &nat_dr, 0);\
	\
	if(armac > 11) armac = (armac-10)/2 + 10;\
	\
	if(nat_dr && armac){\
		base += sqrt(nat_dr*nat_dr + armac*armac);\
	} else if(nat_dr){\
		base += nat_dr;\
	} else {\
		base += armac;\
	}

void
mdrslotline(mtmp)
register struct monst *mtmp;
{
	int slot;
	int base, nat_dr, armac;
	char mbuf[BUFSZ] = {'\0'};
	struct permonst *mdat = mtmp->data;
	winid en_win;
	en_win = create_nhwindow(NHW_MENU);
	putstr(en_win, 0, "Monster Damage Reduction:");
	putstr(en_win, 0, "");
	
	if(!has_head_mon(mtmp)){
		Sprintf(mbuf, "No head; shots hit upper body");
		putstr(en_win, 0, mbuf);
	} else {
		mslotdrtotal(HEAD_DR);
		Sprintf(mbuf, "Head Armor:       %d", base);
		putstr(en_win, 0, mbuf);
	}
	mslotdrtotal(UPPER_TORSO_DR);
	Sprintf(mbuf, "Upper Body Armor: %d", base);
	putstr(en_win, 0, mbuf);
	mslotdrtotal(LOWER_TORSO_DR);
	Sprintf(mbuf, "Lower Body Armor: %d", base);
	putstr(en_win, 0, mbuf);
	if(!can_wear_gloves(mdat)){
		Sprintf(mbuf, "No hands; shots hit upper body");
		putstr(en_win, 0, mbuf);
	} else {
		mslotdrtotal(ARM_DR);
		Sprintf(mbuf, "Hand Armor:       %d", base);
		putstr(en_win, 0, mbuf);
	}
	if(!can_wear_boots(mdat)){
		Sprintf(mbuf, "No feet; shots hit lower body");
		putstr(en_win, 0, mbuf);
	} else {
		mslotdrtotal(LEG_DR);
		Sprintf(mbuf, "Foot Armor:       %d", base);
		putstr(en_win, 0, mbuf);
	}
	
	display_nhwindow(en_win, TRUE);
	destroy_nhwindow(en_win);
	return;
}

void
mstatusline(mtmp)
register struct monst *mtmp;
{
	aligntyp alignment;
	char info[BUFSZ], monnambuf[BUFSZ];

	if (get_mx(mtmp, MX_EPRI))
		alignment = EPRI(mtmp)->shralign;
	else if (get_mx(mtmp, MX_EMIN))
		alignment = EMIN(mtmp)->min_align;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
	//This comes up often enough for debug that it's worth it.
	if (!is_neuter(mtmp->data)) {
		if (mtmp->female) Strcat(info, ", female");
		else Strcat(info, ", male");
	}
	if (mtmp->mtame) {	  Strcat(info, ", tame");
#ifdef WIZARD
	    if (wizard) {
		Sprintf(eos(info), " (%d", mtmp->mtame);
		if (get_mx(mtmp, MX_EDOG))
		    Sprintf(eos(info), "; hungry %ld; apport %d",
			EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
		Strcat(info, ")");
	    }
#endif
	}
	else if (mtmp->mpeaceful){
		if(mtmp->mtyp == PM_UVUUDAUM) Strcat(info, ", in contemplative meditation");
		else Strcat(info, ", peaceful");
	}
	else if (mtmp->mtyp==PM_DREAD_SERAPH && mtmp->mvar_dreadPrayer_progress)  Strcat(info, ", in prayer");
	else if (mtmp->mtraitor)  Strcat(info, ", traitor");
	else if (mtmp->mferal)  Strcat(info, ", feral");
	if (mtmp->meating)	  Strcat(info, ", eating");
	if (mtmp->mcan)		  Strcat(info, ", cancelled");
	if (mtmp->mconf)	  Strcat(info, ", confused");
	if (mtmp->mcrazed)	  Strcat(info, ", crazed");
	if (mtmp->mberserk)	  Strcat(info, ", berserk");
	if (mon_healing_penalty(mtmp))	  Strcat(info, ", itchy");
	if (mtmp->mblinded || !mtmp->mcansee)
				  Strcat(info, ", blind");
	else if(is_blind(mtmp)) Strcat(info, ", dazzled");
	if (mtmp->mstun)	  Strcat(info, ", stunned");
	if (mtmp->msleeping)	  Strcat(info, ", asleep");
	if (mtmp->mstdy > 0)	  Sprintf(eos(info), ", vulnerable (%d)", mtmp->mstdy);
	if (mtmp->encouraged != 0)	  Sprintf(eos(info), ", morale (%d)", mtmp->encouraged);
	else if (mtmp->mstdy < 0)	  Sprintf(eos(info), ", protected (%d)", mtmp->mstdy);
#if 0	/* unfortunately mfrozen covers temporary sleep and being busy
	   (donning armor, for instance) as well as paralysis */
	else if (mtmp->mfrozen)	  Strcat(info, ", paralyzed");
#else
	else if (mtmp->mfrozen || !mtmp->mcanmove)
				  Strcat(info, ", can't move");
#endif
	else if (mtmp->mlaughing || !mtmp->mnotlaugh)
				  Strcat(info, is_silent_mon(mtmp) ? ", shaking uncontrollably" : ", laughing hysterically");
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
				  Strcat(info, ", meditating");
	else if (mtmp->mflee && mtmp->mtyp != PM_BANDERSNATCH) Strcat(info, ", scared");
	if (mtmp->mtrapped)	  Strcat(info, ", trapped");
	if (mtmp->mspeed)	  Strcat(info,
					mtmp->mspeed == MFAST ? ", fast" :
					mtmp->mspeed == MSLOW ? ", slow" :
					", ???? speed");
	if (mtmp->mundetected)	  Strcat(info, ", concealed");
	if (mtmp->minvis)	  Strcat(info, ", invisible");
	if (mtmp == u.ustuck)	  Strcat(info,
			(sticks(&youmonst)) ? ", held by you" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				", swallowed you" :
				", engulfed you") :
				", holding you");
#ifdef STEED
	if (mtmp == u.usteed)	  Strcat(info, ", carrying you");
#endif

	/* avoid "Status of the invisible newt ..., invisible" */
	/* and unlike a normal mon_nam, use "saddled" even if it has a name */
	Strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, (char *)0,
	    (SUPPRESS_IT|SUPPRESS_INVISIBLE), FALSE));

	pline("Status of %s (%s):  Level %d  HP %d(%d)  AC %d  DR %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		full_mac(mtmp),
		avg_mdr(mtmp),
		info);
}

void
ustatusline()
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Invulnerable)		Strcat(info, ", invulnerable");
	if (Sick) {
		Strcat(info, ", dying from");
		if (u.usick_type & SICK_VOMITABLE)
			Strcat(info, " food poisoning");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				Strcat(info, " and");
			Strcat(info, " illness");
		}
	}
	if (Stoned)		Strcat(info, ", solidifying");
	if (Golded)		Strcat(info, ", aurelifying");
	if (Golded)		Strcat(info, ", salifying");
	if (Slimed)		Strcat(info, ", becoming slimy");
	if (BloodDrown)		Strcat(info, ", drowning");
	if (FrozenAir)		Strcat(info, ", can't breath");
	if (Strangled)		Strcat(info, ", being strangled");
	if (Vomiting)		Strcat(info, ", nauseated"); /* !"nauseous" */
	if (Confusion)		Strcat(info, ", confused");
	if (Blind) {
	    Strcat(info, ", blind");
	    if (u.ucreamed) {
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youracedata))
		    Strcat(info, ", cover");
		Strcat(info, "ed by sticky goop");
	    }	/* note: "goop" == "glop"; variation is intentional */
	}
	if (Stunned)		Strcat(info, ", stunned");
	if (u.ustdy > 0)	  Sprintf(eos(info), ", vulnerable (%d)", u.ustdy);
	if (u.uencouraged != 0)	  Sprintf(eos(info), ", morale (%d)", u.uencouraged);
#ifdef STEED
	if (!u.usteed)
#endif
	if (Wounded_legs) {
	    const char *what = body_part(LEG);
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
				Sprintf(eos(info), ", injured %s", what);
	}
	if (Glib)		Sprintf(eos(info), ", slippery %s",
					makeplural(body_part(HAND)));
	if (u.utrap)		Strcat(info, ", trapped");
	if (Fast)		Strcat(info, Very_fast ?
						", very fast" : ", fast");
	if (u.uundetected)	Strcat(info, ", concealed");
	if (Invis)		Strcat(info, ", invisible");
	if (u.ustuck) {
	    if (sticks(&youmonst))
		Strcat(info, ", holding ");
	    else
		Strcat(info, ", held by ");
	    Strcat(info, mon_nam(u.ustuck));
	}

	pline("Status of %s (%s%s):  Level %d  HP %d(%d)  AC %d  DR %d%s.",
		plname,
		    (u.ualign.record >= 20) ? "piously " :
		    (u.ualign.record > 13) ? "devoutly " :
		    (u.ualign.record > 8) ? "fervently " :
		    (u.ualign.record > 3) ? "stridently " :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "haltingly " :
		    (u.ualign.record == 0) ? "nominally " :
					    "insufficiently ",
		align_str(u.ualign.type),
		Upolyd ? mons[u.umonnum].mlevel : u.ulevel,
		Upolyd ? u.mh : u.uhp,
		Upolyd ? u.mhmax : u.uhpmax,
		u.uac,
		u.udr,
		info);
}

void
self_invis_message()
{
	if(Role_if(PM_PIRATE)){
	pline("%s %s.",
	    Hallucination ? "Arr, Matey!  Ye" : "Avast!  All of a sudden, ye",
	    See_invisible(u.ux,u.uy) ? "can see right through yerself" :
		"can't see yerself");
	}
	else{
	pline("%s %s.",
	    Hallucination ? "Far out, man!  You" : "Gee!  All of a sudden, you",
	    See_invisible(u.ux,u.uy) ? "can see right through yourself" :
		"can't see yourself");
	}
}

const char *
replace(st, orig, repl)
const char *st, *orig, *repl;
{
	static char retval[TBUFSZ];
	char buffer[TBUFSZ];
	const char *ch, *pos;
	size_t len;
	memset(buffer, 0, TBUFSZ);
	pos = st;
	while ((ch = strstr(pos, orig))){
		len = (ch - pos);
		strncat(buffer, pos, len);
		strncat(buffer, repl, strlen(repl));
		pos = (ch + strlen(orig));
	}
	if (pos == st) {
		return st;
	}
	if (pos < (st + strlen(st))) {
		strncat(buffer, pos, (st - pos));
	}
	strcpy(retval, buffer);
	return retval;
}

/*Ben Collver's fixes*/
const char *
piratesay(orig)
const char *orig;
{
		orig = replace(orig,"You","Ye");
		orig = replace(orig,"you","ye");
		orig = replace(orig,"His","'is");
		orig = replace(orig," his"," 'is");
		orig = replace(orig,"Her","'er");
		orig = replace(orig," her"," 'er");
		orig = replace(orig,"Are","Be");
		orig = replace(orig," are"," be");
		orig = replace(orig,"Is","Be");
		orig = replace(orig," is"," be");
		orig = replace(orig,"Of","O'");
		orig = replace(orig," ear"," lug");
		orig = replace(orig," lugth"," earth");
		orig = replace(orig,"Ear","Lug");
		orig = replace(orig,"Lugth","Earth");
		orig = replace(orig," eye"," deadlight");
		orig = replace(orig,"Eye","Deadlight");
		orig = replace(orig,"zorkmid","doubloon");
		orig = replace(orig,"Zorkmid","Doubloon");
		orig = replace(orig,"gold coins","pieces of eight");
		orig = replace(orig,"Gold coins","Pieces of eight");
		orig = replace(orig,"gold coin","piece of eight");
		orig = replace(orig,"Gold coin","Piece of eight");
		orig = replace(orig,"gold pieces","pieces of eight");
		orig = replace(orig,"Gold pieces","Pieces of eight");
		orig = replace(orig,"gold piece","piece of eight");
		orig = replace(orig,"Gold piece","Piece of eight");
		return orig;
}
#endif /* OVLB */
/*pline.c*/
