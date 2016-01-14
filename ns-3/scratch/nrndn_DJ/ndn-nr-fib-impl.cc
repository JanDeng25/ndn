/*
 * ndn-nr-fib-impl.cc
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#include "ndn-nr-fib-impl.h"
#include "ndn-fib-entry-nrimpl.h"
//#include "NodeSensor.h"


#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.fib.NrFibImpl");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ndn
{
namespace fib
{
namespace nrndn
{


NS_OBJECT_ENSURE_REGISTERED (NrFibImpl);

TypeId
NrFibImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::fib::nrndn::NrFibImpl")
    .SetGroupName ("Ndn")
    .SetParent<Fib> ()
    .AddConstructor< NrFibImpl > ()
    .AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of FIB entry",
   			                    TimeValue (Seconds (10)),
   			                    MakeTimeAccessor (&NrFibImpl::m_cleanInterval),
   			                    MakeTimeChecker ())
    ;

  return tid;
}

NrFibImpl::NrFibImpl ():
		m_cleanInterval(Seconds(10.0))
{
}

NrFibImpl::~NrFibImpl ()
{
}


void
NrFibImpl::NotifyNewAggregate ()
{
	////delete by DJ on Dec 27,2015:no fib itself
	/*if (m_fib == 0)
	{
		m_fib = GetObject<Fib>();
	}*/
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
		// Setup Lane change action
		if (m_sensor != NULL)
		{
			m_sensor->TraceConnectWithoutContext("LaneChange",
					MakeCallback(&NrFibImpl::laneChange, this));

			//NrFibEntry needs m_sensor. Initialize immediately after m_sensor is aggregated
			InitializeNrFibEntry();
		}
	}

  Fib::NotifyNewAggregate ();
}

void
NrFibImpl::AddFibEntry (const Name &prefix, std::string lane,uint32_t ttl){
	std::vector<Ptr<Entry> >::iterator fib=m_fibContainer.begin();
	for(;fib!=m_fibContainer.end();++fib)
		{
			Ptr<EntryNrImpl> fibEntry = DynamicCast<EntryNrImpl>(*fib);
			//const name::Component &fibName=(*fib)->GetInterest()->GetName().get(0);
            //when find source name
			if(fibEntry->getEntryName() == prefix.toUri())
			{

				fibEntry->AddIncomingNeighbors(lane,ttl);

			}

		}

	//if not,insert it to the container;

	Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,prefix,m_cleanInterval);
	m_fibContainer.push_back(entry);

}

void
NrFibImpl::Remove (const Ptr<const Name> &prefix){

}

//modify by DJ Jan 5,2016. Fib update according to source packet.
/*bool NrFibImpl::UpdateFib(std::string lane,Ptr<const Data> data)
{
	//std::ostringstream os;
	std::vector<Ptr<Entry> >::iterator fib=m_fibContainer.begin();
	Ptr<Entry> entry = *fib;
	/*
	 Name::const_iterator head=entry->GetInterest()->GetName().begin();
	//Can name::Component use "=="?
	std::vector<std::string>::const_iterator it=
			std::find(route.begin(),route.end(),head->toUri());
	if(it==route.end())
		return false;
		*/
	/*for(;fib!=m_fibContainer.end();++fib)
	{
		Ptr<EntryNrImpl> fibEntry = DynamicCast<EntryNrImpl>(*fib);
		//const name::Component &fibName=(*fib)->GetInterest()->GetName().get(0);
		if(fibEntry->getEntryName() == data->GetName().toUri())
		{
			std::unordered_set< std::string >::iterator it = pitEntry->getIncomingnbs().find(lane);
			if(it==fibEntry->getIncomingnbs().end())
				fibEntry->AddIncomingNeighbors(lane);
			//os<<(*fib)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
		    return true;
		}
		//else
			//break;

	}

	//NS_LOG_UNCOND("update fib:"<<os.str());
	//NS_LOG_DEBUG("update fib:"<<os.str());
	return true;
}*/

void
NrFibImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	//m_fib = 0;
  
	Fib::DoDispose ();
 }
  


//modify by DJ Dec 25,2015. Fib update according to source packet.
Ptr<Entry>
NrFibImpl::Find (const Name &prefix)
{
	//NS_ASSERT_MSG(false,"In NrFibImpl,NrFibImpl::Find (const Name &prefix) should not be invoked");
	 NS_LOG_INFO ("Finding prefix"<<prefix.toUri());
	 std::vector<Ptr<Entry> >::iterator it;
	 NS_ASSERT_MSG(m_fibContainer.size()!=0,"Empty fib container. No initialization?");
	 for(it=m_fibContainer.begin();it!=m_fibContainer.end();++it)
	 {
		 if((*it)->GetPrefix()==prefix)
			 return *it;
	 }
	return 0;
}
  


//modify by DJ Dec 25,2015. Fib update according to source packet.
Ptr<Entry>
NrFibImpl::Create (Ptr<const Data> header)
 {

	NS_LOG_DEBUG (header->GetName ());
	NS_ASSERT_MSG(false,"In NrFibImpl,NrFibImpl::Create (Ptr<const Data> header) "
			"should not be invoked, use "
			"NrFibImpl::CreateNrFibEntry instead");
	return 0;
}

