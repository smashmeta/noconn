/* 
 *
 */

#include <locale>
#include <codecvt>
#include <iostream>
#include <winsock2.h>
#include <NetCon.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>

#include <fmt/core.h>

#include "noconn/network_adapter.hpp"
#include "whatlog/logger.hpp"

namespace noconn
{
network_adapter::network_adapter(const std::string& name, const std::string& guid,
        const std::string& description, const std::string& adapter_type, int adapter_index, bool enabled)
    : m_name(name), m_guid(guid), m_description(description), m_type(adapter_type), m_adapter_index(adapter_index), m_enabled(enabled)
{
    // nothing for now
}

route_entry::route_entry(const std::string& destination, const std::string& mask, const std::string& gateway, int interface_index, int metric)
    : m_destination(destination), m_mask(mask), m_gateway(gateway), m_interface_index(interface_index), m_metric(metric)
{
    // nothing for now
}

void list_adapter_ip_addresses()
{   
    whatlog::logger log("list_adapter_ip_addresses");
    ULONG  outBufLen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES  pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;


    // Make an initial call to GetAdaptersAddresses to get the 
    // size needed into the outBufLen variable
    if (GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW) 
    {
        free(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    }

    if (pAddresses == NULL) 
    {
        log.error("Memory allocation failed for IP_ADAPTER_ADDRESSES struct.");
        return;
    }

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    DWORD dwRetVal =  GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    if (dwRetVal == NO_ERROR) 
    {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) 
        {
            log.info(fmt::format("length of the IP_ADAPTER_ADDRESS struct: {}.", pCurrAddresses->Length));
            log.info(fmt::format("IfIndex (IPv4 interface): {}.", pCurrAddresses->IfIndex));
            log.info(fmt::format("Adapter name: {}.", pCurrAddresses->AdapterName));

            PIP_ADAPTER_UNICAST_ADDRESS unicast_address = pCurrAddresses->FirstUnicastAddress;
            for (size_t index = 0; unicast_address != nullptr; ++index)
            {
                ULONG buffer_size = 256;
                std::array<char, 256> buffer;
                long ip_to_string_cast_result = WSAAddressToStringA(unicast_address->Address.lpSockaddr, unicast_address->Address.iSockaddrLength, nullptr, const_cast<char*>(buffer.data()), &buffer_size);
                if (ip_to_string_cast_result == NO_ERROR)
                {
                    log.info(fmt::format("{} - address: {}.", index, buffer.data()));
                }
                else
                {
                    log.error("failed to cast unicast address struct to string.");
                }

                if (unicast_address->Address.lpSockaddr->sa_family == AF_INET)
                {
                    unsigned long netmask = 0;
                    if (ConvertLengthToIpv4Mask(unicast_address->OnLinkPrefixLength, &netmask) == NO_ERROR)
                    {
                        in_addr netmask_address;
                        netmask_address.S_un.S_addr = netmask;
                        log.info(fmt::format("{} - mask: {}.", index, inet_ntoa(netmask_address)));
                    }
                    else
                    {
                        log.error(fmt::format("failed to convert netmask length \"{}\" to ip address.", unicast_address->OnLinkPrefixLength));
                    }
                }
                else if (unicast_address->Address.lpSockaddr->sa_family == AF_INET6)
                {
                    // ignore for now
                }
                else
                {
                    log.warning(fmt::format("unicast_address->Address.lpSockaddr->sa_family = {}.", unicast_address->Address.lpSockaddr->sa_family));
                }
                
                unicast_address = unicast_address->Next;
            }


            pCurrAddresses = pCurrAddresses->Next;
        }
    } 
    else 
    {
        log.error(fmt::format("Call to GetAdaptersAddresses failed with error: {}.", dwRetVal));
        if (dwRetVal == ERROR_NO_DATA)
        {
            log.error("No addresses were found for the requested parameters.");
        }
        else 
        {
            LPVOID lpMsgBuf = NULL;
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)) 
            {
                log.error(fmt::format("error: {}", std::string((LPTSTR)lpMsgBuf)));
                LocalFree(lpMsgBuf);

                free(pAddresses);
                exit(1);
            }
        }
    }

    free(pAddresses);
}


