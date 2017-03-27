#pragma once

namespace ssrp
{
	using std::string;
	using std::vector;
	
	// see [MC-SQLR] reference: https://msdn.microsoft.com/en-us/library/cc219703.aspx

	// packet type constants
	constexpr uint8_t CLNT_BCAST_EX = 0x02;		// broadcast
	constexpr uint8_t CLNT_UCAST_EX = 0x03;		// unicast

#pragma pack(push, 1)
	struct wire_request
	{
		uint8_t type = 0x03;
	};
#pragma pack(pop)

#pragma pack(push, 1)
	struct wire_response
	{
		uint8_t type = 0;		// must be 0x05;
		uint16_t len = 0;
		char data[4096] = { 0 };	// data[len], semicolon-delimited key-value pairs.
	};
#pragma pack(pop)

	/* sample response for multiple instances (newlines added for clarity)

	0x5
	len
	ServerName;BLUE;InstanceName;SQL2008;IsClustered;No;Version;10.50.6000.34;tcp;51297;np;\\BLUE\pipe\MSSQL$SQL2008\sql\query;;
	ServerName;BLUE;InstanceName;SQL2012STD;IsClustered;No;Version;11.0.5058.0;tcp;1534;np;\\BLUE\pipe\MSSQL$SQL2012STD\sql\query;;
	ServerName;BLUE;InstanceName;SQL2014STD;IsClustered;No;Version;12.0.4100.1;tcp;7317;np;\\BLUE\pipe\MSSQL$SQL2014STD\sql\query;;

	CLNT_UCAST_INST_RESPONSE = "ServerName" SEMICOLON SERVERNAME SEMICOLON
	"InstanceName" SEMICOLON INSTANCENAME SEMICOLON "IsClustered"
	SEMICOLON YES_OR_NO SEMICOLON "Version" SEMICOLON VERSION_STRING
	[NP_INFO] [TCP_INFO] [VIA_INFO] [RPC_INFO] [SPX_INFO] [ADSP_INFO]
	[BV_INFO] SEMICOLON SEMICOLON

	*/

	struct prop
	{
		string key;
		string value;
	};

	struct response
	{
		string raw;

		string server;
		string instance;
		string version;

		string tcp;
		string np;

		string clustered;

		vector<prop> props;

		void add(prop p)
		{
			if (p.key == "ServerName") {
				server = p.value;
			}
			else if (p.key == "InstanceName") {
				instance = p.value;
			}
			else if (p.key == "IsClustered") {
				clustered = p.value;
			}
			else if (p.key == "Version") {
				version = p.value;
			}
			else if (p.key == "tcp") {
				tcp = p.value;
			}
			else if (p.key == "np") {
				np = p.value;
			}
			else {
				props.push_back(p);
			}
		}
	};

	vector<response> parse(const char* response_data, size_t response_len)
	{
		vector<response> responses;

		// a response can contain multiple entries, so let's split them.
		// all begin with ServerName; and end with ;; but ;; can appear
		// elsewhere, too.

		auto begin = response_data;
		auto end = response_data + response_len;

		auto pos = begin;

		auto token = pos;
		auto line = pos;

		responses.push_back(response{});
		auto* r = &responses.back();

		while (pos < end) {
			if (*pos != ';') {
				++pos;
			}
			else {
				prop p;

				p.key = string(token, pos - token);

				++pos;
				token = pos;

				// an empty key is the end of a single response
				if (p.key.empty()) {
					r->raw = string(line, pos - line);

					responses.push_back(response{});
					r = &responses.back();
					line = pos;
					continue;
				}

				while (pos < end) {
					if (*pos != ';') {
						++pos;
					}
					else {
						p.value = string(token, pos - token);

						r->add(p);

						++pos;
						token = pos;
						break;
					}
				}
			}
		}

		r->raw = string(line, pos - line);

		if (r->raw.empty()) {
			responses.pop_back();
		}

		return responses;
	}

	vector<response> parse(const wire_response& response)
	{
		return parse(response.data, response.len);
	}
}

