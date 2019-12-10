#include "spaceworld.h"

SpaceWorld::SpaceWorld(){}

SpaceWorld::SpaceWorld(string filename):
  randomStart(true), mapPoolSize(-1)
{
  //    paramLabels = {"Gavity", "Damping","Planet Density", "Ship Mass", "Ship Maximum Thrust", "Step Size", "Win Reward", "Lose Reward", "Wrong signal on Waypoint Reward", "Signal Off Waypoint Reward"};
  //   paramValues = {GRAVITY,DAMPING,PLANET_DENSITY,SHIP_MASS,SHIP_MAX_THRUST,STEP_SIZE,RIGHT_SIGNAL_ON_WAYPOINT_REWARD, CRASH_REWARD, WRONG_SIGNAL_ON_WAYPOINT_REWARD,SIGNAL_OFF_WAYPOINT_REWARD};
  map.load(filename);
  init();
}

SpaceWorld::SpaceWorld(string filename, Ship s):
  ship(s), initShip(s), randomStart(false), mapPoolSize(-1)
{
  map.load(filename);
  init();
}

SpaceWorld::SpaceWorld(string pathToDir, int mapPoolSize):
  mapPoolPath(pathToDir), randomStart(true), mapPoolSize(mapPoolSize)
{
  map.load(mapPoolPath+"map0");
  init();
}

void SpaceWorld::init()
{
  vector<DiscreteAction> dactions = {DiscreteAction(waypoints.size()+1)};
  vector<ContinuousAction> cactions {ContinuousAction(0,SHIP_MAX_THRUST), ContinuousAction(0,2*M_PI)};
  actions = ActionSpace(dactions,cactions);
  takenAction = vector<float>(3,0);
  size = map.getSize();
  currentState.setStateVector(vector<float>(5+3*(map.getPlanets().size()+map.getWaypoints().size()),0));
  ship.setWidth(SHIP_WIDTH);
  ship.setHeight(SHIP_HEIGHT);
  reset();
}


float SpaceWorld::transition()
{
  previousState.update(0,ship.getP().x),previousState.update(1,ship.getP().y);
  previousState.update(2,ship.getV().x), previousState.update(3,ship.getV().y);
  
  int signal = (int)takenAction[0];
  float thrustPow = takenAction[1];
  float thrustOri = takenAction[2];
  float r;  
  if (!isTerminal(currentState))
    {
      //Transition function

      if (!isCrashed())
	{
	  ship.setThrust(Vect2d(cos(thrustOri),sin(thrustOri)).dilate(thrustPow));
	  ship.setSignalColor(signal);
	  Vect2d gravForce(0,0);
	  for (unsigned int i=0;i<planets.size();i++)
	    {
	      Planet p = planets[i];
	      Vect2d vectPS = p.getCentre().sum(ship.getP().dilate(-1));
	      gravForce = gravForce.sum(vectPS.dilate(GRAVITY*SHIP_MASS*p.getMass()/pow(vectPS.norm(),3)));
	      ship.setA(Vect2d(gravForce.x-DAMPING*ship.getV().x-ship.getThrust().x,gravForce.y-DAMPING*ship.getV().y-ship.getThrust().y).dilate(1./SHIP_MASS));
	      ship.setP(ship.getP().sum(ship.getV().dilate(STEP_SIZE)));
	      ship.setV(ship.getV().sum(ship.getA().dilate(STEP_SIZE)));
	    }
	  currentState.update(0,ship.getP().x),currentState.update(1,ship.getP().y);
	  currentState.update(2,ship.getV().x), currentState.update(3,ship.getV().y);	  
	}            
      //Reward function
      
      if (isCrashed())
	{
	  r = CRASH_REWARD;
	}            
      if (ship.getSignalColor() != actions.getDiscreteActions()[0].getSize())
	{
	  for (unsigned int i=0;i<waypoints.size();i++)
	    {
	      if (ship.getP().distance(waypoints[i].getCentre()) < waypoints[i].getRadius())
		{
		  if (ship.getSignalColor() == i)
		    {
		      r = RIGHT_SIGNAL_ON_WAYPOINT_REWARD;
		    }
		  else
		    {
		      r = WRONG_SIGNAL_ON_WAYPOINT_REWARD;
		    }
		}
	      r = SIGNAL_OFF_WAYPOINT_REWARD;	  
	    }
	}
    }
  actionSequence.push_back({signal,thrustPow,thrustOri});
  stateSequence.push_back(currentState.getStateVector());
  rewardHistory.back()+=r;
  return r;
}