std::vector<network_adapter> get_network_adapters(noconn::shared_wbem_consumer consumer)
{
    whatlog::logger log("get_network_adapters");
    
    std::vector<network_adapter> result;
    std::wstring query = L"SELECT * FROM Win32_NetworkAdapter";
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    IEnumWbemClassObject* pEnumerator = consumer->exec_query(query);
    if (pEnumerator != nullptr)
    {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;

        while (pEnumerator)
        {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
                &pclsObj, &uReturn);

            if (0 == uReturn)
            {
                break;
            }

            VARIANT vtProp;
            HRESULT hr_guid = pclsObj->Get(L"GUID", 0, &vtProp, 0, 0);
            // only list adapters with a valid GUID
            if (vtProp.pbstrVal != nullptr)
            {
                std::wstring guid = vtProp.bstrVal;

                HRESULT hr_name = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
                std::wstring name = vtProp.bstrVal;

                HRESULT hr_desc = pclsObj->Get(L"Description", 0, &vtProp, 0, 0);
                std::wstring desc = vtProp.bstrVal;

                HRESULT hr_adapter_type = pclsObj->Get(L"AdapterType", 0, &vtProp, 0, 0);
                std::wstring adapter_type = L"[UNAVAILABLE]";
                if (vtProp.pbstrVal != nullptr)
                {
                    adapter_type = vtProp.bstrVal;
                }

                HRESULT hr_interface_index = pclsObj->Get(L"InterfaceIndex", 0, &vtProp, 0, 0);
                int index = vtProp.intVal;

                HRESULT hr_enabled = pclsObj->Get(L"NetEnabled", 0, &vtProp, 0, 0);
                bool enabled = vtProp.boolVal;

                if (SUCCEEDED(hr_guid) && 
                    SUCCEEDED(hr_name) && 
                    SUCCEEDED(hr_desc) && 
                    SUCCEEDED(hr_adapter_type) && 
                    SUCCEEDED(hr_interface_index) && 
                    SUCCEEDED(hr_enabled))
                {
                    
                    noconn::network_adapter adapter(converter.to_bytes(name), converter.to_bytes(guid),
                        converter.to_bytes(desc), converter.to_bytes(adapter_type), index, enabled);

                    result.emplace_back(std::move(adapter));
                }
            }

            VariantClear(&vtProp);
            pclsObj->Release();
        }

        pEnumerator->Release();
    }
    else
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        log.error(fmt::format("failed to execute query \"{}\".", converter.to_bytes(query)));
    }
    
    return result; 
}

std::vector<route_entry> list_routing_table()
{
    whatlog::logger log("print_routing_table");
    std::vector<route_entry> result;

    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    struct in_addr IpAddr;

    PMIB_IPFORWARDTABLE pIpForwardTable = (MIB_IPFORWARDTABLE *)malloc(sizeof (MIB_IPFORWARDTABLE));
    if (pIpForwardTable == nullptr) 
    {
        log.error("Error allocating memory for the ip forward table struct.");
    }

    if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) 
    {
        free(pIpForwardTable);
        pIpForwardTable = (MIB_IPFORWARDTABLE *) malloc(dwSize);
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
        log.info(fmt::format("Number of entries: {}", pIpForwardTable->dwNumEntries));
        for (int i = 0; i < (int)pIpForwardTable->dwNumEntries; i++)
        {
            /* Convert IPv4 addresses to strings */
            IpAddr.S_un.S_addr = (u_long) pIpForwardTable->table[i].dwForwardDest;
            std::string destination = inet_ntoa(IpAddr);
            IpAddr.S_un.S_addr = (u_long) pIpForwardTable->table[i].dwForwardMask;
            std::string mask = inet_ntoa(IpAddr);
            IpAddr.S_un.S_addr = (u_long) pIpForwardTable->table[i].dwForwardNextHop;
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
                route_type_desc ="remote route where next hop is not final destination";
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

            route_entry entry(destination, mask, gateway, interface_index, metric);
            result.push_back(entry);
            
            // log.info(fmt::format("Route[{}] Proto: {} -> {}", i, pIpForwardTable->table[i].dwForwardProto, proto_desc));
            // log.info(fmt::format("Route[{}] Age: {}", i, pIpForwardTable->table[i].dwForwardAge));
            // log.info(fmt::format("Route[{}] Metric1: {}", i, pIpForwardTable->table[i].dwForwardMetric1));
        }
        free(pIpForwardTable);
    } else {
        log.info("GetIpForwardTable failed.");
        free(pIpForwardTable);
    }

    return result;
}
} // !namespace noconn