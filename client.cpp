#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <Windows.h>
#include <math.h>
#include <fstream>
#define pi 3.1415926
using namespace std;
SOCKET sockClient;  // 连接成功后的套接字
HANDLE bufferMutex; // 令其能互斥成功正常通信的信号量句柄
HANDLE sendThread;
HANDLE receiveThread;
DWORD WINAPI SendMessageThread(LPVOID IpParameter);
DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter);
int flag_client = 1;
int i = 1;
class follower
{
public:
    int iteration, round, number;
    double *x, *y, dis, error[2], integrator[2], P, I, D, dt;
    double (*func)(double dis, int number, double *p, bool x_y);
    follower(int n, double d, int num, double PID[3], double t, double (*px)(double dis, int number, double *p, bool x_y));
    void iterative(int round, double *p);
    void Motion(void);
};
follower::follower(int n, double d, int num, double PID[3], double t, double (*px)(double dis, int number, double *p, bool x_y))
{
    iteration = n;
    dis = d;
    number = num;
    dt = t;
    error[0] = error[1] = integrator[0] = integrator[1] = 0;
    P = PID[0];
    I = PID[1];
    D = PID[2];
    x = new double[n + 1];
    y = new double[n + 1];
    func = px;
};
void follower::iterative(int round, double *p)
{
    double target_x, target_y, error_x, error_y, P_x, P_y, I_x, I_y, D_x, D_y, out_x, out_y;
    if (round == 1)
    {
        if (number == 1)
        {
            x[round] = -0.5;
            y[round] = 1.5;
        }
        else if (number == 2)
        {
            x[round] = -0.5;
            y[round] = 0.5;
        }
    }
    target_x = (*func)(dis, number, p, true);
    target_y = (*func)(dis, number, p, false);
     if (number == 1)
     {
         target_x = p[0] - dis * cos(p[2]) - dis * sin(p[2]);
         target_y = p[1] - dis * sin(p[2]) + dis * cos(p[2]);
     }
     else if (number == 2)
     {
         target_x = p[0] - dis * cos(p[2]) + dis * sin(p[2]);
         target_y = p[1] - dis * sin(p[2]) - dis * cos(p[2]);
     }
    error_x = target_x - x[round - 1];
    error_y = target_y - y[round - 1];
    integrator[0] += error_x;
    integrator[1] += error_y;
    P_x = error_x;
    P_y = error_y;
    I_x = integrator[0] * dt;
    I_y = integrator[1] * dt;
    D_x = (error_x - error[0]) / dt;
    D_y = (error_y - error[1]) / dt;
    error[0] = error_x;
    error[1] = error_y;
    out_x = P * P_x + I * I_x + D * D_x;
    out_y = P * P_y + I * I_y + D * D_y;
    if (round >= 2)
    {
        x[round] = x[round - 1] + out_x;
        y[round] = y[round - 1] + out_y;
    }
    cout << "iterative succeeded" << endl;
}
double sinx(double x)
{
    return sin(x - 2.5 + pi / 2);
};
double isosceles_right_triangle(double dis, int number, double *p, bool x_y)
{
    if (number == 1 && x_y)
    {
        return p[0] - dis * cos(p[2]) - dis * sin(p[2]);
    }
    else if (number == 1 && (!x_y))
    {
        return p[1] - dis * sin(p[2]) + dis * cos(p[2]);
    }
    else if (number == 2 && x_y)
    {
        return p[0] - dis * cos(p[2]) + dis * sin(p[2]);
    }
    else if (number == 2 && (!x_y))
    {
        return p[1] - dis * sin(p[2]) - dis * cos(p[2]);
    }
}
void follower::Motion(void)
{
    bufferMutex = CreateSemaphore(NULL, 1, 1, NULL);

    //创建发送信息与接收信息的线程
    sendThread = CreateThread(NULL, 0, SendMessageThread, NULL, 0, NULL);
    receiveThread = CreateThread(NULL, 0, ReceiveMessageThread, NULL, 0, NULL);

    WaitForSingleObject(sendThread, INFINITE); // 等待线程结束
};
int iteration = 250;
double dis = 0.4;
double PID[3] = {0.6, 0.01, 0.001};
double *p = new double[3];
double dt = 0.005;
follower car_follower_1(iteration, dis, 1, PID, dt, isosceles_right_triangle);
follower car_follower_2(iteration, dis, 2, PID, dt, isosceles_right_triangle);

