#pragma once
#include"TaskQueue.h"

class ThreadPool
{
public:
	ThreadPool(int min,int max);//初始化线程池
	~ThreadPool();//销毁线程池
	void addTask(Task task);//添加任务
	int getBusyNum();//获取忙的线程的个数
	int getAliveNum();//获得活着的线程
private:
	static void* worker(void* arg);//消费者线程
	static void* manager(void* arg);//管理者线程
	void threadExit();//单个线程退出
private:
	TaskQueue* taskQ;

	pthread_t managerID;//管理的线程ID
	pthread_t* threadIDs;//工作的线程ID
	int minNum;//最小的线程数量
	int maxNum;//最大的线程数量
	int busyNum;//工作中的线程的个数
	int liveNum;//存活的线程个数
	int exitNum;//要销毁的线程个数
	static const int NUMBER = 2;//每次添加线程添加两个

	pthread_mutex_t mutexPool;//锁整个的线程池
	pthread_cond_t notEmpty;//任务队列是否为空


	bool shutdown;//是否销毁
};

