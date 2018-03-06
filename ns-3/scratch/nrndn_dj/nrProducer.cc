/*
 * nrProducer.cc
 *
 *  Created on: Jan 12, 2015
 *      Author: chen
 */

#include "nrProducer.h"
#include "NodeSensor.h"
#include "nrUtils.h"
#include "nrndn-Header.h"
#include "ndn-packet-type-tag.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/string.h"
#include "ns3/log.h"
//#include "ns3/core-module.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

NS_LOG_COMPONENT_DEFINE ("ndn.nrndn.nrProducer");

namespace ns3
{
namespace ndn
{
namespace nrndn
{
NS_OBJECT_ENSURE_REGISTERED (nrProducer);

TypeId nrProducer::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::ndn::nrndn::nrProducer")
			    .SetGroupName ("Nrndn")
			    .SetParent<App> ()
			    .AddConstructor<nrProducer> ()
			    .AddAttribute ("Prefix","Prefix, for which producer has the data",
			                    StringValue ("/"),
			                    MakeNameAccessor (&nrProducer::m_prefix),
			                    MakeNameChecker ())
			    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
			                    StringValue ("/"),
			                    MakeNameAccessor (&nrProducer::m_postfix),
			                    MakeNameChecker ())
			    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
			                    UintegerValue (1024),
			                    MakeUintegerAccessor (&nrProducer::m_virtualPayloadSize),
			                    MakeUintegerChecker<uint32_t> ())
			    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
			                    TimeValue (Seconds (0)),
			                    MakeTimeAccessor (&nrProducer::m_freshness),
			                    MakeTimeChecker ())
			    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
			                    UintegerValue (0),
			                    MakeUintegerAccessor (&nrProducer::m_signature),
			                    MakeUintegerChecker<uint32_t> ())
			    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
			                    NameValue (),
			                    MakeNameAccessor (&nrProducer::m_keyLocator),
			                    MakeNameChecker ())
			    ;
			  return tid;
}


nrProducer::nrProducer():
		m_rand (0, std::numeric_limits<uint32_t>::max ()),
		m_virtualPayloadSize(1024),
		m_signature(0)
{
	//NS_LOG_FUNCTION(this);
}

nrProducer::~nrProducer()
{
	// TODO Auto-generated destructor stub
}



void nrProducer::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT(GetNode()->GetObject<Fib>() != 0);//??????????????????????????????????

	if(m_forwardingStrategy)
		m_forwardingStrategy->Start();

	App::StartApplication();
	NS_LOG_INFO("NodeID: " << GetNode ()->GetId ());

	double delay= GetNode ()->GetId () / 2;

	for(uint32_t i = 1; i <= 20; ++i)
	{
		//delay 50s to broadcast resource packet
		//Simulator::Schedule (Seconds (50.0 + delay), &nrProducer::sendResourcePacket, this, i);
		
		//No
		Simulator::Schedule (Seconds (delay), &nrProducer::sendResourcePacket, this, i);
	}

}

void nrProducer::StopApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	NS_ASSERT(GetNode()->GetObject<Fib>() != 0);

	if(m_forwardingStrategy)
		m_forwardingStrategy->Stop();

	std::cout<<"siu:"<<"Stop producer Application:" << GetNode ()->GetId ()<<endl;
	App::StopApplication();
}

void nrProducer::OnInterest(Ptr<const Interest> interest)
{
	NS_ASSERT_MSG(false,"nrProducer should not be supposed to"
				" receive Interest Packet!!");
}

bool nrProducer::isJuction(string lane)
{
	for(uint32_t i = 0; i<lane.length(); ++i)
		if(lane[i] == 't')
			return false;
	return true;
}

