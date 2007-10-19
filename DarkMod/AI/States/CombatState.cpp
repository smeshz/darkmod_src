/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#include "../idlib/precompiled.h"
#pragma hdrstop

static bool init_version = FileVersionList("$Id: CombatState.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "CombatState.h"
#include "../Memory.h"
#include "../Tasks/EmptyTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

void CombatState::Init(idAI* owner)
{
	DM_LOG(LC_AI, LT_INFO).LogString("CombatState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMind()->GetMemory();

	// Issue a communication stim
	owner->IssueCommunication_Internal(
		static_cast<float>(CAIComm_Message::DetectedEnemy_CommType), 
		YELL_STIM_RADIUS, 
		NULL,
		owner->GetEnemy(),
		memory.lastEnemyPos
	);

	// greebo: Check for weapons and flee if we are unarmed.
	if (owner->GetNumMeleeWeapons() == 0 && owner->GetNumRangedWeapons() == 0)
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm unarmed, I'm afraid!\r");
		// TODO pushTaskIfHighestPriority("task_Flee", PRIORITY_FLEE);
		return;
	}

	// greebo: Check for civilian AI, which will always flee in face of a combat (this is a temporary query)
	if (owner->spawnArgs.GetBool("is_civilian", "0"))
	{
		DM_LOG(LC_AI, LT_INFO).LogString("I'm civilian. I'm afraid.\r");
		// TODO pushTaskIfHighestPriority("task_Flee", PRIORITY_FLEE);
		return;
	}

	// Check if the AI has an enemy.

	// Fill the subsystems with their tasks

	// The movement subsystem should start running to the last enemy position
	owner->GetSubsystem(SubsysMovement)->ClearTasks();
	owner->GetSubsystem(SubsysMovement)->QueueTask(EmptyTask::CreateInstance());

	// The communication system is barking in regular intervals
	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->GetSubsystem(SubsysCommunication)->QueueTask(EmptyTask::CreateInstance());

	// The sensory system does its Idle tasks
	owner->GetSubsystem(SubsysSenses)->ClearTasks();
	owner->GetSubsystem(SubsysSenses)->QueueTask(EmptyTask::CreateInstance());

	// No action so far
	owner->GetSubsystem(SubsysAction)->ClearTasks();
	owner->GetSubsystem(SubsysAction)->QueueTask(EmptyTask::CreateInstance());
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
