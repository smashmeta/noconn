/* 
 *
 */

#pragma once

#include <string>

namespace noconn
{
namespace net
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

    class adapter_manager
    {
    public:
    private:
    
    };
} // !namespace net
} // !namespace noconn