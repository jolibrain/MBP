#include "controllergw.h"

ControllerGW::ControllerGW()
{
}

ControllerGW::ControllerGW(string mapTag)
{
    randomStart = true;
    init(mapTag);
    default_random_engine generator(std::random_device{}());
    uniform_int_distribution<int> dist(1,size-1);
    agentX = dist(generator), agentY = dist(generator);
    while ((agentX == goalX && agentY == goalY) || obstacles[agentX][agentY] == 1)
    {
        agentX = dist(generator), agentY = dist(generator);
    }
}

ControllerGW::ControllerGW(string mapTag, float agentXInit, float agentYInit):
    initX(agentXInit), initY(agentYInit), agentX(agentXInit),agentY(agentYInit)
{
    randomStart=false;
    init(mapTag);
}

void ControllerGW::init(string mapTag)
{
    MapGW map;
    map.load(mapTag);
    vector<DiscreteAction> dactions = {DiscreteAction(4)};
    actions = ActionSpace(dactions, vector<ContinuousAction>());
    rewardHistory.push_back(0);
    takenAction = vector<float>(1,0);
    size = map.getSize();
    path = "../GridWorld/Map"+mapTag+"/";
    for (int i=0;i<size;i++)
    {
        obstacles.push_back(vector<float>(map.getSize(),0));
    }

    for (int i=0;i<size;i++)
    {
        for (int j=0;j<size;j++)
        {
            switch(map.getMap()[i][j])
            {
            case 1:
                obstacles[i][j]=1;
                break;
            case 2:
                goalX=i, goalY=j;
                break;
            }
        }
    }
}

float ControllerGW::transition()
{
    int a = (int)takenAction[0];
    float r = 0;
    previousState.update(0,agentX), previousState.update(1,agentY);
    if (!isTerminal(currentState))
    {
        switch (a)
        {
        case 0:
            agentX--;
            break;
        case 1:
            agentY++;
            break;
        case 2:
            agentX++;
            break;
        case 3:
            agentY--;
            break;
        }
        currentState.update(0,agentX), currentState.update(1,agentY);
        actionSequence.push_back({a});
        stateSequence.push_back(currentState.getStateVector());
    }   
    if (obstacles[agentX][agentY] == 1)
    {
        r = LOSE_REWARD;
    }
    else if (agentX == goalX && agentY == goalY)
    {
        r = WIN_REWARD;
    }
    else
    {
        r = EMPTY_SQUARE_REWARD;
    }
    rewardHistory.back()+= r;//*(actionSequence.size());
    return r;
}

bool ControllerGW::isTerminal(State s)
{
    float ax = s.getStateVector()[0];
    float ay = s.getStateVector()[1];
    return obstacles[ax][ay] == 1 || (ax == goalX && ay == goalY);
}

void ControllerGW::generateStates()
{
    currentState.add(agentX),currentState.add(agentY),
            currentState.add(goalX), currentState.add(goalY);
    for (int i=0;i<size;i++)
    {
        for (int j=0;j<size;j++)
        {
            currentState.add(obstacles[i][j]);
        }
    }
    previousState = State(currentState);
    stateSequence.push_back(currentState.getStateVector());
}

void ControllerGW::generateImage()
{

}

int ControllerGW::stateId(State s)
{
    float ax = s.getStateVector()[0];
    float ay = s.getStateVector()[1];
    return ax*size+ay;
}

void ControllerGW::reset()
{
    rewardHistory.push_back(0);
    if (randomStart)
    {
        default_random_engine generator(std::random_device{}());
        uniform_int_distribution<int> dist(1,size-1);
        agentX = dist(generator), agentY = dist(generator);
        while ((agentX == goalX && agentY == goalY) || obstacles[agentX][agentY] == 1)
        {
            agentX = dist(generator), agentY = dist(generator);
        }
    }
    else
    {
        agentX = initX; agentY = initY;
    }
    currentState.update(0,agentX), currentState.update(1,agentY);
    actionSequence = vector<vector<float>>();
    stateSequence = {currentState.getStateVector()};
}

vector<int> ControllerGW::accessibleStates(State s)
{
    int ax = s.getStateVector()[0];
    int ay = s.getStateVector()[1];
    vector<int> accessibleStates = {(ax-1)*size+ay,ax*size+ay+1,(ax+1)*size+ay,ax*size+ay-1};
    return accessibleStates;
}

int ControllerGW::spaceStateSize()
{
    return size*size;
}

int ControllerGW::getSize() const
{
    return size;
}
