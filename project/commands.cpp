#include "commands.h"

DEFINE_string(mdl,"../temp/model","Path to a model file. Do not add the .pt extension.");
DEFINE_string(tag,"","Suffix for auto generated files"); 
DEFINE_string(f,"../file","The path to a file");
DEFINE_string(dir,"../temp/","The path to a directory (must end with a /)");
DEFINE_string(map,"","Path to a map file");
DEFINE_string(mp,"","Path to a directory containing your maps");
DEFINE_int32(nmaps,1,"The number of maps in the map pool directory");
DEFINE_string(seed,"","Path to the .pt file containing the seed tensor"); 

//GridWorld flags

DEFINE_int32(size,8,"The generated maps will be of size sizexsize");
DEFINE_int32(maxObst,1,"The maps will be generated having a random number of obstacles between 1 and maxObst");

//Starship flags

DEFINE_int32(nplan,N_PLANETS,"Number of planets for mapss generation");
DEFINE_int32(pmin,PLANET_MIN_SIZE,"Planet minimum radius for mapss generation");
DEFINE_int32(pmax,PLANET_MAX_SIZE,"Planet maximum radius for mapss generation");
DEFINE_int32(nwp,N_WAYPOINTS,"Number of waypoints for mapss generation");
DEFINE_int32(rwp,WAYPOINT_RADIUS,"Waypoint radius for mapss generation");
DEFINE_double(trp,0.9,"Share of the training set from the whole dataset");
DEFINE_double(px,-1,"ship x coordinate");
DEFINE_double(py,-1,"ship y coordinate");
DEFINE_double(alpha,1,"Multiplicative coefficient for thrust coordinates");
DEFINE_bool(woda,false,"Discrete actions are made useless by removing dependancies between reward and signal value");

//Model flags

DEFINE_int32(sc1,16,"Number of feature maps of the first conv layer of the encoder. Next layers have twice as many features maps and the NN is shaped accordingly");

//Learning parameters flags

DEFINE_double(eps,0.1,"Probability of exploring for agents using epsilon greedy policies");
DEFINE_double(g,0.95,"Discount factor");
DEFINE_double(lr,0.001,"Learning Rate");
DEFINE_double(beta,1,"Coefficient applied to the entropy loss");
DEFINE_double(zeta,1,"Coefficient applied to the value loss");
DEFINE_int32(bs,32,"Batch Size");
DEFINE_double(lp1,0,"Loss penality parameter");
DEFINE_double(lp2,0,"Loss penality parameter");

//Planning flags

DEFINE_int32(K,1,"Number of rollouts");
DEFINE_int32(T,1,"Number of timesteps to unroll");
DEFINE_int32(gs,1,"Number of gradient steps");
DEFINE_int32(n,10000,"Number of training episodes");
DEFINE_int32(e,100,"Number of training epochs");
DEFINE_double(wp,0.1,"Percentage of forced win scenarios during the dataset generation"); 
DEFINE_bool(wn,false,"Adding white noise to the one-hot encoded action vectors");
DEFINE_double(sd,0.25,"Standard deviation");
DEFINE_int32(dist,0,"Chose your distribution");
DEFINE_int32(i,1,"Number of iterations");

Commands::Commands(){}

void Commands::generateMapGW()
{
  MapGW map(FLAGS_size);
  map.generate(FLAGS_maxObst);
  map.save(FLAGS_map);
}

void Commands::generateMapPoolGW()
{
  MapGW map(FLAGS_size); 
  map.generateMapPool(FLAGS_maxObst,FLAGS_mp,FLAGS_nmaps);
}

void Commands::showMapGW(int argc, char* argv[])
{
  QApplication a(argc,argv);
  EpisodePlayerGW ep(FLAGS_map);
  ep.showMap();
  a.exec();
}

void Commands::trainQLAgentGW()
{
  GridWorld gw(FLAGS_map);
  gw.generateVectorStates();
  QLearning<GridWorld> agent(gw);
  agent.train(FLAGS_n,FLAGS_eps,FLAGS_g);
  agent.saveQValues("../temp/QValuesGW");
  agent.saveRewardHistory();
}

void Commands::playQLAgentGW(int argc, char* argv[])
{
  QApplication a(argc,argv);
  GridWorld gw(FLAGS_map);
  gw.generateVectorStates();
  QLearning<GridWorld> agent(gw);
  agent.loadQValues(FLAGS_f);
  agent.playOne();
  EpisodePlayerGW ep(FLAGS_map);
  ep.playEpisode(agent.getWorld().getStateSequence());
  a.exec();
}

void Commands::evaluateQLPolicy(int argc, char* argv[])
{
  vector<vector<string>> toDisplay;
  string filename = FLAGS_map;
  MapGW map;
  map.load(filename);
  int size = map.getSize();
  for (int i=0;i<size;i++)
    {
      vector<string> line;
      for (int j=0;j<size;j++)
	{
	  GridWorld gw(filename,i,j);
	  gw.generateVectorStates();
	  string sDirection; 
	  if (gw.isTerminal(gw.getCurrentState().getStateVector()))
	    {
	      sDirection = "";
	    }
	  else
	    {	  
	      
	      QLearning<GridWorld> agent(gw);
	      agent.loadQValues(FLAGS_f);    	  
	      agent.epsilonGreedyPolicy(0);	  
	      int iDirection = agent.takenAction()[0];
	      switch (iDirection)
		{
		case 0:
		  sDirection = "UP";
		  break;
		case 1:
		  sDirection = "RIGHT";
		  break;
		case 2:
		  sDirection = "DOWN";
		  break;
		case 3:
		  sDirection = "LEFT";
		  break;
		}
	    }
	  line.push_back(sDirection);
	}
      toDisplay.push_back(line);
    }
  QApplication a(argc,argv);  
  EpisodePlayerGW ep(FLAGS_map);
  ep.displayOnGrid(toDisplay);
  a.exec();  
}

void Commands::trainA2CGW()
{
  GridWorld gw(FLAGS_mp,FLAGS_nmaps);
  int size = gw.getSize();
  ConvNetGW net(size,FLAGS_conv1,FLAGS_conv2,FLAGS_fc1);
  ActorCritic<GridWorld,ConvNetGW> agent(gw,net);
  agent.train(FLAGS_n,FLAGS_g,FLAGS_beta,FLAGS_zeta,FLAGS_lr,FLAGS_bs);
  torch::save(agent.getModel(),"../temp/CNN_A2C_GW.pt");
}



void Commands::testA2C()
{
  ConvNetGW net(8,32,32,128);
  torch::load(net,"../temp/CNN_A2C_GW.pt");
  float tot=0;
  for (int i=0;i<FLAGS_nmaps;i++)
    {
      MapGW map;
      map.load(FLAGS_mp+"map"+to_string(i));
      int size = map.getSize();
      vector<float> mapRewards;
      int emptySpaces=0;
      int accuracy = 0;
      for (int x=0;x<size;x++)
	{
	  for (int y=0;y<size;y++)
	    {
	      if (map.getMap()[x][y] == 0)
		{
		  emptySpaces++;
		  GridWorld gw(FLAGS_mp+"map"+to_string(i),x,y);
		  gw.generateVectorStates();
		  ActorCritic<GridWorld,ConvNetGW> agent(gw,net);
		  agent.playOne();
		  if (agent.takenReward()>0)
		    {
		      accuracy++;
		    }
		}
	    }
	}
      accuracy=accuracy*100./emptySpaces;
      tot+=accuracy;
      cout << "The model completed a " + to_string(accuracy) + "% accuracy on map" + to_string(i) << endl
	;
    }
  cout<<"The overall accuracy on the test set: " + to_string(tot/FLAGS_nmaps) +"%"<<endl;
}
	       
