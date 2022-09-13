/*
 *
 */

#include <locale>
#include <codecvt>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <iphlpapi.h>
#include <fmt/core.h>
#include "noconn/net/wbem_consumer.hpp"
#include "whatlog/logger.hpp"

namespace noconn
{
namespace net
{
    namespace 
    {
        std::string hresult_to_hex_str(HRESULT result)
        {
            std::stringstream stream;
            stream << std::hex << result;
            return "0x" + stream.str();
        }
    } // !anonymous namespace

    struct wbem_consumer::impl
    {
        IWbemLocator* m_wbem_location = nullptr;
        IWbemServices* m_wbem_services = nullptr;

        static impl* create_wbem_impl()
        {
            impl* result = new impl;
            whatlog::logger log("create_wbem_impl");

            // create the wbem locator
            HRESULT hres = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator, (LPVOID*)&result->m_wbem_location);

            if (FAILED(hres))
            {
                log.error(fmt::format("Failed to create IWbemLocator object. Error code = {}.", hresult_to_hex_str(hres)));
                delete result;
                return nullptr;
            }

            // Connect to the root\cimv2 namespace with
            // the current user and obtain pointer pSvc
            // to make IWbemServices calls.
            hres = result->m_wbem_location->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
                NULL,                    // User name. NULL = current user
                NULL,                    // User password. NULL = current
                0,                       // Locale. NULL indicates current
                NULL,                    // Security flags.
                0,                       // Authority (for example, Kerberos)
                0,                       // Context object 
                &result->m_wbem_services // pointer to IWbemServices proxy
            );

            if (FAILED(hres))
            {
                log.error(fmt::format( "Could not connect. Error code = {}.", hresult_to_hex_str(hres)));

                result->m_wbem_location->Release();
                delete result;
                return nullptr;
            }

            log.info("connected to ROOT\\CIMV2 WMI namespace");

            // Step 5: --------------------------------------------------
            // Set security levels on the proxy -------------------------

            hres = CoSetProxyBlanket(
                result->m_wbem_services,                        // Indicates the proxy to set
                RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
                RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
                NULL,                        // Server principal name 
                RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
                RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
                NULL,                        // client identity
                EOAC_NONE                    // proxy capabilities 
            );

            if (FAILED(hres))
            {
                log.error(fmt::format("Could not set proxy blanket. Error code = {}.", hresult_to_hex_str(hres)));

                result->m_wbem_services->Release();
                result->m_wbem_location->Release();
                delete result;
                return nullptr;
            }

            return result;
        }
    };

    namespace 
    {
        bool initialize_windows_com_library_access()
        {
            whatlog::logger log("initialize_windows_com_library_access");
            // initialize the COM library for use by the calling thread
            HRESULT hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
            if (FAILED(hres))
            {
                log.error(fmt::format("Failed to initialize COM library. Error code = {}.", hresult_to_hex_str(hres)));

                return false; // Program has failed.
            }

            // set the access permissions that a server will use to receive calls.
            hres = CoInitializeSecurity(
                NULL,
                -1,                          // COM authentication
                NULL,                        // Authentication services
                NULL,                        // Reserved
                RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
                RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
                NULL,                        // Authentication info
                EOAC_NONE,                   // Additional capabilities 
                NULL                         // Reserved
            );

            if (FAILED(hres))
            {
                std::string error_message = "Failed to initialize security. Error code = " + hresult_to_hex_str(hres);
                log.error(error_message);

                CoUninitialize();
                return false; // Program has failed.
            }

            return true;
        }
    } // !anonymouse namespace

    wbem_consumer::wbem_consumer(wbem_consumer::impl* impl)
        : m_impl(impl)
    {
        // nothing for now
    }
    
    shared_wbem_consumer wbem_consumer::get_consumer()
    {
        static bool com_initialized = initialize_windows_com_library_access();
        if (com_initialized)
        {
            wbem_consumer::impl* impl = wbem_consumer::impl::create_wbem_impl();
            if (impl != nullptr)
            {
                static shared_wbem_consumer consumer(new wbem_consumer(impl));
                return consumer;
            }
        }

        return shared_wbem_consumer(nullptr);
    }

    IEnumWbemClassObject* wbem_consumer::exec_query(const std::wstring& query)
    {
        whatlog::logger log("wbem_consumer::exec_query");
        IEnumWbemClassObject* pEnumerator = nullptr;
        HRESULT hres = m_impl->m_wbem_services->ExecQuery(bstr_t("WQL"), bstr_t(query.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

        if (FAILED(hres))
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
            std::string query_str = converter.to_bytes(query);
            log.error(fmt::format("Query {} failed. Error code = {}.", query_str, hresult_to_hex_str(hres)));
        }

        return pEnumerator;
    }
} // !namespace net
} // !namespace noconn