//how to initialize fibEntry?
//need to modify by DJ Dec 25,2015. Fib update according to source packet.
/*bool
NrFibImpl::InitializeNrFibEntry()
{
	NS_LOG_FUNCTION (this);
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	std::vector<std::string>::const_iterator rit;
	for(rit=route.begin();rit!=route.end();++rit)
	{
		Ptr<Name> name = ns3::Create<Name>('/'+*rit);
		Ptr<Data> data=ns3::Create<Data> ();
		//Ptr<Interest> interest=ns3::Create<Interest> ();
		//interest->SetInterestLifetime();
		data->SetName				(name);
		//by DJ on Dec 27,2015:setLifeTime
		//data->SetInterestLifetime	(Time::Max());//never expire

		//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using fibEntry->GetFibEntry)
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));

		Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,data,fibEntry,m_cleanInterval) ;
		m_fibContainer.push_back(entry);
		NS_LOG_DEBUG("Initialize fib:Push_back"<<name->toUri());
	}
	return true;
}*/
  
  
void
NrFibImpl::Print (std::ostream& os) const
{

}

uint32_t
NrFibImpl::GetSize () const
{
	return m_fibContainer.size ();
}
  
Ptr<Entry>
NrFibImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrFibImpl,NrFibImpl::Begin () should not be invoked");

	if(m_fibContainer.begin() == m_fibContainer.end())
		return End();
	else
		return *(m_fibContainer.begin());
}

Ptr<Entry>
NrFibImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrFibImpl,NrFibImpl::End () should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrFibImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrFibImpl,NrFibImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_fibContainer.begin(),m_fibContainer.end(),from);
	if(it==m_fibContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_fibContainer.end())
			return End();
		else
			return *it;
	}
}

//灏忛敓娣诲姞锛�015-8-23
std::string NrFibImpl::uriConvertToString(std::string str)
{
	//鍥犱负鑾峰彇鍏磋叮鏃朵娇鐢╰oUri锛岄伩鍏嶅嚭鐜扮被浼糩]鐨勭鍙凤紝杩涜缂栫爜杞崲
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

//杩涘叆涓�釜鏂扮殑璺鍚庣殑fib琛ㄦ牸鍙樺寲锛�void NrFibImpl::laneChange(std::string oldLane, std::string newLane)
/*void laneChange(std::string oldLane, std::string newLane){
	if (oldLane.empty()
			|| (ndn::nrndn::NodeSensor::emptyLane == oldLane
					&& ndn::nrndn::NodeSensor::emptyLane != newLane))
		return;
	NS_LOG_INFO ("Deleting old lane fib entry of "<<oldLane);

	std::vector<Ptr<Entry> >::iterator it;
	it =m_fibContainer.begin();

	bool IsOldLaneAtFibBegin =(  uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())==(oldLane));

	if(!IsOldLaneAtFibBegin)
	{
		std::cout<<"鏃ц矾娈典笉鍦ㄥご閮�"<<"oldLane:"<<(oldLane)<<" newLane:"<<uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())<<std::endl;

		//閬嶅巻鏁翠釜Fib
		std::vector<Ptr<Entry> >::iterator itTraversal;
		itTraversal =m_fibContainer.begin();
		bool findOldLane=false;
		std::cout<<"瀵绘壘oldLane涓�..\n";
		for(;itTraversal!=m_fibContainer.end();itTraversal++)
		{//閬嶅巻鏁翠釜FIB琛紝瀵绘壘oldLane鏄惁鍦ㄨ〃涓�			if( uriConvertToString((*itTraversal)->GetInterest()->GetName().get(0).toUri()) == (oldLane) )
			{//濡傛灉鎵惧埌鍒欑洿鎺ヨ烦鍑�				findOldLane=true;
				break;
			}
		}
		if(findOldLane)
		{
			it =m_fibContainer.begin();
			int a=0;
			while(  uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())!=(oldLane)
					&&it!=m_fibContainer.end())
			{
				std::cout<<a<<"閬嶅巻鍒犻櫎涓細"<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				a++;
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				m_fibContainer.erase(it);
				it =m_fibContainer.begin();
			}
			if(it<=m_fibContainer.end())
			{
				std::cout<<"鏈�悗閬嶅巻鍒犻櫎涓細"<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				//2. erase it
				m_fibContainer.erase(it);
				std::cout<<"鍒犻櫎瀹屾瘯\n";
			}
			else
				std::cout<<"鍒犻櫎瀹屾瘯锛氳凯浠ｅ櫒涓虹┖\n";

		}
		else
		{
			std::cout<<"娌℃壘鍒�..\n";
		}
	}
	else
	{//鏃ц矾娈靛湪fib澶撮儴鎵嶈繘琛屽垹闄�
			//鎶ラ敊锛�		//NS_ASSERT_MSG(IsOldLaneAtFibBegin,"The old lane should at the beginning of the fibContainer. Please Check~");
		//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
		DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();

		//2. erase it
		m_fibContainer.erase(it);
		//std::cout<<"erase OK!"<<std::endl;
		return;
	}

}*/

void NrFibImpl::DoInitialize(void)
{
	Fib::DoInitialize();
}

} /* namespace nrndn */
} /* namespace fib */
} /* namespace ndn */
} /* namespace ns3 */


