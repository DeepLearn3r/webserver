#include "EventLoop.h"
#include "Server.h"
#include "Logging.h"



int main()
{
    EventLoop mainLoop_;
    Server server(&mainLoop_, 6, 8080);
    server.start();
    mainLoop_.loop();

    return 0;
}