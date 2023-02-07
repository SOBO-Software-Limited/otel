#include <boost/asio.hpp>

#include <opentelemetry/trace/propagation/detail/hex.h>
#include <opentelemetry/trace/propagation/detail/string.h>
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

#include "libMetrics/Api.h"


using namespace boost;

class Service {
 public:
  Service(std::shared_ptr<asio::ip::tcp::socket> sock) :
                                                         m_sock(sock)
  {}

  void StartHandling() {

    asio::async_read_until(*m_sock.get(),
                           m_request,
                           '\n',
                           [this](
                               const boost::system::error_code& ec,
                               std::size_t bytes_transferred)
                           {
                             onRequestReceived(ec,
                                               bytes_transferred);
                           });
  }

 private:
  void onRequestReceived(const boost::system::error_code& ec,
                         std::size_t bytes_transferred) {
    if (ec.value() != 0) {
      std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();

      onFinish();
      return;
    }

    std::cout << "bytes transferred " << bytes_transferred << std::endl;

    // Process the request.
    m_response = ProcessRequest(m_request);

    // Initiate asynchronous write operation.
    asio::async_write(*m_sock.get(),
                      asio::buffer(m_response),
                      [this](
                          const boost::system::error_code& ec,
                          std::size_t bytes_transferred)
                      {
                        onResponseSent(ec,
                                       bytes_transferred);
                      });
  }

  void onResponseSent(const boost::system::error_code& ec,
                      std::size_t bytes_transferred) {
    if (ec.value() != 0) {
      std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();
    }

    onFinish();
  }

  // Here we perform the cleanup.
  void onFinish() {
    delete this;
  }

  opentelemetry::trace::TraceId TraceIdFromHex(opentelemetry::nostd::string_view trace_id)
  {
    uint8_t buf[trace_id.size() / 2];
    opentelemetry::trace::propagation::detail::HexToBinary(trace_id, buf, sizeof(buf));
    return opentelemetry::trace::TraceId(opentelemetry::nostd::span<uint8_t>(buf));
  }

  static SpanId SpanIdFromHex(nostd::string_view span_id)
  {
    uint8_t buf[kSpanIdHexStrLength / 2];
    detail::HexToBinary(span_id, buf, sizeof(buf));
    return SpanId(buf);
  }

  static TraceFlags TraceFlagsFromHex(nostd::string_view trace_flags)
  {
    if (trace_flags.length() != 1 || (trace_flags[0] != '1' && trace_flags[0] != 'd'))
    {  // check for invalid length of flags and treat 'd' as sampled
      return TraceFlags(0);
    }
    return TraceFlags(TraceFlags::kIsSampled);
  }

  std::string ProcessRequest(asio::streambuf& request) {

    // In this method we parse the request, process it
    // and prepare the request.
    std::istream buffer( &request );
    std::stringstream string_buffer;
    string_buffer <<  buffer.rdbuf();
    std::string_view buf(string_buffer.str()) ;
    std::string_view cmd;
    std::string_view time;
    std::string_view trace_id_hex;
    std::string_view span_id_hex;
    std::string_view trace_flags_hex;

    if (!buf.empty())
    {
      std::array<std::string_view, 5> fields{};

      if (opentelemetry::trace::propagation::detail::SplitString(buf, '-', fields.data(), 5) < 5)
      {
        return "invalid format";
      }

      cmd = fields[0];
      time = fields[1];
      trace_id_hex    = fields[2];
      span_id_hex     = fields[3];
      trace_flags_hex = fields[4];
    }

    if (!opentelemetry::trace::propagation::detail::IsValidHex(trace_id_hex) || !opentelemetry::trace::propagation::detail::IsValidHex(span_id_hex))
    {
      return  "invalid hex";
    }

    opentelemetry::trace::TraceId trace_id = opentelemetry::trace::propagation::detail::TraceIdFromHex(trace_id_hex);
    opentelemetry::trace::SpanId span_id   = opentelemetry::trace::propagation::detail::SpanIdFromHex(span_id_hex);

    // Emulate CPU-consuming operations.
    int i = 0;
    while (i != 1000000)
      i++;

    // Emulate operations that block the thread
    // (e.g. synch I/O operations).
    std::this_thread::sleep_for(
        std::chrono::milliseconds(100));

    // Prepare and return the response message.
    std::string response = "Response\n";
    return response;
  }