void Commands::showCriticOnMapGW(int argc, char* argv[])
{
  QApplication a(argc,argv);
  vector<vector<string>> toDisplay;
  string filename = FLAGS_map;
  MapGW map;
  map.load(filename);
  int size = map.getSize();
  ConvNetGW net(8,32,32,128);
  torch::load(net,"../temp/CNN_A2C_GW.pt");
  for (int i=0;i<size;i++)
    {
      vector<string> line;
      for (int j=0;j<size;j++)
	{
	  GridWorld gw(filename,i,j);
	  gw.generateVectorStates();
	  torch::Tensor output = net->criticOutput(torch::tensor(gw.getCurrentState().getStateVector())).to(net->getUsedDevice());
	  string val = to_string(*output.to(torch::Device(torch::kCPU)).data<float>());
	  string val2;
	  val2+=val[0],val2+=val[1],val2+=val[2],val2+=val[3],val2+=val[4];
	  line.push_back(val2);
	}
      toDisplay.push_back(line);
    }
  EpisodePlayerGW ep(FLAGS_f);
  ep.displayOnGrid(toDisplay);
  a.exec(); 
}

void Commands::showActorOnMapGW(int argc, char* argv[])
{
  QApplication a(argc,argv);
  vector<vector<string>> toDisplay;
  string filename = FLAGS_map;
  MapGW map;
  map.load(filename);
  int size = map.getSize();
  ConvNetGW net(8,32,32,128);
  torch::load(net,"../temp/CNN_A2C_GW.pt");
  for (int i=0;i<size;i++)
    {
      vector<string> line;
      for (int j=0;j<size;j++)
	{
	  GridWorld gw(filename,i,j);
	  gw.generateVectorStates();
	  torch::Tensor output = net->actorOutput(torch::tensor(gw.getCurrentState().getStateVector()).unsqueeze(0));
	  float max =0;
	  int iDirection;
	  string sDirection;
	  for (int k=0;k<4;k++)
	    {
	      float val = *output[0][k].to(torch::Device(torch::kCPU)).data<float>();
	      if (val>max)
                {
		  max = val;
		  iDirection = k;
                }
            }
	  switch (iDirection)
            {
            case 0:
	      sDirection = "UP";
	      break;
            case 1:
	      sDirection = "RIGHT";
	      break;
            case 2:
	      sDirection = "DOWN";
	      break;
            case 3:
	      sDirection = "LEFT";
	      break;
            }
	  line.push_back(sDirection);
	}
      toDisplay.push_back(line);
    }
  EpisodePlayerGW ep(FLAGS_map);
  ep.displayOnGrid(toDisplay);
  a.exec();
}

void Commands::generateDataSetGW()
{
  ToolsGW t;
  t.generateDataSet(FLAGS_mp,FLAGS_nmaps,FLAGS_n,FLAGS_T, FLAGS_trp, FLAGS_wp);
}


