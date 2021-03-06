#include "qlearning.h"

template <class C>
QLearning<C>::QLearning(): episodeId(0)
{
}

template <class W>
QLearning<W>::QLearning(W world):
  Agent<W>(world),episodeId(0)
{
    //Initialising the Q values to 0 for each state action pair

    for (int i=0;i<world.spaceStateSize();i++)
    {
        qvalues.push_back(vector<float>(this->actions().cardinal(),0));
    }
}

template <class W>
void QLearning<W>::epsilonGreedyPolicy(float e)
{
    default_random_engine generator = default_random_engine(random_device{}());
    uniform_real_distribution<float> dist(0,1);
    if (dist(generator) < e)
    {
        for (int i=0;i<this->daSize();i++)
        {
            this->world.updateTakenAction(i,this->discreteActions()[i].pick());
        }
    }
    else
    {
        vector<float> possibleQValues;

        for (int i=0;i<this->actions().cardinal();i++)
        {
            possibleQValues.push_back(qvalues[this->world.stateId(this->currentState())][i]);
        }
        float maxQValue = *max_element(possibleQValues.begin(),possibleQValues.end());
	  
        //If several qvalues are equal to the max, pick one of them randomly

        vector<int> indexOfMax;
        for (unsigned int i=0;i<possibleQValues.size();i++)
        {
            if (possibleQValues[i]==maxQValue)
            {
                indexOfMax.push_back(i);
            }
        }
	
        default_random_engine generator(random_device{}());
        uniform_int_distribution<int> dist(0,indexOfMax.size()-1);
	int actionId = indexOfMax[dist(generator)];
        this->world.setTakenAction(this->actions().actionFromId(actionId,new vector<float>()));
		  
    }
    this->world.setTakenReward(this->world.transition(this->takenAction()));
}

template <class W>
void QLearning<W>::updateQValues()
{
    int psIndex = this->world.stateId(this->previousState());
    int csIndex = this->world.stateId(this->currentState());
    int actionId = this->actions().idFromAction(this->takenAction());
    if (this->world.isTerminal(this->previousState()))
    {
        for (int i=0;i<this->actions().cardinal();i++)
        {
            qvalues[psIndex][3-i] += (1./(this->episodeId+1))*(this->takenReward()+gamma*qvalues[psIndex][0]-qvalues[psIndex][0]);
        }
    }
    else
    {
        vector<float> updateChoice;
        for (int i=0;i<this->actions().cardinal();i++)
        {
            updateChoice.push_back(qvalues[csIndex][i]);
        }
        float bestChoice = *max_element(updateChoice.begin(),updateChoice.end());
        qvalues[psIndex][actionId] += (1./(sqrt(this->episodeId)+1))*(this->takenReward()+gamma*bestChoice-qvalues[psIndex][actionId]);
    }
}

template <class W>
void QLearning<W>::train(int nEpisodes, float epsilon, float gamma)
{
  this->epsilon = epsilon, this->gamma=gamma;
  float e = epsilon;
    for (int k=0;k<nEpisodes;k++)
    {
      e = epsilon*exp(-k*5./nEpisodes); //Decreasing exploration rate throughout training

        //Displaying a progression bar in the terminal

        if (nEpisodes > 100 && k%(5*nEpisodes/100) == 0)
        {
            cout << "Training in progress... " + to_string(k/(nEpisodes/100)) + "%" << endl;
        }
        bool terminal = false;
        while(!terminal)
        {
	  epsilonGreedyPolicy(e);
	  terminal = this->world.isTerminal(this->currentState());
	  updateQValues();
        }
        this->world.transition(this->takenAction());
        updateQValues();
        episodeId++;
        this->world.reset();
    }
}

template<class W>
void QLearning<W>::playOne()
{
    bool terminal = false;
    while(!terminal)
    {
      epsilonGreedyPolicy(0);
      terminal = this->world.isTerminal(this->currentState().getStateVector());
    }
    this->saveLastEpisode();    
}

template <class W>
void QLearning<W>::saveQValues(string filename)
{
  ofstream f(filename);
    if (f)
    {
        f << to_string(this->epsilon) << endl;
        f << to_string(gamma) << endl;
        for (unsigned int i=0;i<qvalues.size();i++)
        {
            for (unsigned int j=0;j<qvalues[0].size();j++)
            {
                f << to_string(qvalues[i][j]) + " ";
            }
            f << endl;
        }
    }
    else
    {
        cout << "An error has occured while trying to save the qvalues for qlearning" << endl;
    }
}

template <class W>
void QLearning<W>::loadQValues(string filename)
{
    ifstream f(filename);
    if (f)
    {
        string line;
        getline(f,line);
        this->epsilon = stof(line);
        getline(f,line);
        gamma = stof(line);
        for (unsigned int i=0;i<qvalues.size();i++)
        {
            getline(f,line);            
            int k=0;
            for (unsigned int j=0;j<qvalues[0].size();j++)
            {
                string num;
                while(line[k]!=' ')
                {
                    num+=line[k];
                    k++;
                }
                qvalues[i][j] = stof(num);
                k++;
            }
        }
    }
    else
    {
        cout << "An error has occured while trying to load the policy file" << endl;
    }
}


template <class W>
void QLearning<W>::saveTrainingData()
{
  
}

template class QLearning<GridWorld>;
