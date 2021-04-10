/*
 *
 */

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <fmt/core.h>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>

#include "noconn/network_adapter.hpp"
#include "noconn/wbem_consumer.hpp"
#include "whatlog/logger.hpp"

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
} // !namespace noconn

int main(int argument_count, char** arguments)
{
    QApplication qt_application(argument_count, arguments);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    whatlog::rename_thread(GetCurrentThread(), "main");
    if (!noconn::initialize_console_logger())
    {
        return EXIT_FAILURE;
    }

    boost::filesystem::path cwd = boost::filesystem::current_path();
    std::cout << "current path is set to " << cwd << std::endl;
    whatlog::logger::initialize_file_logger(cwd, "noconn.log");
    whatlog::logger log("main");

    boost::asio::io_context io_context(1);

    noconn::shared_wbem_consumer consumer = noconn::wbem_consumer::get_consumer();
    if (!consumer)
    {
        log.error("failed to create wbem consumer!");
        return EXIT_FAILURE;
    }
    
    std::vector<noconn::network_adapter> adapters = noconn::get_network_adapters(consumer);
    for (const noconn::network_adapter& adapter : adapters)
    {
        std::string adapter_info_txt = fmt::format("name: {}, index: {}, enabled: {}, guid: {}.", adapter.m_name, adapter.m_adapter_index, adapter.m_enabled, adapter.m_guid);
        log.info(adapter_info_txt);
    }

    noconn::list_adapter_ip_addresses();
    noconn::print_routing_table();

    // QQmlApplicationEngine engine;
    // QQuickStyle::setStyle("Material");
    // const QUrl url(QStringLiteral("qrc:/noconn.qml"));
// 
    // QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
	// 	&qt_application, [url](QObject* obj, const QUrl& objUrl) {
	// 		if (!obj && url == objUrl)
	// 			QCoreApplication::exit(-1);
	// 	}, Qt::QueuedConnection);
// 
    // engine.load(url);
// 
    // log.info("Qt application started.");
// 
    // return qt_application.exec();

    return EXIT_SUCCESS;
}