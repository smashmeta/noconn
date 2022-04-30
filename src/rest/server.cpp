/*
 *
 */

#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include "noconn/rest/helper.hpp"
#include "noconn/rest/server.hpp"

namespace noconn
{
namespace rest
{
	shared_server server::create(std::shared_ptr<boost::asio::io_context> io_context)
	{
		return shared_server(new server(io_context));
	}

	server::server(std::shared_ptr<boost::asio::io_context> io_context)
		:	m_io_context(io_context),
			m_signals(*io_context)
	{
		whatlog::logger log("server::ctr()");
		log.info("server created.");

		m_signals.add(SIGINT);
		m_signals.add(SIGTERM);
	}

	bool server::open(boost::asio::ip::address address, ip_port port)
	{
		whatlog::logger log("server::open");
		std::lock_guard<std::mutex> lock(m_mutex);

		boost::asio::ip::tcp::endpoint local_endpoint(address, port);
		log.info(fmt::format("new acceptor on local endpoint {} created.", noconn::rest::to_string(local_endpoint)));
		
		
		boost::beast::error_code error_code;

		m_acceptor.reset(new boost::asio::ip::tcp::acceptor(m_io_context->get_executor()));

		m_acceptor->open(local_endpoint.protocol(), error_code);
		if (error_code)
		{
			log.error(fmt::format("failed to open acceptor. message: {}.", error_code.message()));
			return false;
		}

		m_acceptor->set_option(boost::asio::socket_base::reuse_address(true), error_code);
		if (error_code)
		{
			log.error(fmt::format("failed to set option (reuse address) on acceptor. message: {}.", error_code.message()));
			return false;
		}

		m_acceptor->bind(local_endpoint, error_code);
		if (error_code)
		{
			log.error(fmt::format("failed to bind acceptor to endpoint {}. message: {}.", local_endpoint.address().to_string(), error_code.message()));
			return false;
		}

		m_acceptor->listen(boost::asio::socket_base::max_listen_connections, error_code);
		if (error_code)
		{
			log.error(fmt::format("acceptor failed to start listening. message: {}.", error_code.message()));
			return false;
		}

		do_accept();

		return true;
	}

	void server::close()
	{
		// todo: shutdown open connections
	}

	void server::close(shared_connection connection)
	{
		whatlog::logger log("server::close(connection)");
		auto itr_connection = std::find_if(m_connections.begin(), m_connections.end(), [&](const shared_connection& temp) { return temp->id() == connection->id(); });
		if (itr_connection != m_connections.end())
		{
			log.info(fmt::format("terminating connection {}.", connection->id()));
			// m_connections.erase(itr_connection);
		}
		else
		{
			log.warning(fmt::format("failed to find connection {}.", connection->id()));
		}
	}

	void server::do_accept()
	{
		whatlog::logger log("server::do_accept");
		log.info("now accepting requests.");

		m_acceptor->async_accept(
			boost::asio::make_strand(m_io_context->get_executor()), 
			boost::beast::bind_front_handler(&server::handle_accept, shared_from_this())
		);
	}

	void server::handle_accept(boost::beast::error_code error_code, boost::asio::ip::tcp::socket socket)
	{
		whatlog::logger log("server::handle_accept");
		log.info(fmt::format("accepting connection on socket {}.", to_string(socket)));

		shared_connection connection = connection::create(std::move(socket), shared_from_this());
		m_connections.emplace_back(connection);
		
		connection->open();

		do_accept();
	}

	void server::handle_exit_signal(const boost::system::error_code& error, int signal)
	{
		whatlog::logger log("server::handle_exit_signal");
		log.info(fmt::format("received system exit signal. Code: {}, message: {}.", signal, error.what()));

		close();
	}
} // !namespace rest
} // !namespace noconn