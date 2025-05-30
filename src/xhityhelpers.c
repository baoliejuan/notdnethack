/*	SCCS Id: @(#)xhityhelpers.c	3.4	2003/10/20	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include <math.h>
#include "hack.h"
#include "artifact.h"
#include "monflag.h"

#include "xhity.h"

extern boolean notonhead;
extern struct attack noattack;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/* unhide creatures, possibly with message, before they make an attack */
void
xmakingattack(magr, mdef, tarx, tary)
struct monst * magr;
struct monst * mdef;
int tarx;
int tary;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

boolean
magr_can_attack_mdef(magr, mdef, tarx, tary, active)
struct monst * magr;
struct monst * mdef;
int tarx;
int tary;
boolean active;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;

	/* cases where the agressor cannot make any attacks at all */
	/* player is invincible*/
	if (youdef && Invulnerable) {
		/* monsters won't attack you */
		if (active) {
			/* only print messages if they were actively attacking you */
			if (magr == u.ustuck)
				pline("%s loosens its grip slightly.", Monnam(magr));
			else if (distmin(x(magr), y(magr), x(mdef), y(mdef)) <= 1) {
				if (canseemon(magr) || sensemon(magr))
					pline("%s starts to attack you, but pulls back.",
					Monnam(magr));
				else
					You_feel("%s move nearby.", something);
			}
		}
		return FALSE;
	}

	/* agr can't attack */
	if (cantmove(magr))
		return FALSE;

	/* agr has no line of sight to def */
	if (!clear_path(x(magr), y(magr), tarx, tary))
		return FALSE;

	/* madness only prevents your active attempts to hit things */
	if (youagr && active && madness_cant_attack(mdef))
		return FALSE;

	/* some creatures are limited in *where* they can attack */
	/* Grid bugs and Bebeliths cannot attack at an angle. */
	if ((pa->mtyp == PM_GRID_BUG || pa->mtyp == PM_BEBELITH)
		&& x(magr) != tarx && y(magr) != tary)
		return FALSE;

	/* limited attack angles (monster-agressor only) */
	if (!youagr && is_vectored_mtyp(pa->mtyp)){
		if (x(magr) + xdir[(int)magr->mvar1] != tarx ||
			y(magr) + ydir[(int)magr->mvar1] != tary)
			return FALSE;
	}

	/* Monsters can't attack a player that's underwater unless the monster can swim; asymetric */
	if (youdef && Underwater && !mon_resistance(magr, SWIMMING))
		return FALSE;

	/* Monsters can't attack a player that's swallowed unless the monster *is* u.ustuck */
	if (youdef && u.uswallow) {
		if (magr != u.ustuck)
			return FALSE;
		/* they also know exactly where you are */
		/* ...if they are making an attack action. */
		if (active) {
			u.ustuck->mux = u.ux;
			u.ustuck->muy = u.uy;
		}
		/* if you're invulnerable, you're fine though */
		if (Invulnerable)
			return FALSE; /* stomachs can't hurt you! */
	}
	/* While swallowed OR stuck, you can't attack other monsters */
	if (youagr && u.ustuck) {
		if (mdef != u.ustuck) {
			if (u.uswallow)		/* when swallowed, always not */
				return FALSE;
			else if (active)	/* when held, only active attacks aren't allowed to hit others */
				return FALSE;
		}
	}
	/* if we made it through all of that, then we can attack */
	return TRUE;
}


/* attack_checks()
 * 
 * the player is attempting to attack [mdef]
 * 
 * old behaviour: FALSE means it's OK to attack
 * 
 * new behaviour: returns
 * 0 : can not attack
 * 1 : attack normally
 * 2 : bloodthirsty forced attack (a la Stormbringer, not 'F')
 */
int
attack_checks(mdef, wep)
struct monst * mdef;
struct obj * wep;	/* uwep for attack(), null for kick_monster() */
{
	/* if you're close enough to attack, alert any waiting monster */
	mdef->mstrategy &= ~STRAT_WAITMASK;

	/* if you're overtaxed (or worse), you cannot attack */
	if (check_capacity("You cannot fight while so heavily loaded."))
		return ATTACKCHECK_NONE;

	/* certain "pacifist" monsters don't attack */
	if (Upolyd && noattacks(youracedata)) {
		You("have no way to attack monsters physically.");
		return ATTACKCHECK_NONE;
	}

	/* if you are swallowed, you can attack */
	if (u.uswallow && mdef == u.ustuck)
		return ATTACKCHECK_ATTACK;

	if (flags.forcefight) {
		/* Do this in the caller, after we checked that the monster
		* didn't die from the blow.  Reason: putting the 'I' there
		* causes the hero to forget the square's contents since
		* both 'I' and remembered contents are stored in .glyph.
		* If the monster dies immediately from the blow, the 'I' will
		* not stay there, so the player will have suddenly forgotten
		* the square's contents for no apparent reason.
		if (!canspotmon(mdef) &&
		!glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph))
		map_invisible(u.ux+u.dx, u.uy+u.dy);
		*/
		return ATTACKCHECK_ATTACK;
	}

	/* Put up an invisible monster marker, but with exceptions for
	* monsters that hide and monsters you've been warned about.
	* The former already prints a warning message and
	* prevents you from hitting the monster just via the hidden monster
	* code below; if we also did that here, similar behavior would be
	* happening two turns in a row.  The latter shows a glyph on
	* the screen, so you know something is there.
	*/
	if (!canspotmon(mdef) &&
		!glyph_is_warning(glyph_at(u.ux + u.dx, u.uy + u.dy)) &&
		!glyph_is_invisible(levl[u.ux + u.dx][u.uy + u.dy].glyph) &&
		!(!Blind && mdef->mundetected && hides_under(mdef->data))) {
		pline("Wait!  There's %s there you can't see!",
			something);
		map_invisible(u.ux + u.dx, u.uy + u.dy);
		/* if it was an invisible mimic, treat it as if we stumbled
		* onto a visible mimic
		*/
		if (mdef->m_ap_type && !Protection_from_shape_changers) {
			if (!u.ustuck && !mdef->mflee && dmgtype(mdef->data, AD_STCK))
				u.ustuck = mdef;
		}
		if (!mdef->mpeaceful)
			wakeup2(mdef, TRUE); /* always necessary; also un-mimics mimics */
		return ATTACKCHECK_NONE;
	}

	if ((mdef->m_ap_type && mdef->m_ap_type != M_AP_MONSTER) && !Protection_from_shape_changers &&
		!sensemon(mdef) &&
		!glyph_is_warning(glyph_at(u.ux + u.dx, u.uy + u.dy))) {
		/* If a hidden mimic was in a square where a player remembers
		* some (probably different) unseen monster, the player is in
		* luck--he attacks it even though it's hidden.
		*/
		if (glyph_is_invisible(levl[mdef->mx][mdef->my].glyph)) {
			seemimic(mdef);
			return ATTACKCHECK_ATTACK;
		}
		stumble_onto_mimic(mdef);
		return ATTACKCHECK_NONE;
	}

	if (mdef->mundetected && !canseemon(mdef) && !sensemon(mdef) &&
		!glyph_is_warning(glyph_at(u.ux + u.dx, u.uy + u.dy)) &&
		!MATCH_WARN_OF_MON(mdef) &&
		(hides_under(mdef->data) || is_underswimmer(mdef->data))) {
		mdef->mundetected = mdef->msleeping = 0;
		newsym(mdef->mx, mdef->my);
		if (glyph_is_invisible(levl[mdef->mx][mdef->my].glyph)) {
			seemimic(mdef);
			return ATTACKCHECK_ATTACK;
		}
		if (!(Blind ? Blind_telepat : Unblind_telepat)) {
			struct obj *obj;

			if (Blind || (is_pool(mdef->mx, mdef->my, FALSE) && !Underwater))
				pline("Wait!  There's a hidden monster there!");
			else if ((obj = level.objects[mdef->mx][mdef->my]) != 0)
				pline("Wait!  There's %s hiding under %s!",
				an(l_monnam(mdef)), doname(obj));
			return ATTACKCHECK_NONE;
		}
	}

	/*
	* make sure to wake up a monster from the above cases if the
	* hero can sense that the monster is there.
	*/
	if ((mdef->mundetected || mdef->m_ap_type) && sensemon(mdef)) {
		mdef->mundetected = 0;
		wakeup2(mdef, TRUE);
	}

	/* generally, don't attack peaceful monsters */
	if (mdef->mpeaceful && !Confusion && !Hallucination && !Stunned) {
		if (canspotmon(mdef)) {
			if (iflags.attack_mode == ATTACK_MODE_CHAT
				|| iflags.attack_mode == ATTACK_MODE_PACIFIST) {
				if (mdef->ispriest) {
					/* Prevent accidental donation prompt. */
					pline("%s mutters a prayer.", Monnam(mdef));
				}
				else if (dochat(FALSE, u.dx, u.dy, 0) & (MOVE_CANCELLED|MOVE_INSTANT)) {
					flags.move |= MOVE_INSTANT;
				}
				return ATTACKCHECK_NONE;
			}
			else if (iflags.attack_mode == ATTACK_MODE_ASK){
				char qbuf[QBUFSZ];
				Sprintf(qbuf, "Really attack %s?", mon_nam(mdef));
				if (yesno(qbuf, iflags.paranoid_hit) != 'y') {
					flags.move |= MOVE_CANCELLED;
					return ATTACKCHECK_NONE;
				}
			}
		}
	}

	/* attack checks specific to the pacifist attack mode */
	if (iflags.attack_mode == ATTACK_MODE_PACIFIST) {
		/* Being not in full control of yourself causes you to attack */
		if (Confusion || Stunned)
			return ATTACKCHECK_ATTACK;
		/* Otherwise, be a pacifist. */
		You("stop for %s.", mon_nam(mdef));
		flags.move |= MOVE_CANCELLED;
		return ATTACKCHECK_NONE;
	}

	/* default case: you can attack */
	return ATTACKCHECK_ATTACK;
}

/* madness_cant_attack()
 * 
 * returns TRUE if because of the player's madness, they cannot attack mon.
 */
boolean
madness_cant_attack(mon)
struct monst * mon;
{
	if (mon->female && humanoid_torso(mon->data) && roll_madness(MAD_SANCTITY)){
		You("can't bring yourself to strike %s!", mon_nam(mon));
		return TRUE;
	}

	if (triggers_ophidiophobia(mon) && roll_madness(MAD_OPHIDIOPHOBIA)){
		pline("You're afraid to go near that horrid serpent!");
		return TRUE;
	}

	if (triggers_entomophobia(mon->data) && roll_madness(MAD_ENTOMOPHOBIA)){
		pline("You're afraid to go near that frightful bug!");
		return TRUE;
	}

	if (triggers_arachnophobia(mon->data) && roll_madness(MAD_ARACHNOPHOBIA)){
		pline("You're afraid to go near that terrifying spider!");
		return TRUE;
	}

	if (mon->female && humanoid_upperbody(mon->data) && roll_madness(MAD_ARACHNOPHOBIA)){
		You("can't bring yourself to strike %s!", mon_nam(mon));
		return TRUE;
	}

	if (is_aquatic(mon->data) && roll_madness(MAD_THALASSOPHOBIA)){
		pline("You're afraid to go near that sea monster!");
		return TRUE;
	}

	if (u.umadness&MAD_PARANOIA && !BlockableClearThoughts && NightmareAware_Sanity < rnd(100)){
		You("attack %s's hallucinatory twin!", mon_nam(mon));
		return TRUE;
	}

	if (triggers_helminthophobia(mon) && roll_madness(MAD_HELMINTHOPHOBIA)){
		pline("You're afraid to go near that wormy thing!");
		return TRUE;
	}

	if (mon->mtyp == PM_UNMASKED_TETTIGON
		&& canspotmon(mon)
		&& !ClearThoughts
		&& !roll_generic_madness(TRUE)
		&& !roll_impurity(TRUE)
	){
		You("can't bring yourself to strike %s!", mon_nam(mon));
		return TRUE;
	}
	return FALSE;
}

/* Note: caller must ascertain mtmp is mimicking... */
void
stumble_onto_mimic(mtmp)
struct monst *mtmp;
{
	const char *fmt = "Wait!  That's %s!",
		   *generic = "a monster",
		   *what = 0;

	if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
	    u.ustuck = mtmp;

	/* clear mimicking before generating message to get an accurate a_monnam() */
	seemimic(mtmp);

	if (Blind) {
	    if (!Blind_telepat)
		what = generic;		/* with default fmt */
	    else if (mtmp->m_ap_type == M_AP_MONSTER)
		what = a_monnam(mtmp);	/* differs from what was sensed */
	} else {
	    int glyph = levl[u.ux+u.dx][u.uy+u.dy].glyph;

	    if (glyph_is_cmap(glyph) &&
		    (glyph_to_cmap(glyph) == S_hcdoor ||
		     glyph_to_cmap(glyph) == S_vcdoor))
		fmt = "The door actually was %s!";
	    else if (glyph_is_object(glyph) &&
		    glyph_to_obj(glyph) == GOLD_PIECE)
		fmt = "That gold was %s!";

	    /* cloned Wiz starts out mimicking some other monster and
	       might make himself invisible before being revealed */
	    if (mtmp->minvis && !See_invisible(mtmp->mx,mtmp->my))
		what = generic;
	    else
		what = a_monnam(mtmp);
	}
	if (what) pline(fmt, what);

