#include <ESPressio_Sockets.hpp>

int main() {
    ESPressio::Sockets::SocketWorkerConfig config;
    return config.StackSize == 0 ? 1 : 0;
}
