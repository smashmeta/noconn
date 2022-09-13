/*
 *
 */

#include <iostream>
#include <string>
#include <optional>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/dll.hpp>
#include <boost/signals2.hpp>
#include <boost/json.hpp>
#include <fmt/core.h>
#include <whatlog/logger.hpp>
#include "noconn/net/adapter_manager.hpp"
#include "noconn/net/wbem_consumer.hpp"
#include "noconn/net/route_manager.hpp"
#include "noconn/rest/server.hpp"


namespace noconn
{
    struct add_route_arg
    {
        std::string m_address;
        std::string m_mask;
    };

    struct rest_method
    {
        enum class type
        {
            get,
            post,
            put,
            del
        };

        using method_result_t = std::optional<type>;

        static method_result_t validate(const std::string& method)
        {
            method_result_t result;

            std::string method_lower_case = method; 
            boost::algorithm::to_lower(method_lower_case);

            if (method_lower_case == "get")
            {
                result = type::get;
            }
            else if (method_lower_case == "post")
            {
                result = type::post;
            }
            else if (method_lower_case == "put")
            {
                result = type::put;
            }
            else if (method_lower_case == "delete")
            {
                result = type::del;
            }

            return result;
        }
    };
    

    struct response
    {
        const static response server_error; 
        const static response invalid_path;
        const static response bad_request;
        const static response invalid_rest_method;

        response(int code, const std::string& message)
            : m_code(code), m_message(message) 
        {
            // nothing for now
        }

        int m_code;
        std::string m_message;
    };

    const response response::server_error = { 500, "server failed to handle request." };
    const response response::invalid_path = { 404, "service requested not found." };
    const response response::bad_request = { 400, "invalid json." };
    const response response::invalid_rest_method = { 500, "invalid rest method." };

    struct json_validator
    {
        enum json_response
        {
            invalid,
            valid,
            empty
        };

        using optional_json_t = std::optional<boost::json::value>;
        using json_validator_response_t = std::pair<json_response, optional_json_t>;

        json_validator_response_t validate(const std::string& message)
        {
            whatlog::logger log("json_validator::validate");
            auto result = std::make_pair<json_response, optional_json_t>(json_response::invalid, {});

            if (!message.empty())
            {
                try
                {
                    auto json = boost::json::parse(message);
                    result.first = json_response::valid;
                    result.second = json;

                }
                catch (const std::exception& ex)
                {
                    // todo: log error message
                    log.error(fmt::format("failed to parse message. error: {}.", ex.what()));
                }
            }
            else
            {
                result.first = json_response::empty;
                result.second = boost::json::parse("{}");
            }
            

            return result;
        }
    };

    struct path_validator
    {
        enum path_response
        {
            invalid_path,
            route_path
        };

        std::array<std::string, 2> valid_paths_str{"my_page/invalid", "my_page/inet/route"};
        std::array<path_response, 2> valid_paths_enum{path_response::invalid_path, path_response::route_path};

        path_response validate(const std::string& path)
        {
            std::string path_lower_key = path;
            std::transform(path_lower_key.begin(), path_lower_key.end(), path_lower_key.begin(),
                [](unsigned char c) { return std::tolower(c); });

            if (path_lower_key == valid_paths_str[path_response::route_path])
            {
                return path_response::route_path;
            }

            return path_response::invalid_path;
        }
    };

    struct req_handler_route
    {
        response handle(const boost::json::value& message)
        {
            response result(100, "temp");
            return result;
        }
    };

    struct request_handler
    {
        response handle(const std::string& path, const std::string& method, const std::string& message)
        {
            whatlog::logger log("test_connection::handle_data");

            // validate method
            rest_method::method_result_t method_t = rest_method::validate(method);
            if (!method_t.has_value())
            {
                return response::invalid_rest_method;
            }

            // validate service path
            response result = response::server_error;
            path_validator::path_response path_result = m_path_validator.validate(path);
            if (path_result != path_validator::path_response::invalid_path)
            {
                // validate json
                json_validator::json_validator_response_t json_result = m_json_validator.validate(message);
                if (json_result.first != json_validator::json_response::invalid)
                {
                    // validate request
                    switch (path_result)
                    {
                    case path_validator::path_response::route_path:
                        result = m_req_handler_route.handle(json_result.second.value());
                        break;
                    }
                }
                else
                {
                    // invalid json
                    result = response::bad_request;
                }
            }
            else
            {
                // invalid service path
                result = response::invalid_path;
            }

            return result;

            // todo? should the request handler communicate directly with the route handler?
            // or should the action be passed to the main thread which then does the work?
            // but then how will we respond to the request?
        }