void Commands::learnForwardModelGW()
{
  GridWorld gw;
  string path = FLAGS_mp;
  torch::Tensor stateInputsTr, actionInputsTr, stateLabelsTr, rewardLabelsTr;
  torch::Tensor stateInputsTe, actionInputsTe, stateLabelsTe, rewardLabelsTe;
  torch::load(stateInputsTr,path+"stateInputsTrain.pt");
  torch::load(actionInputsTr, path+"actionInputsTrain.pt");
  torch::load(stateLabelsTr,path+"stateLabelsTrain.pt");
  torch::load(rewardLabelsTr, path+"rewardLabelsTrain.pt");
  torch::load(stateInputsTe,path+"stateInputsTest.pt");
  torch::load(actionInputsTe, path+"actionInputsTest.pt");
  torch::load(stateLabelsTe,path+"stateLabelsTest.pt");
  torch::load(rewardLabelsTe, path+"rewardLabelsTest.pt");
  int nTr = stateInputsTr.size(0), nTe = stateInputsTe.size(0), T = stateLabelsTe.size(1), s = stateInputsTe.size(3);  
  if (FLAGS_wn)
    {
      actionInputsTr+=torch::zeros({actionInputsTr.size(0),T,4}).normal_(0,FLAGS_sd);      
    }
  //  actionInputsTr = torch::softmax(actionInputsTr,2);
  ForwardGW forwardModel(stateInputsTr.size(3),FLAGS_sc1);
  forwardModel->to(forwardModel->usedDevice);
  ModelBased<GridWorld,ForwardGW, PlannerGW> agent(gw,forwardModel);
  torch::optim::Adam optimizer(forwardModel->parameters(),FLAGS_lr); 
  int e=0;
  while(e!=FLAGS_e)
    {
      e++;
      agent.learnForwardModel(&optimizer, actionInputsTr, stateInputsTr,stateLabelsTr, rewardLabelsTr,1,FLAGS_bs, FLAGS_beta);
      agent.saveTrainingData();
      torch::save(agent.getForwardModel(),FLAGS_mdl+".pt");
      torch::save(optimizer,FLAGS_mdl+"_opti.pt");
      agent.getForwardModel()->saveParams(FLAGS_mdl+"_Params");

      //Computing accuracy

      {	
	torch::NoGradGuard no_grad;
	ToolsGW t;
	auto model = agent.getForwardModel();
	int splitSize = 1000;
	vector<torch::Tensor> siteSplit = torch::split(stateInputsTe,splitSize,0);
	vector<torch::Tensor> aiteSplit = torch::split(actionInputsTe,splitSize,0);
	vector<torch::Tensor> slteSplit = torch::split(stateLabelsTe,splitSize,0);
	vector<torch::Tensor> rlteSplit = torch::split(rewardLabelsTe,splitSize,0);	
        unsigned int nSplit = siteSplit.size();	
	for (unsigned int i=0;i<nSplit;i++)
	  {
	    int nSpl = siteSplit[i].size(0); 
	    model->forward(siteSplit[i],aiteSplit[i]);
	    t.transitionAccuracy(model->predictedStates.slice(-3,0,1,1),slteSplit[i].to(model->usedDevice),nSplit);
	    t.rewardAccuracy(model->predictedRewards,rlteSplit[i].to(model->usedDevice), nSplit);
	  }	
	t.displayTAccuracy(nTe*T);
	t.displayRAccuracy();
      }
    }
}

void Commands::playModelBasedGW(int argc, char* argv[])
{
  ForwardGW fm(FLAGS_mdl+"_Params");
  torch::load(fm,FLAGS_mdl+".pt");
  GridWorld gw(FLAGS_map);
  ModelBased<GridWorld,ForwardGW,PlannerGW> agent(gw,fm,PlannerGW());
  //  agent.gradientBasedPlanner(FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr);
  //  agent.playOne(FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr);
  QApplication a(argc,argv);
  EpisodePlayerGW ep(FLAGS_map);
  ep.playEpisode(agent.getWorld().getStateSequence());
  a.exec();
}


void Commands::tc1()
{
  /*
  ForwardGW fm("../temp/ForwardGW_Params");
  torch::load(fm,"../temp/ForwardGW.pt");
  GridWorld gw("../GridWorld/Maps/Easy8x8/test/map0",7,6);
  gw.generateVectorStates();
  ModelBased<GridWorld,ForwardGW,PlannerGW> agent(gw,fm,PlannerGW());
  agent.gradientBasedPlanner(FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr);
  */

  ForwardGW fm(FLAGS_mdl+"_Params");
  torch::load(fm,FLAGS_mdl+".pt");
  GridWorld gw("../GridWorld/Maps/Hard8x8/train/map0",4,2);
  ModelBased<GridWorld,ForwardGW,PlannerGW> agent(gw,fm,PlannerGW());
  ofstream f("../temp/frgw");
    for (int i=0;i<1000;i++)
      {
	torch::Tensor a = torch::tensor({1-(i/1000.),i/1000.,0.,0.}).to(torch::kFloat32);
	torch::Tensor s = torch::tensor(gw.getCurrentState().getStateVector());
	fm->forward(s.unsqueeze(0),a.unsqueeze(0).to(fm->usedDevice));
	f<<*fm->predictedRewards.to(torch::Device(torch::kCPU)).data<float>()<<endl;    
      }
}

