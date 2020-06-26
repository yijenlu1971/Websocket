#include <stdio.h>
//#include <unistd.h>
#include <iostream>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#define NUM 320

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

int closeConnect(client::connection_ptr);

bool bIsConnectedServer = false;
int end = 0;

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the
// server.
void on_message(websocketpp::connection_hdl hdl, message_ptr msg)
{
	std::cout << "hdl.lock():" << hdl.lock().get() << std::endl;
	//             << " and message: " << msg->get_payload() << std::endl;
	// msg->get_payload();

	std::string text = msg->get_payload().c_str();
	//int pos = text.find("\"text\":\"");
	//text = text.substr(pos);
	//int end = text.find("\"", 8);
	//if (end != 8)
	{
//		text = text.substr(8, end - 8);
		printf("%s\n\n", text.c_str());
	}
}

void on_open(client* c, websocketpp::connection_hdl hdl)
{
	std::cout << "open handler hdl.lock():" << hdl.lock().get() << std::endl;
	client::connection_ptr conn = c->get_con_from_hdl(hdl);
	// websocketpp::config::core_client::request_type requestClient =
	// con->get_request();
	if (conn->get_ec().value() != 0)
	{
		bIsConnectedServer = false;
		printf("on open error....\n");
	}
	else
	{
		bIsConnectedServer = true;
		printf("on open ok....\n");
	}
}

void on_close(client* c, websocketpp::connection_hdl hdl)
{
	client::connection_ptr conn = c->get_con_from_hdl(hdl);
	bIsConnectedServer = false;
	printf("on_close....\n");
	end = 1;
	closeConnect(conn);
}

void on_termination_handler(client* c, websocketpp::connection_hdl hdl)
{
	client::connection_ptr conn = c->get_con_from_hdl(hdl);
	bIsConnectedServer = false;
	end = 1;
	printf("on_termination_handler....\n");
	closeConnect(conn);
}

struct Param
{
	char uri[60];
	char audio[60];
};

int main()
{
	// Create a client endpoint
	client c;
	client::connection_ptr conn;

	FILE* fd = NULL;
	end = 0;
	struct Param param;
		
	sprintf_s(param.uri, "ws://114.67.123.29:14910/ws/asr/pcm");
	sprintf_s(param.audio, "test1.wav");
	printf("url:%s, audio:%s\n", param.uri, param.audio);
	std::string uri(param.uri);  //"ws://192.168.5.25:8101/ws/asr/pcm";
	printf("uri:%s\n", uri.c_str());
	
	try
	{
		// Set logging to be pretty verbose (everything except message payloads)
		c.set_access_channels(websocketpp::log::alevel::none);
//		c.clear_access_channels(websocketpp::log::alevel::frame_payload);

		// Initialize ASIO
		c.init_asio();
		c.set_reuse_addr(true);

		// Register our message handler
		c.set_message_handler(bind(&on_message, ::_1, ::_2));
		c.set_open_handler(bind(&on_open, &c, ::_1));
		c.set_close_handler(bind(&on_close, &c, ::_1));

		websocketpp::lib::error_code ec;
		conn = c.get_connection(uri, ec);
		if (ec)
		{
			std::cout << "could not create connection because: " << ec.message() << std::endl;
			return -1;
		}
		// set http header
		conn->append_header("rate", "16000");
		conn->append_header("appkey", "uus46rhwq3x75562p2cd7f7qplwso6wt5xd4qqae");
		conn->append_header("closeVad", "true");
		conn->append_header("session-id", "fe945929-dfa4-4960-8e31-f8c057ed2b99");
		conn->append_header("eof", "eof");
		// conn->append_header("vocab-enhance", "5");
		// conn->append_header("group-id", "5e58e7ec39de65172c496471");
		// conn->append_header("vocab-id", "5e707b7639de65172c496476");
		// conn->append_header();
		// Note that connect here only requests a connection. No network
		// messages are exchanged until the event loop starts running in the
		// next line.
		c.connect(conn);
		conn->set_termination_handler(bind(on_termination_handler, &c, ::_1));
		// Start the ASIO io_service run loop
		// this will cause a single connection to be made to the server. c.run()
		// will exit when this connection is closed.
		// c.run();

		int n;
		char pdata[NUM] = { 0 };
		int eof = 0;
		// open audio file
		fopen_s(&fd, param.audio, "rb");
		if (fd == NULL)
		{
			printf("===fopen error\n");
			return -1;
		}

		while (true)
		{
			if (bIsConnectedServer)
			{
				if (eof == 0 && !feof(fd))
				{
					n = fread(pdata, 1, NUM, fd);
					if (n == 0)
					{
						continue;
					}
					try
					{
						ec = conn->send(pdata, n, websocketpp::frame::opcode::binary);
						if (ec)
						{
							std::cout << "Echo failed because: " << ec.message() << std::endl;
							break;
						}
					}
					catch (websocketpp::exception const& e)
					{
						std::cout << e.what() << std::endl;
						break;
					}
				}
				else
				{
					eof = eof + 1;
					if (eof == 1)
					{
						try
						{
							ec = conn->send("eof");
							if (ec)
							{
								std::cout << "...Echo failed because: " << ec.message() << std::endl;
								break;
							}
						}
						catch (websocketpp::exception const& e)
						{
							std::cout << e.what() << std::endl;
							break;
						}
					}

					printf("eof:%d.............\n", eof);
				}
			}
			// after sever close,exit while.
			if (end)
			{
				printf("end end.........\n");
				break;
			}
			// printf("before run_one...\n");
			c.run_one();
			// printf("after run_one...\n");
			Sleep(10);
		}
	}
	catch (websocketpp::exception const& e)
	{
		std::cout << e.what() << std::endl;
	}

	//   sleep(1);
	fclose(fd);
	printf("stop........file close...\n");
	c.stop();
}

int closeConnect(client::connection_ptr conn)
{
	int nRet = 0;
	
	try
	{
		if (conn != NULL &&
			conn->get_state() == websocketpp::session::state::open)
		{
			printf("============== closeConnect\n");
			websocketpp::close::status::value cvValue = 0;
			std::string strReason = "";
			conn->close(cvValue, strReason);
		}
		else
		{
			printf("==============no closeConnect\n");
		}
	}
	catch (websocketpp::exception const& e)
	{
		nRet = -1;
	}
	return nRet;
}