bool SpaceWorld::isTerminal(State s)
{
    for (unsigned int i=0;i<planets.size();i++)
    {
        if (ship.getP().distance(planets[i].getCentre()) < ship.getWidth()+planets[i].getRadius())
        {
            return true;
        }
    }
    if (ship.getSignalColor() != actions.getDiscreteActions()[0].getSize())
    {
        for (unsigned int i=0;i<waypoints.size();i++)
        {
            if (ship.getP().distance(waypoints[i].getCentre()) < waypoints[i].getRadius())
            {
                if (ship.getSignalColor() == i)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void SpaceWorld::generateVectorStates()
{
  currentState.update(0,ship.getP().x), currentState.update(1,ship.getP().y), currentState.update(2,ship.getV().x), currentState.update(3,ship.getV().y);    
  
  for (unsigned int i=0;i<waypoints.size();i++)
    {
      currentState.update(3*i+4,waypoints[i].getCentre().x);
      currentState.update(3*i+5,waypoints[i].getCentre().y);
      currentState.update(3*i+6,waypoints[i].getRadius());
    }
  for (unsigned int i=0;i<planets.size();i++)
    {
      currentState.update(3*i+3*waypoints.size()+4,planets[i].getCentre().x);
      currentState.update(3*i+3*waypoints.size()+5,planets[i].getCentre().y);
      currentState.update(3*i+3*waypoints.size()+6,planets[i].getRadius());
    }
  previousState = currentState;
}

void SpaceWorld::reset()
{
  rewardHistory.push_back(0);  
  if (mapPoolSize!=-1)
    {
      default_random_engine generator(std::random_device{}());
      uniform_int_distribution<int> dist(0,mapPoolSize-1);
      int mapId = dist(generator);
      map.load(mapPoolPath+"map"+to_string(mapId));
      size = map.getSize();      
    }
  if (planets.size() == 0 || mapPoolSize != -1)
    {
      planets = map.getPlanets();
      for (unsigned int i=0;i<planets.size();i++)
	{
	  planets[i].setMass(4*PLANET_DENSITY*pow(planets[i].getRadius(),3)*M_PI/3);
	}
      waypoints = map.getWaypoints();
      ship = map.getShip();
    }
  placeShip();
  ship.setSignalColor(waypoints.size());
}

void SpaceWorld::placeShip()
{
  default_random_engine generator(std::random_device{}());
  uniform_int_distribution<int> dist(0,size-ship.getHeight());
  if (randomStart)
    {
      bool invalidPosition = true;
      while (invalidPosition)
	{
	  invalidPosition = false;
	  Vect2d spawn(dist(generator),dist(generator));
	  ship.setP(spawn);
	  for (unsigned int i=0;i<planets.size();i++)
	    {
	      if (spawn.distance(planets[i].getCentre()) < 1.1 * (ship.getHeight()+planets[i].getRadius())) {
                invalidPosition = true;
                break;
	      }
        }
	  if (!invalidPosition)
	    {
	      for (unsigned int i=0;i<waypoints.size();i++)
		{
		  if (spawn.distance(waypoints[i].getCentre()) < 1.1 * (waypoints[i].getRadius() + ship.getHeight()))
		    {
		      invalidPosition = true;
		      break;
		    }
		}
	    }
	}
    }
  else
    {
      ship = initShip;
    }
  generateVectorStates();
  stateSequence = {currentState.getStateVector()};
}

 bool SpaceWorld::isCrashed()
 {
   for (unsigned int i=0;i<planets.size();i++)
     {
       if (ship.getP().distance(planets[i].getCentre()) < ship.getWidth()+planets[i].getRadius())
	 {
	   return true;
	 }
     }
   return false;
 }

 
 
