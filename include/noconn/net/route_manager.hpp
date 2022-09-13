/* 
 *
 */

#pragma once

#include <string>
#include <vector>

namespace noconn
{
namespace net
{
	class route_identifier
	{
	public:
		route_identifier(const std::string& destination, const std::string& mask, int interface_index);
	public:
		std::string m_destination;
		std::string m_mask;
		int m_interface_index;
	};

	class route_entry
	{
	public:
		route_entry(const std::string& destination, const std::string& mask, int interface_index, const std::string& gateway, int metric);
		route_entry(const route_identifier& identifier, const std::string& gateway, int metric);
	public:
		route_identifier m_identifier;
		std::string m_gateway;
		int m_metric;
	};

	class route_manager
	{
	public:
		void tick();

		bool add_route(const route_entry& entry);
		bool remove_route(const route_identifier& entry);
		std::vector<route_entry> get_routes() const;
	private:
		std::vector<route_entry> m_routes;
	};
} // !namespace net
} // !namespace noconn