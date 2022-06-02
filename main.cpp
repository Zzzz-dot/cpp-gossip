#include <memberlist/memberlist.h>
#include "cmakeConfig.h"

using namespace std;

int main(int argc, char *argv[])
{
    shared_ptr<Config> config = DebugConfig();
    if (argc > 1)
    {
        config->Name = argv[1];
        config->BindPort = stoul(argv[1]);
        config->AdvertisePort = stoul(argv[1]);
    }
    shared_ptr<memberlist> m = make_shared<memberlist>(config);
    if (argc > 1)
    {
        m->Join("127.0.0.1:6666");
    }
    while (true)
    {
        if (argc > 1)
        {
            string msg;
            getline(cin, msg);
            if(msg=="Leave()"){
                m->Leave(-1);
            }
            if(msg=="ShutDown()"){
                m->ShutDown();
                sleep(10);
                return 0;
            }
            auto node = make_shared<Node>("", "127.0.0.1", 6666);
            m->SendBestEffort(node, msg);
            m->SendReliable(node, msg);
        }
    }
    return 0;
}