void Commands::generateMapSS()
{
  MapSS map;
  map.generate(FLAGS_nplan,FLAGS_pmin,FLAGS_pmax,FLAGS_nwp,FLAGS_rwp);
  map.save(FLAGS_map);
}

void Commands::generateMapPoolSS()
{
  MapSS map; 
  map.generateMapPool(FLAGS_nplan,FLAGS_pmin,FLAGS_pmax,FLAGS_nwp,FLAGS_rwp,FLAGS_mp,FLAGS_nmaps);
}


void Commands::showMapSS(int argc, char* argv[])
{
  QApplication a(argc,argv);
  EpisodePlayerSS ep(FLAGS_map);
  ep.showMap();
  a.exec();
}

void Commands::playRandomSS(int argc, char* argv[])
{
  MapSS map;
  map.generate(FLAGS_nplan,FLAGS_pmin,FLAGS_pmax,FLAGS_nwp,FLAGS_rwp);
  map.save("../Starship/Maps/test");
  SpaceWorld sw("../Starship/Maps/test");
  for (int i=0;i<FLAGS_n;i++)
    {
      vector<float> a = sw.randomAction();
      sw.transition(a);
    }
  QApplication a(argc,argv);
  EpisodePlayerSS ep("../Starship/Maps/test");
  ep.playEpisode(sw.getActionSequence(), sw.getStateSequence(), SHIP_MAX_THRUST);  
  a.exec();
}

void Commands::generateDataSetSS()
{
  ToolsSS t;
  t.generateDataSet(FLAGS_mp,FLAGS_nmaps,FLAGS_n,FLAGS_T,FLAGS_trp, FLAGS_wp, FLAGS_dist, FLAGS_alpha, FLAGS_sd, FLAGS_woda);
}

