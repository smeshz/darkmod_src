/*****************************************************************************
                    The Dark Mod GPL Source Code
 
 This file is part of the The Dark Mod Source Code, originally based 
 on the Doom 3 GPL Source Code as published in 2011.
 
 The Dark Mod Source Code is free software: you can redistribute it 
 and/or modify it under the terms of the GNU General Public License as 
 published by the Free Software Foundation, either version 3 of the License, 
 or (at your option) any later version. For details, see LICENSE.TXT.
 
 Project: The Dark Mod (http://www.thedarkmod.com/)
 
 $Revision$ (Revision of last commit) 
 $Date$ (Date of last commit)
 $Author$ (Author of last commit)
 
******************************************************************************/

#include "precompiled_game.h"
#pragma hdrstop

static bool versioned = RegisterVersionedFile("$Id$");

#include "CombatState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/ChaseEnemyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MeleeCombatTask.h"
#include "../Tasks/RangedCombatTask.h"
#include "../Tasks/ChaseEnemyRangedTask.h"
#include "LostTrackOfEnemyState.h"
#include "AgitatedSearchingState.h"
#include "FleeState.h"
#include "../Library.h"

#define REACTION_TIME_MIN  100	// grayman debug
#define REACTION_TIME_MAX 2000	// grayman debug

namespace ai
{

const float s_DOOM_TO_METERS = 0.0254f; // grayman debug

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

bool CombatState::CheckAlertLevel(idAI* owner)
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("%d: CombatState::CheckAlertLevel - %s - checking alert level; are we in the correct state?\r",gameLocal.time,owner->name.c_str()); // grayman debug
	if (owner->AI_AlertIndex < 5)
	{
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("     %s - alert level too low, dropping back to Agitated Searching\r",owner->name.c_str()); // grayman debug
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void CombatState::OnTactileAlert(idEntity* tactEnt)
{
	// do nothing as of now, we are already in combat mode
}

void CombatState::OnVisualAlert(idActor* enemy)
{
	// do nothing as of now, we are already in combat mode
}

void CombatState::OnAudioAlert()
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	memory.alertClass = EAlertAudio;
	memory.alertPos = owner->GetSndDir();

	if (!owner->AI_ENEMY_VISIBLE)
	{
		if (owner->m_ignorePlayer) // grayman debug
		{
			memory.lastTimeEnemySeen = gameLocal.time;
		}
		owner->lastReachableEnemyPos = memory.alertPos;
		// gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), memory.alertPos, 2, 1000);
	}
}

void CombatState::OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead)
{
	// Ignore failed knockout attempts in combat mode
}

void CombatState::OnPersonEncounter(idEntity* stimSource, idAI* owner)
{
	if (!stimSource->IsType(idActor::Type))
	{
		return; // No Actor, quit
	}

	if (owner->IsFriend(stimSource))
	{
		// Remember last time a friendly AI was seen
		owner->GetMemory().lastTimeFriendlyAISeen = gameLocal.time;
	}
	// angua: ignore other people during combat
}

void CombatState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	// Set end time to something invalid
	_endTime = -1;

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CombatState initialised.\r");
	assert(owner);
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("%d: CombatState::Init - %s\r",gameLocal.time,owner->name.c_str()); // grayman debug

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	if (!owner->GetMind()->PerformCombatCheck())
	{
		return;
	}

	if ( ( owner->GetMoveType() == MOVETYPE_SIT ) || ( owner->GetMoveType() == MOVETYPE_SLEEP) )
	{
		owner->GetUp();
	}

	// say something along the lines of "huh?"

	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("     %s barks 'Huh?'\r",owner->name.c_str()); // grayman debug
	// The communication system plays reaction bark
	CommMessagePtr message;
	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_alert1s", message)));

	// All remaining init code is moved into Think() and done in the EStateInit substate,
	// because the things it does need to occur after the initial reaction delay.

	// grayman debug
	// Add a delay before you process the remainder of Init().
	// The length of the delay depends on the distance to the enemy.

	// We have an enemy, store the enemy entity locally
	_enemy = owner->GetEnemy();
	idActor* enemy = _enemy.GetEntity();

	float dist2Enemy = ( enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin() ).LengthFast();
	int reactionTime =  REACTION_TIME_MIN + (dist2Enemy*(REACTION_TIME_MAX - REACTION_TIME_MIN))/(cv_ai_sight_combat_cutoff.GetFloat()/s_DOOM_TO_METERS);
	if ( reactionTime > REACTION_TIME_MAX )
	{
		reactionTime = REACTION_TIME_MAX;
	}

	_combatSubState = EStateReaction;
	_reactionEndTime = gameLocal.time + reactionTime;
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Init - %s - reactionTime = %d\r",owner->name.c_str(),reactionTime); // grayman debug
}

