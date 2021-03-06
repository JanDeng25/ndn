/*
 * ndn-nr-pit-impl.cc
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#include "ndn-nr-pit-impl.h"
#include "ndn-pit-entry-nrimpl.h"
//#include "NodeSensor.h"


#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.pit.NrPitImpl");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"


namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{


NS_OBJECT_ENSURE_REGISTERED (NrPitImpl);

TypeId
NrPitImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::pit::nrndn::NrPitImpl")
    .SetGroupName ("Ndn")
    .SetParent<Pit> ()
    .AddConstructor< NrPitImpl > ()
   // .AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of PIT entry",
   	//		                    TimeValue (Seconds (10)),
   	//		                    MakeTimeAccessor (&NrPitImpl::m_cleanInterval),
   	//		                    MakeTimeChecker ())
    ;

  return tid;
}

NrPitImpl::NrPitImpl ():
		m_cleanInterval(Seconds(300.0))
{
}

NrPitImpl::~NrPitImpl ()
{
}


void
NrPitImpl::NotifyNewAggregate ()
{
	if (m_fib == 0)
	{
		m_fib = GetObject<Fib>();
	}
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
		// Setup Lane change action
		/*if (m_sensor != NULL)
		{
			//m_sensor->TraceConnectWithoutContext("LaneChange",
				//	MakeCallback(&NrPitImpl::laneChange, this));

			//NrPitEntry needs m_sensor. Initialize immediately after m_sensor is aggregated
			InitializeNrPitEntry();
		}*/
	}

  Pit::NotifyNewAggregate ();
}

  //By DJ on Dec 21,2017: auto update pit
 void NrPitImpl::auto_update_pit(std::string pre_lane, std::string next_lane){
 	if(m_pitContainer.empty())
 		return;
 	std::vector<Ptr<pit::Entry> >::iterator pit=m_pitContainer.begin();
 	for(; pit != m_pitContainer.end(); ++pit){
 		Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
 		pitEntry->auto_table_change(pre_lane, next_lane);
 	}
 	return;
 }


//add by DJ on Jan 4,2016:update pit
bool NrPitImpl::UpdatePit(std::string lane,Ptr<const Interest> interest)
{
	std::cout << "PIT UpdatePit" << ":" << "lane:" << lane << std::endl;
	if(m_pitContainer.empty())
	{
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));
		Ptr<EntryNrImpl> fentry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry);
		fentry->AddIncomingNeighbors(lane);
		Ptr<Entry> pitEntry = DynamicCast<Entry>(fentry);
		m_pitContainer.push_back(pitEntry);
		this->Print(std::cout);
		return true;
	}
	else
	{
		//std::ostringstream os;
		std::vector<Ptr<pit::Entry> >::iterator pit=m_pitContainer.begin();
		for(;pit!=m_pitContainer.end();++pit)
		{
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			//const name::Component &pitName=(*pit)->GetInterest()->GetName().get(0);
			if(pitEntry->getEntryName() == interest->GetName().toUri())
			{
				std::unordered_set< std::string >::const_iterator it = pitEntry->getIncomingnbs().find(lane);
				if(it==pitEntry->getIncomingnbs().end())
				{
					pitEntry->AddIncomingNeighbors(lane);
				}
				this->Print(std::cout);
				return true;
			}
		}
	    Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));
	    Ptr<EntryNrImpl> fentry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry);
	    fentry->AddIncomingNeighbors(lane);
		Ptr<Entry> pitEntry = DynamicCast<Entry>(fentry);
		m_pitContainer.push_back(pitEntry);
		this->Print(std::cout);
	}
	return true;
}

//Mar 17,2016: merge two pit table
 void
 NrPitImpl::mergePit(std::vector<Ptr<Entry> >  pitCon)
 {
	 if(pitCon.size()==0)
		 return;
	 std::vector<Ptr<Entry> >::iterator pit_pitCon = pitCon.begin();
	 for(;pit_pitCon!=pitCon.end();++pit_pitCon)
	 {
		 Ptr<EntryNrImpl> pit_pitConEntry = DynamicCast<EntryNrImpl>(*pit_pitCon);
		 std::unordered_set< std::string >::const_iterator incomingnb = pit_pitConEntry->getIncomingnbs().begin();
		 for(;incomingnb != pit_pitConEntry->getIncomingnbs().end(); ++incomingnb){
			 UpdatePit(*incomingnb, pit_pitConEntry->GetInterest());
		 }
	 }

 }

