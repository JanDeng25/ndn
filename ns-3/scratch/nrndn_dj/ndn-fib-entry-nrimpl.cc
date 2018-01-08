/*
 * ndn-fib-entry-nrimpl.cc
 *
 *  Created on: Jan 21, 2015
 *      Author: chenyishun
 */


#include "ndn-fib-entry-nrimpl.h"
#include "ns3/ndn-data.h"
#include "ns3/core-module.h"
#include "ns3/ndn-forwarding-strategy.h"

#include "ns3/log.h"
NS_LOG_COMPONENT_DEFINE ("ndn.fib.nrndn.EntryNrImpl");

namespace ns3 {
namespace ndn {

class Fib;

namespace fib {
namespace nrndn{
EntryNrImpl::EntryNrImpl(Ptr<Fib> fib, const Ptr<const NameComponents> &prefix,Time cleanInterval)
	:Entry(fib,prefix),
	 m_infaceTimeout(cleanInterval)
{
	/*NS_ASSERT_MSG(prefix.size()<2,"In EntryNrImpl, "
			"each name of data should be only one component, "
			"for example: /routeSegment, do not use more than one slash, "
			"such as/route1/route2/...");*/
	m_data_name=prefix->toUri();
}

EntryNrImpl::~EntryNrImpl ()
{
  
}


std::unordered_map<std::string,std::pair<uint32_t, uint32_t> >::iterator
EntryNrImpl::AddIncomingNeighbors(std::string lane, std::pair<uint32_t, uint32_t> p)
{
	//std::cout<<"add FIB incomingNeighbors  name:  "<<m_data_name<<"  lane: "<<lane<<"  TTL: "<<ttl<<std::endl;
	if(m_incomingnbs.empty())
	{
		m_incomingnbs.insert(m_incomingnbs.begin(),std::pair<std::string,std::pair<uint32_t, uint32_t> >(lane, p));
		//this->Print(std::cout);
		return m_incomingnbs.begin();
	}
	//AddNeighborTimeoutEvent(id);

	//isSamelane
	if(lane == m_incomingnbs.begin()->first || isSameLane(lane,m_incomingnbs.begin()->first)){
		if(m_incomingnbs.begin()->second.first > p.first)
		{
			m_incomingnbs.erase(m_incomingnbs.begin());
			m_incomingnbs.insert(m_incomingnbs.begin(), std::pair<std::string,std::pair<uint32_t, uint32_t> >(lane,p));
		}
		return m_incomingnbs.begin();
	}
	std::unordered_map< std::string, std::pair<uint32_t, uint32_t> >::iterator incomingnb = m_incomingnbs.find(lane);

	if(incomingnb==m_incomingnbs.end())
	{//Not found
		//std::pair<std::unordered_set< std::string >::iterator,bool> ret =
				//m_incomingnbs.insert (lane);
		//return ret.first;
		incomingnb = m_incomingnbs.begin();
		if(incomingnb->second.first > p.first)
		{
			m_incomingnbs.erase(m_incomingnbs.begin());
			m_incomingnbs.insert(m_incomingnbs.begin(),std::pair<std::string,std::pair<uint32_t, uint32_t> >(lane,p));
		}

		/*while(incomingnb != m_incomingnbs.end() && incomingnb->second < ttl)
		{
			incomingnb++;
		}
		m_incomingnbs.insert(incomingnb,std::pair<std::string,uint32_t>(lane,ttl));
		//this->Print(std::cout);*/
		return incomingnb;
	}
	else
	{
		//this->Print(std::cout);
		return incomingnb;
	}
}

void EntryNrImpl::Print(std::ostream& os) const
{
	os<<"FIB Entry content: "
			<<" data name="<<m_data_name<<"   ";
	if(m_incomingnbs.empty())
	{
		os<<",  empty"<<std::endl;
		return;
	}
	for(std::unordered_map< std::string,uint32_t >::const_iterator it = m_incomingnbs.begin(); it != m_incomingnbs.end(); ++it)
		os<<(*it).first<<"   "<<(*it).second<<"    ";
	os<<std::endl;
}
void EntryNrImpl::setDataName(std::string name)
{
	m_data_name = name;
}

void EntryNrImpl::setNb(std::unordered_map< std::string,std::pair<uint32_t, uint32_t >  >  nb)
{
	m_incomingnbs = nb;
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

//By DJ on Dec 21, 2017: find neighbor lane !!!!!!!!!
bool EntryNrImpl::is_neighbor_lane(std::string lane1, std::string lane2){
	return true;
}
	
//By DJ on Dec 21, 2017: Automatically change FIB
void EntryNrImpl::auto_table_change(std::string pre_lane, std::string next_lane){
	std::unordered_map< std::string, std::pair<uint32_t, uint32_t > >::iterator it;
	std::pair<uint32_t, uint32_t > temp(100, 100);    						//initialize to 100 hops
	for(it = m_incomingnbs.begin(); it != m_incomingnbs.end(); ++it){
		if(!is_neighbor_lane(next_lane, it->first)){
			if(temp.first > it->second.first){
				temp = it->second;
			}
			m_incomingnbs.erase(it);
		}
	}
	temp.first++;
	temp.second++;
	if(temp.second < 3)
		m_incomingnbs.insert(std::pair<std::string,std::pair<uint32_t, uint32_t > >(pre_lane,temp));
	return ;
}

/*void EntryNrImpl::RemoveEntry()
{

}*/

} // namespace nrndn
} // namespace fib
} // namespace ndn
} // namespace ns3


