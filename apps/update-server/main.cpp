#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <json.hpp>
#include <fstream>
#include <thread>
#include <boost/program_options.hpp>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

struct Options
{
    std::string cfg;
    std::string dataPath;
};


bool parseOptions(int argc, char **argv, Options &options)
{
    namespace bpo = boost::program_options;
    typedef std::vector<std::string> StringsVector;

    bpo::options_description desc("Syntax: update-server <options>\nAllowed options");
    desc.add_options()
            ("help", "print help message")
            ("config", bpo::value<std::string>(), "path to config file (e.g. --config=updater.json)")
            ("dataPath", bpo::value<std::string>(), "path to data");

    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
            .options(desc).allow_unregistered().run();

    bpo::variables_map variables;

    // Runtime options override settings from all configs
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (variables.count("help"))
    {
        std::cout << desc << std::endl;
        return false;
    }

    options.cfg = variables["config"].as<std::string>();

    if(variables.count("dataPath"))
        options.dataPath = variables["dataPath"].as<std::string>();

    return true;
}

boost::beast::string_view mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const pos = path.rfind(".");
    if (pos != boost::beast::string_view::npos)
    {
        if (iequals(path.substr(pos), ".htm")) return "text/html";
        if (iequals(path.substr(pos), ".html")) return "text/html";
        if (iequals(path.substr(pos), ".json")) return "application/json";
    }
    return "application/octet-stream";
}

unsigned char hexToInt(boost::beast::string_view hex)
{
    std::stringstream sstr;
    int val;
    sstr << std::hex << hex;
    sstr >> val;
    return (unsigned char) val;
}

std::string decodeURL(boost::beast::string_view url)
{
    std::string result;
    boost::beast::string_view::const_iterator beginBlock = url.begin();
    for(auto it = url.begin(); it != url.end(); ++it)
    {
        if(*it == '%')
        {
            result.append(beginBlock, it);
            char v[2] {*(++it), *(++it)};
            unsigned char value = hexToInt(v); // todo: validate URI value
            result.push_back(value);
            beginBlock = ++it;
        }
    }

    if(result.empty())
        return url.to_string();
    else
    {
        if(beginBlock != url.end())
            result.append(beginBlock, url.end());
        return result;
    }
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    if (base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if (result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// This function produces an HTTP response for the given request. The type of the response object depends on the
// contents of the request, so the interface requires the caller to pass a generic lambda for receiving the response.
template<class Body, class Allocator, class Send>
void handle_request(boost::beast::string_view doc_root, http::request<Body, http::basic_fields<Allocator>> &&req, Send &&send)
{

    auto const response_msg = [&req] (boost::beast::string_view message, http::status status = http::status::ok) {
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = message.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a bad request response
    auto const bad_request = [&req, response_msg](boost::beast::string_view why) {
        return response_msg(why.to_string(), http::status::bad_request);
    };

    // Returns a not found response
    auto const not_found = [&req, response_msg](boost::beast::string_view target) {
        return response_msg("The resource '" + target.to_string() + "' was not found.", http::status::not_found);
    };

    // Returns a server error response
    auto const server_error = [&req, response_msg](boost::beast::string_view what) {
        return response_msg("An error occurred: '" + what.to_string() + "'", http::status::internal_server_error);
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if (req.target().empty() || req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if (req.target().back() == '/')
    {
        //path.append("index.html");
        return send(response_msg("<h1>Welcome to the tes3mp update server</h1>"));
    }

    // Attempt to open the file
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(decodeURL(path).c_str(), boost::beast::file_mode::scan, ec);

    std::cout << path << std::endl;

    // Handle the case where the file doesn't exist
    if (ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // Handle an unknown error
    if (ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if (req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(body)), std::make_tuple(http::status::ok, req.version())
    };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(boost::system::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
    // The function object is used to send an HTTP message.
    struct send_lambda
    {
        Session &self;

        explicit send_lambda(Session &self) : self(self) {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields> &&msg) const
        {
            // The lifetime of the message has to extend for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared pointer in the class to keep it alive.
            self.res = sp;

            // Write the response
            http::async_write(self._socket, *sp,
                              boost::asio::bind_executor(self.strand,
                                                         std::bind(&Session::on_write, self.shared_from_this(),
                                                                   std::placeholders::_1, std::placeholders::_2,
                                                                   sp->need_eof())
                              )
            );
        }
    };

    tcp::socket _socket;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::beast::flat_buffer buffer;
    std::shared_ptr<std::string const> doc_root;
    http::request<http::string_body> req;
    std::shared_ptr<void> res;
    send_lambda lambda;

public:
    explicit Session(tcp::socket socket, std::shared_ptr<std::string const> const &doc_root)
            : _socket(std::move(socket)), strand(_socket.get_executor()), doc_root(doc_root), lambda(*this) {}

    // Start the asynchronous operation
    void run()
    {
        do_read();
    }

    void do_read()
    {
        // Make the request empty before reading, otherwise the operation behavior is undefined.
        req = {};

        http::async_read(_socket, buffer, req,
                         boost::asio::bind_executor(strand, std::bind(&Session::on_read, shared_from_this(),
                                                                      std::placeholders::_1, std::placeholders::_2)
                         )
        );
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        // Send the response
        handle_request(*doc_root, std::move(req), lambda);
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (close)
        {
            // We should close the connection, usually because the response indicated
            // the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res = nullptr;

        // Read another request
        do_read();
    }

    void do_close()
    {
        // Send a TCP shutdown
        boost::system::error_code ec;
        _socket.shutdown(tcp::socket::shutdown_send, ec);
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener>
{
    tcp::acceptor acceptor;
    tcp::socket _socket;
    std::shared_ptr<std::string const> doc_root;

public:
    Listener(boost::asio::io_context &ioc, tcp::endpoint endpoint, std::shared_ptr<std::string const> const &doc_root)
            : acceptor(ioc), _socket(ioc), doc_root(doc_root)
    {
        boost::system::error_code ec;

        acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        if (!acceptor.is_open())
            return;
        do_accept();
    }

    void do_accept()
    {
        acceptor.async_accept(_socket, std::bind(&Listener::on_accept, shared_from_this(), std::placeholders::_1));
    }

    void on_accept(boost::system::error_code ec)
    {
        if (ec)
            fail(ec, "accept");
        else // Create the Session and run it
            std::make_shared<Session>(std::move(_socket), doc_root)->run();

        // Accept another connection
        do_accept();
    }

    static void spawn(boost::asio::io_context &ioc,
                      const boost::asio::ip::address &address, unsigned short port,
                      boost::beast::string_view dataRoot)
    {
        std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, std::make_shared<std::string>(dataRoot))->run();
    }
};


// todo: generate file_list.json

int main(int argc, char **argv)
{
    Options options;
    if (!parseOptions(argc, argv, options))
        return 0;
    nlohmann::json config;

    {
        if(options.cfg.empty())
            options.cfg = "update-server.json";
        std::ifstream cfg_file(options.cfg);
        cfg_file >> config;
    }

    auto const threads = std::max<int>(1, config["maxThreads"]);

    boost::asio::io_context ioc{threads};

    std::string dataPath = options.dataPath;
    if (options.dataPath.empty())
        config["dataPath"].get<std::string>();

    if(dataPath.empty())
        throw std::runtime_error("dataPath is not present in the config neither in the running options");


    Listener::spawn(ioc,
                    boost::asio::ip::make_address(config["address"].get<std::string>()),
                    config["externalPort"],
                    dataPath);

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
    {
        v.emplace_back([&ioc] {
            ioc.run();
        });
    }

    std::cout << "Listening on: " << config["address"].get<std::string>() << ":" << config["externalPort"] << std::endl;
    std::cout << "Data path: " << dataPath << std::endl;

    ioc.run();

    return 0;
}