void nrProducer::sendResourcePacket(uint32_t n)
{
	////m_sensor->getLane();
	if (!m_active)  return;

	 //cout<<"producer send resource packet"<<endl;
	if(isJuction(m_sensor->getLane()))
	{
		 Simulator::Schedule (Seconds (3.0), &nrProducer::sendResourcePacket, this,n);
		 return;
	}

	uint32_t num = GetNode()->GetId() * 10 + n;
	if(num > 100)
		num -= (GetNode()->GetId() * 10 + 10);
	Name prefix("/");
	//std::cout<<"siu:"<<GetNode()->GetId()<<"sendResourcePacket:"<<m_prefix.toUri()<<std::endl;
	prefix.appendNumber(num);
	//std::cout<<"siu:"<<GetNode()->GetId()<<"sendResourcePacket:"<<prefix.toUri()<<std::endl;
	Ptr<Data> data = Create<Data>(Create<Packet>(m_virtualPayloadSize));
	Ptr<Name> dataName = Create<Name>(prefix);
	//dataName->append(m_postfix);

	data->SetName(dataName);
	data->SetFreshness(m_freshness);
	data->SetTimestamp(Simulator::Now());
	data->SetSignature(m_rand.GetValue());//just generate a random number

	if (m_keyLocator.size() > 0)
	{
		data->SetKeyLocator(Create<Name>(m_keyLocator));
	}

	ndn::nrndn::nrndnHeader nrheader;
	nrheader.setSourceId(GetNode()->GetId() );
	nrheader.setX(m_sensor->getX());
	nrheader.setY(m_sensor->getY());
	string lane = m_sensor->getLane();
	nrheader.setCurrentLane(lane);
	nrheader.setPreLane(lane);
	Ptr<Packet> newPayload	= Create<Packet> (m_virtualPayloadSize);

	newPayload->AddHeader(nrheader);

	data->SetPayload(newPayload);

	PacketTypeTag typeTag(RESOURCE_PACKET);
	data->GetPayload()->AddPacketTag(typeTag);

	FwHopCountTag hopCountTag;
	data->GetPayload()->AddPacketTag(hopCountTag);

	//std::cout<<"siu:"<<"node("<< GetNode()->GetId() <<")\t send Resource Packet in producer:" << data->GetName ()<<",signature:"<<data->GetSignature()<<std::endl;

	m_face->ReceiveData(data);
	m_transmittedDatas(data, this, m_face);

	Simulator::Schedule (Seconds (150.0), &nrProducer::sendResourcePacket, this,n);
}

//Initialize the callback function after the base class initialize
void nrProducer::DoInitialize(void)
{
	if (m_forwardingStrategy == 0)
	{
		Ptr<ForwardingStrategy> forwardingStrategy=m_node->GetObject<ForwardingStrategy>();
		NS_ASSERT_MSG(forwardingStrategy,"nrProducer::DoInitialize cannot find ns3::ndn::fw::ForwardingStrategy");
		if(forwardingStrategy)
			m_forwardingStrategy = DynamicCast<fw::nrndn::NavigationRouteHeuristic>(forwardingStrategy);
	}

	if (m_sensor == 0)
	{
		m_sensor = m_node->GetObject<NodeSensor>();
		NS_ASSERT_MSG(m_sensor,"nrProducer::DoInitialize cannot find ns3::ndn::nrndn::NodeSensor");
	}
	super::DoInitialize();
}

void nrProducer::NotifyNewAggregate()
{
	super::NotifyNewAggregate();
}

bool nrProducer::IsActive()
{
	return m_active;
}

//By DJ on Dec 26, 2017: function name might be section change?
void nrProducer::laneChange(string oldLane, string newLane){
	if (!m_active) return;
	 //cout<<"consumer lane change"<<endl;

	//By DJ on Dec 26, 2017: temporarily delete to avoid error???
	//if(interestSent.empty()) return;
	if(isJuction(newLane) ) return;
	if(oldLane == m_oldLane) return;

	m_oldLane = oldLane;

	  //cout<<m_node->GetId()<<" lane changed from "<<oldLane<<" to "<<newLane<<endl;
	
	//By DJ on Dec 26, 2017:it should be interest's name on consumer. How about on producer???
	//string name = interestSent.begin()->second;
	Name prefix(m_prefix);

	Ptr<Interest> interest = Create<Interest> (Create<Packet>(m_virtualPayloadSize));
	Ptr<Name> interestName = Create<Name> (prefix);
	interest->SetName(interestName);
	interest->SetNonce(m_rand.GetValue());//just generate a random number
	//interest->SetInterestLifetime(m_interestLifeTime);
	//By DJ on Dec 26, 2017:may this time right???
	interest->SetInterestLifetime(m_freshness);
	interest->SetScope(MOVE_TO_NEW_LANE);

	//add header;
	ndn::nrndn::nrndnHeader nrheader;
	nrheader.setSourceId(GetNode()->GetId());
	nrheader.setX(m_sensor->getX());
	nrheader.setY(m_sensor->getY());
	std::string lane = m_sensor->getLane();
	nrheader.setPreLane(oldLane);
	nrheader.setCurrentLane(lane);

	Ptr<Packet> newPayload = Create<Packet> (m_virtualPayloadSize);
	newPayload->AddHeader(nrheader);
	interest->SetPayload(newPayload);

	//cout<<"node: "<<GetNode()->GetId()<<"  send MOVE_TO_NEW_LANE packet,name: "<<prefix.toUri()<<" in consumer"<<endl;

	m_transmittedInterests (interest, this, m_face);
	m_face->ReceiveInterest (interest);
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */


