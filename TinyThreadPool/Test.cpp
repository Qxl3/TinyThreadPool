#include"ThreadPool.h"
#include<unistd.h>
#include<iostream>
using namespace std;
void taskFunc(void* arg)
{
	int num = *(int*)arg;
	cout << "工作线程：" << pthread_self() << "工作中。。。。" 
		<<"工作任务号为："<<num << endl;
	sleep(1);
}

int main()
{
	//创建线程池
	cout << "创建线程池。。。。" << endl;
	ThreadPool pool(3, 10);
	cout << "线程池创建完毕！" << endl;

	for (int i = 0; i < 100; i++)
	{
		int* num = new int(i+100);
		pool.addTask(Task(taskFunc, num));
		//sleep(1);
	}

	sleep(20);
	return 0;
}