//add by DJ on Jan 4,2016:update pit
bool NrPitImpl::RemovePitEntry(const Name& name)
{
	std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();
	//Ptr<Entry> entry = *pit;
	for(;pit!=m_pitContainer.end();++pit)
	{
		Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
		if(pitEntry->getEntryName() == name.toUri())
		{
			//pitEntry->RemoveIncomingNeighbors(name.toUri());
			m_pitContainer.erase(pit);
			//this->Print(std::cout);
			return true;
		}
	}
	//this->Print(std::cout);
	return false;
}

void
NrPitImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	//m_fib = 0;
  
	Pit::DoDispose ();
 }
  

Ptr<Entry>
NrPitImpl::Lookup (const Data &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Data &header) should not be invoked");
	return 0;
 }
  
Ptr<Entry>
NrPitImpl::Lookup (const Interest &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Interest &header) should not be invoked");
	return 0;
 }
  

Ptr<Entry>
NrPitImpl::Find (const Name &prefix)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Find (const Name &prefix) should not be invoked");
	NS_LOG_INFO ("Finding prefix"<<prefix.toUri());
	 
	std::vector<Ptr<Entry> >::iterator it;
	//NS_ASSERT_MSG(m_pitContainer.size()!=0,"Empty pit container. No initialization?");
	for(it=m_pitContainer.begin();it!=m_pitContainer.end();++it)
	{
		if((*it)->GetPrefix()==prefix){
			//std::cout<<"Found PIT Entry name:"<< (*it)->GetPrefix().toUri() << std::endl; // test
			return *it;
		}
	}
	return 0;
}
  

Ptr<Entry>
NrPitImpl::Create (Ptr<const Interest> header)
 {

	NS_LOG_DEBUG (header->GetName ());
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Create (Ptr<const Interest> header) "
			"should not be invoked, use "
			"NrPitImpl::CreateNrPitEntry instead");
	return 0;
}

// need to modify:how to initialize?
//test pit
/*bool
NrPitImpl::InitializeNrPitEntry()
{
	NS_LOG_FUNCTION (this);
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	std::vector<std::string>::const_iterator rit;
	for(rit=route.begin();rit!=route.end();++rit)
	{
		Ptr<Name> name = ns3::Create<Name>('/'+*rit);
		Ptr<Interest> interest=ns3::Create<Interest> ();
		interest->SetName				(name);
		interest->SetInterestLifetime	(Time::Max());//never expire

		//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using pitEntry->GetFibEntry)
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));

		Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry) ;

		m_pitContainer.push_back(entry);
		NS_LOG_DEBUG("Initialize pit:Push_back"<<name->toUri());
	}
	return true;
}
  */

void
NrPitImpl::MarkErased (Ptr<Entry> item)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}
  
  
void
NrPitImpl::Print (std::ostream& os) const
{
	os<<"my lane:"<<m_sensor->getLane()<<std::endl;
	os<<"PIT content "<<std::endl;;
	std::vector<Ptr<Entry> >::const_iterator it;
	for(it=m_pitContainer.begin();it!=m_pitContainer.end();++it)
	{
		Ptr<ndn::pit::nrndn::EntryNrImpl>  temp = DynamicCast<ndn::pit::nrndn::EntryNrImpl>(*it);
		os<<"name:   "<< temp->getEntryName();
		std::unordered_set< std::string >::const_iterator i = temp->getIncomingnbs().begin();
		for(; i !=  temp->getIncomingnbs().end(); ++i)
			os<<" lane:"<<(*i);
		os<<std::endl;
	}
}

uint32_t
NrPitImpl::GetSize () const
{
	return m_pitContainer.size ();
}
  
Ptr<Entry>
NrPitImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Begin () should not be invoked");

	if(m_pitContainer.begin() == m_pitContainer.end())
		return End();
	else
		return *(m_pitContainer.begin());
}

Ptr<Entry>
NrPitImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::End () should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrPitImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_pitContainer.begin(),m_pitContainer.end(),from);
	if(it==m_pitContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_pitContainer.end())
			return End();
		else
			return *it;
	}
}

//小锟添加，2015-8-23
std::string NrPitImpl::uriConvertToString(std::string str)
{
	//因为获取兴趣时使用toUri，避免出现类似[]的符号，进行编码转换
	std::string ret="";
	for(uint32_t i=0;i<str.size();i++)
	{
		if(i+2<str.size())
		{
			if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='B')
			{
				ret+="[";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='D')
			{
				ret+="]";
				i=i+2;
			}
			else
				ret+=str[i];
		}
		else
			ret+=str[i];
	}
	return ret;
}


void NrPitImpl::DoInitialize(void)
{
	Pit::DoInitialize();
}

} /* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */


