#define BOOST_ASIO_NO_DEPRECATED
#include <boost/asio.hpp>
#include <thread>

namespace io  = boost::asio;
using tcp = io::ip::tcp;
using work_guard_type = io::executor_work_guard<io::io_context::executor_type>;
using error_code = boost::system::error_code;

class IOContextGroup
{
public:
  IOContextGroup(std::size_t size)
  {
    for(std::size_t n = 0; n<size; ++n)
    {
      contexts.emplace_back(std::make_shared<io::io_context>());
      guards.emplace_back(std::make_shared<work_guard_type>(contexts.back()->get_executor()));
    }
  }

  void run()
  {
    for(auto &io_context : contexts)
    {
      threads.emplace_back(&io::io_context::run, io_context.get());
    }
  }

  io::io_context& query()
  {
    std::size_t i = 0;
    for(auto& c : contexts) {
      if(i==index) {
        return *c;
      }
      if(i==contexts.size()-1) {
        return *c;
      }
      else {
        ++i;
      }
    }
    return *contexts[0];
  }

private:
  template<typename T>
  using vector_ptr = std::vector<std::shared_ptr<T>>;
  vector_ptr<io::io_context> contexts;
  vector_ptr<work_guard_type> guards;
  std::vector<std::thread> threads;

  std::atomic<std::size_t> index = 0;
};

int main()
{
  IOContextGroup ioc_group(std::thread::hardware_concurrency() * 2);
  tcp::socket socket(ioc_group.query());

  /*
    Create instances of io_context and add them to the threads vector.
    Then create the server and let it accept incoming connections.
  */

  ioc_group.run();
  return 0;
}
