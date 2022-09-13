/*
 *
 */

#include <winsock2.h>
#include <NetCon.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include "noconn/net/wbem_consumer.hpp"
#include "noconn/net/adapter_manager.hpp"

namespace noconn
{
namespace net
{
    namespace
    {
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
            DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
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

        std::vector<network_adapter> get_network_adapters(noconn::net::shared_wbem_consumer consumer)
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
                    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

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

                            noconn::net::network_adapter adapter(converter.to_bytes(name), converter.to_bytes(guid),
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
    } // !anonymous namespace 
    network_adapter::network_adapter(const std::string& name, const std::string& guid,
        const std::string& description, const std::string& adapter_type, int adapter_index, bool enabled)
        : m_name(name), m_guid(guid), m_description(description), m_type(adapter_type), m_adapter_index(adapter_index), m_enabled(enabled)
    {
        // nothing for now
    }
} // !namespace net
} // !namespace noconn