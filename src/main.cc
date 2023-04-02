#include <iostream>
#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/future.hh>
#include <seastar/core/loop.hh>
#include <seastar/core/reactor.hh>
#include <seastar/core/seastar.hh>
#include <seastar/net/api.hh>
#include <seastar/util/later.hh>

seastar::future<> handle_connection(seastar::connected_socket s,
                                    seastar::socket_address a) {
  auto out = s.output();
  auto in = s.input();
  std::cout << "established connection" << std::endl;
  while (true) {
    std::cout << "entered looop" << std::endl;
    auto buf = co_await in.read();
    if (buf) {
      co_await out.write(std::move(buf));
      co_await out.flush();
    } else {
      std::cout << "done" << std::endl;
      co_return;
    }
  }
}

seastar::future<> service_loop_3() {
  seastar::listen_options lo;
  lo.reuse_address = true;
  return seastar::do_with(
      seastar::listen(seastar::make_ipv4_address({1234}), lo),
      [](auto &listener) {
        return seastar::keep_doing([&listener]() {
          return listener.accept().then([](seastar::accept_result res) {
            // Note we ignore, not return, the future returned by
            // handle_connection(), so we do not wait for one
            // connection to be handled before accepting the next one.
            (void)handle_connection(std::move(res.connection),
                                    std::move(res.remote_address))
                .handle_exception([](std::exception_ptr ep) {
                  fmt::print(stderr, "Could not handle connection: {}\n", ep);
                });
          });
        });
      });
}

int main(int ac, char **av) {
  seastar::app_template app;
  app.run(ac, av, [] {
    return seastar::parallel_for_each(
        boost::irange<unsigned>(0, seastar::smp::count),
        [](unsigned c) { return seastar::smp::submit_to(c, service_loop_3); });
  });
  return 0;
}
