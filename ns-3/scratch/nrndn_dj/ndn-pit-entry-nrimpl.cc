/*
 * ndn-pit-entry-nrimpl.cc
 *
 *  Created on: Jan 21, 2015
 *      Author: chenyishun
 */


#include "ndn-pit-entry-nrimpl.h"
#include "ns3/ndn-interest.h"
#include "ns3/core-module.h"
#include "ns3/ndn-forwarding-strategy.h"

#include "ns3/log.h"
NS_LOG_COMPONENT_DEFINE ("ndn.pit.nrndn.EntryNrImpl");

namespace ns3 {
namespace ndn {

class Pit;

namespace pit {
namespace nrndn{
EntryNrImpl::EntryNrImpl(Pit &container, Ptr<const Interest> header,Ptr<fib::Entry> fibEntry)
	:Entry(container,header,fibEntry)
	 //m_infaceTimeout(cleanInterval)
{
	/*NS_ASSERT_MSG(header->GetName().size()<2,"In EntryNrImpl, "
			"each name of interest should be only one component, "
			"for example: /routeSegment, do not use more than one slash, "
			"such as/route1/route2/...");*/
	m_interest_name=header->GetName().toUri();
}

EntryNrImpl::~EntryNrImpl ()
{
  
}

std::unordered_set< std::string  >::iterator
EntryNrImpl::AddIncomingNeighbors(std::string lane)
{
	std::cout<<"add PIT incomingNeighbors"<<std::endl;
	if(m_incomingnbs.empty())
	{
			m_incomingnbs.insert(lane);
			this->Print(std::cout);
			return m_incomingnbs.begin();
	}
	//AddNeighborTimeoutEvent(id);
	std::unordered_set< std::string >::iterator incomingnb = m_incomingnbs.find(lane);

	if(incomingnb==m_incomingnbs.end())
	{//Not found
		std::unordered_set< std::string >::iterator incomingnb_same = m_incomingnbs.begin();
		while(incomingnb_same!=m_incomingnbs.end())
		{
			if(lane == (*incomingnb_same) || isSameLane(lane,(*incomingnb_same)))
			{
				return incomingnb_same;
			}
			incomingnb_same++;
		}
		std::pair<std::unordered_set< std::string >::iterator,bool> ret = m_incomingnbs.insert (lane);
		this->Print(std::cout);
		return ret.first;
	}
	else
	{
		this->Print(std::cout);
		return incomingnb;
	}
}

void EntryNrImpl::setNb(std::unordered_set< std::string > nb)
{
	m_incomingnbs = nb;
}

void EntryNrImpl::setInterestName(std::string name)
{
	m_interest_name = name;
}

/*void EntryNrImpl::AddNeighborTimeoutEvent(uint32_t id)
{
	if(m_nbTimeoutEvent.find(id)!=m_nbTimeoutEvent.end())
	{
		m_nbTimeoutEvent[id].Cancel();
		Simulator::Remove (m_nbTimeoutEvent[id]); // slower, but better for memory
	}
	//Schedule a new cleaning event
	m_nbTimeoutEvent[id]
	                   = Simulator::Schedule(m_infaceTimeout,
	                		   &EntryNrImpl::CleanExpiredIncomingNeighbors,this,id);
}
*/

/*
void EntryNrImpl::CleanExpiredIncomingNeighbors(uint32_t id)
{
	std::unordered_set< uint32_t >::iterator it;
*/
	//std::ostringstream os;
	//os<<"At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id;
	/*
	os<<"To delete neighbor:"<<id<<"\tList is\t";
	for(it=m_incomingnbs.begin();it!=m_incomingnbs.end();++it)
	{
		os<<*it<<"\t";
	}
	*/
/*
	NS_LOG_DEBUG("At PIT Entry:"<<GetInterest()->GetName().toUri()<<" To delete neighbor:"<<id);

	std::unordered_set< uint32_t >::iterator incomingnb  = m_incomingnbs.find(id);

	if (incomingnb != m_incomingnbs.end())
		m_incomingnbs.erase(incomingnb);
}*/

//add by DJ on Jan 4,2016:when it receives corresponding data packet,remove the pit entry
void EntryNrImpl::RemoveIncomingNeighbors(std::string name)
{
	std::cout<<"remove pit incoming neighbors"<<std::endl;
	std::unordered_set< std::string >::iterator it;
    if(name==m_interest_name)
    {
    	for(it=m_incomingnbs.begin();it!=m_incomingnbs.end();++it)
    		{
    			m_incomingnbs.erase(it);
    		}
    }

}

void EntryNrImpl::Print(std::ostream& os) const
{
	os<<"PIT Entry content: "<<" interest name: "<<m_interest_name<<" ";
	for(std::unordered_set< std::string >::const_iterator it = m_incomingnbs.begin(); it != m_incomingnbs.end(); ++it)
		os<<(*it)<<" ";
	os<<std::endl;
}

bool EntryNrImpl::isSameLane(std::string lane1, std::string lane2)
{
	if(lane1.length() != 8 || lane2.length() != 8)
				return false;
	if(lane1[0] == lane2[5] && lane1[2]==lane2[7] && lane1[5]==lane2[0] && lane1[7]==lane2[2])
				return true;
	if(lane1 == lane2)
				return true;
	return false;
}

//By DJ on Dec 20, 2017: find neighbor lane !!!!!!!!!!!!!!!
bool EntryNrImpl::is_neighbor_lane(std::string lane1, std::string lane2){
	return true;
}

//By DJ on Dec 20, 2017: Automatically change PIT
void EntryNrImpl::auto_table_change(std::string pre_lane, std::string next_lane){
	std::unordered_set< std::string >::iterator it;
	for(it = m_incomingnbs.begin(); it != m_incomingnbs.end(); ++it){
		if(!is_neighbor_lane(next_lane, (*it)))
		{
			m_incomingnbs.erase(it); //it指针的操作，需要输出看看正确性
		}
	}
	m_incomingnbs.insert(pre_lane);
	return ;
}

/*
void EntryNrImpl::RemoveAllTimeoutEvent()
{
	std::unordered_map< uint32_t,EventId>::iterator it;
	for(it=m_nbTimeoutEvent.begin();it!=m_nbTimeoutEvent.end();++it)
	{
		it->second.Cancel();
		Simulator::Remove (it->second); // slower, but better for memory
	}
}*/

} // namespace nrndn
} // namespace pit
} // namespace ndn
} // namespace ns3


