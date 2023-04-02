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

seastar::future<> handle_connection(seastar::accept_result &&res) {
  auto out = res.connection.output();
  auto in = res.connection.input();

  while (true) {
    auto buf = co_await in.read();
    if (buf) {
      co_await out.write(std::move(buf));
      co_await out.flush(); // for debugging
    } else {
      co_return;
    }
  }
}

seastar::future<> service_loop() {
  seastar::listen_options lo;
  lo.reuse_address = true;
  auto listener = seastar::listen(seastar::make_ipv4_address({1234}), lo);

  while (true) {
    auto res = co_await listener.accept();

    (void)handle_connection(std::move(res))
        .handle_exception([](std::exception_ptr ep) {
          fmt::print(stderr, "Could not handle connection: {}\n", ep);
        });
  }
}

int main(int ac, char **av) {
  seastar::app_template app;
  app.run(ac, av, [] {
    return seastar::parallel_for_each(
        boost::irange<unsigned>(0, seastar::smp::count),
        [](unsigned c) { return seastar::smp::submit_to(c, service_loop); });
  });
  return 0;
}
