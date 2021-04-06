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
        log.error("Memory allocation failed for IP_ADAPTER_ADDRESSES struct");
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
            log.info("length of the IP_ADAPTER_ADDRESS struct: " + std::to_string(pCurrAddresses->Length));
            log.info("IfIndex (IPv4 interface): " + std::to_string(pCurrAddresses->IfIndex));
            log.info("Adapter name: " + std::string(pCurrAddresses->AdapterName));

            pCurrAddresses = pCurrAddresses->Next;
        }
    } 
    else 
    {
        log.error("Call to GetAdaptersAddresses failed with error: " + std::to_string(dwRetVal));
        if (dwRetVal == ERROR_NO_DATA)
        {
            log.error("No addresses were found for the requested parameters");
        }
        else 
        {
            LPVOID lpMsgBuf = NULL;
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)) 
            {
                log.error("error: " + std::string((LPTSTR)lpMsgBuf));
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
    std::vector<network_adapter> result;
    std::wstring query = L"SELECT * FROM Win32_NetworkAdapter";
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
                    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                    noconn::network_adapter adapter(converter.to_bytes(name), converter.to_bytes(guid),
                        converter.to_bytes(desc), converter.to_bytes(adapter_type), index, enabled);

                    result.emplace_back(std::move(adapter));
                }
            }

            VariantClear(&vtProp);
            pclsObj->Release();
        }
    }

    pEnumerator->Release();

    return result; // Program has failed.
}
} // !namespace noconn