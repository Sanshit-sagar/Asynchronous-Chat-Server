#include <boost/asio.hpp>
#include <optional>
#include <queue>
#include <unordered_set>

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;

using message_handler = std::function<void (std::string)>;
using error_handler = std::function<void ()>;
using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;


class server
{
public:

    server(io::io_context& io_context, std::uint16_t port)
    : io_context(io_context)
    , acceptor  (io_context, tcp::endpoint(tcp::v4(), port))
    {
    }

    void async_accept()
    {
        socket.emplace(io_context);

        acceptor.async_accept(*socket, [&] (error_code error)
        {
            auto client = std::make_shared<session>(std::move(*socket));
            client->post("Welcome to chat\n\r");
            post("We have a newcomer\n\r");

            clients.insert(client);

            client->start
            (
                std::bind(&server::post, this, std::placeholders::_1),
                [&, client]
                {
                    if(clients.erase(client))
                    {
                        post("We are one less\n\r");
                    }
                }
            );

            async_accept();
        });
    }

    void post(std::string const& message)
    {
        for(auto& client : clients)
        {
            client->post(message);
        }
    }

private:

    io::io_context& io_context;
    tcp::acceptor acceptor;
    std::optional<tcp::socket> socket;
    std::unordered_set<std::shared_ptr<session>> clients;
};

int main()
{
    io::io_context io_context;
    work_guard_type work_guard(io_context.get_executor());
    server srv(io_context, 15001);
    srv.async_accept();
    /* Use the code below if server should shut after a a specific time.
    * needs to be used again, call io_context::reset
    *
    * std::thread watchdog([&]
    * {
    *     std::this_thread::sleep_for(10s);
    *     work_guard.reset(); // Work guard is destroyed, io_context::run is free to return
    * });
    */
    io_context.run();
    return 0;
}
