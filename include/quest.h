/*	SCCS Id: @(#)quest.h	3.4	1992/11/15	*/
/* Copyright (c) Mike Stephenson 1991.				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef QUEST_H
#define QUEST_H

struct q_score {			/* Quest "scorecard" */
	Bitfield(first_start,1);	/* only set the first time */
	Bitfield(met_leader,1);		/* has met the leader */
	Bitfield(not_ready,3);		/* rejected due to alignment, etc. */
	Bitfield(pissed_off,1);		/* got the leader angry */
	Bitfield(got_quest,1);		/* got the quest assignment */
	/*7*/
	Bitfield(first_locate,1);	/* only set the first time */
	Bitfield(met_intermed,1);	/* used if the locate is a person. */
	Bitfield(got_final,1);		/* got the final quest assignment */
	/*10*/
	Bitfield(made_goal,3);		/* # of times on goal level */
	Bitfield(met_nemesis,1);	/* has met the nemesis before */
	Bitfield(killed_nemesis,1);	/* set when the nemesis is killed */
	Bitfield(in_battle,1);		/* set when nemesis fighting you */
	/*16*/
	Bitfield(cheater,1);		/* set if cheating detected */
	Bitfield(touched_artifact,1);	/* for a special message */
	Bitfield(offered_artifact,1);	/* offered to leader */
	Bitfield(got_thanks,1);		/* final message from leader */
	/*20*/

	/* keep track of leader presence/absence even if leader is
	   polymorphed, raised from dead, etc */
	Bitfield(leader_is_dead,1);
	Bitfield(second_thoughts,1); /*turned stag AFTER completing version 1 of the quest*/
	Bitfield(fakeleader_greet_1,1);
	Bitfield(fakeleader_greet_2,1);
	/*24*/
	Bitfield(moon_close,1);
	Bitfield(uh_shop_created,1);
	/*26*/
	int time_on_home;
#define	MAX_HOME_TIMER 255
/* Note: should be > 1 (urrent 70->6 */
#define	ANA_HOME_PROB (70 - (quest_status.time_on_home+1)/4)
#define ANA_SPAWN_TWO (quest_status.time_on_home > MAX_HOME_TIMER/4)
#define ANA_SPAWN_THREE (quest_status.time_on_home > MAX_HOME_TIMER/2)
#define ANA_SPAWN_FOUR (quest_status.time_on_home > MAX_HOME_TIMER*3/4)
	/*30*/
	long time_doing_quest;
	unsigned leader_m_id;
};
//Random spawns invade city:
#define UH_QUEST_TIME_0	16000
//Citizens begin turning outside cathedral:
#define UH_QUEST_TIME_1	28000
//Citizens begin turning inside cathedral, stake fails:
#define UH_QUEST_TIME_2	32000
//Amalia first becomes vulnerable to turning:
#define UH_QUEST_TIME_3	38000
//Quest "failed", everyone turns instantly:
#define UH_QUEST_TIME_4	48000

#define MAX_QUEST_TRIES  7	/* exceed this and you "fail" */
#ifdef CONVICT
#define MIN_QUEST_ALIGN ((Role_if(PM_CONVICT) || Role_if(PM_MADMAN)) ? 4 : 20)	/* at least this align.record to start */
  /* note: align 20 matches "pious" as reported by enlightenment (cmd.c) */
  /* note: align 20 matches "stridently" as reported by enlightenment (cmd.c) */
#else
#define MIN_QUEST_ALIGN 20	/* at least this align.record to start */
  /* note: align 20 matches "pious" as reported by enlightenment (cmd.c) */
#endif
#define MIN_QUEST_LEVEL 14	/* at least this u.ulevel to start */
#define GNOMISH_MIN_QUEST_LEVEL 6	/* at least this u.ulevel to start */
  /* note: exp.lev. 14 is threshold level for 5th rank (class title, role.c) */

#endif /* QUEST_H */
