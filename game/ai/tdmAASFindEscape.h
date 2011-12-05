/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __tdmAASFindEscape_H__
#define __tdmAASFindEscape_H__

#include "AAS.h"

/**
 * greebo: This evaluator is designed to find escape routes
 *         for fleeing actors. The TestArea method evaluates area
 *         candidates based on their distance to the threatening entity
 *         and the current AI location.
 */
class tdmAASFindEscape : public idAASCallback {
public:
	// Constructor
	tdmAASFindEscape(
		const idVec3& threatPosition, 
		const idVec3& selfPosition, 
		float minDistToThreat,
		float minDistToSelf
	);

	virtual bool		TestArea(const idAAS *aas, int areaNum);

	// Returns the best escape goal found
	inline aasGoal_t&	GetEscapeGoal() { return _goal; };

private:
	idVec3				_threatPosition;
	idVec3				_selfPosition;
	float				_minDistThreatSqr;
	float				_minDistSelfSqr;

	aasGoal_t			_goal;
	float				_bestDistSqr;
	int					_bestAreaNum;
};

#endif /* __tdmAASFindEscape_H__ */
