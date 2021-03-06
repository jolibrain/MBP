#ifndef WORLD_H
#define WORLD_H
#include "state.h"
#include "actionspace.h"

class World
{
public:
    World();
    /**
     * @brief transition
     * This method uses the value of takenAction to update the currentState using the laws of the world.
     * This method also saves the value of the state before update in previousState.
     * After the state is updated, the reward is computed depending on the previousState, takenAction, currentState transition
     * @return the reward for taking takenAction and landing in currentState
     */
    virtual float transition(vector<float> action);

    /**
     * @brief isTerminal
     * This method detects whether or not a state is a terminal state (i.e stops the episode)
     * @param s The tested state
     * @return True if s is terminal, false otherwise
     */
    virtual bool isTerminal(State s);

    /**
     * @brief generateStates
     * Generates the state vectors for previous state and current state condensing the world's vision of
     * the world into a generic representation
     */
    virtual void generateVectorStates();

    /**
     * @brief stateId
     * Attributes an Id to a state by defining an order between states
     * @param s the state being identified
     * @return the Id of state s or -1 if the state space is infinite
     */
    virtual int stateId(State s);

    /**
     * @brief reset
     * Resets the world. This method is called each time the trainer changes episode.
     */
    virtual void reset();

    /**
     * @brief accessibleStates
     * A simple way to get the accessible states
     * @param s the state being evaluated
     * @return A vector containing the id of each state that is accessible from s in one transition
     * or the vector {-1} there is an infinite amount of accessible states.
     */
    virtual vector<int> accessibleStates(State s);
    
    /**
     * @brief spaceStateSize
     * A quality of life method to access the size of the world's space state
     * @return The cardinal of the space state or -1 if the space state is infinite
     */
    virtual int spaceStateSize();
    vector<float> randomAction();
    void saveRewardHistory();
    void saveLastEpisode();
    void loadEpisode(string filename);
    int actionSpaceSize();
    int saPairSpaceSize();
    
    ActionSpace getActions() const;
    State getPreviousState() const;
    vector<float> getTakenAction() const;
    float getTakenReward() const;
    State getCurrentState() const;
    vector<float> getRewardHistory() const;
    void addToRewardHistory(float r);
    void updateTakenAction(int actionIndex, float value);
    void setActions(const ActionSpace &value);
    void setTakenAction(const vector<float> &value);
    void setTakenReward(float value);
    void setCurrentState(State s);
    string getTag() const;
    vector<vector<float> > getStateSequence() const;
    vector<vector<float> > getActionSequence() const;


protected:
    string tag; //Path and prefix for file saving 
    ActionSpace actions;
    
    State previousState;
    vector<float> takenAction;
    float takenReward;
    State currentState;
    
    vector<vector<float>> stateSequence;
    vector<vector<float>> actionSequence;
    vector<float> rewardHistory;
};

#endif // WORLD_H
