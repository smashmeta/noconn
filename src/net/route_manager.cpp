/* 
 *
 */

#include <winsock2.h>
#include <NetCon.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include "noconn/net/wbem_consumer.hpp"
#include "noconn/net/route_manager.hpp"


namespace noconn
{
namespace net
{
    namespace
    {
        std::vector<route_entry> list_routing_table()
        {
            whatlog::logger log("print_routing_table");
            std::vector<route_entry> result;

            DWORD dwSize = 0;
            DWORD dwRetVal = 0;
            struct in_addr IpAddr;

            PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE*)malloc(sizeof(MIB_IPFORWARDTABLE));
            if (pIpForwardTable == nullptr)
            {
                log.error("Error allocating memory for the ip forward table struct.");
            }

            if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
            {
                free(pIpForwardTable);
                pIpForwardTable = (MIB_IPFORWARDTABLE*)malloc(dwSize);
                if (pIpForwardTable == nullptr)
                {
                    log.error("Error allocating memory for the ip forward table struct.");
                }
            }

            /* Note that the IPv4 addresses returned in
            * GetIpForwardTable entries are in network byte order
            */
            if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0)) == NO_ERROR)
            {
                // log.info(fmt::format("Number of entries: {}", pIpForwardTable->dwNumEntries));
                for (int i = 0; i < (int)pIpForwardTable->dwNumEntries; i++)
                {
                    /* Convert IPv4 addresses to strings */
                    IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardDest;
                    std::string destination = inet_ntoa(IpAddr);
                    IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardMask;
                    std::string mask = inet_ntoa(IpAddr);
                    IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardNextHop;
                    std::string gateway = inet_ntoa(IpAddr);

                    int interface_index = pIpForwardTable->table[i].dwForwardIfIndex;
                    int metric = pIpForwardTable->table[i].dwForwardMetric1;

                    // log.info(fmt::format("Route[{}] Dest IP: {}", i, destination));
                    // log.info(fmt::format("Route[{}] Subnet Mask: {}", i, mask));
                    // log.info(fmt::format("Route[{}] Gateway: {}", i, gateway));
                    // log.info(fmt::format("Route[{}] If Index: {}", i, pIpForwardTable->table[i].dwForwardIfIndex));

                    std::string route_type_desc;
                    switch (pIpForwardTable->table[i].dwForwardType)
                    {
                    case MIB_IPROUTE_TYPE_OTHER:
                        route_type_desc = "other";
                        break;
                    case MIB_IPROUTE_TYPE_INVALID:
                        route_type_desc = "invalid route";
                        break;
                    case MIB_IPROUTE_TYPE_DIRECT:
                        route_type_desc = "local route where next hop is final destination";
                        break;
                    case MIB_IPROUTE_TYPE_INDIRECT:
                        route_type_desc = "remote route where next hop is not final destination";
                        break;
                    default:
                        route_type_desc = "UNKNOWN Type value";
                        break;
                    }

                    // log.info(fmt::format("Route[{}] Type: {} -> {}", i, pIpForwardTable->table[i].dwForwardType, route_type_desc));

                    std::string proto_desc;
                    switch (pIpForwardTable->table[i].dwForwardProto)
                    {
                    case MIB_IPPROTO_OTHER:
                        proto_desc = "other";
                        break;
                    case MIB_IPPROTO_LOCAL:
                        proto_desc = "local interface";
                        break;
                    case MIB_IPPROTO_NETMGMT:
                        proto_desc = "static route set through network management";
                        break;
                    case MIB_IPPROTO_ICMP:
                        proto_desc = "result of ICMP redirect";
                        break;
                    case MIB_IPPROTO_EGP:
                        proto_desc = "Exterior Gateway Protocol (EGP)";
                        break;
                    case MIB_IPPROTO_GGP:
                        proto_desc = "Gateway-to-Gateway Protocol (GGP)";
                        break;
                    case MIB_IPPROTO_HELLO:
                        proto_desc = "Hello protocol";
                        break;
                    case MIB_IPPROTO_RIP:
                        proto_desc = "Routing Information Protocol (RIP)";
                        break;
                    case MIB_IPPROTO_IS_IS:
                        proto_desc = "Intermediate System-to-Intermediate System (IS-IS) protocol";
                        break;
                    case MIB_IPPROTO_ES_IS:
                        proto_desc = "End System-to-Intermediate System (ES-IS) protocol";
                        break;
                    case MIB_IPPROTO_CISCO:
                        proto_desc = "Cisco Interior Gateway Routing Protocol (IGRP)";
                        break;
                    case MIB_IPPROTO_BBN:
                        proto_desc = "BBN Internet Gateway Protocol (IGP) using SPF";
                        break;
                    case MIB_IPPROTO_OSPF:
                        proto_desc = "Open Shortest Path First (OSPF) protocol";
                        break;
                    case MIB_IPPROTO_BGP:
                        proto_desc = "Border Gateway Protocol (BGP)";
                        break;
                    case MIB_IPPROTO_NT_AUTOSTATIC:
                        proto_desc = "special Windows auto static route";
                        break;
                    case MIB_IPPROTO_NT_STATIC:
                        proto_desc = "special Windows static route";
                        break;
                    case MIB_IPPROTO_NT_STATIC_NON_DOD:
                        proto_desc = "special Windows static route not based on Internet standards";
                        break;
                    default:
                        proto_desc = "UNKNOWN Proto value";
                        break;
                    }

                    route_entry entry(destination, mask, interface_index, gateway, metric);
                    result.push_back(entry);

                    // log.info(fmt::format("Route[{}] Proto: {} -> {}", i, pIpForwardTable->table[i].dwForwardProto, proto_desc));
                    // log.info(fmt::format("Route[{}] Age: {}", i, pIpForwardTable->table[i].dwForwardAge));
                    // log.info(fmt::format("Route[{}] Metric1: {}", i, pIpForwardTable->table[i].dwForwardMetric1));
                }
                free(pIpForwardTable);
            }
            else {
                log.info("GetIpForwardTable failed.");
                free(pIpForwardTable);
            }

            return result;
        }
    } // !anonymous namespace

    route_identifier::route_identifier(const std::string& destination, const std::string& mask, int interface_index)
        : m_destination(destination), m_mask(mask), m_interface_index(interface_index)
    {
        // nothing for now
    }

    route_entry::route_entry(const std::string& destination, const std::string& mask, int interface_index, const std::string& gateway, int metric)
        : route_entry(route_identifier(destination, mask, interface_index), gateway, metric)
    {
        // nothing for now
    }

    route_entry::route_entry(const route_identifier& identifier, const std::string& gateway, int metric)
        : m_identifier(identifier), m_gateway(gateway), m_metric(metric)
    {
        // nothing for now
    }

    void route_manager::tick()
    {
        whatlog::logger log("route_manager::tick");
        std::vector<route_entry> curr_routes = list_routing_table();
        std::vector<route_entry>& prev_routes = m_routes;

        // detect changes made to the routing table
        // 1. added new routes or changed existing routes
        for (const auto& curr_route : curr_routes)
        {
            bool is_route_added = true;
            bool is_route_changed = false;
            const route_identifier& curr_ident = curr_route.m_identifier;

            // see if we can find existing route
            for (const auto& prev_route : prev_routes)
            {
                const route_identifier& prev_ident = prev_route.m_identifier;
                if (curr_ident.m_destination == prev_ident.m_destination &&
                    curr_ident.m_mask == prev_ident.m_mask &&
                    curr_ident.m_interface_index == prev_ident.m_interface_index)
                {
                    bool is_gateway_changed = curr_route.m_gateway != prev_route.m_gateway;
                    bool is_metric_changed = curr_route.m_metric != prev_route.m_metric;

                    if (is_gateway_changed)
                    {
                        log.info(fmt::format("route_changed: dst: {}, mask: {} (gateway changed) {} => {}.", curr_ident.m_destination, curr_ident.m_mask, prev_route.m_gateway, curr_route.m_gateway));
                    }

                    if (is_metric_changed)
                    {
                        // tips: run "netsh interface ipv4 set interface 1 metric=10" to change metric for a given adapter
                        log.info(fmt::format("route_changed: dst: {}, mask: {} (metric changed) {} => {}.", curr_ident.m_destination, curr_ident.m_mask, prev_route.m_metric, curr_route.m_metric));
                    }

                    is_route_changed = is_gateway_changed || is_metric_changed;
                    is_route_added = false;

                    break;
                }
            }

            if (is_route_changed)
            {
                // todo: implement
            }

            if (is_route_added)
            {
                // todo: implement
                log.info(fmt::format("route_added: dst: {}, mask: {}, gateway: {}, if: {}, metric: {}.", 
                    curr_ident.m_destination, curr_ident.m_mask, curr_route.m_gateway, curr_ident.m_interface_index, curr_route.m_metric));
            }
        }

        // 2. old routes removed
        for (const auto& prev_route : prev_routes)
        {
            bool is_route_removed = true;
            const route_identifier& prev_ident = prev_route.m_identifier;

            // see if route is now missing
            for (const auto& curr_route : curr_routes)
            {
                const route_identifier& curr_ident = curr_route.m_identifier;
                if (prev_ident.m_destination == curr_ident.m_destination &&
                    prev_ident.m_mask == curr_ident.m_mask)
                {
                    is_route_removed = false;
                    break;
                }
            }

            if (is_route_removed)
            {
                // todo: implement
            }
        }

        m_routes = curr_routes;
    }
} // !namespace net
} // !namespace noconn