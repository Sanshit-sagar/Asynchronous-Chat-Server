#include <boost/asio.hpp>
#include <optional>
#include <queue>
#include <unordered_set>

namespace io = boost::asio;
using tcp = io::ip::tcp;
using error_code = boost::system::error_code;

using message_handler = std::function<void (std::string)>;
using error_handler = std::function<void ()>;

class session : public std::enable_shared_from_this<session>
{
public:

    session(tcp::socket&& socket)
    : socket(std::move(socket))
    {
    }

    void start(message_handler&& on_message, error_handler&& on_error)
    {
        this->on_message = std::move(on_message);
        this->on_error = std::move(on_error);
        async_read();
    }

    void post(std::string const& message)
    {
        bool idle = outgoing.empty();
        outgoing.push(message);

        if(idle)
        {
            async_write();
        }
    }

private:

    void async_read()
    {
        io::async_read_until(socket, streambuf, "\n", [self = shared_from_this()] (error_code error, std::size_t bytes_transferred)
        {
            self->on_read(error, bytes_transferred);
        });
    }

    void on_read(error_code error, std::size_t bytes_transferred)
    {
        if(!error)
        {
            std::stringstream message;
            message << socket.remote_endpoint(error) << ": " << std::istream(&streambuf).rdbuf();
            streambuf.consume(bytes_transferred);
            on_message(message.str());
            async_read();
        }
        else
        {
            socket.close(error);
            on_error();
        }
    }

    void async_write()
    {
        io::async_write(socket, io::buffer(outgoing.front()), [self = shared_from_this()] (error_code error, std::size_t bytes_transferred)
        {
            self->on_write(error, bytes_transferred);
        });
    }

    void on_write(error_code error, std::size_t bytes_transferred)
    {
        if(!error)
        {
            outgoing.pop();

            if(!outgoing.empty())
            {
                async_write();
            }
        }
        else
        {
            socket.close(error);
            on_error();
        }
    }

    tcp::socket socket;
    io::streambuf streambuf;
    std::queue<std::string> outgoing;
    message_handler on_message;
    error_handler on_error;
};