void Commands::trainForwardModelSS()
{
  SpaceWorld sw;
  string path = FLAGS_mp;
  torch::Tensor stateInputsTr, actionInputsTr, stateLabelsTr, rewardLabelsTr;
  torch::Tensor stateInputsTe, actionInputsTe, stateLabelsTe, rewardLabelsTe;
  torch::load(stateInputsTr,path+"stateInputsTrain.pt");
  torch::load(actionInputsTr, path+"actionInputsTrain.pt");
  torch::load(stateLabelsTr,path+"stateLabelsTrain.pt");
  torch::load(rewardLabelsTr, path+"rewardLabelsTrain.pt");
  torch::load(stateInputsTe,path+"stateInputsTest.pt");
  torch::load(actionInputsTe, path+"actionInputsTest.pt");
  torch::load(stateLabelsTe,path+"stateLabelsTest.pt");
  torch::load(rewardLabelsTe, path+"rewardLabelsTest.pt");
  int nTr = stateInputsTr.size(0), nTe = stateInputsTe.size(0), T = actionInputsTe.size(1), s = stateInputsTe.size(1);  
  if (FLAGS_wn)
    {
      actionInputsTr+=torch::cat({torch::zeros({nTr,T,4}).normal_(0,FLAGS_sd),torch::zeros({nTr,T,2})},2);
    }
  ForwardSS forwardModel(s,512,2, FLAGS_lp1, FLAGS_lp2);
  forwardModel->to(forwardModel->usedDevice); 
  torch::optim::Adam optimizer(forwardModel->parameters(),FLAGS_lr); 
  ModelBased<SpaceWorld,ForwardSS, PlannerGW> agent(sw,forwardModel);
  int e=0;
  ofstream ftep("../temp/tep_mse"+FLAGS_tag);
  ofstream ftev("../temp/tev_mse"+FLAGS_tag);  
  ofstream fter("../temp/ter_mse"+FLAGS_tag);  
  
  while(e!=FLAGS_e)
    {
      e++;
      agent.learnForwardModel(&optimizer, actionInputsTr, stateInputsTr,stateLabelsTr, rewardLabelsTr,1,FLAGS_bs, FLAGS_beta);
      if (e%40 == 0)
	{
	  torch::save(agent.getForwardModel(),"../temp/cps/"+FLAGS_tag+"cp"+to_string(e)+".pt");
	  agent.getForwardModel()->saveParams("../temp/cps/"+FLAGS_tag+"cp"+to_string(e)+"_Params");
	  torch::save(optimizer,"../temp/cps/"+FLAGS_tag+"cp"+to_string(e)+"_opti.pt");
	  cout<<"Checkpointing..."<<endl;
	}
      agent.saveTrainingData();
      torch::save(agent.getForwardModel(),FLAGS_mdl+".pt");
      torch::save(optimizer,FLAGS_mdl+"_opti.pt");
      agent.getForwardModel()->saveParams(FLAGS_mdl+"_Params");

      //Computing accuracy

      {	
	torch::NoGradGuard no_grad;
	ToolsSS t;
	auto model = agent.getForwardModel();
	int splitSize = 1000;
	vector<torch::Tensor> siteSplit = torch::split(stateInputsTe,splitSize,0);
	vector<torch::Tensor> aiteSplit = torch::split(actionInputsTe,splitSize,0);
	vector<torch::Tensor> slteSplit = torch::split(stateLabelsTe,splitSize,0);
	vector<torch::Tensor> rlteSplit = torch::split(rewardLabelsTe,splitSize,0);	
        unsigned int nSplit = siteSplit.size();
	
	for (unsigned int i=0;i<nSplit;i++)
	  {
	    int nSpl = siteSplit[i].size(0); 
	    model->forward(siteSplit[i],aiteSplit[i]);
	    t.transitionAccuracy(model->predictedStates.slice(-1,0,4,1),slteSplit[i].to(model->usedDevice),nSplit,true);
	    t.rewardAccuracy(model->predictedRewards,rlteSplit[i].to(model->usedDevice), nSplit,true);
	  }
	ftep<<pow(*t.pMSE.data<float>(),0.5)<<endl;
	ftev<<pow(*t.vMSE.data<float>(),0.5)<<endl;
	fter<<pow(*t.rMSE.data<float>(),0.5)<<endl;
	t.displayTAccuracy(nTe*T);
	t.displayRAccuracy();
      }
    }  
}

void Commands::testForwardModelSS()
{
  string path = FLAGS_mp;
  torch::Tensor stateInputsTe, actionInputsTe, stateLabelsTe, rewardLabelsTe;
  torch::load(stateInputsTe,path+"stateInputsTest.pt");
  torch::load(actionInputsTe, path+"actionInputsTest.pt");
  torch::load(stateLabelsTe,path+"stateLabelsTest.pt");
  int T=actionInputsTe.size(1), n=actionInputsTe.size(0);
  ForwardSS forwardModel(FLAGS_mdl+"_Params");
  torch::load(forwardModel,FLAGS_mdl+".pt");
  forwardModel->forward(stateInputsTe,actionInputsTe);
  ofstream f(FLAGS_f);
  for (int i=0;i<stateLabelsTe.size(0);i++)
    {
      f<<*ToolsSS().moduloMSE(stateLabelsTe[i].slice(-1,0,2,1).to(forwardModel->usedDevice),forwardModel->predictedStates[i].slice(-1,0,2,1),false).pow(0.5).to(torch::Device(torch::kCPU)).data<float>()<<endl;
    }
  cout<<"AVERAGE POSITION ERROR ON THIS TEST SET: "+to_string(*ToolsSS().moduloMSE(stateLabelsTe.slice(-1,0,2,1).to(forwardModel->usedDevice),forwardModel->predictedStates.slice(-1,0,2,1),false).pow(0.5).to(torch::Device(torch::kCPU)).data<float>())<<endl;
}

