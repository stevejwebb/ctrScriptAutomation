#include <iostream>
#include <csignal>

using namespace std;

void signal_handler(int signal_num)
{
    cout << "Signal is " << signal_num << endl;
    exit(signal_num);
}

int main()
{
    signal(SIGINT, signal_handler);

    for(;;)
    {
        //do nothing but wait
    }
}