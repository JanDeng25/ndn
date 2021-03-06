/*
 * simulation.cc
 *
 *  Created on: jan 7, 2016
 *      Author: ch
 *
 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"

#include "ns3/vanetmobility-helper.h"
#include "ns3/ndnSIM-module.h"
#include "nrProducer.h"
#include "NodeSensorHelper.h"
#include "nrUtils.h"
//#include "ndn-nr-pit-impl.h"

#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

NS_LOG_COMPONENT_DEFINE("nrndn.Example");

using namespace ns3;
using namespace std;
using namespace ns3::ndn::nrndn;
//using namespace ns3::ndn;
using namespace ns3::vanetmobility;
class nrndnExample
{
public:
  nrndnExample ();
  ~nrndnExample();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);

  void Run();
  /// Run simulation of nrndn
  void RunNrndnSim ();
  /// Run simulation of distance based forwarding
  void RunTraNdnSim ();
  /// Run simulation of CDS based forwarding
  void RunDjNdnSim ();
  /// Report results
  void Report ();

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
  /// Simulation time, seconds
  double totalTime;
  /// total time from sumo
  double readTotalTime;
  /// nodes stop moving time, default is totalTime
  double alltoallTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  /// look at clock interval
  double clockInterval;
  /// dominator percentage interval
  double dpInterval;
  /// Wifi Phy mode
  string phyMode;
  /// Tell echo applications to log if true
  bool verbose ;
  /// indicate whether use flooding to forward messages
  bool flood;
  /// transmission range
  double transRange;
  ///the switch which can turn on the log on Functions about hello messages
  bool HelloLogEnable;
  ///the number of accident. It will randomly put into the whole simulation
  uint32_t accidentNum;

  uint32_t method;

  string inputDir;
  /// Output data path
  string outputDir;
  /// Programme name
  string name;

  std::ofstream os;

  // Interest Packet Sending Frequency
  double interestFrequency;

  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

  ///\name traffic information
  //\{
//  RoadMap roadmap;
//  VehicleLoader vl;
  Ptr<VANETmobility> mobility;
  //\}

  //hitRate: among all the interested nodes, how many are received
  double hitRate;
  uint32_t ResourceForwardTimes;
  double averageInterestForwardTimes;
  double averageDataForwardTimes;
  double averageDelay;
  uint32_t tableSum;
  double averageDetectRate;
  double averageConfirmRate;
  double averageForwardSum;
  uint32_t InterestedNodeReceivedSum;

  bool noFwStop;

  uint32_t TTLMax;
  uint32_t virtualPayloadSize;
private:
  // Initialize
  /// Load traffic data
  void LoadTraffic();
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallSensor();
  void InstallNrNdnStack();
  void InstallTraNdnStack();
  void InstallDjNdnStack();
  void InstallMobility();
  void InstallNrndnApplications ();
  void InstallTraNdnApplications();
  void InstallDjNdnApplications();
  void InstallTraffics();

  // Utility funcitons
  void Look_at_clock();
  void ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack);
  void SetPos(Ptr<MobilityModel> mob);
  void getStatistic();
};

int main (int argc, char **argv)
{
	nrndnExample test;
	if (!test.Configure(argc, argv))
		NS_FATAL_ERROR("Configuration failed. Aborted.");

	test.Run();
	test.Report();

	return 0;
}



//-----------------------------------------------------------------------------
//构造函数
nrndnExample::nrndnExample () :
  size (1000),
  totalTime (3600),
  readTotalTime(0),
  alltoallTime(-1),
  pcap (false),
  printRoutes (true),
  clockInterval(1),
  dpInterval(1),
  phyMode("DsssRate1Mbps"),
  //phyMode("OfdmRate24Mbps"),
  verbose (false),
  flood(false),
  transRange(436),//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  HelloLogEnable(true),
  accidentNum(30),//默认3
  method(0),
  interestFrequency(0.5),//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
  hitRate(0),
  ResourceForwardTimes(0),
  averageInterestForwardTimes(0),
  averageDataForwardTimes(0),
  averageDelay(0),
  tableSum(0),
  averageDetectRate(0),
  averageConfirmRate(0),
  averageForwardSum(0),
  InterestedNodeReceivedSum(0),
  noFwStop(false),
  TTLMax(3),
  virtualPayloadSize(1024)
{
	//os =  std::cout;
	string home         = getenv("HOME");
	inputDir  = home +"/input";
	outputDir = home +"/input";
}

nrndnExample::~nrndnExample()
{
	os.close();
}
bool
nrndnExample::Configure (int argc, char **argv)
{
  // Enable logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  //name = strrchr(argv[0], '/');
  time_t now = time(NULL);
  cout<<"NR-NDN simulation begin at "<<ctime(&now);

  SeedManager::SetSeed (12345);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("clockInterval","look at clock interval.",clockInterval);
  cmd.AddValue ("dpInterval","dominator percentage interval ",dpInterval);
  cmd.AddValue ("phyMode","Wifi Phy mode",phyMode);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("inputDir","The Simulation data path",inputDir);
  cmd.AddValue ("outputDir","The Simulation output path",outputDir);
  cmd.AddValue ("flood","indicate whether to use flooding to forward messages",flood);
  cmd.AddValue ("alltoallTime","Nodes begin to broadcast emergency messages, it is an all-to-all broadcast",alltoallTime);
  cmd.AddValue ("transRange","transmission range",transRange);
  cmd.AddValue("HelloLogEnable","the switch which can turn on the log on Functions about hello messages",HelloLogEnable);
  cmd.AddValue("accidentNum","the number of accident. It will randomly put into the whole simulation(default 3)",accidentNum);
  cmd.AddValue("method","Forward method,0=Nrndn, 1=DJ, 2=Trad",method);
  cmd.AddValue("noFwStop","When the PIT covers the nodes behind, no broadcast stop message",noFwStop);
  cmd.AddValue("TTLMax","This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded",TTLMax);
  cmd.AddValue("interestFreq","Interest Packet Sending Frequency(Hz)",interestFrequency);
  cmd.AddValue("virtualPayloadSize","Virtual payload size for traffic Content packets",virtualPayloadSize);

  cmd.Parse (argc, argv);
  return true;
}

void nrndnExample::Run()
{
	std::cout<<"运行方法是："<<method<<std::endl;
	switch(method)
	{
	case 0:
		RunNrndnSim();
		break;
	case 1:
		RunDjNdnSim();
		break;
	case 2:
		RunTraNdnSim();
		break;
	default:
		cout<<"Undefine method"<<endl;
		break;
	}
}

void
nrndnExample::RunNrndnSim ()
{
	name = "NR-NDN-Simulation";

	std::cout<<"读取交通数据"<<std::endl;
	LoadTraffic();
	std::cout<<"创造节点"<<std::endl;
	CreateNodes();
	std::cout<<"创造设备"<<std::endl;
	CreateDevices();
	//InstallInternetStack();
	std::cout<<"初始化Mobility"<<std::endl;
	InstallMobility();
	std::cout<<"安装传感器"<<std::endl;
	InstallSensor();
	std::cout<<"初始化NrNdnStack"<<std::endl;
	InstallNrNdnStack();

	//InstallTestMobility();
	std::cout<<"安装Nrndn应用程序"<<std::endl;
	InstallNrndnApplications();

	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	std::cout << "开始运行：\n";
	Simulator::Run();

	Simulator::Destroy();
}

void nrndnExample::RunTraNdnSim()
{
	name = "TRA-NDN-Simulation";
	std::cout<<"读取交通数据"<<std::endl;
	LoadTraffic();
	std::cout<<"创造节点"<<std::endl;
	CreateNodes();
	std::cout<<"创造设备"<<std::endl;
	CreateDevices();
	std::cout<<"初始化Mobility"<<std::endl;
	InstallMobility();
	std::cout<<"安装传感器"<<std::endl;
	InstallSensor();
	std::cout<<"初始化DistNdnStack"<<std::endl;
	InstallTraNdnStack();
	std::cout<<"安装Dist应用程序"<<std::endl;
	InstallTraNdnApplications();

	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();
	Simulator::Destroy();
}

void nrndnExample::RunDjNdnSim()
{
	name = "DJ-Simulation";
	LoadTraffic();
	CreateNodes();
	CreateDevices();
	InstallMobility();
	InstallSensor();
	InstallDjNdnStack();
	InstallDjNdnApplications();

	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();
	Simulator::Destroy();
}

void
nrndnExample::Report ()
{
	NS_LOG_UNCOND ("Report data outputs here");
	//1. get statistic first
	getStatistic();

	//2. output the result
	os//<<arrivalRate <<'\t'
			//<<accuracyRate<<'\t'
			<<"hit rate:"<<hitRate<<'\t'
			<<" average delay:"<<averageDelay<<'\t'
			<<" Resourceforward times:"<< ResourceForwardTimes<<'\t'
			<<" average interest forwards times:"<<averageInterestForwardTimes<<'\t'
			<<" average data froward times:"<<averageDataForwardTimes<<'\t'
			<<" table Sum:"<<tableSum<<'\t'
			<<" average Detect Rate:"<<averageDetectRate<<'\t'
			<<" average Confirm Rate:"<<averageConfirmRate<<'\t'
			<<" average Forward Sum:"<<averageForwardSum<<'\t'
			<<" InterestedNodeReceivedSum"<<InterestedNodeReceivedSum<<'\t'
			<<endl;

}

void
nrndnExample::LoadTraffic()
{
	cout<<"Method: "<<name<<endl;
	DIR* dir=NULL;
	DIR* subdir=NULL;
	//打开数据源
	if((dir = opendir(inputDir.data()))==NULL)
		NS_FATAL_ERROR("Cannot open input path "<<inputDir.data()<<", Aborted.");
	outputDir += '/' + name;
	if((subdir = opendir(outputDir.data()))==NULL)
	{
		cout<<outputDir.data()<<" not exist, create a new one"<<endl;
		if(mkdir(outputDir.data(),S_IRWXU)==-1)
			NS_FATAL_ERROR("Cannot create dir "<<outputDir.data()<<", Aborted.");
	}
	string netxmlpath   = inputDir + "/input_net.net.xml";
	string routexmlpath = inputDir + "/routes.rou.xml";
	string fcdxmlpath   = inputDir + "/fcdoutput.xml";

	string outfile      = outputDir + "/result.txt";

	std::cout<<"输出文件路径："<<outfile<<std::endl;
	os.open(outfile.data(),ios::out);
	std::cout<<"文件打开成功。"<<std::endl;

	VANETmobilityHelper mobilityHelper;
	mobility=mobilityHelper.GetSumoMObility(netxmlpath,routexmlpath,fcdxmlpath);
	std::cout<<"读取完毕！"<<std::endl;
//获取结点size
	size = mobility->GetNodeSize();
	std::cout<<"节点size："<<size<<std::endl;
}

void
nrndnExample::CreateNodes ()
{
	std::cout << "Creating " << (unsigned) size << " vehicle nodes.\n";
	nodes.Create(size);
	// Name nodes
	for (uint32_t i = 0; i < size; ++i)
	{
		std::ostringstream os;
		os << "vehicle-" << i;
		Names::Add(os.str(), nodes.Get(i));
	}

	std::cout << "创建完毕\n";
}

void
nrndnExample::CreateDevices ()
{//设置网卡的，不用修改
	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	//request to send / clear to send 请求发送/清除发送协议
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue("2200"));
	// Fix non-unicast data rate to be the same as that of unicast//单播
	Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue(phyMode));

	WifiHelper wifi = WifiHelper::Default ();
	if (verbose)
	{
		wifi.EnableLogComponents();  // Turn on all Wifi logging
	}
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
				StringValue(phyMode), "ControlMode", StringValue(phyMode));



	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	// ns-3 supports RadioTap and Prism tracing extensions for 802.11b

	//wifiPhy.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
	//				DoubleValue(transRange));


	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	//YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

	wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
						DoubleValue(transRange));
	wifiPhy.SetChannel(wifiChannel.Create());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

	// Set it to adhoc mode
	//wifiMac.SetType("ns3::AdhocWifiMac");

	devices = wifi.Install(wifiPhy, wifiMac, nodes);

    if (pcap)
	{
		wifiPhy.EnablePcapAll(std::string("aodv"));
	}
}

void
nrndnExample::InstallNrNdnStack()
{
	NS_LOG_INFO ("Installing NDN stack");
	ndn::StackHelper ndnHelper;
	// ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (MyNetDeviceFaceCallback));
	//ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
	string str("false");
	string noFwStopStr("false");
	if(HelloLogEnable)
		str="true";
	if(noFwStop)
		noFwStopStr="true";
	std::ostringstream TTLMaxStr;
	TTLMaxStr<<TTLMax;
	std::ostringstream pitCleanIntervalStr;
	uint32_t pitCleanInterval = 1.0 / interestFrequency * 3.0;
	pitCleanIntervalStr<<pitCleanInterval;
	cout<<"pitInterval="<<pitCleanIntervalStr.str()<<endl;
	ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::nrndn::NavigationRouteHeuristic","HelloLogEnable",str,"NoFwStop",noFwStopStr);
	ndnHelper.SetPit("ns3::ndn::pit::nrndn::NrPitImpl");
	ndnHelper.SetFib("ns3::ndn::fib::nrndn::NrFibImpl");
	ndnHelper.SetContentStore("ns3::ndn::cs::nrndn::NrCsImpl");
	ndnHelper.SetDefaultRoutes (true);
	ndnHelper.Install (nodes);
}

void nrndnExample::InstallTraNdnStack()//////////////////////////////////////gai!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
  NS_LOG_INFO("Installing Tra NDN stack");
  ndn::StackHelper ndnHelper;
  	// ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (MyNetDeviceFaceCallback));
  	//ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  	string str("false");
  	string noFwStopStr("false");
  	if(HelloLogEnable)
  		str="true";
  	if(noFwStop)
  		noFwStopStr="true";
  	std::ostringstream TTLMaxStr;
  	TTLMaxStr<<TTLMax;
  	std::ostringstream pitCleanIntervalStr;
  	uint32_t pitCleanInterval = 1.0 / interestFrequency * 5.0;
  	pitCleanIntervalStr<<pitCleanInterval;
  	cout<<"pitInterval="<<pitCleanIntervalStr.str()<<endl;
  	ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::nrndn::TrNavigationRouteHeuristic","HelloLogEnable",str,"NoFwStop",noFwStopStr);
  	ndnHelper.SetPit("ns3::ndn::pit::nrndn::TrNrPitImpl");
  	ndnHelper.SetFib("ns3::ndn::fib::nrndn::TrNrFibImpl");
  	ndnHelper.SetContentStore("ns3::ndn::cs::nrndn::NrCsImpl");
  	ndnHelper.SetDefaultRoutes (true);
  	ndnHelper.Install (nodes);
}

void nrndnExample::InstallDjNdnStack()//////////////////////////////////////gai!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{
	ndn::StackHelper ndnHelper;
		// ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (MyNetDeviceFaceCallback));
		//ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
		string str("false");
		string noFwStopStr("false");
		if(HelloLogEnable)
			str="true";
		if(noFwStop)
			noFwStopStr="true";
		std::ostringstream TTLMaxStr;
		TTLMaxStr<<TTLMax;
		std::ostringstream pitCleanIntervalStr;
		uint32_t pitCleanInterval = 1.0 / interestFrequency * 3.0;
		pitCleanIntervalStr<<pitCleanInterval;
		cout<<"pitInterval="<<pitCleanIntervalStr.str()<<endl;
		ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::nrndn::djNavigationRouteHeuristic","HelloLogEnable",str,"NoFwStop",noFwStopStr);
		ndnHelper.SetPit("ns3::ndn::pit::nrndn::NrPitImpl");
		ndnHelper.SetFib("ns3::ndn::fib::nrndn::NrFibImpl");
		ndnHelper.SetContentStore("ns3::ndn::cs::nrndn::NrCsImpl");
		ndnHelper.SetDefaultRoutes (true);
		ndnHelper.Install (nodes);
}

void
nrndnExample::InstallInternetStack ()
{
//  AodvHelper aodv;
  InternetStackHelper stack;
 // stack.SetRoutingHelper(aodv);
  stack.Install (nodes);
  Ipv4AddressHelper address;
  NS_LOG_INFO ("Assign IP Addresses.");
  address.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces = address.Assign (devices);
}

void
nrndnExample::InstallMobility()
{
	//double maxTime = 0;
	std::cout<<"正在安装mobility..."<<std::endl;
	mobility->Install();
	std::cout<<"正在读取总时间...："<<std::endl;
	readTotalTime = mobility->GetReadTotalTime();
	totalTime = readTotalTime < totalTime ? readTotalTime : totalTime;
	std::cout<<"总时间："<<totalTime<<std::endl;
}

void
nrndnExample::InstallNrndnApplications ()
{
	NS_LOG_INFO ("Installing nrndn Applications");
	ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::nrConsumer");
	//Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
	//consumerHelper.SetAttribute ("Frequency", DoubleValue (interestFrequency));
	consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));

	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
		if((*i)->GetId() > 9)
			consumerHelper.Install(*i);
	/////nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"]=0;

	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::nrProducer");
	//producerHelper.SetPrefix ("/");
	producerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
	for(uint32_t i=0; i<=9; ++i)
		producerHelper.Install(nodes.Get (i));
	/////nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]=1;

	//Setup start and end time;
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
	{
		double start=mobility->GetStartTime((*i)->GetId());
		double stop =mobility->GetStopTime ((*i)->GetId());
		(*i)->GetApplication(0)->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(0)->SetAttribute("StopTime", TimeValue (Seconds (stop )));
	}
}

void nrndnExample::InstallTraNdnApplications()////////////////////////////////gai!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
{

	NS_LOG_INFO ("Installing TRA NDN Applications");
	ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::trConsumer");
	consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
			if((*i)->GetId() > 9)
				consumerHelper.Install(*i);

	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::trProducer");
	producerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
		for(uint32_t i=0; i<=9; ++i)
			producerHelper.Install(nodes.Get (i));

	//Setup start and end time;
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
	{
		double start=mobility->GetStartTime((*i)->GetId());
		double stop =mobility->GetStopTime ((*i)->GetId());
		(*i)->GetApplication(0)->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(0)->SetAttribute("StopTime", TimeValue (Seconds (stop )));
	}
}

void nrndnExample::InstallDjNdnApplications()/////////////////////////////////////////////gai/11111111111111111111111111111111111
{
	NS_LOG_INFO ("Installing DJ NDN Applications");
		ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::djConsumer");
		consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
		for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
				if((*i)->GetId() > 9)
					consumerHelper.Install(*i);

		ndn::AppHelper producerHelper ("ns3::ndn::nrndn::djProducer");
		producerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
			for(uint32_t i=0; i<=9; ++i)
				producerHelper.Install(nodes.Get (i));

		//Setup start and end time;
		for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
		{
			double start=mobility->GetStartTime((*i)->GetId());
			double stop =mobility->GetStopTime ((*i)->GetId());
			(*i)->GetApplication(0)->SetAttribute("StartTime",TimeValue (Seconds (start)));
			(*i)->GetApplication(0)->SetAttribute("StopTime", TimeValue (Seconds (stop )));
		}
}

void nrndnExample::Look_at_clock()
{
	cout<<"\nTime now: "<<Simulator::Now().GetSeconds()<<endl;
	//if(int(Simulator::Now().GetSeconds()) % 10 == 3)
		//nrUtils::CoutFullFibNum();
	//if(int(Simulator::Now().GetSeconds() )% 10 == 7)
			//nrUtils::SetFullFibNumZero();


	Simulator::Schedule(Seconds(clockInterval),&nrndnExample::Look_at_clock,this);
}


void nrndnExample::ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack)
{
	for(uint32_t i=0;i<mobilityStack.size();i++)
	{
		Ptr<WaypointMobilityModel> mob = mobilityStack[i]->GetObject<WaypointMobilityModel>();
		Waypoint waypoint = mob->GetNextWaypoint();
		Ptr<MobilityModel> model = nodes.Get(i)->GetObject<MobilityModel>();
		if (model == 0)
		{
			model = mobilityStack[i];

		}
		model->SetPosition(waypoint.position);
	}

}


void nrndnExample::SetPos(Ptr<MobilityModel> mob)
{
	ns3::Vector pos = mob->GetPosition();
	pos.x+=50;
	mob->SetPosition(pos);
	Simulator::Schedule (Seconds (1.0), &nrndnExample::SetPos, this, mob);
}

void nrndnExample::InstallSensor()
{
	NS_LOG_INFO ("Installing Sensors");
	NodeSensorHelper sensorHelper;
	sensorHelper.SetSensorModel("ns3::ndn::nrndn::SumoNodeSensor",
			"sumodata",PointerValue(mobility));
	sensorHelper.InstallAll();
}

void nrndnExample::InstallTraffics()
{

}



void
nrndnExample::getStatistic()
{
	//1. get average arrival rate
	//arrivalRate = nrUtils::GetAverageArrivalRate();

	//2. get average accurate rate
	//accuracyRate=nrUtils::GetAverageAccurateRate();

	//3. get average hit rate
	hitRate = nrUtils::GetAverageHitRate();

	//4. get average delay
	averageDelay = nrUtils::GetAverageDelay();

	//5. get average data forward times
	ResourceForwardTimes = nrUtils::GetResourceForwardSum();

	//6. get average interest forward times
	averageInterestForwardTimes = nrUtils::GetAverageInterestForwardTimes();

	averageDataForwardTimes = nrUtils::GetAverageDataForwardTimes();

	tableSum = nrUtils::GetTableSum();

	averageDetectRate = nrUtils::GetAverageDetectRate();

	averageConfirmRate = nrUtils::GetAverageConfirmRate();

	averageForwardSum = nrUtils::GetAverageForwardSum();

	InterestedNodeReceivedSum = nrUtils::GetInterestedNodeReceivedSum();

}


