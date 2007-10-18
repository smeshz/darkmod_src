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

static bool init_version = FileVersionList("$Id: IdleSensoryTask.cpp 1435 2007-10-16 16:53:28Z greebo $", init_version);

#include "IdleSensoryTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

	namespace
	{
		// Some constants for random head turning
		const float MIN_CLOCKWISE_YAW = -60;
		const float MAX_CLOCKWISE_YAW = 60;
		const float MIN_PITCH = -40;
		const float MAX_PITCH = 40;
		const float MIN_DURATION_SECONDS = 1;
		const float MAX_DURATION_SECONDS = 3;
	}

// Get the name of this task
const idStr& IdleSensoryTask::GetName() const
{
	static idStr _name(TASK_IDLE_SENSORY);
	return _name;
}

void IdleSensoryTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (owner->GetMind()->GetMemory().lastRandomHeadTurnCheckTime == -1)
	{
		owner->GetMind()->GetMemory().lastRandomHeadTurnCheckTime = gameLocal.time;
	}
}

void IdleSensoryTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO).LogString("IdleSensory Task performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	PerformRandomHeadTurnCheck();
}

void IdleSensoryTask::PerformRandomHeadTurnCheck()
{
	// greebo: This routine has been ported from the scripts written by SophisticatedZombie (SZ)
	idAI* owner = _owner.GetEntity();

	// SZ: Dec 30, 2006
	// The time between random head turns is now affected by how many out of place things
	// they have witnessed. In other words, they get more agitated and nervous and look around
	// more if they have seen too many things out of place.
	int timeMultiplier = owner->GetMind()->GetMemory().countEvidenceOfIntruders;

	if (timeMultiplier > 5)
	{
		timeMultiplier = 5;
	}
	else if (timeMultiplier < 1)
	{
		timeMultiplier = 1;
	}

	// Check if it is time to see if we should turn our head, don't go below 1 second
	float nowTime = gameLocal.time;
	if (((nowTime - owner->GetMind()->GetMemory().lastRandomHeadTurnCheckTime) * timeMultiplier) < 1000)
	{
		return;
	}
	owner->GetMind()->GetMemory().lastRandomHeadTurnCheckTime = nowTime;

	// Chance to turn head
	float chance = owner->AI_chancePerSecond_RandomLookAroundWhileIdle;

	float rand = gameLocal.random.RandomFloat();
	if (rand >= chance)
	{
		// Nope
		return;
	}

	// Generate yaw angle in degrees
	float range = MAX_CLOCKWISE_YAW - MIN_CLOCKWISE_YAW;
	float headYawAngle = gameLocal.random.RandomFloat()*range + MIN_CLOCKWISE_YAW;
	
	// Generate pitch angle in degrees
	range = MAX_PITCH - MIN_PITCH;
	float headPitchAngle = gameLocal.random.RandomFloat()*range + MIN_PITCH;

	// Generate duration in seconds
	range = MAX_DURATION_SECONDS - MIN_DURATION_SECONDS;
	float durationInSeconds = gameLocal.random.RandomFloat()*range + MIN_DURATION_SECONDS;
	
	// Call event
	owner->Event_LookAtAngles(headYawAngle, headPitchAngle, 0.0, durationInSeconds);
}

// Save/Restore methods
void IdleSensoryTask::Save(idSaveGame* savefile) const
{
	// TODO
}

void IdleSensoryTask::Restore(idRestoreGame* savefile)
{
	// TODO
}

IdleSensoryTaskPtr IdleSensoryTask::CreateInstance()
{
	return IdleSensoryTaskPtr(new IdleSensoryTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar idleSensoryTaskRegistrar(
	TASK_IDLE_SENSORY, // Task Name
	TaskLibrary::CreateInstanceFunc(&IdleSensoryTask::CreateInstance) // Instance creation callback
);

} // namespace ai
