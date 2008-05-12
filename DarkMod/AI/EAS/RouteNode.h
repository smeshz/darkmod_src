/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Revision: 1435 $
 * $Date: 2008-05-12 18:53:28 +0200 (Mo, 12 May 2008) $
 * $Author: greebo $
 *
 ***************************************************************************/

#ifndef __AI_EAS_ROUTENODE_H__
#define __AI_EAS_ROUTENODE_H__

#include <list>
#include <boost/shared_ptr.hpp>

namespace eas {

enum ActionType {
	ACTION_WALK = 0,		// AI just needs to walk to the target
	ACTION_USE_ELEVATOR,	// AI needs to use an elevator
	NUM_ACTIONS,
};

struct RouteNode
{
	ActionType type;		// what needs to be done in this route section (walk?, use elevator?)
	int toArea;				// the target AAS area number
	int toCluster;			// the target AAS cluster number
	int elevator;			// the elevator number (is -1 if no elevator to be used in this node)

	RouteNode() :
		type(ACTION_WALK),
		toArea(0),
		toCluster(0),
		elevator(-1)
	{}

	RouteNode(ActionType t, int goalArea, int goalCluster, int elevatorNum = -1) :
		type(t),
		toArea(goalArea),
		toCluster(goalCluster),
		elevator(elevatorNum)
	{}

	// Copy constructor
	RouteNode(const RouteNode& other) :
		type(other.type),
		toArea(other.toArea),
		toCluster(other.toCluster),
		elevator(other.elevator)
	{}

	bool operator==(const RouteNode& other) const
	{
		return (type == other.type && toArea == other.toArea && toCluster == other.toCluster && elevator == other.elevator);
	}

	bool operator!=(const RouteNode& other) const
	{
		return !operator==(other);
	}
};
typedef boost::shared_ptr<RouteNode> RouteNodePtr;
typedef std::list<RouteNodePtr> RouteNodeList;

} // namespace eas

#endif /* __AI_EAS_ROUTENODE_H__ */
