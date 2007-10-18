/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2007-10-16 18:53:28 +0200 (Di, 16 Okt 2007) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_MIND_H__
#define __AI_MIND_H__

#include "../idlib/precompiled.h"

#include <boost/shared_ptr.hpp>

namespace ai
{

// Forward Declaration
class State;
typedef boost::shared_ptr<State> StatePtr;

class Memory;

enum EAlertState {
	ERelaxed,
	EAroused,
	EInvestigating,
	EAgitatedSearching,
	ECombat,
};

/**
 * greebo: This defines the ABC of an AI mind. It basically
 *         handles the incoming stimuli and emits signals to the 
 *         AI subsystems like movement, interaction and sensory stuff.
 */
class Mind
{
public:
	/**
	 * greebo: This should be called each frame to let the AI
	 *         think. This distributes the call to the various
	 *         subsystem's Think() methods, maybe in and interleaved way.
	 */
	virtual void Think() = 0;

	/**
	 * greebo: Sets the new state of this mind (this can be Idle, Combat).
	 * This new state is attached and its Init() method is invoked.
	 */
	virtual void ChangeState(const idStr& stateName) = 0;

	/**
	 * Returns the reference to the current state (can be NULL).
	 */
	virtual StatePtr& GetState() = 0;

	// Returns the Memory structure, which holds the various mind variables
	virtual Memory& GetMemory() = 0;

	// Save/Restore routines
	virtual void Save(idSaveGame* savefile) const = 0;
	virtual void Restore(idRestoreGame* savefile) = 0;
};
typedef boost::shared_ptr<Mind> MindPtr;

} // namespace ai

#endif /*__AI_MIND_H__*/
