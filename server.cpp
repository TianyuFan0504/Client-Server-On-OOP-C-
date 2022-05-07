#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <vector>
#include <iterator>
#include <algorithm>
#include <Winsock2.h>
#include <Windows.h>
#include <math.h>
#include <fstream>
#define pi 3.1415926
using namespace std;
HANDLE bufferMutex; // 令其能互斥成功正常通信的信号量句柄
SOCKET sockConn;    // 客户端的套接字
vector<SOCKET> clientSocketGroup;
int flag = 0;
int i = 1;
DWORD WINAPI SendMessageThread(LPVOID IpParameter);
DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter);
double *follower_1_x=new double[251],*follower_1_y=new double[251],*follower_2_x=new double[251],*follower_2_y=new double[251];
class leader
{
    public:
        int iteration, round;
        double *x, *y;
        double (*func)(double x);
        leader(int n, double (*px)(double x));
        double *iterative(int round);
        int Init();
        void output(void);
        ~leader();

    private:
        SOCKET sockSrv;
        HANDLE sendThread;
};
leader::leader(int n, double (*px)(double x))
{
    iteration = n;
    x = new double[n + 1];
    y = new double[n + 1];
    func = px;
    round = 1;
};
double *leader::iterative(int round)
{
    double *p = new double[3];
    double ang;
    if (round < 50)
    {
        x[round] = 1.0 * round / 20;
        y[round] = 1.00001;
    }
    else
    {
        x[round] = 1.0 * round / 20;
        y[round] = (*func)(x[round]);
    }
    if (round >= 2)
    {
        ang = atan(1.0 * (y[round] - y[round - 1]) / (x[round] - x[round - 1]));
    }
    else
    {
        ang = 0;
    }
    p[0] = x[round];
    p[1] = y[round];
    p[2] = ang;
    return p;
};
double sinx(double x)
{
    return sin(x - 2.5 + pi / 2);
};
int leader::Init()
{
    // 加载socket动态链接库(dll)
    WORD wVersionRequested;
    //加载套接字库
    WSADATA wsaData;                    //初始化网络环境
    wVersionRequested = MAKEWORD(2, 2); // 请求2.2版本的WinSock库
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        return -1; // 返回值为零的时候是表示成功申请WSAStartup
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) // 检测是否2.2版本的socket库
    {
        WSACleanup();
        return -1;
    }

    // 创建socket操作，建立流式套接字，返回套接字号sockSrv
    sockSrv = socket(AF_INET, SOCK_STREAM, 0);

    //初始化服务器地址族变量
    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSrv.sin_family = AF_INET;   //指定ipv4协议
    addrSrv.sin_port = htons(6000); //主机字节序

    // 绑定套接字
    if (SOCKET_ERROR == bind(sockSrv, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR)))
    {
        return -1;
    }

    // 将套接字设置为监听模式（连接请求），listen()通知TCP服务器准备好接收连接，最大连接数为20
    listen(sockSrv, 20);

    cout << "Server prepared\n";

    // 设置信号量的值
    bufferMutex = CreateSemaphore(NULL, 1, 1, NULL);

    // 建立一个向所有客户端发送信息的线程
    sendThread = CreateThread(NULL, 0, SendMessageThread, NULL, 0, NULL);

    while (true) // 不断等待客户端请求的到来
    {
        sockConn = accept(sockSrv, NULL, NULL);
        if (SOCKET_ERROR != sockConn)
        {
            clientSocketGroup.push_back(sockConn); //若连接成功则将客户端套接字加入clientSocketGroup
        }

        // 建立一个接收指定客户端信息的线程
        HANDLE receiveThread = CreateThread(NULL, 0, ReceiveMessageThread, (LPVOID)sockConn, 0, NULL);

        WaitForSingleObject(bufferMutex, INFINITE); // P操作（资源未被占用则使用资源）
        if (NULL == receiveThread)
        {
            cout << "CreatThread AnswerThread() failed." << endl;
        }
        else
        {
            cout << "Create Receive Client Thread OK." << endl;
        }
        ReleaseSemaphore(bufferMutex, 1, NULL); // V操作（资源占用完毕将其释放）
    }

    WaitForSingleObject(sendThread, INFINITE); // 等待线程结束
    CloseHandle(bufferMutex);
}
leader::~leader()
{
    output(); 
	CloseHandle(sendThread);
    WSACleanup(); // 终止对套接字库的使用
}
int iteration = 250;
double (*func)(double x) = sinx;
double dis = 0.4;
double PID[3] = {0.6, 0.01, 0.001};
double *p = new double[3];
double dt = 0.005;
ofstream oFile;
leader car_leader(iteration, func);
int main()
{
    car_leader.Init();
    car_leader.~leader();
    car_leader.output(); 
    system("pause");
    return 0;
}

DWORD WINAPI SendMessageThread(LPVOID IpParameter)
{
    while (i <= iteration)
    {
        while (flag)
        {
            string talk;
            p = car_leader.iterative(i); //返回x,y,ang

            double b[3] = {0, 0, 0};
            b[0] = p[0] + 0.00001;	
			b[1] = p[1] + 0.002;
            b[2] = p[2] + 0.00001;
            cout << b[0] << "," << b[1] << "," << b[2] << endl;
            char *c;
            c = (char *)&b;
            talk = c;
            i++;
            WaitForSingleObject(bufferMutex, INFINITE); // P（资源未被占用）
            //若服务器要主动关闭
            if ("quit" == talk)
            {
                talk.push_back('\0');
                for (int i = 0; i < clientSocketGroup.size(); ++i)
                {
                    send(clientSocketGroup[i], talk.c_str(), talk.size(), 0); // 发送信息
                }
                ReleaseSemaphore(bufferMutex, 1, NULL); // V（资源占用完毕）
                exit(0);
            }
            cout << p[0] << endl;
            cout << p[1] << endl;
            cout << p[2] << endl;
            for (int i = 0; i < clientSocketGroup.size(); ++i)
            {
                send(clientSocketGroup[i], talk.c_str(), talk.size(), 0); // 发送信息
            }
            ReleaseSemaphore(bufferMutex, 1, NULL); // V（资源占用完毕）
            flag = 0;
        }
    }
    car_leader.output(); 
    return 0;
}

DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter)
{
    SOCKET ClientSocket = (SOCKET)(LPVOID)IpParameter;
    while (1)
    {
        while (!flag)
        {
            char recvBuf[200];
            cout << "before recv" << endl;
            int recv_id = recv(ClientSocket, recvBuf, 300, 0);
            cout << "after recv" << endl;
            WaitForSingleObject(bufferMutex, INFINITE); 

            char *b = recvBuf;
            double *p = (double *)(b);
            cout << p[0] << endl;
            cout << p[1] << endl;
            cout << p[2] << endl;
            cout << p[3] << endl;
            cout << "i=" << i << endl;
            follower_1_x[i]=p[0];
            follower_1_y[i]=p[1];
            follower_2_x[i]=p[2];
            follower_2_y[i]=p[3];
            ReleaseSemaphore(bufferMutex, 1, NULL); 
            flag = 1;
        }
    }
    car_leader.output(); 
    return 0;
}

void leader::output(void)
{
    ofstream oFile;
    oFile.open("follower_1.csv", ios::out | ios::trunc);
    for (int i = 1; i <= iteration; i++)
    {
        oFile << car_leader.x[i] << "," << car_leader.y[i] << ","<<follower_1_x[i]<<","<<follower_1_y[i]<< ","<<follower_2_x[i]<<","<<follower_2_y[i]<<endl;
    }
    oFile.close();
}
