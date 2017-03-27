#pragma once

namespace util
{
	using std::string;
	using std::vector;

	using std::unique_ptr;
	using std::make_unique;

	using std::error_code;

	using std::cerr;
	using std::endl;

	// on windows if you bind to 0.0.0.0 for broadcast it seems to choose just one adapter
	// to broadcast on multiple adapters we need the individual addresses so we can bind
	// a socket for each.
	vector<asio::ip::address_v4> calculate_broadcast_addresses()
	{
		vector<asio::ip::address_v4> addrs;

		ULONG buffer_size = 8192;
		unique_ptr<BYTE[]> buf;

		ULONG ret = 0;
		while (ERROR_BUFFER_OVERFLOW == (ret = GetAdaptersInfo((IP_ADAPTER_INFO*)buf.get(), &buffer_size)))
		{
			buf = make_unique<BYTE[]>(buffer_size);
		}

		if (ERROR_SUCCESS != ret) {
			buf.reset();
		}

		for (
			IP_ADAPTER_INFO* pAdapter = (IP_ADAPTER_INFO*)buf.get();
			pAdapter != nullptr;
			pAdapter = pAdapter->Next
			)
		{
			auto ip = asio::ip::address_v4::from_string(pAdapter->IpAddressList.IpAddress.String);
			auto mask = asio::ip::address_v4::from_string(pAdapter->IpAddressList.IpMask.String);
			auto broadcast_ip = asio::ip::address_v4::broadcast(ip, mask); // asio makes this easy!

			if (broadcast_ip == asio::ip::address_v4::broadcast()) {
				//cerr << "  ignore global broadcast address " << broadcast_ip << endl;
			}
			else if (addrs.end() != find(addrs.begin(), addrs.end(), broadcast_ip)) {
				//cerr << "  ignore duplicate broadcast address " << broadcast_ip << endl;
			}
			else {
				cerr << "  interface " << ip << " | ~" << mask << " = " << broadcast_ip << endl;
				addrs.push_back(broadcast_ip);
			}
		}

		if (addrs.empty()) {
			cerr << "  global (broadcast:" << asio::ip::address_v4::broadcast() << ")" << endl;
			addrs.push_back(asio::ip::address_v4::broadcast());
		}

		return addrs;
	}
	
	vector<asio::ip::address> resolve_hostname(asio::io_service& io_service, const char* hostname)
	{
		vector<asio::ip::address> addrs;

		// if it is already numeric, just use that
		{
			error_code ec;

			auto addr = asio::ip::address::from_string(hostname, ec);
			if (!ec) {
				addrs.push_back(addr);
				return addrs;
			}
		}

		asio::ip::tcp::resolver resolver(io_service);

		asio::ip::address best_addr;
		asio::ip::address first_addr;

		auto query = asio::ip::tcp::resolver::query(hostname, "1434");
		for (auto iter = resolver.resolve(query); iter != asio::ip::tcp::resolver::iterator(); ++iter) {

			auto endpoint = iter->endpoint();

			cerr << "  " << endpoint << endl;

			addrs.push_back(endpoint.address());
		}

		return addrs;
	}
}