        path_validator m_path_validator;
        json_validator m_json_validator;
        req_handler_route m_req_handler_route;
    };


    struct route_monitor
    {
        boost::signals2::signal<void(const add_route_arg&)> m_sig_added;
        boost::signals2::signal<void(const add_route_arg&)> m_sig_removed;
        boost::signals2::signal<void(const add_route_arg&)> m_sig_changed;

        void update()
        {
            if (true)
            {
                add_route_arg arg;
                m_sig_added(arg);
            }
        }
    };

    struct test_route_handler
    {
        void add_route(const add_route_arg& arg)
        {
            whatlog::logger log("test_route_handler::add_route");
            log.info(fmt::format("adding new route {{ address: {}, mask: {} }}.", arg.m_address, arg.m_mask));
        }
        
        void remove_route()
        {

        }

        void change_route()
        {

        }
    };

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
        noconn::net::shared_wbem_consumer consumer = noconn::net::wbem_consumer::get_consumer();
        if (!consumer)
        {
            log.error("failed to create wbem consumer!");
            return;
        }

        route_monitor route_monitor;
        test_route_handler route_handler;
        
        route_monitor.m_sig_added.connect(std::bind(&test_route_handler::add_route, &route_handler, std::placeholders::_1));

        route_monitor.update();

        // std::vector<noconn::network_adapter> adapters = noconn::get_network_adapters(consumer);
        // for (const noconn::network_adapter& adapter : adapters)
        // {
        //     std::string adapter_info_txt = fmt::format("name: {}, index: {}, enabled: {}, guid: {}.", adapter.m_name, adapter.m_adapter_index, adapter.m_enabled, adapter.m_guid);
        //     log.info(adapter_info_txt);
        // }

        // noconn::list_adapter_ip_addresses();

        // std::vector<noconn::net::route_entry> routes = noconn::net::list_routing_table();
        // log.info(fmt::format("{:20} {:20} {:20} {:10} {:10}", "destination", "mask", "gateway", "interface", "metric"));
        // for (const noconn::net::route_entry& route : routes)
        // {
        //     std::string route_entry_txt = fmt::format("{:20} {:20} {:20} {:<10} {:<10}", route.m_destination, route.m_mask, route.m_gateway, route.m_interface_index, route.m_metric);
        //     log.info(route_entry_txt);
        // }

        // CreateIpForwardEntry (MIB_IPFORWARDROW)
    }

    void work_handler(const std::string& thread_name, std::shared_ptr<boost::asio::io_context> io_context)
    {
        whatlog::rename_thread(GetCurrentThread(), thread_name);
        whatlog::logger log("work_handler");
        log.info(fmt::format("starting thread {}.", thread_name));

        temporary();

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

    noconn::request_handler req_handler;
    req_handler.handle("my_page/inet/route", "get", "");

    // Invoke-RestMethod -Uri 'http://192.168.0.13:3031/test' -Method GET
    const auto address = boost::asio::ip::make_address("192.168.0.13");
    const unsigned short port = 3031;
    const auto document_root = std::make_shared<std::string>(".");
    
    std::vector<std::string> thread_names = { "mercury", "venus", "earth", "mars", "jupiter" };
    const int worker_threads_count = thread_names.size();

    auto io_context = std::make_shared<boost::asio::io_context>(worker_threads_count);
    auto work = std::make_shared<boost::asio::io_context::work>(*io_context);
    
    boost::thread_group worker_threads;
    for (size_t i = 0; i < worker_threads_count; ++i)
    {
        log.info("spawning worker_thread {" + thread_names[i] + "} for boost asio.");
        worker_threads.create_thread(boost::bind(&noconn::work_handler, thread_names[i], io_context));
    }

    auto server = noconn::rest::server::create(io_context);
    server->open(address, noconn::rest::ip_port(port));

    noconn::net::route_manager route_mgr;

    while (true)
    {
        route_mgr.tick();
        // never ending loop
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
    }

    return EXIT_SUCCESS;
}