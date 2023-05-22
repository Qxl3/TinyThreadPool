#include "ThreadPool.h"
#include<iostream>
#include<string.h>
#include<string>
#include<unistd.h>
#include<pthread.h>
using namespace std;
ThreadPool::ThreadPool(int min, int max)
{
	do
	{
		taskQ = new TaskQueue;
		if (taskQ == nullptr)
		{
			cout << " taskQ fail..." << endl;
			break;
		}
		
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << " malloc threadIds fail..." << endl;
			break;
		}
		memset(threadIDs, 0, sizeof(pthread_t) * max);//初始化threadIDs数组
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;//初始化线程数量和最小数量相等
		exitNum = 0;
		if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
			pthread_cond_init(&notEmpty, NULL) != 0)
		{
			cout << "初始化锁失败。。。。" << endl;
			break;
		}
		
		shutdown = false;

		//创建管理者线程和工作线程
		pthread_create(&managerID, NULL, manager,this);
		for (int i = 0; i < min; ++i)
		{
			pthread_create(&threadIDs[i], NULL, worker, this);
		}
		sleep(5);
		return;
	} while (0);

	//如果创建线程池失败则释放资源
	if (threadIDs) delete[]threadIDs;
	if (taskQ)delete taskQ;
	
}



ThreadPool::~ThreadPool()
{
	//关闭线程池
	shutdown = true;
	//阻塞回收管理者线程
	pthread_join(managerID, NULL);
	//唤醒阻塞的消费者线程
	for (int i = 0; i < liveNum; i++)
	{
		pthread_cond_signal(&notEmpty);
	}
	//释放堆内存
	if (taskQ)
	{
		delete taskQ;
	}
	if (threadIDs)
	{
		delete[]threadIDs;
	}
	//释放锁
	pthread_mutex_destroy(&mutexPool);
	pthread_cond_destroy(&notEmpty);
}

void ThreadPool::addTask(Task task)
{
	if (shutdown)
	{
		return;
	}
	//添加任务
	taskQ->addTask(task);
	//唤醒等待的线程
	pthread_cond_signal(&notEmpty);
}

int ThreadPool::getBusyNum()
{
	pthread_mutex_lock(&mutexPool);
	int busyNum = this->busyNum;
	pthread_mutex_unlock(&mutexPool);
	return busyNum;
}

int ThreadPool::getAliveNum()
{
	pthread_mutex_lock(&mutexPool);
	int liveNum = this->liveNum;
	pthread_mutex_unlock(&mutexPool);
	return liveNum;
}

void* ThreadPool::worker(void* arg)
{
	cout << "工作线程：" << pthread_self()<<"已被创立+++++" << endl;
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (true)
	{
		pthread_mutex_lock(&pool->mutexPool);
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//阻塞工作线程
			//当pthread_cond_wait将该线程放入条件变量的等待队列后，会将mutexPool解锁
			//当该线程被唤醒后，mutexPool又会锁上
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			//判断是否要销毁线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexPool);
					pool->threadExit();
				}
			}

			//判断线程池是否关闭
			if (pool->shutdown)
			{
				pthread_mutex_unlock(&pool->mutexPool);
				pool->threadExit();
			}
			//从任务的队列中取出一个任务
			Task task = pool->taskQ->takeTask();
			pool->busyNum++;
			pthread_mutex_unlock(&pool->mutexPool);
			cout << "工作线程：" << pthread_self() << "开始工作。。。。" << endl;

			//没看懂
			task.function(task.arg);
			delete task.arg;//这里因为在测试的时候，arg指向的是一块堆内存，所以删除
			task.arg = nullptr;

			cout << "工作线程：" << pthread_self() << "结束工作。。。。" << endl;
			pthread_mutex_lock(&pool->mutexPool);
			pool->busyNum--;
			pthread_mutex_unlock(&pool->mutexPool);

		}
	}
	
}


void* ThreadPool::manager(void* arg)
{
	cout << "管理者线程创建成功++++" << endl;
	cout << "管理者线程开始工作。。。。。" << endl;
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (!pool->shutdown)
	{
		//每隔3秒钟检测一次
		sleep(3);
		//取出线程池中任务的数量和当前线程和忙的线程的数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexPool);

		//根据情况添加线程
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			cout << "工作线程过少，任务过多，开始增加线程。。。" << endl;
			pthread_mutex_lock(&pool->mutexPool);
			int counter = 0;
			for (int i = 0; i < pool->maxNum && counter<NUMBER
				&&pool->liveNum<pool->maxNum;++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, worker, pool);
					counter++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
			cout << "增加线程成功!" << endl;
		}
		//销毁线程
		//忙的线程数*2<存活的线程数&&存活的线程>最小的线程
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);//唤醒阻塞的线程
			}
		}


	}
}

void ThreadPool::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; i++)
	{
		if (threadIDs[i] == tid)
		{
			threadIDs[i] = 0;
			cout << "线程" << tid << "已退出。。。" << endl;
			break;
		}
	}
	pthread_exit(NULL);//终结当前线程
}