// Gets called each time the mind is thinking
void CombatState::Think(idAI* owner)
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("%d: CombatState::Think - %s\r",gameLocal.time,owner->name.c_str()); // grayman debug
	// Do we have an expiry date?
	if (_endTime > 0)
	{
		if (gameLocal.time >= _endTime)
		{
			owner->GetMind()->EndState();
		}

		return;
	}

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		owner->GetMind()->EndState();
		return;
	}

	idActor* enemy = _enemy.GetEntity();

	if (!CheckEnemyStatus(enemy, owner))
	{
		owner->GetMind()->EndState();
		return; // state has ended
	}

	// angua: look at enemy
	owner->Event_LookAtPosition(enemy->GetEyePosition(), gameLocal.msec);

	Memory& memory = owner->GetMemory();

	switch(_combatSubState)
	{
	case EStateReaction:
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - EStateReaction\r",owner->name.c_str()); // grayman debug
		if ( gameLocal.time >= _reactionEndTime )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - EStateReaction delay is over\r",owner->name.c_str()); // grayman debug

			// Check to see if the enemy is still visible.

			if ( !owner->CanSee(enemy, true) )
			{
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - ClearEnemy()\r",owner->name.c_str()); // grayman debug
				owner->ClearEnemy();
				owner->SetAlertLevel(owner->thresh_5 - 0.1); // reset alert level just under Combat
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - Lost the enemy, so drop down to Agitated Searching\r",owner->name.c_str()); // grayman debug
				owner->GetMind()->EndState();
				return;
			}

			// Can still see the enemy, so proceed with Combat

			_combatSubState = EStateInit;
			return;
		}

		// Is there anything you need to watch out for while in this substate?

		break;

	case EStateInit:
	{
		owner->m_ignorePlayer = false; // grayman debug - clear flag that prevents mission statistics on player sightings
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - EStateInit\r",owner->name.c_str()); // grayman debug
		// The AI has processed his reaction, and need to move into combat, or flee.

		// Handle the things you were doing in Init() ...

		// greebo: Check for weapons and flee if we are unarmed.
		_criticalHealth = owner->spawnArgs.GetInt("health_critical", "0");
		_meleePossible = owner->GetNumMeleeWeapons() > 0;
		_rangedPossible = owner->GetNumRangedWeapons() > 0;

		if (!_meleePossible && !_rangedPossible)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm unarmed, I'm afraid!\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		// greebo: Check for civilian AI, which will always flee in face of a combat (this is a temporary query)
		if (owner->spawnArgs.GetBool("is_civilian", "0"))
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm civilian. I'm afraid.\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		owner->StopMove(MOVE_STATUS_DONE);
		memory.stopRelight = true; // grayman #2603 - abort a relight in progress
		memory.stopExaminingRope = true; // grayman #2872 - stop examining a rope

		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		owner->DrawWeapon();

		// Fill the subsystems with their tasks

		// This will hold the message to be delivered with the bark, if appropriate
		CommMessagePtr message;
	
		// Only alert the bystanders if we didn't receive the alert by message ourselves
		if (!memory.alertedDueToCommunication)
		{
			message = CommMessagePtr(new CommMessage(
				CommMessage::DetectedEnemy_CommType, 
				owner, NULL, // from this AI to anyone 
				enemy,
				memory.lastEnemyPos
			));
		}

		// The communication system plays starting bark
		idPlayer* player(NULL);
		if (enemy->IsType(idPlayer::Type))
		{
			player = static_cast<idPlayer*>(enemy);
		}

		if (player && player->m_bShoulderingBody)
		{
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_spotted_player_with_body", message)));
		}
		else if ((MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen)) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK)
		{
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_to_combat_company", message)));
		}
		else
		{
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_to_combat", message)));
		}

		// Ranged combat
		if (_rangedPossible)
		{
			if (_meleePossible && 
				(owner->GetPhysics()->GetOrigin()-enemy->GetPhysics()->GetOrigin()).LengthFast() < 3 * owner->GetMeleeRange())
			{
				ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
				chaseEnemy->SetEnemy(enemy);
				owner->movementSubsystem->PushTask(chaseEnemy);

				owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
				_combatType = COMBAT_MELEE;
			}
			else
			{
				owner->actionSubsystem->PushTask(RangedCombatTask::CreateInstance());
				owner->movementSubsystem->PushTask(ChaseEnemyRangedTask::CreateInstance());
				_combatType = COMBAT_RANGED;
			}
		}
		// Melee combat
		else
		{
			// The movement subsystem should start running to the last enemy position
			ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
			chaseEnemy->SetEnemy(enemy);
			owner->movementSubsystem->PushTask(chaseEnemy);

			owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
			_combatType = COMBAT_MELEE;
		}

		// Let the AI update their weapons (make them nonsolid)
		owner->UpdateAttachmentContents(false);

		_combatSubState = EStateThink;
		break;
	}

	case EStateThink:
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("CombatState::Think - %s - EStateThink\r",owner->name.c_str()); // grayman debug

		// Flee if we are damaged and the current melee action is finished
		if ( ( owner->health < _criticalHealth ) && ( owner->m_MeleeStatus.m_ActionState == MELEEACTION_READY ) )
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm badly hurt, I'm afraid!\r");
			owner->GetMind()->SwitchState(STATE_FLEE);
			return;
		}

		if (owner->GetMoveType() != MOVETYPE_ANIM)
		{
			owner->GetUp();
			return;
		}

		// Switch between melee and ranged combat based on enemy distance
		float enemyDist = (owner->GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin()).LengthFast();

		if ( ( _combatType == COMBAT_MELEE ) && _rangedPossible && ( enemyDist > 3 * owner->GetMeleeRange() ) )
		{
			owner->movementSubsystem->ClearTasks();
			owner->actionSubsystem->ClearTasks();

			owner->actionSubsystem->PushTask(RangedCombatTask::CreateInstance());
			owner->movementSubsystem->PushTask(ChaseEnemyRangedTask::CreateInstance());
			_combatType = COMBAT_RANGED;
		}

		if ( ( _combatType == COMBAT_RANGED ) && _meleePossible && ( enemyDist <= 3 * owner->GetMeleeRange() ) )
		{
			owner->movementSubsystem->ClearTasks();
			owner->actionSubsystem->ClearTasks();

			// Allocate a ChaseEnemyTask
			owner->movementSubsystem->PushTask(TaskPtr(new ChaseEnemyTask(enemy)));

			owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
			_combatType = COMBAT_MELEE;
		}

		// Check the distance to the enemy, the subsystem tasks need it.
		memory.canHitEnemy = owner->CanHitEntity(enemy, _combatType);
		if ( owner->m_bMeleePredictProximity )
		{
			memory.willBeAbleToHitEnemy = owner->WillBeAbleToHitEntity(enemy, _combatType);
		}

		// Check whether the enemy can hit us in the near future
		memory.canBeHitByEnemy = owner->CanBeHitByEntity(enemy, _combatType);

		if (!owner->AI_ENEMY_VISIBLE && 
			(( _combatType == COMBAT_MELEE  && !memory.canHitEnemy) || _combatType == COMBAT_RANGED))
		{
			// The enemy is not visible, let's keep track of him for a small amount of time
			if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_CHASE_TIME)
			{
				// Cheat a bit and take the last reachable position as "visible & reachable"
				owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
			}
			else if (owner->ReachedPos(owner->lastVisibleReachableEnemyPos, MOVE_TO_POSITION) || 
					( gameLocal.time - memory.lastTimeEnemySeen > 2 * MAX_BLIND_CHASE_TIME) )
			{
				// BLIND_CHASE_TIME has expired, we have lost the enemy!
				owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
				return;
			}
		}

		break;
	}
}