	wakeup(mtmp, TRUE);
}


/*
 * It is unchivalrous for a knight to attack the defenseless or from behind.
 */
void
check_caitiff(mtmp)
struct monst *mtmp;
{
	//Animals and mindless creatures are always considered fair game
	if(mindless_mon(mtmp) || is_animal(mtmp->data))
		return;

	//If a monster attacked you last turn, it's fair game
	if(mtmp->mattackedu)
		return;
	
	if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL &&
	    (!mtmp->mcanmove || !mtmp->mnotlaugh || mtmp->msleeping || mtmp->mequipping ||
		(mtmp->mflee && mtmp->mtyp != PM_BANDERSNATCH && !mtmp->mavenge))){
		    You("caitiff!");
			if(u.ualign.record > 10) {
				u.ualign.sins++;
			    adjalign(-2); //slightly stiffer penalty
				change_hod(1);
			}
			else if(u.ualign.record > -10) {
			    adjalign(-5); //slightly stiffer penalty
			}
			else{
			    adjalign(-5); //slightly stiffer penalty
				change_hod(1);
			}
	}
/*	attacking peaceful creatures is bad for the samurai's giri */
	if (Role_if(PM_SAMURAI) && mtmp->mpeaceful){
        if(!(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_NINJA)){
          You("dishonorably attack the innocent!");
          u.ualign.sins++;
          u.ualign.sins++;
          change_hod(1);
          adjalign(-1);
          if(u.ualign.record > -10) {
              adjalign(-4);
          }
        } else {
          You("dishonorably attack the innocent!");
          adjalign(1);
        }
	}
}

/*
 * Player uses theft attack against monster.
 *
 * If the target is wearing body armor, take all of its possessions;
 * otherwise, take one object.  [Is this really the behavior we want?]
 *
 * This routine implicitly assumes that there is no way to be able to
 * resist petfication (ie, be polymorphed into a xorn or golem) at the
 * same time as being able to steal (poly'd into nymph or succubus).
 * If that ever changes, the check for touching a cockatrice corpse
 * will need to be smarter about whether to break out of the theft loop.
 */
void
steal_it(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *otmp, *stealoid, **minvent_ptr;
	long unwornmask;
	int petrifies = FALSE;
	char kbuf[BUFSZ];
	boolean mi_only = is_chuul(youracedata);

	if (!mdef->minvent) return;		/* nothing to take */

	/* look for worn body armor */
	stealoid = (struct obj *)0;
	if (could_seduce(&youmonst, mdef, mattk)) {
	    /* find armor, and move it to end of inventory in the process */
	    minvent_ptr = &mdef->minvent;
	    while ((otmp = *minvent_ptr) != 0)
			if (otmp->owornmask & (W_ARM|W_ARMU)){
				if (stealoid){ /*Steal suit or undershirt*/
					minvent_ptr = &otmp->nobj;
				}
				else {
					*minvent_ptr = otmp->nobj;	/* take armor out of minvent */
					stealoid = otmp;
					stealoid->nobj = (struct obj *)0;
				}
			} else {
				minvent_ptr = &otmp->nobj;
			}
	    *minvent_ptr = stealoid;	/* put armor back into minvent */
	}

	/*stealing is impure*/
	IMPURITY_UP(u.uimp_theft)

	if (stealoid) {		/* we will be taking everything */
	    if (gender(mdef) == (int) (Upolyd ? u.mfemale : flags.female) &&
			youracedata->mlet == S_NYMPH)
		You("charm %s.  She gladly hands over her possessions.",
		    mon_nam(mdef));
	    else
		You("seduce %s and %s starts to take off %s clothes.",
		    mon_nam(mdef), mhe(mdef), mhis(mdef));
		IMPURITY_UP(u.uimp_seduction)
	}
	while ((otmp = mdef->minvent) != 0) {
		if(mi_only && !is_magic_obj(otmp) && otmp != stealoid){
			for(otmp = otmp->nobj; otmp && !is_magic_obj(otmp) && otmp != stealoid; otmp = otmp->nobj); //Fast forward through objects array
			if(!otmp)
				break; //No fitting objects, break while
		}
	    /* take the object away from the monster */
	    obj_extract_self(otmp);
	    if ((unwornmask = otmp->owornmask) != 0L) {
			mdef->misc_worn_check &= ~unwornmask;
			if (otmp->owornmask & W_WEP) {
				setmnotwielded(mdef,otmp);
				MON_NOWEP(mdef);
			}
			if (otmp->owornmask & W_SWAPWEP){
				setmnotwielded(mdef,otmp);
				MON_NOSWEP(mdef);
			}
			otmp->owornmask = 0L;
			update_mon_intrinsics(mdef, otmp, FALSE, FALSE);

			if (otmp == stealoid)	/* special message for final item */
				pline("%s finishes taking off %s suit.",
				  Monnam(mdef), mhis(mdef));
	    }

		if(near_capacity() < calc_capacity(otmp->owt) || u.uavoid_theft){
			You("steal %s %s and drop it to the %s.",
				  s_suffix(mon_nam(mdef)), xname(otmp), surface(u.ux, u.uy));
			if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
				Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
				petrifies = TRUE;
			}
			dropy(otmp);
		} else {
	    /* give the object to the character */
			if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
				Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
				petrifies = TRUE;
			}
			otmp = hold_another_object(otmp, "You snatched but dropped %s.",
						   doname(otmp), "You steal: ");
		}
	    /* more take-away handling, after theft message */
	    if (unwornmask & W_WEP || unwornmask & W_SWAPWEP) {		/* stole wielded weapon */
		possibly_unwield(mdef, FALSE);
	    } else if (unwornmask & W_ARMG) {	/* stole worn gloves */
		mselftouch(mdef, (const char *)0, TRUE);
		if (mdef->mhp <= 0)	/* it's now a statue */
		    return;		/* can't continue stealing */
	    }
	    if (petrifies) {
			instapetrify(kbuf);
			break;		/* stop the theft even if hero survives */
	    }

	    if (!stealoid) break;	/* only taking one item */
	}
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* mattacku()
 * mattackm()
 *
 * Monster is attacking something. Use xattacky().
 */
int
mattacku(mtmp)
register struct monst *mtmp;
{
	return xattacky(mtmp, &youmonst, mtmp->mux, mtmp->muy);
}

int
mattackm(magr, mdef)
register struct monst *magr, *mdef;
{
	/* this needs both attacker and defender, currently */
	if (!magr || !mdef)
		return MM_MISS;

	return xattacky(magr, mdef, mdef->mx, mdef->my);
}

/* fightm()  -- mtmp fights some other monster
 *
 * Returns:
 *	0 - Monster did nothing.
 *	1 - If the monster made an attack.  The monster might have died.
 *
 * There is an exception to the above.  If mtmp has the hero swallowed,
 * then we report that the monster did nothing so it will continue to
 * digest the hero.
 */
