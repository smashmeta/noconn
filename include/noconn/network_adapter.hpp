/* 
 *
 */

#pragma once

#include <string>
#include <vector>
#include "noconn/wbem_consumer.hpp"

namespace noconn
{

class network_adapter
{
public:
    network_adapter(const std::string& name, const std::string& guid,
        const std::string& description, const std::string& adapter_type, int adapter_index, bool enabled);
public:
    std::string m_name;
    std::string m_guid;
    std::string m_description;
    std::string m_type;
    int m_adapter_index; 
    bool m_enabled;
};

class route_entry
{
public:
    route_entry(const std::string& destination, const std::string& mask, const std::string& gateway, int interface_index, int metric);
public:
    std::string m_destination;
    std::string m_mask;
    std::string m_gateway;
    int m_interface_index;
    int m_metric;
};

std::vector<network_adapter> get_network_adapters(noconn::shared_wbem_consumer consumer);
std::vector<route_entry> list_routing_table();
void list_adapter_ip_addresses();

} // !namespace noconn