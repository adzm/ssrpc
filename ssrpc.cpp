#include "stdafx.h"

#include "ssrp.hpp"
#include "util.hpp"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

using std::vector;

using std::shared_ptr;
using std::make_shared;

using std::error_code;

using std::cout;
using std::clog;
using std::cerr;
using std::endl;

using namespace std::chrono_literals;

void print_response(std::ostream& out, ssrp::wire_response& response, asio::ip::udp::endpoint sender, std::chrono::nanoseconds elapsed)
{
	if (response.type != 0x05) {
		// warn but continue
	}

	if (response.len > 4096) {
		// should not be possible really but truncate just in case
		response.len = 4096;
	}

	auto responses = parse(response);

	for (auto&& r : responses) {
		if (r.instance.empty()) {
			out << r.server << endl;
		}
		else {
			out << r.server << "\\" << r.instance << endl;
		}

		out << "  ver: " << r.version << endl;

		if (r.clustered == "Yes") {
			out << "  Clustered" << endl;
		}
		if (!r.np.empty()) {

			out << "  np:  " << r.np << endl;
		}
		if (!r.tcp.empty()) {
			out << "  tcp: " << r.tcp << endl;
		}
		out << "  ip:  " << sender.address() << endl;

		for (auto&& p : r.props) {
			out << "  " << p.key.c_str() << ":\t" << p.value << endl;
		}
	}

	// clutters output with ping
	/*
	if (elapsed > 5ms) {
		out << " ping: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << "ms" << endl;
	}
	*/

	out << endl;
}

struct query
{
	asio::io_service& io_service;
	asio::ip::udp::socket sock;

	asio::ip::udp::endpoint destination;
	ssrp::wire_request request;

	asio::ip::udp::endpoint sender;
	ssrp::wire_response response;

	std::chrono::high_resolution_clock::time_point time_sent;

	size_t response_count = 0;

	query(asio::io_service& io_service, asio::ip::address addr)
		: io_service(io_service)
		, destination(addr, 1434)
		, sock(io_service)
	{
		sock.open(destination.protocol());
	}

	void init_broadcast()
	{
		sock.set_option(asio::ip::udp::socket::broadcast(true));
		request.type = 0x02;
	}

	void begin_query()
	{
		time_sent = std::chrono::high_resolution_clock::now();
		sock.async_send_to(request_buffer(), destination, [this](const error_code& ec, size_t bytes_transferred) {
			if (ec) {
				if (ec != asio::error::operation_aborted) {
					cerr << ec << " " << ec.message() << endl;
				}
				return;
			}

			begin_receive_from();
		});
	}

	void begin_receive_from()
	{
		sock.async_receive_from(response_buffer(), sender, [this](const error_code& ec, size_t bytes_transferred) {
			
			auto elapsed = std::chrono::high_resolution_clock::now() - time_sent;

			if (ec) {
				if (ec != asio::error::operation_aborted) {
					cerr << ec << " " << ec.message() << endl;
				}
				return;
			}

			++response_count;
			print_response(cout, response, sender, elapsed);

			begin_receive_from();
		});
	}

protected:
	asio::mutable_buffers_1 response_buffer()
	{
		return asio::buffer(&response, sizeof(response));
	}

	asio::mutable_buffers_1 request_buffer()
	{
		return asio::buffer(&request, sizeof(request));
	}
};

int run(asio::io_service& io_service, vector<shared_ptr<query>> queries, std::chrono::milliseconds deadline)
{
	asio::steady_timer timer(io_service);
	asio::steady_timer crickets(io_service);

	for (auto&& q : queries) {
		q->begin_query();
		cerr << "  packet sent to " << q->destination << endl;
	}

	timer.expires_from_now(deadline);
	crickets.expires_from_now(deadline / 2);

	crickets.async_wait([&queries](const error_code& ec) {
		for (auto&& q : queries) {
			if (q->response_count > 0) {
				return;
			}
		}
		cerr << "  (crickets...)" << endl;
	});

	size_t response_count = 0;
	timer.async_wait([&queries, &response_count](const error_code& ec) {
		for (auto&& q : queries) {
			response_count += q->response_count;
			error_code meh;
			q->sock.close(meh);
		}
	});

	cerr << "Listening..." << endl;
	cerr << endl;

	io_service.run();

	if (response_count > 0) {
		return 0;
	} else {
		return E_FAIL;
	}
}

int broadcast()
{
	asio::io_service io_service;

	vector<shared_ptr<query>> queries;

	cerr << "Detecting network interfaces..." << endl;
	auto addresses = util::calculate_broadcast_addresses();
	
	for (auto&& addr : addresses) {
		auto q = make_shared<query>(io_service, addr);

		q->init_broadcast();

		queries.push_back(q);
	}

	cerr << "Broadcasting..." << endl;
	return run(io_service, queries, 2s);
}

int unicast(const char* hostname)
{
	asio::io_service io_service;

	vector<shared_ptr<query>> queries;

	cerr << "Resolving " << hostname << "..." << endl;
	auto addresses = util::resolve_hostname(io_service, hostname);
	
	for (auto&& addr : addresses) {
		auto q = make_shared<query>(io_service, addr);

		queries.push_back(q);
	}

	cerr << "Querying..." << endl;
	return run(io_service, queries, 1s);
}

int fail_usage()
{
	cerr << endl << "Syntax was incorrect or unexpected!" << endl << endl
		<< "Usage: " << endl
		<< "  ssrpc servername" << endl
		<< "  ssrpc 192.168.1.1" << endl
		<< "  ssrpc" << endl
		<< endl;

	return E_INVALIDARG;
}

int main(int argc, const char* argv[])
{
	cerr << 
		"ssrpc 1.0.0 - SSRP (SQL Server Resolution Protocol) Client\n"
		"  usage: ssrpc [hostname|ip]\n" << endl;
	

	if (argc > 2) {
		return fail_usage();
	}

	// gather settings
	const char* hostname = nullptr;

	if (argc > 1) {
		hostname = argv[1];
	}


	// go!
	int ret = 0;

	try {
		if (hostname) {
			ret = unicast(hostname);
		}
		else {
			ret = broadcast();
		}
	}
	catch (std::exception& e) {
		ret = E_FAIL;
		cerr << endl << "Exception: " << e.what() << endl;
	}
	catch (...) {
		ret = E_FAIL;
		cerr << endl << "Exception: " << "unknown" << endl;
	}

	if (!ret) {
		cerr << endl << "Everything's shiny cap'n!" << endl;
	}

	return ret;
}