bool CombatState::CheckEnemyStatus(idActor* enemy, idAI* owner)
{
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		owner->GetMind()->EndState();
		return false;
	}

	if (enemy->AI_DEAD)
	{
		owner->ClearEnemy();
		owner->StopMove(MOVE_STATUS_DONE);

		// Stop doing melee fighting
		owner->actionSubsystem->ClearTasks();

		// TODO: Check if more enemies are in range
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);

		// Emit the killed player bark
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_killed_enemy"))
		);

		// Set the expiration date of this state
		_endTime = gameLocal.time + 3000;

		return false;
	}

	if (!owner->IsEnemy(enemy))
	{
		// angua: the relation to the enemy has changed, this is not an enemy any more
		owner->StopMove(MOVE_STATUS_DONE);
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);
		owner->ClearEnemy();
		owner->GetMind()->EndState();
		
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();

		return false;
	}

	return true; // Enemy still alive and kicking
}

void CombatState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_criticalHealth);
	savefile->WriteBool(_meleePossible);
	savefile->WriteBool(_rangedPossible);

	savefile->WriteInt(static_cast<int>(_combatType));

	_enemy.Save(savefile);

	savefile->WriteInt(_endTime);

	savefile->WriteInt(static_cast<int>(_combatSubState)); // grayman debug

	savefile->WriteInt(_reactionEndTime); // grayman debug
}

void CombatState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_criticalHealth);
	savefile->ReadBool(_meleePossible);
	savefile->ReadBool(_rangedPossible);

	int temp;
	savefile->ReadInt(temp);
	_combatType = static_cast<ECombatType>(temp);

	_enemy.Restore(savefile);

	savefile->ReadInt(_endTime);

	// grayman debug
	savefile->ReadInt(temp);
	_combatSubState = static_cast<ECombatSubState>(temp);

	savefile->ReadInt(_reactionEndTime); // grayman debug
}

StatePtr CombatState::CreateInstance()
{
	return StatePtr(new CombatState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar combatStateRegistrar(
	STATE_COMBAT, // Task Name
	StateLibrary::CreateInstanceFunc(&CombatState::CreateInstance) // Instance creation callback
);

} // namespace ai
