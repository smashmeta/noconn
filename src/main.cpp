/*
 *
 */

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>
#include <fmt/core.h>
#include <whatlog/logger.hpp>
#include "noconn/network_adapter.hpp"
#include "noconn/wbem_consumer.hpp"
#include "noconn/rest/server.hpp"


namespace noconn
{
    bool initialize_console_logger()
	{
		try
		{
            whatlog::logger::initialize_console_logger();
		}
		catch (const std::exception& ex)
		{
			std::cout << fmt::format("failed to initialize console logger. exception: {}", ex.what()) << std::endl;
			return false;
		}
		catch (...)
		{
			std::cout << "failed to initialize console logger. unknown exception! " << std::endl;
			return false;
		}

		return true;
	}

    void temporary()
    {
        whatlog::logger log("*::temporary");
        noconn::shared_wbem_consumer consumer = noconn::wbem_consumer::get_consumer();
        if (!consumer)
        {
            log.error("failed to create wbem consumer!");
            return;
        }

        std::vector<noconn::network_adapter> adapters = noconn::get_network_adapters(consumer);
        for (const noconn::network_adapter& adapter : adapters)
        {
            std::string adapter_info_txt = fmt::format("name: {}, index: {}, enabled: {}, guid: {}.", adapter.m_name, adapter.m_adapter_index, adapter.m_enabled, adapter.m_guid);
            log.info(adapter_info_txt);
        }

        // noconn::list_adapter_ip_addresses();

        std::vector<noconn::route_entry> routes = noconn::list_routing_table();
        log.info(fmt::format("{:20} {:20} {:20} {:10} {:10}", "destination", "mask", "gateway", "interface", "metric"));
        for (const noconn::route_entry& route : routes)
        {
            std::string route_entry_txt = fmt::format("{:20} {:20} {:20} {:<10} {:<10}", route.m_destination, route.m_mask, route.m_gateway, route.m_interface_index, route.m_metric);
            log.info(route_entry_txt);
        }

        // CreateIpForwardEntry (MIB_IPFORWARDROW)
    }

    void work_handler(const std::string& thread_name, std::shared_ptr<boost::asio::io_context> io_context)
    {
        whatlog::rename_thread(GetCurrentThread(), thread_name);
        whatlog::logger log("work_handler");
        log.info(fmt::format("starting thread {}.", thread_name));

        for (;;)
        {
            try
            {
                boost::system::error_code error_code;
                io_context->run(error_code);

                if (error_code)
                {
                    log.error(fmt::format("worker thread encountered an error. message: ", error_code.message()));
                }

                break;
            }
            catch (std::exception& ex)
            {
                log.warning(fmt::format("worker thread encountered an error. exception: {}.", std::string(ex.what())));
            }
            catch (...)
            {
                log.warning("worker thread encountered an unknown exception.");
            }
        }
    }
} // !namespace noconn

int main(int argument_count, char** arguments)
{
    whatlog::rename_thread(GetCurrentThread(), "main");
    if (!noconn::initialize_console_logger())
    {
        return EXIT_FAILURE;
    }

    boost::filesystem::path executable_directory = boost::dll::program_location().parent_path();
    std::cout << "current path is set to " << executable_directory << std::endl;
    whatlog::logger::initialize_file_logger(executable_directory, "noconn.log");
    whatlog::logger log("main");

    // Invoke-RestMethod -Uri 'http://192.168.0.13:3031/test' -Method GET
    const auto address = boost::asio::ip::make_address("127.0.0.1");
    const unsigned short port = 3031;
    const auto document_root = std::make_shared<std::string>(".");
    
    std::vector<std::string> thread_names = { "mercury", "venus", "earth", "mars", "jupiter" };
    const size_t worker_threads_count = thread_names.size();

    auto io_context = std::shared_ptr<boost::asio::io_context>(new boost::asio::io_context(worker_threads_count));
    auto work = std::make_shared<boost::asio::io_context::work>(*io_context);
    
    boost::thread_group worker_threads;
    for (size_t i = 0; i < worker_threads_count; ++i)
    {
        log.info("spawning worker_thread {" + thread_names[i] + "} for boost asio.");
        worker_threads.create_thread(boost::bind(&noconn::work_handler, thread_names[i], io_context));
    }

    auto server = noconn::rest::server::create(io_context);
    server->open(address, noconn::rest::ip_port(port));

    while (true)
    {
        // never ending loop
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
    }

    return EXIT_SUCCESS;
}