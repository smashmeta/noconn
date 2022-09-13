/*
 *
 */

#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include <boost/json.hpp>
#include "noconn/rest/helper.hpp"
#include "noconn/rest/server.hpp"
#include "noconn/rest/connection.hpp"

namespace noconn
{
namespace rest
{
namespace
{
	boost::beast::string_view mime_type(boost::beast::string_view path)
	{
		using boost::beast::iequals;
		auto const ext = [&path]
		{
			auto const pos = path.rfind(".");
			if (pos == boost::beast::string_view::npos)
				return boost::beast::string_view{};
			return path.substr(pos);
		}();

		if (iequals(ext, ".htm"))  return "text/html";
		if (iequals(ext, ".html")) return "text/html";
		if (iequals(ext, ".php"))  return "text/html";
		if (iequals(ext, ".css"))  return "text/css";
		if (iequals(ext, ".txt"))  return "text/plain";
		if (iequals(ext, ".js"))   return "application/javascript";
		if (iequals(ext, ".json")) return "application/json";
		if (iequals(ext, ".xml"))  return "application/xml";
		if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
		if (iequals(ext, ".flv"))  return "video/x-flv";
		if (iequals(ext, ".png"))  return "image/png";
		if (iequals(ext, ".jpe"))  return "image/jpeg";
		if (iequals(ext, ".jpeg")) return "image/jpeg";
		if (iequals(ext, ".jpg"))  return "image/jpeg";
		if (iequals(ext, ".gif"))  return "image/gif";
		if (iequals(ext, ".bmp"))  return "image/bmp";
		if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
		if (iequals(ext, ".tiff")) return "image/tiff";
		if (iequals(ext, ".tif"))  return "image/tiff";
		if (iequals(ext, ".svg"))  return "image/svg+xml";
		if (iequals(ext, ".svgz")) return "image/svg+xml";
		return "application/text";
	}

	boost::json::value route_entry(const std::string& destination, const std::string& mask, const std::string& gateway, const std::string& adapter, int metric)
	{
		boost::json::value json_value { {"destination", destination}, {"mask", mask}, {"gateway", gateway}, {"interface", adapter}, {"metric", metric} };
		return json_value;
	}
} // !anonymous namespace

	connection::connection(boost::asio::ip::tcp::socket&& socket, uint32_t session_id, shared_server server)
		:	m_stream(std::move(socket)), m_id(session_id), m_server(server)
	{
		whatlog::logger log("connection::ctor()");
		log.info(fmt::format("{} [CREATED] on socket {}.", m_id, to_string(m_stream.socket())));
		// nothing for now
	}

	connection::~connection()
	{
		whatlog::logger log("connection::ctor()");
		log.info(fmt::format("{} [TERMINATED] connection.", m_id));
	}

	shared_connection connection::create(boost::asio::ip::tcp::socket&& socket, shared_server server)
	{
		static uint32_t session_id = 0;
		return shared_connection(new connection(std::move(socket), session_id++, server));
	}

	void connection::open()
	{
		read();
	}

	void connection::read()
	{
		// clear previous request
		m_request = {};

		m_stream.expires_after(std::chrono::seconds(30));

		boost::beast::http::async_read(m_stream, m_buffer, m_request,
			boost::beast::bind_front_handler(&connection::on_read, shared_from_this())
		);
	}

	void connection::on_read(boost::beast::error_code error_code, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);
		whatlog::logger log("connection::on_read");

		if (error_code == boost::beast::http::error::end_of_stream ||
			error_code == boost::asio::error::connection_reset ||
			error_code == boost::asio::error::eof ||
			error_code == boost::asio::error::timed_out)
		{
			log.info(fmt::format("{} [CLOSED] by remote endpoint.", m_id));

			// connection closed by sender
			close();
			return;
		}

		if (error_code)
		{
			log.error(fmt::format("{} [ERROR] during read. message: {}.", m_id, error_code.message()));
			close();
			return;
		}

		{
			std::string target = m_request.target().to_string();
			std::string method = m_request.method_string().to_string();
			std::string body = m_request.body();
			log.info(fmt::format("{} [REQUEST] target: {}, method: {}, body: {}.", m_id, target, method, body));
		}

		// Make sure we can handle the method
		if (m_request.method() != boost::beast::http::verb::get && 
			m_request.method() != boost::beast::http::verb::head)
		{
			m_response = { boost::beast::http::status::bad_request, m_request.version() };
			m_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			m_response.set(boost::beast::http::field::content_type, "text/html");
			m_response.keep_alive(false);
			m_response.body() = std::string("Unknown HTTP-method");
			m_response.prepare_payload();

			boost::beast::http::async_write(
				m_stream, m_response,
				boost::beast::bind_front_handler(
					&connection::on_write,
					shared_from_this(),
					m_response.need_eof()
				)
			);

			return;
		}

		boost::json::object json;
		json["entry_0"] = route_entry("0.0.0.0", "0.0.0.0", "192.168.0.1", "192.168.0.13", 55);
		json["entry_1"] = route_entry("127.0.0.0", "255.0.0.0", "0.0.0.0", "127.0.0.1", 331);
		json["entry_2"] = route_entry("127.0.0.1", "255.255.255.255", "0.0.0.0", "127.0.0.1", 331);
		std::string json_routes = boost::json::serialize(json);
		size_t json_routes_size = json_routes.size();

		m_response = { std::piecewise_construct, std::make_tuple(std::move(json_routes)), std::make_tuple(boost::beast::http::status::ok, m_request.version()) };
		m_response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		m_response.set(boost::beast::http::field::content_type, "application/json");
		m_response.content_length(json_routes_size);
		m_response.keep_alive(m_request.keep_alive());

		{
			std::string body = m_response.body();
			log.info(fmt::format("{} [RESPONSE] body: {}.", m_id, body));
		}

		boost::beast::http::async_write(
			m_stream, m_response,
			boost::beast::bind_front_handler(
				&connection::on_write,
				shared_from_this(),
				m_response.need_eof()
			)
		);

		read();
	}

	void connection::on_write(bool close_connection, boost::beast::error_code error_code, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);
		whatlog::logger log("connection::on_write");

		if (error_code)
		{
			log.error(fmt::format("{} [FAILED] write. message: {}.", m_id, error_code.message()));
		}

		if (close_connection)
		{
			close();
			return;
		}
	}

	void connection::close()
	{
		boost::beast::error_code error_code;
		whatlog::logger log("connection::close");

		log.info(fmt::format("{} [DISCONNECTED].", m_id));
		m_stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, error_code);

		if (error_code)
		{
			log.error(fmt::format("{} [ERROR] during shutdown of session. message: {}.", m_id, error_code.message()));
		}

		m_server->close(shared_from_this());
	}

	uint32_t connection::id() const
	{
		return m_id;
	}
} // !namespace rest
} // !namespace noconn