void Commands::actionOverfitSS()
{
  ForwardSS forwardModel(FLAGS_mdl+"_Params");
  torch::load(forwardModel,FLAGS_mdl+".pt");
  ofstream f(FLAGS_f);
  for (int i=0;i<FLAGS_i;i++)
    {
      ToolsSS().generateDataSet(FLAGS_mp,FLAGS_nmaps,FLAGS_n,FLAGS_T,FLAGS_trp, FLAGS_wp,10, -SHIP_MAX_THRUST+i*SHIP_MAX_THRUST*2./FLAGS_i, FLAGS_sd, FLAGS_woda);      
      string path=FLAGS_mp;
      torch::Tensor stateInputsTe, actionInputsTe, stateLabelsTe, rewardLabelsTe;
      torch::load(stateInputsTe,path+"stateInputsTest.pt");
      torch::load(actionInputsTe, path+"actionInputsTest.pt");
      torch::load(stateLabelsTe,path+"stateLabelsTest.pt");
      torch::load(rewardLabelsTe, path+"rewardLabelsTest.pt");
      int T=actionInputsTe.size(1), n=actionInputsTe.size(0);
      forwardModel->forward(stateInputsTe,actionInputsTe);      
      f<<*(ToolsSS().moduloMSE(stateLabelsTe.slice(2,0,2,1).to(forwardModel->usedDevice),forwardModel->predictedStates.slice(2,0,2,1),false).pow(0.5)).to(torch::Device(torch::kCPU)).data<float>()<<endl;
    }
  cout<<"Your file was successfully generated"<<endl;
}

void Commands::generateSeedSS()
{
  ToolsSS().generateSeed(FLAGS_T,FLAGS_K,FLAGS_seed);
}

void Commands::playPlannerSS(int argc, char* argv[])
{
  ForwardSS fm(FLAGS_mdl+"_Params");
  torch::load(fm,FLAGS_mdl+".pt");
  SpaceWorld sw(FLAGS_map);
  sw.woda = FLAGS_woda;
  
  if (FLAGS_px != -1 && FLAGS_py != -1)
    {
      sw.repositionShip(Vect2d(FLAGS_px,FLAGS_py));
      sw.generateVectorStates();
    }
  ModelBased<SpaceWorld,ForwardSS,PlannerGW> agent(sw,fm);
  torch::Tensor actions = torch::zeros(0);
  if (FLAGS_seed != "")
    {
      torch::load(actions,FLAGS_seed+".pt");
      agent.playOne(sw.getActions(),FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr,actions);
    }
  else
    {
      agent.playOne(sw.getActions(),FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr,actions);
    }

  QApplication a(argc,argv);
  EpisodePlayerSS ep(FLAGS_map);
  ep.playEpisode(agent.getWorld().getActionSequence(),agent.getWorld().getStateSequence(), SHIP_MAX_THRUST);
  a.exec();
}

void Commands::testPlannerSS()
{
  ForwardSS fm(FLAGS_mdl+"_Params");
  torch::load(fm,FLAGS_mdl+".pt");
  SpaceWorld sw(FLAGS_mp, FLAGS_nmaps);
  sw.woda = FLAGS_woda;
  ModelBased<SpaceWorld,ForwardSS,PlannerGW> agent(sw,fm);  
  ofstream f(FLAGS_f+"_reward");
  ofstream g(FLAGS_f+"_error");
  auto start = std::chrono::system_clock::now();
  for (int i=0;i<FLAGS_n;i++)
    {
      agent.playOne(sw.getActions(),FLAGS_K,FLAGS_T,FLAGS_gs,FLAGS_lr);
      f<<agent.rewardHistory().back()<<endl;
      g<<*(ToolsSS().moduloMSE(agent.sPred.slice(1,0,2,1).slice(0,1,FLAGS_T+1,1),agent.sTruth.slice(1,0,2,1).slice(0,1,FLAGS_T+1,1),false).pow(0.5)).data<float>()<<endl;
      agent.resetWorld();
    }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  cout<<"Average execution time: "+to_string(elapsed_seconds.count()/FLAGS_n)<<endl;
}