 private:
  std::shared_ptr<asio::ip::tcp::socket> m_sock;
  std::string m_response;
  asio::streambuf m_request;
};

class Acceptor {
 public:
  Acceptor(asio::io_service& ios, unsigned short port_num) :
                                                             m_ios(ios),
                                                             m_acceptor(m_ios,
                                                                        asio::ip::tcp::endpoint(
                                                                            asio::ip::address_v4::any(),
                                                                            port_num)),
                                                             m_isStopped(false)
  {}

  // Start accepting incoming connection requests.
  void Start() {
    m_acceptor.listen();
    InitAccept();
  }

  // Stop accepting incoming connection requests.
  void Stop() {
    m_isStopped.store(true);
  }

 private:
  void InitAccept() {
    std::shared_ptr<asio::ip::tcp::socket>
        sock(new asio::ip::tcp::socket(m_ios));

    m_acceptor.async_accept(*sock.get(),
                            [this, sock](
                                const boost::system::error_code& error)
                            {
                              onAccept(error, sock);
                            });
  }

  void onAccept(const boost::system::error_code& ec,
                std::shared_ptr<asio::ip::tcp::socket> sock)
  {
    if (ec.value() == 0) {
      (new Service(sock))->StartHandling();
    }
    else {
      std::cout << "Error occured! Error code = "
                << ec.value()
                << ". Message: " << ec.message();
    }

    // Init next async accept operation if
    // acceptor has not been stopped yet.
    if (!m_isStopped.load()) {
      InitAccept();
    }
    else {
      // Stop accepting incoming connections
      // and free allocated resources.
      m_acceptor.close();
    }
  }

 private:
  asio::io_service& m_ios;
  asio::ip::tcp::acceptor m_acceptor;
  std::atomic<bool> m_isStopped;
};

class Server {
 public:
  Server() {
    m_work.reset(new asio::io_service::work(m_ios));
  }

  // Start the server.
  void Start(unsigned short port_num,
             unsigned int thread_pool_size) {

    assert(thread_pool_size > 0);

    // Create and start Acceptor.
    acc.reset(new Acceptor(m_ios, port_num));
    acc->Start();

    // Create specified number of threads and
    // add them to the pool.
    for (unsigned int i = 0; i < thread_pool_size; i++) {
      std::unique_ptr<std::thread> th(
          new std::thread([this]()
                          {
                            m_ios.run();
                          }));

      m_thread_pool.push_back(std::move(th));
    }
  }

  // Stop the server.
  void Stop() {
    acc->Stop();
    m_ios.stop();

    for (auto& th : m_thread_pool) {
      th->join();
    }
  }

 private:
  asio::io_service m_ios;
  std::unique_ptr<asio::io_service::work> m_work;
  std::unique_ptr<Acceptor> acc;
  std::vector<std::unique_ptr<std::thread>> m_thread_pool;
};


const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

int main()
{
  unsigned short port_num = 3333;

  try {
    Server srv;

    unsigned int thread_pool_size =
        std::thread::hardware_concurrency() * 2;

    if (thread_pool_size == 0)
      thread_pool_size = DEFAULT_THREAD_POOL_SIZE;

    srv.Start(port_num, thread_pool_size);

    std::this_thread::sleep_for(std::chrono::seconds(6000000));

    srv.Stop();
  }
  catch (system::system_error &e) {
    std::cout << "Error occured! Error code = "
              << e.code() << ". Message: "
              << e.what();
  }

  return 0;
}