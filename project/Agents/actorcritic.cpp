#include "actorcritic.h"

template <class W, class M>
ActorCritic<W,M>::ActorCritic()
{
}

template <class W,class M>
ActorCritic<W,M>::ActorCritic(W world,M model): Agent<W>(world), model(model){}

template <class W,class M>
void ActorCritic<W,M>::evaluateRunValues()
{
    float nextReturn;
    float thisReturn;
    if (runAreTerminal[batchSize-1])
    {
        nextReturn = runRewards[batchSize-1];
    }
    else
    {
      torch::Tensor prediction = model->criticOutput(runStates[batchSize-1].unsqueeze(0));
      nextReturn = *prediction.to(torch::Device(torch::kCPU)).data<float>();
    }
    runValues[batchSize-1] = nextReturn;
    for (int i=batchSize-2;i>=0;i--)
      {
        if (runAreTerminal[i])
	  {
            nextReturn=0;
	  }
        thisReturn=runRewards[i] + gamma*nextReturn;
        runValues[i][0] = thisReturn;
        nextReturn = thisReturn;        
    }
}

template <class W,class M>
void ActorCritic<W,M>::backPropagate(torch::optim::Adam *opti)
{
  evaluateRunValues();
  torch::Tensor actionProbs = model->actorOutput(runStates);  
  torch::Tensor valuesEstimate = model->criticOutput(runStates);
  torch::Tensor actionLogProbs = actionProbs.log();
  torch::Tensor chosenActionLogProbs = actionLogProbs.gather(1,runActions.unsqueeze(1).to(torch::kLong)).to(torch::kFloat32);
  torch::Tensor advantages = runValues - valuesEstimate; //TD Error
  torch::Tensor entropy = -(actionProbs*actionLogProbs).sum(1).mean();
  torch::Tensor entropyLoss = beta*entropy;
  torch::Tensor policyLoss = -(chosenActionLogProbs*advantages).mean();
  torch::Tensor valueLoss = zeta*advantages.pow(2).mean();
  torch::Tensor totalLoss = valueLoss + policyLoss - entropyLoss;    
  policyLossHistory.push_back(*policyLoss.to(torch::Device(torch::kCPU)).data<float>());
  valueLossHistory.push_back(*valueLoss.to(torch::Device(torch::kCPU)).data<float>());
  entropyHistory.push_back(*entropyLoss.to(torch::Device(torch::kCPU)).data<float>());
  lossHistory.push_back(*totalLoss.to(torch::Device(torch::kCPU)).data<float>());
  opti->zero_grad();
  totalLoss.backward();
  //vector<torch::Tensor> param = model.parameters();
  //torch::nn::utils::clip_grad_norm_(param,0.5);
  opti->step();
}


template <class W,class M>
void ActorCritic<W,M>::train(int nEpisodes, float gamma, float beta, float zeta, float lr, int batchSize)
{
  int nSteps = 0;
  episodeNumber = 0;
  this->batchSize=batchSize, this->gamma=gamma,this->beta=beta,this->zeta=zeta;
  torch::optim::Adam optimizer(model->parameters(),lr);
  while (episodeNumber< nEpisodes)
    {
      runStates = torch::zeros({batchSize,this->currentState().getStateVector().size()});
      runActions = torch::zeros({batchSize}).to(model->getUsedDevice());
      runRewards = {}, runAreTerminal = {};
      runValues = torch::zeros({batchSize,1}).to(model->getUsedDevice());
      for (int i=0;i<batchSize;i++)
        {
	  nSteps++;
	  torch::Tensor stateVector = torch::tensor(this->currentState().getStateVector());
	  torch::Tensor actionProbabilities = model->actorOutput(stateVector.unsqueeze(0));
	  torch::Tensor action = actionProbabilities.multinomial(1).to(torch::kFloat32);
	  this->world.setTakenReward(this->world.transition({*action.to(torch::Device(torch::kCPU)).data<float>()}));
	  runStates[i] = stateVector;
	  runRewards.push_back(this->takenReward());
	  runActions[i] = action.squeeze().to(model->getUsedDevice());
	  runAreTerminal.push_back(this->world.isTerminal(this->currentState()));
	  if (runAreTerminal.back() || nSteps==pow(this->world.getSize(),2)/2)
            {
	      if (nSteps==pow(this->world.getSize(),2)/4)
		{
		  cout <<"Episode " + to_string(this->episodeNumber) + " was interrupted because it reached the maximum number of steps"<<endl;
		}
	      nSteps = 0;
	      this->world.reset();
	      episodeNumber++;
	      //Displaying a progression bar in the terminal
	      
	      if (nEpisodes > 100 && lossHistory.size()>0 &&  episodeNumber%(nEpisodes/100) == 0)
                {
		  cout << "Training in progress... " + to_string(episodeNumber/(nEpisodes/100)) + "%. Current Loss: " + to_string(lossHistory.back())
		    + "  Current entropy: " + to_string(entropyHistory.back()/beta)<< endl;
                }
            }
        }
      backPropagate(&optimizer);
    }
  saveTrainingData();
  this->world.saveRewardHistory();
}

template <class W, class M>
void ActorCritic<W,M>::playOne()
{
  int count =0;
  while(count<pow(this->world.getSize(),2) && !this->world.isTerminal(this->currentState()))
    {
      count++;
      torch::Tensor stateVector = torch::tensor(this->currentState().getStateVector());
      torch::Tensor actionProbabilities = model->actorOutput(stateVector.unsqueeze(0));
      torch::Tensor action = torch::argmax(actionProbabilities).to(torch::kFloat32);
      this->world.setTakenReward(this->world.transition({*action.to(torch::Device(torch::kCPU)).data<float>()}));
    }
}

template <class W,class M>
void ActorCritic<W,M>::saveTrainingData()
{
  ofstream ag("../temp/A2C_PolicyLoss");
  ofstream vl("../temp/A2C_ValueLoss");
  ofstream e("../temp/A2C_Entropy");
  ofstream tl("../temp/A2C_Loss");
  if(!ag)
    {
      cout<<"An error has occured while trying to save training data from ActorCritic"<<endl;
    }
  for (unsigned int i=0;i<policyLossHistory.size();i++)
    {
      ag<<to_string(policyLossHistory[i])<<endl;
      vl<<to_string(valueLossHistory[i]) <<endl;
      e<<to_string(entropyHistory[i])<<endl;
      tl<<to_string(lossHistory[i])<<endl;
    }
}

template <class W,class M>
M ActorCritic<W,M>::getModel() const
{
    return model;
}

template class ActorCritic<GridWorld,ConvNetGW>;