//必须先运行服务器，然后一直等待，然后才能运行客户端
int Init(const char *address);
void Close(void);
void output(void);
int main()
{
    Init("127.0.0.1");//为录制视频方便，绑定在本机，也可在真实网络环境下输入别的ip地址，实测可以
    car_follower_1.Motion();
    Close();
    output();
    system("pause");
    return 0;
}
int Init(const char *address)
{
    // 加载socket动态链接库(dll)
    WORD wVersionRequested;
    WSADATA wsaData;                    // 这结构是用于接收Wjndows Socket的结构信息的
    wVersionRequested = MAKEWORD(2, 2); // 请求2.2版本的WinSock库
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) // 返回值为零的时候是表示成功申请WSAStartup
    {
        return -1;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) // 检查版本号是否正确
    {
        WSACleanup();
        return -1;
    }

    // 创建socket操作，建立流式套接字，返回套接字号sockClient
    sockClient = socket(AF_INET, SOCK_STREAM, 0);
    if (sockClient == INVALID_SOCKET)
    {
        cout << "Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = inet_addr(address); // 本地回路地址是127.0.0.1;
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons(6000);

    // 将套接字sockClient与远程主机相连
    // int connect( SOCKET s,  const struct sockaddr* name,  int namelen);
    // 第一个参数：需要进行连接操作的套接字
    // 第二个参数：设定所需要连接的地址信息
    // 第三个参数：地址的长度
    int con_id = connect(sockClient, (SOCKADDR *)&addrSrv, sizeof(SOCKADDR));
    if (con_id == -1)
    {
        cout << "Error at connect(): " << WSAGetLastError() << endl;
        WSACleanup();
        return -1;
    }

    cout << "Connect prepared\n";
}
void Close(void)
{
    closesocket(sockClient);
    CloseHandle(sendThread);
    CloseHandle(receiveThread);
    CloseHandle(bufferMutex);
    WSACleanup(); // 终止对套接字库的使用
    cout << "End linking..." << endl;
    return;
}
void output(void)
{
    ofstream oFile;
    oFile.open("follow.csv", ios::out | ios::trunc);
    for (int i = 1; i <= iteration; i++)
    {
        oFile << car_follower_1.x[i] << "," << car_follower_1.y[i] << endl;
    }
    oFile.close();
}
DWORD WINAPI SendMessageThread(LPVOID IpParameter)
{
    while (1)
    {
        while (flag_client)
        {
            string talk;
            // getline(cin, talk);
            //         talk="test_client";
            double a[4] = {car_follower_1.x[i-1]+0.00001, car_follower_1.y[i-1]+0.00001,car_follower_2.x[i-1]+0.00001, car_follower_2.y[i-1]+0.00001};
            char *b;
            b = (char *)&a;
            talk = b;
            WaitForSingleObject(bufferMutex, INFINITE); // P（资源未被占用）
            if ("quit" == talk)
            {
                talk.push_back('\0');
                send(sockClient, talk.c_str(), talk.size(), 0);
                break;
            }

            //        cout << "I Say:(\"quit\"to exit):" << talk << endl;
            send(sockClient, talk.c_str(), talk.size(), 0); // 发送信息
            ReleaseSemaphore(bufferMutex, 1, NULL);         // V（资源占用完毕）
            flag_client = 0;
        }
    }
    return 0;
}
DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter)
{
    while (i <= iteration)
    {
        while (!flag_client)
        {
            char recvBuf[200];
            recv(sockClient, recvBuf, 300, 0);
            WaitForSingleObject(bufferMutex, INFINITE); // P操作（资源未被占用）
            if (recvBuf[0] == 'q' && recvBuf[1] == 'u' && recvBuf[2] == 'i' && recvBuf[3] == 't' && recvBuf[4] == '\0')
            {
                closesocket(sockClient);
                ReleaseSemaphore(bufferMutex, 1, NULL); // V操作（资源占用完毕）
                cout << "Server has been closed ..." << endl;
                exit(-1);
            }
            char *b = recvBuf;
            double *p = (double *)(b);
            car_follower_1.iterative(i, p);
            car_follower_2.iterative(i, p);
            i++;
            cout << "Server Says: " << car_follower_1.x[i - 1] << endl;
            cout << "Server Says: " << car_follower_1.y[i - 1] << endl; // 接收信息
            cout << "Server Says: " << car_follower_2.x[i - 1] << endl;
            cout << "Server Says: " << car_follower_2.y[i - 1] << endl; // 接收信息
                                                                        //                cout << "Server Says: " << car_follower_1.[i-1] << endl;
            cout << "i=" << i << endl;
            ReleaseSemaphore(bufferMutex, 1, NULL); // V操作（资源占用完毕）
            flag_client = 1;
            //        strcpy(recvBuf,"");
        }
    }
    return 0;
}
