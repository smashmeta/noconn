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

struct ipv4
{

};

std::vector<network_adapter> get_network_adapters(noconn::shared_wbem_consumer consumer);
void list_adapter_ip_addresses();
} // !namespace noconn