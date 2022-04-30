/*
 *
 */

#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace noconn
{
namespace rest
{
	class connection;
	using shared_connection = std::shared_ptr<connection>;

	class server;
	using shared_server = std::shared_ptr<server>;

	class connection : public std::enable_shared_from_this <connection>
	{
	public:
		static shared_connection create(boost::asio::ip::tcp::socket&& socket, shared_server server);
		~connection();

		void open();
		void close();

		uint32_t id() const;
		
		void read();
		void on_read(boost::beast::error_code error_code, std::size_t bytes_transferred);
		void on_write(bool close_connection, boost::beast::error_code error_code, std::size_t bytes_transferred);
	protected:
		connection(boost::asio::ip::tcp::socket&& socket, uint32_t m_session_id, shared_server server);
	protected:
		uint32_t m_id;
		boost::beast::tcp_stream m_stream;
		boost::beast::http::request<boost::beast::http::string_body> m_request;
		boost::beast::http::response<boost::beast::http::string_body> m_response;
        boost::beast::flat_buffer m_buffer;
		shared_server m_server;
	};
} // !namespace rest
} // !namespace noconn