int
fightm(mtmp)		/* have monsters fight each other */
	register struct monst *mtmp;
{
	register struct monst *mon, *nmon;
	int result, has_u_swallowed;
	boolean conflict = (Conflict && 
						couldsee(mtmp->mx,mtmp->my) && 
						(distu(mtmp->mx,mtmp->my) <= BOLT_LIM*BOLT_LIM) && 
						!resist(mtmp, RING_CLASS, 0, 0))
					|| mtmp->mberserk;
#ifdef LINT
	nmon = 0;
#endif
	/* perhaps the monster will resist Conflict */
	/* if(resist(mtmp, RING_CLASS, 0, 0))
	    return(0); */

	if ((mtmp->mtame || is_covetous(mtmp->data)) && !conflict)
	    return(0);

	if(u.ustuck == mtmp) {
	    /* perhaps we're holding it... */
	    if(itsstuck(mtmp))
		return(0);
	}
	has_u_swallowed = (u.uswallow && (mtmp == u.ustuck));

	for(mon = fmon; mon; mon = nmon) {
	    nmon = mon->nmon;
	    if(nmon == mtmp) nmon = mtmp->nmon;
	    /* Be careful to ignore monsters that are already dead, since we
	     * might be calling this before we've cleaned them up.  This can
	     * happen if the monster attacked a cockatrice bare-handedly, for
	     * instance.
	     */
	    if(mon != mtmp && !DEADMONSTER(mon)) {
		if(monnear(mtmp,mon->mx,mon->my)) {
		    if (!conflict && !mm_aggression(mtmp, mon))
		    	continue;
		    if(!u.uswallow && (mtmp == u.ustuck)) {
			if(!rn2(4)) {
			    pline("%s releases you!", Monnam(mtmp));
			    u.ustuck = 0;
			} else
			    break;
		    }

		    /* mtmp can be killed */
		    bhitpos.x = mon->mx;
		    bhitpos.y = mon->my;
		    notonhead = 0;
		    result = mattackm(mtmp,mon);

		    if (result & MM_AGR_DIED) return 1;	/* mtmp died */
		    /*
		     *  If mtmp has the hero swallowed, lie and say there
		     *  was no attack (this allows mtmp to digest the hero).
		     */
		    if (has_u_swallowed) return 0;

		    /* Allow attacked monsters a chance to hit back. Primarily
		     * to allow monsters that resist conflict to respond.
		     */
		    if ((result & MM_HIT) && !(result & MM_DEF_DIED) &&
				rn2(4) && mon->movement >= NORMAL_SPEED
			) {
				mon->movement -= NORMAL_SPEED;
				notonhead = 0;
				(void) mattackm(mon, mtmp);	/* return attack */
		    }
			
			//If was conflict and a miss, can continue to attack.  Otherwise, ignore you.
		    return (((result & MM_HIT) || !conflict) ? 1 : 0);
		}
	    }
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* expels()
 *
 * The player's engulfer "lets" them leave
 */
void
expels(mtmp, mdat, message)
register struct monst *mtmp;
register struct permonst *mdat; /* if mtmp is polymorphed, mdat != mtmp->data */
boolean message;
{
	if (message) {
		if (is_animal(mdat))
			You("get regurgitated!");
		else {
			char blast[40];
			register int i;

			blast[0] = '\0';
			for(i = 0; i < NATTK; i++)
				if(mdat->mattk[i].aatyp == AT_ENGL)
					break;
			if (mdat->mattk[i].aatyp != AT_ENGL)
			      impossible("Swallower has no engulfing attack?");
			else {
				if (is_whirly(mdat)) {
					switch (mdat->mattk[i].adtyp) {
						case AD_ELEC:
							Strcpy(blast,
						      " in a shower of sparks");
							break;
						case AD_COLD:
							Strcpy(blast,
							" in a blast of frost");
							break;
					}
				} else
					Strcpy(blast, " with a squelch");
				You("get expelled from %s%s!",
				    mon_nam(mtmp), blast);
			}
		}
	}
	unstuck(mtmp);	/* ball&chain returned in unstuck() */
	if (stationary(mdat)) {
		coord cc;
		if (teleok(u.ux0, u.uy0, TRUE) && distmin(u.ux0, u.uy0, mtmp->mx, mtmp->my) <= 1) {
			teleds(u.ux0, u.uy0, FALSE);
		}
		else if (tt_findadjacent(&cc, mtmp)) {
			teleds(cc.x, cc.y, FALSE);
		}
		else {
			mnexto(mtmp);
		}
	}
	else {
		mnexto(mtmp);
	}
	newsym(u.ux,u.uy);
	spoteffects(TRUE);
	/* to cover for a case where mtmp is not in a next square */
	if(um_dist(mtmp->mx,mtmp->my,1))
		pline("Brrooaa...  You land hard at some distance.");
}

/* diseasemu()
 *
 * Some kind of monster mdat tries to make the player ill
 */
boolean
diseasemu(mdat)
struct permonst *mdat;
{
	if (Sick_resistance) {
		You_feel("a slight illness.");
		return FALSE;
	}
	else {
		make_sick(Sick ? Sick / 2L + 1L : (long)rn1(ACURR(A_CON), 20),
			mdat->mname, TRUE, SICK_NONVOMITABLE);
		return TRUE;
	}
}

/* u_slow_down()
 *
 *  called when your intrinsic speed is taken away
 */
void
u_slow_down()
{
	if (HFast){
		HFast = 0L;
		if (!Fast)
			You("slow down.");
		else	/* speed boots */
			Your("quickness feels less natural.");
	}
	exercise(A_DEX, FALSE);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* mpoisons_sibj()
 * 
 * return how a poison attack was delivered
 * works for both player and monster attackers
 */
const char *
mpoisons_subj(mtmp, mattk)
struct monst *mtmp;
struct attack *mattk;
{
	if (mattk->aatyp == AT_WEAP || mattk->aatyp == AT_XWEP || mattk->aatyp == AT_DEVA || mattk->aatyp == AT_MARI) {
		struct obj *mwep = (mtmp == &youmonst) ? uwep : MON_WEP(mtmp);
		/* "Foo's attack was poisoned." is pretty lame, but at least
		it's better than "sting" when not a stinging attack... */
		return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";
	}
	else if(spirit_rapier_at(mattk->aatyp)){
		if (mattk->adtyp == AD_SHDW){
			return "shadow blade";
		}
		else if (mattk->adtyp == AD_STAR){
			return "starlight rapier";
		}
		else if (mattk->adtyp == AD_BSTR){
			return "blackstar rapier";
		}
		else if (mattk->adtyp == AD_MOON){
			return "moonlight rapier";
		}
		else if (mattk->adtyp == AD_BLUD){
			return "blade of rotted blood";
		}
		else if (mattk->adtyp == AD_WET){
			return "water-jet blade";
		}
		else if (mattk->adtyp == AD_HOLY){
			return "holy light-beam";
		}
		else if (mattk->adtyp == AD_UNHY){
			return "unholy light-blade";
		}
		else if (mattk->adtyp == AD_HLUH){
			return "corrupted light-blade";
		}
		else {
			return "blade";
		}
	}
	else if (mattk->adtyp == AD_MERC){
		return "blade of metallic mercury";
	}
	else if (mattk->aatyp == AT_HODS){
		struct obj *mwep = uwep;
		/* "Foo's attack was poisoned." is pretty lame, but at least
		it's better than "sting" when not a stinging attack... */
		return (!mwep || !mwep->opoisoned) ? "attack" : "weapon";
	}
	else {
		return (mattk->aatyp == AT_TUCH || mattk->aatyp == AT_5SQR) ? "contact" :
			(mattk->aatyp == AT_GAZE || mattk->aatyp == AT_WDGZ) ? "gaze" :
			(mattk->aatyp == AT_CLAW) ? "claws" :
			(mattk->aatyp == AT_TENT) ? "tentacles" :
			(mattk->aatyp == AT_ENGL) ? "vapor" :
			(mattk->aatyp == AT_BITE || mattk->aatyp == AT_OBIT || mattk->aatyp == AT_WBIT || mattk->aatyp == AT_LNCK || mattk->aatyp == AT_5SBT) ? "bite" :
			(mattk->aatyp == AT_NONE) ? "attack" :
			"sting";
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* sleep_monst()
 *
 * `mon' is hit by a sleep attack; return 1 if it's affected, 0 otherwise
 */
int
sleep_monst(mon, amt, how)
struct monst *mon;
int amt, how;
{
	if (resists_sleep(mon) ||
		(how >= 0 && resist(mon, (char)how, 0, NOTELL))) {
		shieldeff(mon->mx, mon->my);
	}
	else if (mon->mcanmove) {
		amt += (int)mon->mfrozen;
		if (amt > 0) {	/* sleep for N turns */
			mon->mcanmove = 0;
			mon->mfrozen = min(amt, 127);
		}
		else {		/* sleep until awakened */
			mon->msleeping = 1;
		}
		return 1;
	}
	return 0;
}

/* slept_monst()
 *
 * sleeping grabber releases, engulfer doesn't; don't use for paralysis!
 */
void
slept_monst(mon)
struct monst *mon;
{
	if ((mon->msleeping || !mon->mcanmove) && mon == u.ustuck &&
		!sticks(&youmonst) && !u.uswallow) {
		pline("%s grip relaxes.", s_suffix(Monnam(mon)));
		unstuck(mon);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* hurtarmor()
 *
 * Monster damages player's armor
 */
void
hurtarmor(attk, candestroy)
int attk;
boolean candestroy;
{
	int	hurt;

	switch(attk) {
	    /* 0 is burning, which we should never be called with */
	    case AD_RUST: hurt = 1; break;
	    case AD_CORR: hurt = 3; break;
	    default: hurt = 2; break;
	}

	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
		if (!uarmh || !rust_dmg(uarmh, xname(uarmh), hurt, FALSE, &youmonst, candestroy))
			continue;
		break;
	    case 1:
		if (uarmc) {
		    (void)rust_dmg(uarmc, xname(uarmc), hurt, TRUE, &youmonst, candestroy);
		    break;
		}
		/* Note the difference between break and continue;
		 * break means it was hit and didn't rust; continue
		 * means it wasn't a target and though it didn't rust
		 * something else did.
		 */
		if (uarm && (arm_blocks_upper_body(uarm->otyp) || rn2(2)))
		    (void)rust_dmg(uarm, xname(uarm), hurt, TRUE, &youmonst, candestroy);
		else if (uarmu)
		    (void)rust_dmg(uarmu, xname(uarmu), hurt, TRUE, &youmonst, candestroy);
		break;
	    case 2:
		if (!uarms || !rust_dmg(uarms, xname(uarms), hurt, FALSE, &youmonst, candestroy))
		    continue;
		break;
	    case 3:
		if (!uarmg || !rust_dmg(uarmg, xname(uarmg), hurt, FALSE, &youmonst, candestroy))
		    continue;
		break;
	    case 4:
		if (!uarmf || !rust_dmg(uarmf, xname(uarmf), hurt, FALSE, &youmonst, candestroy))
		    continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

/* hurtmarmor()
 * 
 * modified from hurtarmor()
 * Something (you/monster) daamges a monster's armor
 */
void
hurtmarmor(mdef, attk, candestroy)
struct monst *mdef;
int attk;
boolean candestroy;
{
	int	hurt;
	struct obj *target;

	switch (attk) {
		/* 0 is burning, which we should never be called with */
	case AD_RUST: hurt = 1; break;
	case AD_CORR: hurt = 3; break;
	default: hurt = 2; break;
	}
	/* What the following code does: it keeps looping until it
	* finds a target for the rust monster.
	* Head, feet, etc... not covered by metal, or covered by
	* rusty metal, are not targets.  However, your body always
	* is, no matter what covers it.
	*/
	while (1) {
		switch (rn2(5)) {
		case 0:
			target = which_armor(mdef, W_ARMH);
			if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef, candestroy))
				continue;
			break;
		case 1:
			target = which_armor(mdef, W_ARMC);
			if (target) {
				(void)rust_dmg(target, xname(target), hurt, TRUE, mdef, candestroy);
				break;
			}
			if ((target = which_armor(mdef, W_ARM)) != (struct obj *)0) {
				(void)rust_dmg(target, xname(target), hurt, TRUE, mdef, candestroy);
#ifdef TOURIST
			}
			else if ((target = which_armor(mdef, W_ARMU)) != (struct obj *)0) {
				(void)rust_dmg(target, xname(target), hurt, TRUE, mdef, candestroy);
#endif
			}
			break;
		case 2:
			target = which_armor(mdef, W_ARMS);
			if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef, candestroy))
				continue;
			break;
		case 3:
			target = which_armor(mdef, W_ARMG);
			if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef, candestroy))
				continue;
			break;
		case 4:
			target = which_armor(mdef, W_ARMF);
			if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef, candestroy))
				continue;
			break;
		}
		break; /* Out of while loop */
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* attk_protection()
 * 
 * Returns types of armor needed to prevent specified attack from touching its target
 *   ex) gloves makes claw attacks not touch a cockatrice
 */
long
attk_protection(aatyp)
int aatyp;
{
	long w_mask = 0L;

	switch (aatyp) {
	case AT_NONE:
	case AT_SPIT:
	case AT_EXPL:
	case AT_BOOM:
	case AT_GAZE:
	case AT_WDGZ:
	case AT_BREA:
	case AT_BRSH:
	case AT_MAGC:
	case AT_MMGC:
	case AT_BEAM:
	case AT_SRPR:
	case AT_XSPR:
	case AT_MSPR:
	case AT_DSPR:
	case AT_ESPR:
	case AT_WISP:
	case AT_VOMT:
	case AT_REND:		/* If the previous attacks were OK, this one is too */
		w_mask = ~0L;		/* special case; no defense needed */
		break;
	case AT_CLAW:
	case AT_LRCH:
	case AT_TUCH:
	case AT_5SQR:
	case AT_WEAP:
	case AT_XWEP:
	case AT_MARI:
	case AT_DEVA:
		w_mask = W_ARMG;	/* caller needs to check for weapon */
		break;
	case AT_KICK:
		w_mask = W_ARMF;
		break;
	case AT_BUTT:
		w_mask = W_ARMH;
		break;
	case AT_HUGS:
		w_mask = (W_ARMC | W_ARMG); /* attacker needs both to be protected */
		break;
	case AT_TAIL:
		w_mask = W_ARM;
		break;
	case AT_BITE:
	case AT_OBIT:
	case AT_WBIT:
	case AT_LNCK:
	case AT_5SBT:
	case AT_STNG:
	case AT_ENGL:
	case AT_TENT:
	case AT_TONG:
	default:
		w_mask = 0L;		/* no defense available */
		break;
	}
	return w_mask;
}

/* attk_equip_slot()
 * The offensive version of attk_protection()
 * Returns equipment slot that would be hitting the defender
 *   ex) silver gloves make punches do silver-searing damage
 */
long
attk_equip_slot(mon, aatyp)
struct monst *mon;
int aatyp;
{
	/* some worn armor may be involved depending on the attack type */
	long slot = 0L;
	switch (aatyp)
	{
		/* gloves */
		/* caller needs to check for weapons */
	case AT_CLAW:
		if(!mon)
			slot = W_ARMG;
		else if(nogloves(mon->data))
			slot = W_ARMF;
		else if(
			mon->mtyp == PM_CROW_WINGED_HALF_DRAGON
			|| mon->mtyp == PM_VROCK
			|| mon->mtyp == PM_AGLAOPE
		)
			slot = W_ARMF;
		else if(
			mon->mtyp == PM_ARCADIAN_AVENGER
			|| mon->mtyp == PM_THRONE_ARCHON
			|| mon->mtyp == PM_LIGHT_ARCHON
		)
			slot = 0L; //Wing bash
		else
			slot = W_ARMG;
		break;
	case AT_HODS:
	case AT_DEVA:
	case AT_REND:
	case AT_WEAP:
	case AT_XWEP:
	case AT_MARI:
		slot = W_ARMG;
		break;
		/* boots */
	case AT_KICK:
		slot = W_ARMF;
		break;
		/* helm */
	case AT_BUTT:
		slot = W_ARMH;
		break;
	}
	return slot;
}


/* badtouch()
 * returns TRUE if [attk] will touch [mdef]
 */
boolean
badtouch(magr, mdef, attk, weapon)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
{
	long slot = attk_protection(attk->aatyp);
	boolean youagr = (magr == &youmonst);

	if (/* not using a weapon -- assumes weapons will only be passed if making a weapon attack */
		(!weapon)
		&&
		/* slots aren't covered */
		((!slot || (slot != ~0L)) && (
		/* player */
		(youagr && (
		(!slot) ||
		((slot & W_ARM) && !uarm) ||
		((slot & W_ARMC) && !uarmc) ||
		((slot & W_ARMH) && !uarmh) ||
		((slot & W_ARMS) && !uarms) ||
		((slot & W_ARMG) && !uarmg) ||
		((slot & W_ARMF) && !uarmf) ||
		((slot & W_ARMU) && !uarmu)))
		||
		/* monster */
		(!youagr && ((slot & ~(magr->misc_worn_check)) || !slot))
		))
		/* not a damage type that doesn't actually contact */
		&& !(
		attk->adtyp == AD_SHDW
		|| attk->adtyp == AD_BLUD
		|| attk->adtyp == AD_MERC
		|| attk->adtyp == AD_STAR
		|| attk->adtyp == AD_BSTR
		|| attk->adtyp == AD_MOON
		|| attk->adtyp == AD_HOLY
		|| attk->adtyp == AD_UNHY
		|| attk->adtyp == AD_HLUH
		)
		)
		return TRUE;	// will touch

	/* else won't touch */
	return FALSE;
}

/* safe_attack()
 * 
 * returns FALSE if [attk] will result in death for [magr]
 * due to:
 *  - cockatrice stoning
 *  - eating a fatal corpse
 */
boolean
safe_attack(magr, mdef, attk, weapon, pa, pd)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
struct permonst * pa;
struct permonst * pd;
{
	long slot = attk_protection(attk->aatyp);
	boolean youagr = (magr == &youmonst);

	/* if there is no defender, it's safe */
	if (!mdef)
		return TRUE;

	/* Touching is fatal */
	if ((!pd || touch_petrifies(pd)) && !(Stone_res(magr))
		&& badtouch(magr, mdef, attk, weapon)
		)
		return FALSE;	// don't attack

	/* consuming the defender is fatal */
	if ((!pd || (is_deadly(pd) ||
		((pd->mtyp == PM_GREEN_SLIME || pd->mtyp == PM_FLUX_SLIME) &&
			!(Slime_res(magr)))
		)) && (
		((attk->aatyp == AT_BITE || attk->aatyp == AT_OBIT || attk->aatyp == AT_WBIT || attk->aatyp == AT_LNCK || attk->aatyp == AT_5SBT) && is_vampire(pa)) ||
		(attk->aatyp == AT_ENGL && attk->adtyp == AD_DGST)
		))
		return FALSE;	// don't attack

	/* otherwise, it is safe(ish) to attack */
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* beastmastery()
 * mountedCombat()
 *
 * returns the accuracy bonus a pet/mount gets from the player's skill
 */
int
beastmastery()
{
	int bm;
	switch (P_SKILL(P_BEAST_MASTERY)) {
	case P_ISRESTRICTED: bm = 0; break;
	case P_UNSKILLED:    bm = 0; break;
	case P_BASIC:        bm = 2; break;
	case P_SKILLED:      bm = 5; break;
	case P_EXPERT:       bm = 10; break;
	default: impossible(">Expert beast mastery unhandled"); bm = 10; break;
	}
	if ((uwep && uwep->oartifact == ART_CLARENT) || (uswapwep && uswapwep->oartifact == ART_CLARENT))
		bm *= 2;

	if(uring_art(ART_NARYA))
		bm += narya();
	return bm;
}

int
narya()
{
	return (ACURR(A_CHA) - 11)/2;
}

int
mountedCombat()
{
	int bm;
	switch (P_SKILL(P_RIDING)) {
	case P_ISRESTRICTED: bm = 0; break;
	case P_UNSKILLED:    bm = 0; break;
	case P_BASIC:        bm = 2; break;
	case P_SKILLED:      bm = 5; break;
	case P_EXPERT:       bm = 10; break;
	default: impossible(">Expert riding unhandled"); bm = 10; break;
	}
	return bm;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* obj_silver_searing()
 *
 * returns TRUE if object sears silver-haters
 * 
 * This includes only silver items, not jade
 */
boolean
obj_silver_searing(obj)
struct obj * obj;
{
	if (!obj)
		return FALSE;

	if (is_lightsaber(obj) && litsaber(obj))
		return FALSE;

	if ((obj_is_material(obj, SILVER) || obj_is_material(obj, HEMARGYOS) || arti_silvered(obj)) ||
		(obj->oclass == RING_CLASS && obj->ohaluengr
		&& (isEngrRing(obj->otyp) || isSignetRing(obj->otyp))
		&& obj->oward >= LOLTH_SYMBOL && obj->oward <= LOST_HOUSE) ||
		has_spear_point(obj,SILVER_SLINGSTONE) ||
		(obj->otyp == SHURIKEN && !flags.mon_moving && uwep && uwep->oartifact == ART_SILVER_STARLIGHT)	// THIS IS BAD AND SHOULD BE DONE DIFFERENTLY
		)
		return TRUE;

	return FALSE;
}
/* obj_jade_searing()
 *
 * returns TRUE if object sears silver-haters
 * 
 * This includes only jade items, not silver
 */
boolean
obj_jade_searing(obj)
struct obj * obj;
{
	if (!obj)
		return FALSE;

	static short jadeRing = 0;
	if (!jadeRing) jadeRing = find_jade_ring();

	if (is_lightsaber(obj) && litsaber(obj))
		return FALSE;

	if (
		(obj->otyp == JADE) ||
		(obj->oclass == RING_CLASS && obj->otyp == jadeRing) ||
		(obj->obj_material == GEMSTONE && obj->sub_material == JADE)
		)
		return TRUE;

	return FALSE;
}

/* hatesobjdmg()
 * 
 * Calculates a damage roll from [mdef] being seared by [otmp]
 * Counts silver, jade, iron, holy, unholy
 * Does not print messages
 * 
 */
int
hatesobjdmg(mdef, otmp, magr)
struct monst * mdef;
struct obj * otmp;
struct monst * magr;
{
	//boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	//struct permonst * pa = youagr ? youracedata : magr->data;
	struct permonst * pd = youdef ? youracedata : mdef->data;

	boolean vulnerable = insubstantial(pd);
#define vd(n, x)	(vulnerable ? (n*x) : d(n, x))
	int diesize;
	int ndice;
	int khakharadice = rnd(3);

	int dmg = 0;

	if (!otmp || !mdef)
		return 0;

	if (hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)
		&& (obj_silver_searing(otmp) || obj_jade_searing(otmp))) {
		/* default: 1d20 */
		ndice = 1;
		diesize = 20;
		/* special cases */
		if(otmp->otyp == KHAKKHARA)
			ndice = khakharadice;
		else if (otmp->oartifact == ART_PEN_OF_THE_VOID && mvitals[PM_ACERERAK].died > 0)
			ndice = 2;
		else if (otmp->oartifact == ART_SILVER_STARLIGHT)
			ndice = 2;
		
		/* calculate */
		dmg += vd(ndice, diesize);
	}
	if(hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)
		&& check_oprop(otmp, OPROP_SFLMW)
	){
		/* default: 1d20 */
		ndice = 1;
		diesize = 20;
		/* special cases */
		if(otmp->otyp == KHAKKHARA)
			ndice = khakharadice;
		else if (otmp->oartifact == ART_PEN_OF_THE_VOID && mvitals[PM_ACERERAK].died > 0)
			ndice = 2;
		/* calculate */
		dmg += vd(ndice, diesize);
	}

	if (hates_unholy_mon(mdef) && obj_is_material(otmp, GREEN_STEEL) &&
		!(is_lightsaber(otmp) && litsaber(otmp))
	) {
		/* default: 2d9 */
		/* Note: stacks with curse damage to 3d9 total (pre modifiers). */
		ndice = 2;
		diesize = 9;
		/* special cases */
		if (otmp->otyp == KHAKKHARA)
			ndice *= khakharadice;
		/* calculate */
		if (ndice)
			dmg += vd(ndice, diesize);
	}
	
	if (hates_iron(pd) &&
		is_iron_obj(otmp) &&
		!(is_lightsaber(otmp) && litsaber(otmp))) {
		/* default: 1d(XL) */
		ndice = 1;
		diesize = max(1, mlev(mdef));
		/* special cases */
		if (otmp->otyp == KHAKKHARA)
			ndice = khakharadice;
		/* calculate */
		dmg += vd(ndice, diesize);
		mdef->mironmarked = TRUE;
	}
	if (hates_holy_mon(mdef) &&
		otmp->blessed) {
		/* default: 1d4 */
		ndice = 1;
		diesize = 4;
		/* special cases that don't affect dice */
		if (otmp->oartifact == ART_EXCALIBUR ||
			otmp->oartifact == ART_LANCE_OF_LONGINUS)
			dmg += vd(3, 7);
		else if (otmp->oartifact == ART_ARCOR_KERYM)
			dmg += vd(2, 10); // Crackling holy energy
		else if (otmp->oartifact == ART_GODHANDS)
			dmg += 7;
		else if (otmp->oartifact == ART_DIRGE){
			if(check_mutation(SHUB_RADIANCE))
				dmg += vd(3, 7);
		}
		else if (otmp->oartifact == ART_RED_CORDS_OF_ILMATER)
			dmg += 7;
		else if (otmp->oartifact == ART_JINJA_NAGINATA)
			dmg += vd(1, 12);
		else if (otmp->oartifact == ART_HOLY_MOONLIGHT_SWORD && !otmp->lamplit)
			dmg += vd(1, 10) + otmp->spe;
		else if (otmp->oartifact == ART_VAMPIRE_KILLER)
			dmg += 7;

#define sacred_bonus_dice ((FightingFormSkillLevel(FFORM_KNI_SACRED) >= P_EXPERT) ? 6 : (FightingFormSkillLevel(FFORM_KNI_SACRED) >= P_SKILLED ? 3 : 1))
		if (activeFightingForm(FFORM_KNI_SACRED) && otmp == uwep){
			if (((Holiness_if(HOLY_HOLINESS) || Holiness_if(NEUTRAL_HOLINESS)) && u.ualign.record >= 0) ||
				((Holiness_if(UNHOLY_HOLINESS) || Holiness_if(VOID_HOLINESS)) && u.ualign.record < 0)){
				if (FightingFormSkillLevel(FFORM_KNI_SACRED) >= P_BASIC && u.uen >= 5){
					dmg += vd(sacred_bonus_dice, 8); // 1d8/3d8/6d8 for basic/skilled/expert
					u.uen -= 5;
				}
				use_skill(P_KNI_SACRED, 1);
			}
		}
		else if(otmp->where == OBJ_MINVENT){
			if(magr && (mon_knight(magr) || magr->mtyp == PM_OONA) && MON_WEP(magr) == otmp && mlev(magr) >= 14){
				if(mlev(magr) >= 28)
					dmg += vd(6, 8);
				else if(mlev(magr) >= 21)
					dmg += vd(3, 8);
				else 
					dmg += vd(1, 8);
			}
		}

		/* special cases that do affect dice */
		if (otmp->oartifact == ART_AMHIMITL)
			ndice = 3;
		else if (otmp->oartifact == ART_ROD_OF_SEVEN_PARTS)
			diesize = 20;
		else if (otmp->oartifact == ART_SPEAR_OF_PEACE)
			diesize = 20;

		if (otmp->otyp == KHAKKHARA)
			ndice = khakharadice;
		/* gold has a particular affinity to blessings and curses */
		if ((obj_is_material(otmp, GOLD) || otmp->oartifact == ART_RUYI_JINGU_BANG) &&
			!(is_lightsaber(otmp) && litsaber(otmp))) {
			diesize = 20;
		}
		if (is_self_righteous(otmp) && 
			(otmp->otyp != CHURCH_SHORTSWORD || !(resist_pierce(pd) && !resist_slash(pd)))
		)
			diesize *= otmp->otyp == CHURCH_SHORTSWORD && Insight >= 40 ? 5 : 2.5;
		/* calculate dice */
		dmg += vd(ndice, diesize);
	}
	if (hates_unholy_mon(mdef) &&
		is_unholy(otmp)) {
		/* default: 1d9 */
		ndice = 1;
		diesize = 9;
		/* special cases */
		if (otmp->oartifact == ART_STORMBRINGER)
			ndice = 4; //Extra unholy (4d9 vs excal's 3d7)
		else if (otmp->oartifact == ART_GODHANDS)
			dmg += 9;
		else if (otmp->oartifact == ART_DIRGE){
			dmg += 6;
			if(check_mutation(SHUB_RADIANCE))
				ndice = 4;
			dmg += (u.uimpurity+1)/2;
			dmg += (u.uimp_murder+1)/2;
		}
		else if (otmp->oartifact == ART_LANCE_OF_LONGINUS)
			ndice = 3;
		else if (otmp->oartifact == ART_SCEPTRE_OF_THE_FROZEN_FLOO)
		{	ndice = 0; dmg += 8; } // add directly; no dice rolled
		else if (otmp->oartifact == ART_ROD_OF_SEVEN_PARTS)
			diesize = 20;
		else if (otmp->oartifact == ART_SPEAR_OF_PEACE)
			diesize = 20;
		else if (otmp->oartifact == ART_AMHIMITL)
		{	ndice = 3; diesize = 4; }
		else if (otmp->oartifact == ART_TECPATL_OF_HUHETOTL) /* SCOPECREEP: add ART_TECPATL_OF_HUHETOTL to is_unholy() macro */
		{	ndice = (otmp->cursed ? 4 : 2); diesize = 4; }

		if(otmp->otyp == CHIKAGE && otmp->obj_material == HEMARGYOS){
			dmg += (u.uimpurity+1)/2;
		}

		if (activeFightingForm(FFORM_KNI_SACRED) && otmp == uwep){
			if (((Holiness_if(HOLY_HOLINESS) || Holiness_if(NEUTRAL_HOLINESS)) && u.ualign.record < 0) ||
				((Holiness_if(UNHOLY_HOLINESS) || Holiness_if(VOID_HOLINESS)) && u.ualign.record >= 0)){
				if (FightingFormSkillLevel(FFORM_KNI_SACRED) >= P_BASIC && u.uen >= 5){
					dmg += vd(sacred_bonus_dice, 8); // 1d8/3d8/6d8 for basic/skilled/expert
					u.uen -= 5;
				}
				use_skill(P_KNI_SACRED, 1);
			}
		}
		else if(otmp->where == OBJ_MINVENT){
			if(magr && (mon_dark_knight(magr) || magr->mtyp == PM_ALRUNES) && MON_WEP(magr) == otmp && mlev(magr) >= 14){
				if(mlev(magr) >= 28)
					dmg += vd(6, 8);
				else if(mlev(magr) >= 21)
					dmg += vd(3, 8);
				else 
					dmg += vd(1, 8);
			}
		}
#undef sacred_bonus_dice
		if (otmp->otyp == KHAKKHARA)
			ndice *= khakharadice;
		/* gold has a particular affinity to blessings and curses */
		if (obj_is_material(otmp, GOLD) &&
			!(is_lightsaber(otmp) && litsaber(otmp))) {
			ndice *= 2;
		}
		if (is_self_righteous(otmp) && 
			(otmp->otyp != CHURCH_SHORTSWORD || !(resist_pierce(pd) && !resist_slash(pd)))
		)
			diesize *= otmp->otyp == CHURCH_SHORTSWORD && Insight >= 40 ? 5 : 2.5;
		/* calculate */
		if (ndice)
			dmg += vd(ndice, diesize);
	}

	if (hates_unblessed_mon(mdef) &&
		!(is_unholy(otmp) || otmp->blessed)
	) {
		/* default: 1d8 */
		ndice = 1;
		diesize = 8;
		/* special cases */
		if (otmp->oartifact == ART_GODHANDS)
			dmg += 8;
		else if (otmp->oartifact == ART_ROD_OF_SEVEN_PARTS)
			diesize = 20;
		else if (otmp->oartifact == ART_STAFF_OF_TWELVE_MIRRORS)
			ndice = 2;
		else if (otmp->oartifact == ART_INFINITY_S_MIRRORED_ARC)
			{ ndice = otmp->altmode ? 2 : 1; diesize = 20; }
		else if (otmp->oartifact == ART_SANSARA_MIRROR)
			dmg += 8;
		else if (otmp->oartifact == ART_MIRRORBRIGHT)
			diesize = 24;
		else if (otmp->oartifact == ART_MIRROR_BRAND)
			ndice = 2;
		else if (otmp->oartifact == ART_GRAYSWANDIR){
			ndice = 3;
			diesize = 9;
		}
		
		if (otmp->otyp == KHAKKHARA)
			ndice *= khakharadice;
		/* calculate */
		if (ndice)
			dmg += vd(ndice, diesize);
	}

	if (hates_lawful_mon(mdef) &&
		((obj_is_material(otmp, PLATINUM) &&
		!(is_lightsaber(otmp) && litsaber(otmp)))
		|| otmp->oartifact == ART_GRAYSWANDIR
		)
	) {
		/* default: 1d5 */
		ndice = 1;
		diesize = 5;
		/* spiritual beings are hurt more */
		if(is_minion(mdef->data) || is_demon(mdef->data))
			diesize *= 2;
		/* strongly chaotic beings are hurt more */
		if(youdef ? u.ualign.record >= 100 : ( mdef->mtyp == PM_ANGEL || mdef->data->maligntyp <= -10))
			diesize *= 2;

		/* special cases */
		
		if (otmp->otyp == KHAKKHARA)
			ndice *= khakharadice;
		if (otmp->oartifact == ART_GRAYSWANDIR)
			dmg += 9;
		/* calculate */
		if (ndice)
			dmg += vd(ndice, diesize);
		//wields lawful energies
		if(otmp->where == OBJ_MINVENT){
			if(magr && (magr->mtyp == PM_OONA) && MON_WEP(magr) == otmp && mlev(magr) >= 14){
				if(mlev(magr) >= 28)
					dmg += vd(6, 8);
				else if(mlev(magr) >= 21)
					dmg += vd(3, 8);
				else 
					dmg += vd(1, 8);
			}
		}
	}

	if (hates_chaos_mon(mdef) &&
		otmp->obj_material == MERCURIAL &&
		((magr == &youmonst && u.ualign.type != A_LAWFUL && u.ualign.type != A_NEUTRAL) || /* Note: allows chaos, void, and none */
		 (magr != &youmonst && (magr->data->maligntyp < 0 || magr->data->maligntyp == MON_A_VOID || magr->data->maligntyp == MON_A_NONE))) &&
		!(is_lightsaber(otmp) && litsaber(otmp))
	) {
		ndice = 1;
		diesize = youdef ? u.ualign.record : mdef->data->maligntyp == 0 ? 5 : abs(mdef->data->maligntyp);
#define MIN_OF(x,y) x = min(x,y)
		MIN_OF(diesize, mlev(magr));
		if(magr == &youmonst){
			MIN_OF(diesize, u.ualign.record);
			MIN_OF(diesize, ACURR(A_INT));
			MIN_OF(diesize, ACURR(A_WIS));
			MIN_OF(diesize, ACURR(A_CHA));
		}
		else if(magr){
			MIN_OF(diesize, abs(magr->data->maligntyp));
			//Placeholder, cap the die size high
			// MIN_OF(diesize, ACURR_MON(magr, A_INT));
			// MIN_OF(diesize, ACURR_MON(magr, A_WIS));
			// MIN_OF(diesize, ACURR_MON(magr, A_CHA));
			MIN_OF(diesize, 25);
		}
		else {
			//Raw chaos, cap the die size high.
			MIN_OF(diesize, 30);
		}
#undef MIN_OF
		
		diesize = max(1,diesize);
		/* special cases */
		if (otmp->otyp == KHAKKHARA)
			ndice *= khakharadice;
		/* calculate */
		if (ndice)
			dmg += vd(ndice, diesize);
		//wields chaotic energies
		if(otmp->where == OBJ_MINVENT){
			if(magr && (magr->mtyp == PM_ALRUNES) && MON_WEP(magr) == otmp && mlev(magr) >= 14){
				if(mlev(magr) >= 28)
					dmg += vd(6, 8);
				else if(mlev(magr) >= 21)
					dmg += vd(3, 8);
				else 
					dmg += vd(1, 8);
			}
		}
	}
	if(otmp->oartifact == ART_LOLTH_S_FANG){
		//Cross-aligned
		if(!hates_lawful_mon(mdef)){
			dmg += vd(1, 8);
		}
		if(!is_drow(pd)){
			dmg += vd(1, 8);
		}
		if(!mdef->female){
			dmg += vd(1, 8);
		}
		if(!(is_primordial(pd) || is_great_old_one(pd))){
			dmg += vd(1, 8);
		}
	}
	/* the Rod of Seven Parts gets a bonus vs holy and unholy when uncursed */
	if (otmp->oartifact == ART_ROD_OF_SEVEN_PARTS
		&& !otmp->blessed && !otmp->cursed
		&& (hates_holy_mon(mdef) || hates_unholy_mon(mdef))
	){
		dmg += vd(1, 10);
	}

	/* Glamdring sears orcs and demons */
	if (otmp->oartifact == ART_GLAMDRING &&
		(is_orc(pd) || is_demon(pd)))
		dmg += vd(1, 20);

	/* The Veioistafur stave hurts sea creatures */
	if (otmp->obj_material == WOOD && otmp->otyp != MOON_AXE
		&& (otmp->oward & WARD_VEIOISTAFUR) && pd->mlet == S_EEL) {
		dmg += vd(1, 20);
	}

	/* Occult weapons hurt god-sent minions */
	if (mdef && mdef->isminion){
		if (otmp->oartifact == ART_LIFEHUNT_SCYTHE || otmp->oartifact == ART_VELKA_S_RAPIER || check_oprop(otmp, OPROP_OCLTW))
			dmg += vd(4, 4) + otmp->spe;
	}
#undef vd
	return dmg;
}

/* hits_insubstantial()
 * 
 * Caller is responsible for checking insubstantial(mdef->data) first
 * so that this function can also be used for other cases that can cause a creature
 * to be insubstantial.
 *
 * returns non-zero if [magr] attacking [mdef] with [attk] hits,
 * specifically in the case of [mdef] being insubstantial (as a shade)
 * 
 * returns 1 if the attack should only do on-hit-effects for damage
 * (like silver-hating)
 * 
 * returns 2 if the attack should do full damage
 * (like Sunsword)
 */
int
hits_insubstantial(magr, mdef, attk, weapon)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	struct permonst * pa = (magr ? (youagr ? youracedata : magr->data) : (struct permonst *)0);
	struct permonst * pd = youdef ? youracedata : mdef->data;

	/* Chupoclops makes all your attacks ethereal */
	if (youagr && u.sealsActive&SEAL_CHUPOCLOPS)
		return 2;

	/* Can touch cursed wraiths if you are also "cursed" (here defined as impure or insane rather than buc cursed) */
	if(mdef->mtyp == PM_BEFOULED_WRAITH){
		if(youagr && (u.uimpurity >= 25 || u.usanity < 50 || u.uhpbonus <= 0))
			return 2;
		else if(!youagr){
			if(magr && insightful(magr->data))
				return 2;
		}
	}

	/* no weapon */
	if (!weapon) {
		/* some worn armor may be involved depending on the attack type */
		struct obj * otmp;
		long slot = attk_equip_slot(magr, attk ? attk->aatyp : 0);
		switch (magr ? slot : 0L)
		{
		case W_ARMG:
			otmp = (youagr ? uarmg : which_armor(magr, slot));
			break;
		case W_ARMF:
			otmp = (youagr ? uarmf : which_armor(magr, slot));
			break;
		case W_ARMH:
			otmp = (youagr ? uarmh : which_armor(magr, slot));
			break;
		default:
			otmp = (struct obj *)0;
			break;
		}
		if (otmp && arti_phasing(otmp))
			return 2;

		/* Ghost that thinks it's alive and solid */
		if (magr->mtyp == PM_BEING_OF_IB || magr->mtyp == PM_PRIEST_OF_IB)
			return 2;

		if ((hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) && (
			(attk && (attk->adtyp == AD_STAR || attk->adtyp == AD_MOON))
			))
			return 2;

		if (hates_holy_mon(mdef) &&
			attk && (attk->adtyp == AD_HOLY || attk->adtyp == AD_HLUH))
			return 2;

		if (hates_unholy_mon(mdef) &&
			attk && (attk->adtyp == AD_UNHY || attk->adtyp == AD_HLUH))
			return 2;

		if (has_blood_mon(mdef) &&
			attk && attk->adtyp == AD_BLUD)
			return 2;

		if (!mindless_mon(mdef) &&
			attk && attk->adtyp == AD_PSON)
			return 2;

		if (attk && attk->adtyp == AD_SHDW)
			return 2;

		/*if (attk && (attk->adtyp == AD_MERC))
			return 2; Currently unused, unclear what conditions it should check exactly
		*/

		if ((flaming(mdef->data) || is_iron(mdef->data)) &&
			attk && attk->adtyp == AD_WET)
			return 2;

		if (!((species_resists_fire(mdef))
				|| (ward_at(x(mdef), y(mdef)) == SIGIL_OF_CTHUGHA)
				|| (youdef && ((Race_if(PM_HALF_DRAGON) && flags.HDbreath == AD_FIRE)))
				|| (!youdef && is_half_dragon(pd) && mdef->mvar_hdBreath == AD_FIRE)
				|| (youdef && u.sealsActive&SEAL_FAFNIR)) &&
			attk && attk->adtyp == AD_EFIR)
			return 2;

		if (!Poison_res(mdef) &&
			attk && attk->adtyp == AD_EDRC)
			return 2; /* likely will be swapped out to wormwood (poisonous/starlight/water damage) at some point */

		if ((hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) && (
			(youagr && u.sealsActive&SEAL_EDEN) ||
			(attk && attk->adtyp == AD_GLSS) ||
			(magr && is_silver_mon(magr)) ||
			obj_silver_searing(otmp) || obj_jade_searing(otmp) || check_oprop(otmp, OPROP_SFLMW) ||
			(youagr && slot == W_ARMG && uright && (obj_silver_searing(uright) || obj_jade_searing(uright) || check_oprop(uright, OPROP_SFLMW))) ||
			(youagr && slot == W_ARMG && uleft && (obj_silver_searing(uleft) || obj_jade_searing(uleft) || check_oprop(uleft, OPROP_SFLMW)))
			))
			return 1;

		if (hates_iron(pd) && (
			(attk && attk->adtyp == AD_SIMURGH) ||
			(magr && is_iron_mon(magr)) ||
			(otmp && is_iron_obj(otmp)) ||
			(youagr && slot == W_ARMG && uright && is_iron_obj(uright)) ||
			(youagr && slot == W_ARMG && uleft && is_iron_obj(uleft))
			))
			return 1;

		if (hates_holy_mon(mdef) && (
			(attk->adtyp == AD_ACFR) ||
			(magr && is_holy_mon(magr)) ||
			(otmp && otmp->blessed) ||
			(youagr && slot == W_ARMG && uright && uright->blessed) ||
			(youagr && slot == W_ARMG && uleft && uleft->blessed)
			))
			return 1;

		if (hates_unholy_mon(mdef) && (
			(magr && is_unholy_mon(magr)) ||
			(otmp && obj_is_material(otmp, GREEN_STEEL)) ||
			(otmp && is_unholy(otmp)) ||
			(youagr && slot == W_ARMG && uright && is_unholy(uright)) ||
			(youagr && slot == W_ARMG && uleft && is_unholy(uleft))
			))
			return 1;

		if (hates_unblessed_mon(mdef) && (
			(magr && is_unblessed_mon(magr)) ||
			(otmp && !(is_unholy(otmp) || otmp->blessed)) ||
			(youagr && slot == W_ARMG && uright && (is_unholy(uright) || uright->blessed)) ||
			(youagr && slot == W_ARMG && uleft && (is_unholy(uleft) || uleft->blessed))
			))
			return 1;

		return 0;
	}
	/* weapon */
	else {
		if (arti_phasing(weapon))	/* why is this used for more things than artifacts? >_> */
			return 2;

		if (weapon->oartifact == ART_IBITE_ARM)	/* Ghost touch, not actually phasing */
			return 2;

		if (check_oprop(weapon, OPROP_SFLMW))
			return 2;

		if (weapon->oartifact == ART_GRAYSWANDIR) /* Grayswandir can interact with phantoms */
			return 2;

		if (hatesobjdmg(mdef, weapon, magr))
			return 1;

		if ((hates_silver(pd) && !(youdef && u.sealsActive&SEAL_EDEN)) && (
			weapon->otyp == MIRROR ||
			weapon->otyp == POT_STARLIGHT
			))
			return 1;

		if (is_undead(pd) && (
			(weapon->otyp == CLOVE_OF_GARLIC)	/* causes shades to flee */
			))
			return 1;
	}
	return 0;
}

/* Check things that cause total-no-hit-with-message:
 *  - monster displacement
 *  - shade insubstantiality
 * Returns TRUE if either activated, causing a miss and printing a message
 */
boolean
miss_via_insubstantial(magr, mdef, attk, weapon, vis)
struct monst * magr;
struct monst * mdef;
struct attack * attk;
struct obj * weapon;
int vis;
{
	boolean youagr = (magr == &youmonst);
	boolean youdef = (mdef == &youmonst);
	char buf[BUFSZ];
	/* monster displacement */
	if (!youdef &&
		mon_resistance(mdef, DISPLACED) &&
		!(weapon && check_oprop(weapon, OPROP_SFLMW)) &&
		!(youagr && u.ustuck && u.ustuck == mdef) &&
		!(youagr && u.uswallow) &&
		!(has_passthrough_displacement(mdef->data) && hits_insubstantial(magr, mdef, attk, weapon)) &&
		rn2(2)
		) {
		if (has_passthrough_displacement(mdef->data)){
			if (vis&VIS_MAGR) {
				if (magr) {
					pline("%s attack passes harmlessly through %s.",
						(youagr ? "Your" : s_suffix(Monnam(magr))),
						mon_nam(mdef));
				}
				else {
					pline("%s %s harmlessly through %s.",
						The(cxname(weapon)),
						vtense(cxname(weapon), "pass"),
						mon_nam(mdef));
				}
			}
		}
		else {
			if (vis&VIS_MAGR) {
				if (magr) {
					pline("%s attack%s %s displaced image.",
						(youagr ? "You" : Monnam(magr)),
						(youagr ? "" : "s"),
						(youagr ? "a" : s_suffix(mon_nam(mdef)))
						);
				}
				else {
					pline("%s %s a displaced image.",
						The(cxname(weapon)),
						vtense(cxname(weapon), "hit"));
				}
			}
		}
		return TRUE;
	}
	else if (insubstantial(mdef->data) && !hits_insubstantial(magr, mdef, attk, weapon)) {
		/* Print message */
		if (vis&VIS_MAGR) {
			Sprintf(buf, "%s", ((!weapon || valid_weapon(weapon)) ? "attack" : cxname(weapon)));
			if (magr) {
				pline("%s %s %s harmlessly through %s.",
					(youagr ? "Your" : s_suffix(Monnam(magr))),
					buf,
					vtense(buf, "pass"),
					(youdef ? "you" : mon_nam(mdef))
					);
			}
			else {
				pline("%s %s harmlessly through %s.",
					The(cxname(weapon)),
					vtense(buf, "pass"),
					(youdef ? "you" : mon_nam(mdef)));
			}
		}
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/* item destruction strings */
const char * const destroy_strings[] = {	/* also used in trap.c */
	"freezes and shatters", "freeze and shatter", "shattered potion",
	"boils and explodes", "boil and explode", "boiling potion",
	"catches fire and burns", "catch fire and burn", "burning scroll",
	"catches fire and burns", "catch fire and burn", "burning book",
	"turns to dust and vanishes", "turn to dust and vanish", "",
	"shoots a spray of sparks", "shoots sparks and arcing current", "discharging wand",
	"boils vigorously", "boil vigorously", "boiling potion"
};

/* destroy_item()
*
* Called when item(s) are supposed to be destroyed in a defender's inventory
*
* Works for player and monster mtmp
* Assumes both the defender is alive and existant when called
*
* Can return:
* MM_MISS		0x00	no items destroyed
* MM_HIT		0x01	item(s) destroyed
*
* Only allows lethal damage against the player, so this function can be called
* while voiding the return.
*/
int
destroy_item(mtmp, osym, dmgtyp)
struct monst * mtmp;
int osym;
int dmgtyp;
{
	boolean youdef = mtmp == &youmonst;
	struct permonst * data = (youdef) ? youracedata : mtmp->data;
	int vis = (youdef) ? TRUE : canseemon(mtmp);
	int ndestroyed = 0;
	struct obj *obj, *obj2;
	int dmg, xresist, skip;
	long i, cnt, quan;
	int dindx;
	const char *mult;

	if (osym == RING_CLASS && dmgtyp == AD_ELEC)
		return MM_MISS; /*Rings aren't destroyed by electrical damage anymore*/
	if (ProtectItems(mtmp) && (osym == POTION_CLASS || osym == SCROLL_CLASS || osym == WAND_CLASS)){
		return MM_MISS;
	}
		

	for (obj = (youdef ? invent : mtmp->minvent); obj; obj = obj2) {
		obj2 = obj->nobj;
		if (obj->oclass != osym) continue; /* test only objs of type osym */
		if (obj->oartifact) continue; /* don't destroy artifacts */
		if (obj->in_use && obj->quan == 1) continue; /* not available */
		xresist = skip = 0;
		dmg = dindx = 0;
		quan = 0L;

		switch (dmgtyp) {
			/* Cold freezes potions */
		case AD_COLD:
			if (osym == POTION_CLASS && !(
				obj->otyp == POT_OIL ||
				obj->oerodeproof /* shatterproof */
				)) {
				quan = obj->quan;
				dindx = 0;
				dmg = 4;
			}
			else skip++;
			break;
			/* Fire boils potions, burns scrolls, burns spellbooks */
		case AD_FIRE:
			xresist = (Fire_res(mtmp) && obj->oclass != POTION_CLASS);

			if (obj->oerodeproof && is_flammable(obj))	/* fireproof */
				skip++;
			if (obj->otyp == SCR_FIRE || obj->otyp == SCR_GOLD_SCROLL_OF_LAW
				|| obj->otyp == SPE_FIREBALL || obj->otyp == SPE_FIRE_STORM)
				skip++;
			if (objects[obj->otyp].oc_unique) {
				skip++;
				if (!Blind && vis)
					pline("%s glows a strange %s, but remains intact.",
					The(xname(obj)), hcolor("dark red"));
			}
			quan = obj->quan;
			switch (osym) {
			case POTION_CLASS:
				dindx = 1;
				dmg = 6;
				break;
			case SCROLL_CLASS:
				dindx = 2;
				dmg = 1;
				break;
			case SPBOOK_CLASS:
				dindx = 3;
				dmg = 6;
				break;
			default:
				skip++;
				break;
			}
			break;
			/* electricity sparks charges out of wands */
		case AD_ELEC:
			xresist = (Shock_res(mtmp));
			quan = obj->quan;
			if (osym == WAND_CLASS){
				if (obj->otyp == WAN_LIGHTNING)
					skip++;
				dindx = 5;
				dmg = 6;
			}
			else
				skip++;
			break;
			/* other damage types don't destroy items here */
		default:
			skip++;
			break;
		}
		/* destroy the item, if allowed */
		if (!skip) {
			if (obj->in_use) --quan; /* one will be used up elsewhere */
			int amt = (osym == WAND_CLASS) ? obj->spe : quan;
			/* approx 10% of items in the stack get destroyed */
			for (i = cnt = 0L; i < amt; i++) {
				if (!rn2(10)) cnt++;
			}
			/* No items destroyed? Skip */
			if (!cnt)
				continue;
			/* print message */
			if (vis) {
				if (cnt == quan || quan == 1)	mult = "";
				else if (cnt > 1)				mult = "Some of ";
				else							mult = "One of ";
				pline("%s%s %s %s!",
					mult,
					(youdef) ? ((mult[0] != '\0') ? "your" : "Your") : ((mult[0] != '\0') ? s_suffix(mon_nam(mtmp)) : s_suffix(Monnam(mtmp))),
					xname(obj),
					(cnt > 1L) ? destroy_strings[dindx * 3 + 1]
					: destroy_strings[dindx * 3]);
			}

			/* potion vapors */
			if (osym == POTION_CLASS && dmgtyp != AD_COLD) {
				if (!breathless(data) || haseyes(data)) {
					if (youdef)
						potionbreathe(obj);
					else
						/* no function for monster breathing potions */;
				}
			}
			/* destroy item */
			if (osym == WAND_CLASS)
				obj->spe -= cnt;
			else {
				if (obj == current_wand) current_wand = 0;	/* destroyed */
				for (i = 0; i < cnt; i++) {
					/* use correct useup function */
					if (youdef) useup(obj);
					else m_useup(mtmp, obj);
				}
			}
			ndestroyed += cnt;

			/* possibly deal damage */
			if (dmg) {
				/* you */
				if (youdef) {
					if (xresist)	You("aren't hurt!");
					else {
						const char *how = destroy_strings[dindx * 3 + 2];
						boolean one = (cnt == 1L || osym == WAND_CLASS);

						dmg = d(cnt, dmg);
						losehp(dmg,
							(one ? how : (const char *)makeplural(how)),
							(one ? KILLED_BY_AN : KILLED_BY));
						exercise(A_STR, FALSE);
						/* Let's not worry about properly returning if that killed you. If it did, it's moot. I think. */
						/* at the very least, the return value from this function is being ignored often enough it doesn't matter */
					}
				}
				/* monster */
				else {
					if (xresist);	// no message, reduce spam
					else {
						dmg = d(cnt, dmg);
						/* not allowed to be lethal */
						if (dmg >= mtmp->mhp)
							dmg = min(0, mtmp->mhp - 1);
						mtmp->mhp -= dmg;
					}
				}
			}
		}
	}
	if (ndestroyed && roll_madness(MAD_TALONS) && osym != WAND_CLASS){
		if (ndestroyed > 1)
			You("panic after some of your possessions are destroyed!");
		else You("panic after one of your possessions is destroyed!");
		HPanicking += 1 + rnd(6);
	}

	/* return if anything was destroyed */
	return (ndestroyed ? MM_HIT : MM_MISS);
}

/* Spun out into own function becase 1) it's a lot and 2) it may be useful elsewhere */

int
android_braindamage(dmg, magr, mdef, vis)
int dmg;
struct monst *magr;
struct monst *mdef;
boolean vis;
{
	boolean youdef = mdef == &youmonst;
	boolean youagr = magr == &youmonst;
	int duration, newres = 0;
	int extra_damage = 0;
	boolean petrifies = FALSE;
	struct obj *otmp, *otmp2;
	long unwornmask;
	switch(rnd(12)){
		case 1:
			duration = dmg <= 2 ? dmg+1 : dmg*10;
			if(youdef){
				Your("photoreceptor wires are %s!", dmg <= 2 ? "unplugged" : "severed");
				make_blinded(itimeout_incr(Blinded, duration), FALSE);
			} else {
				if(youagr){
					if(dmg <= 2)
						You("unplug %s photoreceptors!", hisherits(mdef));
					else You("sever %s photoreceptor wires!", hisherits(mdef));
				}
				mdef->mcansee = 0;
				if ((mdef->mblinded + duration) > 127)
					mdef->mblinded = 127;
				else mdef->mblinded += duration;
			}
			extra_damage = d(dmg, 4);
		break;
		case 2:
			if(youdef){
				Your("visual lookup table has been corrupted!");
				make_hallucinated(itimeout_incr(HHallucination, dmg*10), FALSE, 0L);
			} else {
				if(youagr)
					You("reprogram %s visual lookup table!", hisherits(mdef));
				mdef->mberserk = 1;
			}
			extra_damage = d(dmg, 6);
		break;
		case 3:
			if(youdef){
				Your("efferent network has been crosswired!");
				make_stunned(itimeout_incr(HStun, dmg*10), FALSE);
			} else {
				if(youagr)
					You("crosswire %s efferent network!", hisherits(mdef));
				mdef->mconf = 1;
			}
			extra_damage = d(dmg, 4);
		break;
		case 4:
			if(youdef){
				Your("processing array has been crosswired!");
				make_confused(itimeout_incr(HConfusion, dmg*10), FALSE);
			} else {
				if(youagr)
					You("crosswire %s processing array!", hisherits(mdef));
				mdef->mconf = 1;
			}
			extra_damage = d(dmg, 6);
		break;
		case 5:
			if(youdef){
				Your("secondary data store has been compromised!");
				for(int i = dmg; dmg > 0; dmg--){
					forget(10);
				}
			} else {
				if(youagr)
					You("smash %s secondary data store!", hisherits(mdef));
				if(mdef->mtame && !(get_mx(magr, MX_EDOG) && EDOG(mdef)->loyal)) {
					if(mdef->mtame > dmg)
						mdef->mtame -= dmg;
					else untame(mdef, 1);
				}
				if(mdef->mcrazed && rnd(20) < dmg){
					mdef->mcrazed = 0;
					mdef->mberserk = 0;
					mdef->mdoubt = 0;
				}
			}
			extra_damage = d(dmg, 6);
		break;
		case 6:
			if(youdef){
				Your("fear response has been dialed up to maximum!");
				HPanicking += dmg*10L;
			} else {
				if(youagr)
					You("dial %s fear response up to maximum!", hisherits(mdef));
				monflee(mdef, dmg*10, FALSE, TRUE);
			}
			extra_damage = d(dmg, 6);
		break;
		case 7:
			if(youdef){
				Your("CPU has been damaged!");
				(void)adjattrib(A_INT, -dmg, FALSE);
			} else {
				if(youagr)
					You("smash %s CPU!", hisherits(mdef));
				mdef->mconf = 1;
			}
			extra_damage = d(dmg, 10);
		break;
		case 8:
			if(youdef){
				Your("heuristic subprocessor has been damaged!");
				(void)adjattrib(A_WIS, -dmg, FALSE);
			} else {
				if(youagr)
					You("smash %s heuristic subprocessor!", hisherits(mdef));
				mdef->mconf = 1;
			}
			extra_damage = d(dmg, 10);
		break;
		case 9:
			if(youdef){
				Your("nausea subroutine has been activated!");
				if(!Vomiting)
					make_vomiting(d(5,4), TRUE);
			} else {
				if(youagr)
					You("activate %s nausea subroutine!", hisherits(mdef));
				if(vis)
					pline("%s vomits!", Monnam(mdef));
				mdef->mcanmove = 0;
				if ((mdef->mfrozen + 3) > 127)
					mdef->mfrozen = 127;
				else mdef->mfrozen += 3;
			}
			extra_damage = d(dmg, 6);
		break;
		case 10:
			if(youdef){
				Your("efferent network has been damaged!");
				set_itimeout(&HFumbling, itimeout_incr(HFumbling, dmg*10));
			} else {
				if(youagr)
					You("strip %s efferent wires!", hisherits(mdef));
				mdef->mconf = 1;
			}
			extra_damage = d(dmg, 6);
		break;
		case 11:{
			if(youdef){
				Your("primary effector wiring has been compromised!");
				if((otmp = uwep)){
					pline("%s directs your %s to surrender your weapon!", Monnam(magr), mbodypart(mdef, ARM));
				} else if((otmp = uswapwep)){
					pline("%s directs your %s to surrender your secondary weapon!", Monnam(magr), mbodypart(mdef, ARM));
				} else if((otmp = uquiver)){
					pline("%s directs your %s to surrender your quivered weapon!", Monnam(magr), makeplural(mbodypart(mdef, ARM)));
				} else if((otmp = uarms)){
					pline("%s directs your %s to surrender your shield!", Monnam(magr), mbodypart(mdef, ARM));
				}
				if(otmp){
					remove_worn_item(otmp, TRUE);
					obj_extract_self(otmp);
					if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !which_armor(mdef, W_ARMG)){
						petrifies = TRUE;
					}
					mpickobj(magr, otmp); //may free otmp
					if (roll_madness(MAD_TALONS)){
						You("panic after giving away one of your possessions!");
						HPanicking += 1 + rnd(6);
					}
				}
			} else if(youagr){
				char kbuf[BUFSZ];
				You("hotwire %s primary effectors!", hisherits(mdef));
				if((otmp = MON_WEP(mdef))){
					You("force %s to surrender %s weapon!", himherit(mdef), hisherits(mdef));
				} else if((otmp = MON_SWEP(mdef))){
					You("force %s to surrender %s secondary weapon!", himherit(mdef), hisherits(mdef));
				} else if((otmp = which_armor(mdef, W_ARMS))){
					You("force %s to surrender %s shield!", himherit(mdef), hisherits(mdef));
				}
				if(otmp){
					/* take the object away from the monster */
					obj_extract_self(otmp);
					if ((unwornmask = otmp->owornmask) != 0L) {
						mdef->misc_worn_check &= ~unwornmask;
						if (otmp->owornmask & W_WEP) {
							setmnotwielded(mdef,otmp);
							MON_NOWEP(mdef);
						}
						if (otmp->owornmask & W_SWAPWEP){
							setmnotwielded(mdef,otmp);
							MON_NOSWEP(mdef);
						}
						otmp->owornmask = 0L;
						update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
					}

					if(near_capacity() < calc_capacity(otmp->owt)){
						You("take %s %s and drop it to the %s.",
							  s_suffix(mon_nam(mdef)), xname(otmp), surface(u.ux, u.uy));
						if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
							Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
							petrifies = TRUE;
						}
						dropy(otmp); //may free otmp
					} else {
					/* give the object to the character */
						if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
							Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
							petrifies = TRUE;
						}
						otmp = hold_another_object(otmp, "You took but dropped %s.", doname(otmp), "You steal: ");  //may free otmp
					}
					/* more take-away handling, after theft message */
					if (unwornmask & W_WEP || unwornmask & W_SWAPWEP) {		/* stole wielded weapon */
						possibly_unwield(mdef, FALSE);
					}
					if (petrifies) {
						instapetrify(kbuf);
					}
				}
			} else {
				if((otmp = MON_WEP(mdef))){
					if(vis)
						pline("%s surrenders %s weapon!", Monnam(mdef), hisherits(mdef));
				} else if((otmp = MON_SWEP(mdef))){
					if(vis)
						pline("%s surrenders %s secondary weapon!", Monnam(mdef), hisherits(mdef));
				} else if((otmp = which_armor(mdef, W_ARMS))){
					if(vis)
						pline("%s surrenders %s shield!", Monnam(mdef), hisherits(mdef));
				}
				if(otmp){
					/* take the object away from the monster */
					obj_extract_self(otmp);
					if ((unwornmask = otmp->owornmask) != 0L) {
						mdef->misc_worn_check &= ~unwornmask;
						if (otmp->owornmask & W_WEP) {
							setmnotwielded(mdef,otmp);
							MON_NOWEP(mdef);
						}
						if (otmp->owornmask & W_SWAPWEP){
							setmnotwielded(mdef,otmp);
							MON_NOSWEP(mdef);
						}
						otmp->owornmask = 0L;
						update_mon_intrinsics(mdef, otmp, FALSE, FALSE);
					}

					if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !which_armor(mdef, W_ARMG)){
						petrifies = TRUE;
					}
					mpickobj(magr, otmp); //may free otmp
					
					/* more take-away handling, after theft message */
					if (unwornmask & W_WEP || unwornmask & W_SWAPWEP) {		/* stole wielded weapon */
						possibly_unwield(mdef, FALSE);
					}
				}
			}
			
			extra_damage = d(dmg, 4);
		}break;
		case 12:
			if(youdef){
				int drops = 0;
				Your("primary and secondary effector wiring has been compromised!");
				pline("%s directs your body to drop your possessions!", Monnam(magr));
				if (*u.ushops) sellobj_state(SELL_NORMAL); //Shopkeepers will steal your stuff!
				for(otmp = invent; otmp; otmp = otmp2) {
					otmp2 = otmp->nobj;
					drops += drop(otmp);
				}
				if (drops && roll_madness(MAD_TALONS)){
					You("panic after dropping your %s!", drops == 1 ? "property" : "possessions");
					HPanicking += 1 + rnd(6);
				}
			} else {
				if(youagr){
					You("hotwire %s primary and secondary effectors!", hisherits(mdef));
					You("force %s to drop %s possessions!", himherit(mdef), hisherits(mdef));
				} else {
					pline("%s drops %s possessions!", Monnam(mdef), hisherits(mdef));
				}
				otmp2 = mdef->minvent;
				while((otmp = otmp2)){
					otmp2 = otmp->nobj;
					if(!(otmp->owornmask&(~(W_WEP|W_SWAPWEP)))){
						obj_extract_self(otmp);
						mdrop_obj(mdef, otmp, FALSE);
					}
				}
			}
			extra_damage = d(dmg, 4);
		break;
	}
	//needs a last thought fades away-type message?
	newres = xdamagey(magr, mdef, &noattack, extra_damage);
	//Finally handle cockatrice corpses :(
	if(!youdef){
		if (unwornmask & W_ARMG) {	/* stole worn gloves */
			mselftouch(mdef, (const char *)0, TRUE);
			if(mdef->mhp <= 0)
				newres |= MM_DEF_DIED;
		}
	}
	if(!youagr){
		if (petrifies) {
			minstapetrify(magr, youdef); //checks stone resistance
			if(magr->mhp <= 0)
				newres |= MM_AGR_DIED;
		}
	}
	return newres;
}

boolean
nearby_targets(magr)
struct monst *magr;
{
	struct monst *mon;
	int x = x(magr),
		y = y(magr);
	boolean youagr = (magr == &youmonst);
	
	if(!youagr && !magr->mpeaceful)
		if(distmin(x, y, u.ux, u.uy) <= 2)
			return TRUE;
	
	for(mon = fmon;mon;mon = mon->nmon){
		if(DEADMONSTER(mon))
			continue;
		if(youagr && mon->mpeaceful)
			continue;
		if(!youagr && (mon->mpeaceful == magr->mpeaceful))
			continue;
		if(distmin(x, y, mon->mx, mon->my) <= 2)
			return TRUE;
	}
	return FALSE;
}

boolean
adjacent_targets(magr)
struct monst *magr;
{
	struct monst *mon;
	int x = x(magr),
		y = y(magr);
	boolean youagr = (magr == &youmonst);
	
	if(!youagr && !magr->mpeaceful)
		if(distmin(x, y, u.ux, u.uy) <= 1)
			return TRUE;
	
	for(mon = fmon;mon;mon = mon->nmon){
		if(DEADMONSTER(mon))
			continue;
		if(youagr && mon->mpeaceful)
			continue;
		if(!youagr && (mon->mpeaceful == magr->mpeaceful))
			continue;
		if(distmin(x, y, mon->mx, mon->my) <= 1)
			return TRUE;
	}
	return FALSE;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
boolean
wearing_dragon_armor(mtmp, dragontype)
struct monst * mtmp;
int dragontype;
{
	struct obj * otmp;
	
	/* body armor */
	otmp = (mtmp==&youmonst) ? uarm : which_armor(mtmp, W_ARM);
	if (otmp && Is_dragon_armor(otmp)) {
		if (Dragon_armor_matches_mtyp(otmp, dragontype))
			return TRUE;
	}
	/* shield */
	otmp = (mtmp==&youmonst) ? uarms : which_armor(mtmp, W_ARMS);
	if (otmp && Is_dragon_armor(otmp)) {
		if (Dragon_armor_matches_mtyp(otmp, dragontype))
			return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/* Club-claw insight weapons strike additional targets if your insight is high enough to perceive the claw */
///////////////////////////////////////////////////////////////////////////////
int
hit_with_cclaw(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	otmp->otyp = CLAWED_HAND;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	int merc_mult = 1;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;
	if(is_streaming_merc(otmp) && otmp->oartifact == ART_AMALGAMATED_SKIES && mlev(magr) > 20 && (
		(youagr && Insight > 20 && YOU_MERC_SPECIAL)
		|| (!youagr && insightful(magr->data) && is_chaotic_mon(magr)))
	){
		merc_mult++;
		if((youagr ? (Insight > 60) : (mlev(magr) > 30)))
			merc_mult++;
	}
	dx *= merc_mult;
	dy *= merc_mult;
	if (isok(tarx + dx, tary + dy)){
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
		if(otmp->oartifact == ART_IBITE_ARM && artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_DESTROY){
			do_digging_impact(magr, otmp, tarx + dx, tary + dy);
		}
	}
	dx /= merc_mult;
	dy /= merc_mult;
	if(Insight >= 30){
		//45 degree rotation
		nx = sgn(dx+dy);
		ny = sgn(dy-dx);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
		if(otmp->oartifact == ART_IBITE_ARM && artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_DESTROY){
			do_digging_impact(magr, otmp, x(magr) + nx, y(magr) + ny);
		}
		//-45 degree rotation
		nx = sgn(dx-dy);
		ny = sgn(dx+dy);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
		if(otmp->oartifact == ART_IBITE_ARM && artinstance[ART_IBITE_ARM].IbiteUpgrades&IPROP_DESTROY){
			do_digging_impact(magr, otmp, x(magr) + nx, y(magr) + ny);
		}
	}
	otmp->otyp = otmp->oartifact == ART_AMALGAMATED_SKIES ? TWO_HANDED_SWORD : CLUB;
	return result;
}


///////////////////////////////////////////////////////////////////////////////
/* Gun katar shoot extra targets										    */
/////////////////////////////////////////////////////////////////////////////
boolean
safe_shot(struct monst *magr, int dx, int dy, int range)
{
	boolean youagr = magr == &youmonst;
	int ix = x(magr), iy = y(magr);
	struct monst *mdef;
	if(youagr && u.ustuck && u.uswallow){
		return TRUE;
	}
	for(int i = 1; i < range; i++){
		ix += dx;
		iy += dy;
		if(!isok(ix,iy))
			return FALSE;
		mdef = m_at(ix, iy);
		if(!mdef || DEADMONSTER(mdef)){
			if(!ZAP_POS(levl[ix][iy].typ) || closed_door(ix, iy))
				return FALSE;
		}
		else {
			//We'll call this your "sixth sense" talking <_<'
			if(youagr)
				return (!mdef->mpeaceful || Hallucination);
			else {
				if(magr->mtame && mdef->mtame)
					return FALSE;
				else if(magr->mpeaceful != mdef->mpeaceful)
					return TRUE;
				else
					return mm_grudge(magr, mdef, TRUE);
			}
		}
	}
	return FALSE;
}

int
shoot_with_gun_katar(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	int merc_mult = 1;
	int range = 15;
	struct obj *ammo = 0;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if(result&(MM_AGR_DIED|MM_AGR_STOP))
		return result;
	//45 degree rotation
	nx = sgn(dx+dy);
	ny = sgn(dy-dx);
	if(safe_shot(magr, nx, ny, range)){
		if(youagr){
			ammo = uquiver;
		}
		else {
			ammo = select_rwep(magr);
		}
		if(!ammo || !ammo_and_launcher(ammo, otmp))
			return result;
		/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
		result |= projectile(magr, ammo, otmp, HMON_PROJECTILE|HMON_FIRED, x(magr), y(magr), nx, ny, 0, range, FALSE, TRUE, FALSE)&(MM_AGR_DIED|MM_AGR_STOP);
	}

	if(result&(MM_AGR_DIED|MM_AGR_STOP))
		return result;
	//-45 degree rotation
	nx = sgn(dx-dy);
	ny = sgn(dx+dy);
	if(safe_shot(magr, nx, ny, range)){
		if(youagr){
			ammo = uquiver;
		}
		else {
			ammo = select_rwep(magr);
		}
		if(!ammo || !ammo_and_launcher(ammo, otmp))
			return result;
		/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
		result |= projectile(magr, ammo, otmp, HMON_PROJECTILE|HMON_FIRED, x(magr), y(magr), nx, ny, 0, range, FALSE, TRUE, FALSE)&(MM_AGR_DIED|MM_AGR_STOP);
	}
	return result;
}


///////////////////////////////////////////////////////////////////////////////
/* Isamusei hit additional targets, if your insight is high enough to percieve the distortions */
///////////////////////////////////////////////////////////////////////////////
int
hit_with_iwarp(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if (isok(tarx - 2*dx, tary - 2*dy)){
		struct monst *mdef2 = !youagr ? m_u_at(tarx - 2*dx, tary - 2*dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx - 2*dx, tary - 2*dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	if(Insight >= 45){
		//90 degree rotation
		nx = -dy;
		ny = dx;
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
		//-90 degree rotation
		nx = dy;
		ny = -dx;
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
	}
	if(Insight >= 57){
		//45 degree rotation
		nx = sgn(dx+dy);
		ny = sgn(dy-dx);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
		//-45 degree rotation
		nx = sgn(dx-dy);
		ny = sgn(dx+dy);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
	}
	if(Insight >= 70){
		//135 degree rotation
		//x = xcos0 - ysin0
		//x = x*(-0.7) - y*(0.7)
		//y = xsin0 + ycos0
		//y = x*(0.7) + y*(-0.7)
		nx = sgn(-dx-dy);
		ny = sgn(dx-dy);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
		//-135 degree rotation
		//x = xcos0 - ysin0
		//x = x*(-0.7) - y*(-0.7)
		//y = ysin0 + ycos0
		//y = x*(-0.7) + y*(-0.7)
		//-45 degree rotation
		nx = sgn(-dx+dy);
		ny = sgn(-dx-dy);
		if (isok(x(magr) + nx, y(magr) + ny) && !(result&(MM_AGR_DIED|MM_AGR_STOP))){
			struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
									u.uswallow ? u.ustuck : 
									(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
	}
	return result;
}


//////////////////////////////////////////////////////////////////////////////////////////
/* Rakuyo hit additional targets, if your insight is high enough to percieve the blood */
////////////////////////////////////////////////////////////////////////////////////////
int
hit_with_rblood(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	struct attack blood = {AT_ESPR, AD_BLUD, 1, 12+otmp->spe*2};
	int result = 0;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if (isok(tarx + dx, tary + dy)){
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		) { //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, &blood, (struct obj **)0, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
		if(Insight >= 40
		  && ((youagr) ? couldsee(tarx + dx, tary + dy) : clear_path(magr->mx, magr->my, tarx + dx, tary + dy))
		){
			explode(tarx + dx, tary + dy, AD_FIRE, -1, d(6,6), EXPL_FIERY, 1);
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/* Chikage may hit additional targets, if your insight is high enough to percieve the blood */
/////////////////////////////////////////////////////////////////////////////////////////////
int
hit_with_cblood(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	struct attack blood = {AT_ESPR, AD_BLUD, 1, 12};
	int result = 0;
	struct monst *mdef2;
	if(youagr)
		blood.damd += u.uimpurity;
	else
		blood.damd += otmp->spe*2;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if (isok(tarx + dx, tary + dy)){
		mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		) { //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, &blood, (struct obj **)0, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
		int n = (Insight - 20)/15;
		if (n > 2)
			n = 2;
		for(int i = 0; i < n; i++){
			dx += dx;
			dy += dy;
			if (!isok(tarx + dx, tary + dy))
				break;
			mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
									u.uswallow ? u.ustuck : 
									(dx || dy) ? m_at(tarx + dx, tary + dy) : 
									(struct monst *)0;
			if (mdef2 
				&& (!DEADMONSTER(mdef2))
				&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
				&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
					(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
					(youagr && !mdef2->mpeaceful))
			) { //Can hit a worm multiple times
				int vis2 = VIS_NONE;
				if(youagr || canseemon(magr))
					vis2 |= VIS_MAGR;
				if(mdef2 == &youmonst || canseemon(mdef2))
					vis2 |= VIS_MDEF;
				bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
				notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
				subresult = xmeleehity(magr, mdef2, &blood, (struct obj **)0, vis2, tohitmod, TRUE);
				/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
				result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
			}
		}
	}
	return result;
}

/////////////////////////////////////////////
/* Rejection weapons hit targets at range */
///////////////////////////////////////////
int
hit_with_rreject(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	struct attack blood = {AT_WISP, AD_PUSH, 2, 6+otmp->spe*2};
	int result = 0;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if (isok(tarx + dx, tary + dy)){
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		) { //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, &blood, (struct obj **)0, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////////
/* Blade-dancing monsters hit multiple targets                               */
///////////////////////////////////////////////////////////////////////////////
int
hit_with_dance(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	int cleave_range = (mlev(magr) - 16)/2;
	/*Not all attacks can cleave*/
	if(attk->aatyp != AT_WEAP
	 && attk->aatyp != AT_XWEP
	 && attk->aatyp != AT_MARI
	 && attk->aatyp != AT_CLAW
	 && attk->aatyp != AT_KICK
	 && attk->aatyp != AT_BUTT
	 && attk->aatyp != AT_TUCH
	 && attk->aatyp != AT_WHIP
	 && attk->aatyp != AT_LRCH
	 && attk->aatyp != AT_SRPR
	 && attk->aatyp != AT_XSPR
	 && attk->aatyp != AT_MSPR
	 && attk->aatyp != AT_DSPR
	 && attk->aatyp != AT_ESPR
	 && attk->aatyp != AT_DEVA
	 && attk->aatyp != AT_5SQR
	 && attk->aatyp != AT_VINE
	 && attk->aatyp != AT_TAIL
	)
		return result;
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;
	
	for(int i = cleave_range; i > 0; i--){
		if(monstermoves%2 == 1){
			//45 degree rotation
			nx = sgn(dy+dx);
			ny = sgn(dy-dx);
		}
		else {
			//-45 degree rotation
			nx = sgn(dx-dy);
			ny = sgn(dx+dy);
		}
		dx = nx;
		dy = ny;
		if(!isok(x(magr) + nx, y(magr) + ny))
			continue;
		if(result&(MM_AGR_DIED|MM_AGR_STOP))
			return result;
		struct monst *mdef2 = !youagr ? m_u_at(x(magr) + nx, y(magr) + ny) : 
								u.uswallow ? u.ustuck : 
								(nx || ny) ? m_at(x(magr) + nx, y(magr) + ny) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		) { //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = x(magr) + nx; bhitpos.y = y(magr) + ny;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	return result;
}


///////////////////////////////////////////////////////////////////////////////////////////
/* Mercurial weapons may strike behind primary target if the wielder is powerful enough */
/////////////////////////////////////////////////////////////////////////////////////////
int
hit_with_cclaw_streaming(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	
	otmp->otyp = CLAWED_HAND;
	
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;
	if (isok(tarx + 2*dx, tary + 2*dy)){
		tarx += dx;
		tary += dy;
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && (youagr ? (Insight > 60) : (mlev(magr) > 30)) && isok(tarx + 2*dx, tary + 2*dy)){
		tarx += dx;
		tary += dy;
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	otmp->otyp = otmp->oartifact == ART_AMALGAMATED_SKIES ? TWO_HANDED_SWORD : CLUB;
	return result;
}
///////////////////////////////////////////////////////////////////////////////////////////
/* Mercurial weapons may strike behind primary target if the wielder is powerful enough */
/////////////////////////////////////////////////////////////////////////////////////////
int
hit_with_streaming(magr, otmp, tarx, tary, tohitmod, attk)
struct monst * magr;
struct obj * otmp;
int tarx;
int tary;
int tohitmod;
struct attack * attk;
{
	int subresult = 0;
	boolean youagr = magr == &youmonst;
	/* try to find direction (u.dx and u.dy may be incorrect) */
	int dx = sgn(tarx - x(magr));
	int dy = sgn(tary - y(magr));
	int nx, ny;
	int result = 0;
	if(Insight >= 15 && otmp->oartifact != ART_AMALGAMATED_SKIES && is_cclub_able(otmp)){
		return hit_with_cclaw_streaming(magr, otmp, tarx, tary, tohitmod, attk);
	}
	if(!(isok(tarx - dx, tary - dy) &&
		x(magr) == tarx - dx &&
		y(magr) == tary - dy)
	)
		return result;

	if (isok(tarx + dx, tary + dy)){
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	if(!(result&(MM_AGR_DIED|MM_AGR_STOP)) && (youagr ? (Insight > 60) : (mlev(magr) > 30)) && isok(tarx + 2*dx, tary + 2*dy)){
		tarx += dx;
		tary += dy;
		struct monst *mdef2 = !youagr ? m_u_at(tarx + dx, tary + dy) : 
								u.uswallow ? u.ustuck : 
								(dx || dy) ? m_at(tarx + dx, tary + dy) : 
								(struct monst *)0;
		if (mdef2 
			&& (!DEADMONSTER(mdef2))
			&& ((youagr || mdef2 == &youmonst) ? couldsee(mdef2->mx,mdef2->my) : clear_path(magr->mx, magr->my, mdef2->mx, mdef2->my))
			&& ((!youagr && mdef2 != &youmonst && mdef2->mpeaceful != magr->mpeaceful) ||
				(!youagr && mdef2 == &youmonst && !magr->mpeaceful) ||
				(youagr && !mdef2->mpeaceful))
		){ //Can hit a worm multiple times
			int vis2 = VIS_NONE;
			if(youagr || canseemon(magr))
				vis2 |= VIS_MAGR;
			if(mdef2 == &youmonst || canseemon(mdef2))
				vis2 |= VIS_MDEF;
			bhitpos.x = tarx + dx; bhitpos.y = tary + dy;
			notonhead = (bhitpos.x != x(mdef2) || bhitpos.y != y(mdef2));
			subresult = xmeleehity(magr, mdef2, attk, &otmp, vis2, tohitmod, TRUE);
			/* handle MM_AGR_DIED and MM_AGR_STOP by adding them to the overall result, ignore other outcomes */
			result |= subresult&(MM_AGR_DIED|MM_AGR_STOP);
		}
	}
	return result;
}


boolean
is_serration_vulnerable(struct monst *mon){
	if((mon->misc_worn_check&W_ARM) || (mon->misc_worn_check&W_ARMU) || (mon->misc_worn_check&W_ARMC))
		return FALSE;

	int dr = avg_mdr(mon);
	if(dr >= 8)
		return FALSE;
	if(resist_slash(mon->data))
		return FALSE;
	if(resists_all(mon->data))
		return FALSE;
	if(resist_attacks(mon->data))
		return FALSE;
	return TRUE;
}

boolean
obj_is_material(struct obj *obj, int mat)
{
	if(obj->obj_material == mat)
		return TRUE;
	switch(mat){
		case IRON:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_IRON)
					return TRUE;
			}
		break;
		case GREEN_STEEL:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_GREEN)
					return TRUE;
			}
		break;
		case SILVER:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_SILVER)
					return TRUE;
			}
		break;
		case GOLD:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_GOLD)
					return TRUE;
			}
		break;
		case PLATINUM:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_PLATINUM)
					return TRUE;
			}
		break;
		case MITHRIL:
			if(obj->oartifact == ART_SKY_REFLECTED || obj->oartifact == ART_AMALGAMATED_SKIES){
				if(artinstance[ART_SKY_REFLECTED].ZerthMaterials&ZMAT_MITHRIL)
					return TRUE;
			}
		break;
	}
	return FALSE;
}

int
weapon_skill_type(struct obj *weapon, struct obj *launcher, boolean fired)
{
	int wtype = P_NONE;

	if (fired && launcher)
		wtype = weapon_type(launcher);
	else if (!weapon)
		wtype = weapon_type(weapon);
	else if (weapon && martial_aid(weapon))
		wtype = P_BARE_HANDED_COMBAT;
	else if (weapon->oartifact == ART_LIECLEAVER)
		wtype = P_SCIMITAR;
	else if (weapon->oartifact == ART_ROGUE_GEAR_SPIRITS)
		wtype = P_PICK_AXE;
	else if (weapon->oartifact == ART_WAND_OF_ORCUS)
		wtype = P_MACE;
	else if (weapon->otyp == KAMEREL_VAJRA && !litsaber(weapon))
		wtype = P_MACE;
	else if (is_shield(weapon) && activeFightingForm(FFORM_SHIELD_BASH))
		wtype = P_SHIELD_BASH;
	else if (weapon->otyp == WIND_AND_FIRE_WHEELS)
		wtype = P_BOOMERANG;
	else if (weapon->otyp == CARCOSAN_STING)
		wtype = P_DAGGER;
	else if (weapon->otyp == SOLDIER_S_SABER)
		wtype = P_SABER;
	else if (weapon->otyp == TWINGUN_SHANTA)
		wtype = P_BARE_HANDED_COMBAT;
	else if (weapon->otyp == BLADED_BOW)
		wtype = P_QUARTERSTAFF;
	else if (!valid_weapon(weapon) || is_launcher(weapon)){
		if(is_melee_launcher(weapon))
			wtype = weapon_type(weapon);
		else if (weapon && check_oprop(weapon, OPROP_BLADED))
			wtype = P_AXE;
		else if (weapon && check_oprop(weapon, OPROP_SPIKED))
			wtype = P_SPEAR;
		else wtype = P_CLUB;
	}
	else
		wtype = weapon_type(weapon);
	return wtype;
}
